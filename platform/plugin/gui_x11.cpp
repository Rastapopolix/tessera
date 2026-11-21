/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The plugin window on Linux (gui.h): an X11 child window of the host's. CLAP hosts run the plugin's GUI on their
 * own loop: the host's timer (timer-support) calls tick() ~30 times a second, which reads the X events and draws.
 * OPEN / SAVE under the keys ask zenity or kdialog for a file (whichever is installed), on a thread of their own so
 * the host's window never waits; tick() opens or saves once the answer is in. */
#include "gui.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <clap/ext/timer-support.h>

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace
{
// a file dialog's answer, shared with its thread (which may outlive the window)
struct Ask
{
    std::mutex lock;
    bool done = false;
    int req = 0;
    std::string path, err;
};

bool have(const char *prog)
{
    std::string c = std::string("command -v ") + prog + " >/dev/null 2>&1";
    return std::system(c.c_str()) == 0;
}

// the dialog's command line; name: a file name made by projfile (letters, digits, spaces, - _ .), quoted
std::string dialog_command(int req, const std::string &name)
{
    std::string q = "'" + name + "'";
    if (have("zenity"))
        return req == Panel::FILE_OPEN
                   ? "zenity --file-selection --title='Open a Tessera project' "
                     "--file-filter='Tessera project or FM-1 backup | *.tsp *.json' 2>/dev/null"
                   : "zenity --file-selection --save --confirm-overwrite --title='Save the Tessera project' --filename=" + q +
                         " 2>/dev/null";
    if (have("kdialog"))
        return req == Panel::FILE_OPEN ? "kdialog --getopenfilename . '*.tsp *.json' 2>/dev/null"
                                       : "kdialog --getsavefilename " + q + " '*.tsp' 2>/dev/null";
    return "";
}

class GuiX11 : public Gui
{
  public:
    const clap_host_t *host;
    const clap_host_timer_support_t *timers = nullptr;
    clap_id timer = CLAP_INVALID_ID;
    Display *dpy = nullptr;
    Window wnd = 0;
    GC gc = nullptr;
    XImage *img = nullptr;
    Panel panel;
    std::vector<uint32_t> px = std::vector<uint32_t>(Panel::W * Panel::H);
    tess_view_t view{};

    GuiX11(const clap_host_t *h, GuiLink *l) : host(h), link(l)
    {
        panel.set_shell(l->shell());
        panel.set_keys(l->keys_on());
        panel.set_inks(l->inks());
        panel.set_tracks(l->tracks());
        panel.set_version(tess_version());
    }
    GuiLink *link;   // this window's instance
    ~GuiX11() override
    {
        if (timers && timer != CLAP_INVALID_ID)
            timers->unregister_timer(host, timer);
        if (img)
        {
            img->data = nullptr;   // (px owns the pixels)
            XDestroyImage(img);
        }
        if (dpy)
        {
            if (gc) XFreeGC(dpy, gc);
            if (wnd) XDestroyWindow(dpy, wnd);
            XCloseDisplay(dpy);
        }
    }

    bool set_parent(const clap_window_t *w) override
    {
        if (!(dpy = XOpenDisplay(nullptr)))
            return false;
        int scr = DefaultScreen(dpy);
        wnd = XCreateSimpleWindow(dpy, (Window)w->x11, 0, 0, Panel::W, Panel::H, 0, 0, BlackPixel(dpy, scr));
        XSelectInput(dpy, wnd, ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | KeyPressMask |
                                   KeyReleaseMask | FocusChangeMask);
        gc = XCreateGC(dpy, wnd, 0, nullptr);
        Visual *vis = DefaultVisual(dpy, scr);
        img = XCreateImage(dpy, vis, DefaultDepth(dpy, scr), ZPixmap, 0, reinterpret_cast<char *>(px.data()),
                           Panel::W, Panel::H, 32, Panel::W * 4);
        timers = static_cast<const clap_host_timer_support_t *>(host->get_extension(host, CLAP_EXT_TIMER_SUPPORT));
        if (timers)
            timers->register_timer(host, 33, &timer);
        XFlush(dpy);
        return img != nullptr;
    }
    bool show() override
    {
        if (!dpy) return false;
        XMapWindow(dpy, wnd);
        XFlush(dpy);
        return true;
    }
    bool hide() override
    {
        if (!dpy) return false;
        XUnmapWindow(dpy, wnd);
        XFlush(dpy);
        return true;
    }
    std::shared_ptr<Ask> ask;   // a file dialog open, or none

    // OPEN or SAVE clicked: its dialog on a thread; tick() takes the answer
    void files()
    {
        int kr = panel.take_keys_request();    // (KEYS ON or OFF: every window's)
        if (kr >= 0)
            link->keys_set(kr == 1);
        int sh = panel.take_shell_request();   // (a shell picked: its ink)
        if (sh >= 0)
            link->shell_picked(sh);
        int ink = panel.take_ink_request();    // (an ink picked)
        if (ink >= 0)
            link->ink_picked(ink);
        if (ask)
        {
            std::lock_guard<std::mutex> g(ask->lock);
            if (ask->done)
            {
                if (!ask->err.empty())
                    panel.note(ask->err);
                else if (!ask->path.empty())
                    panel.note(ask->req == Panel::FILE_OPEN ? link->open_file(ask->path) : link->save_file(ask->path));
                ask.reset();
            }
            return;
        }
        int req = panel.take_file_request();
        if (req == Panel::FILE_NONE)
            return;
        std::string cmd = dialog_command(req, req == Panel::FILE_SAVE ? link->file_name() : std::string());
        if (cmd.empty())
        {
            panel.note("NO FILE DIALOG: INSTALL ZENITY OR KDIALOG");
            return;
        }
        ask = std::make_shared<Ask>();
        ask->req = req;
        std::thread([a = ask, cmd] {
            std::string out;
            if (FILE *f = popen(cmd.c_str(), "r"))
            {
                char buf[4096];
                while (fgets(buf, sizeof buf, f))
                    out += buf;
                pclose(f);
            }
            while (!out.empty() && (out.back() == '\n' || out.back() == '\r'))
                out.pop_back();
            std::lock_guard<std::mutex> g(a->lock);
            a->path = out;   // (empty: cancelled)
            a->done = true;
        }).detach();
    }

    // the screen's ink changed since the last frame (not at the window's first): the next window's
    int ink_last = -1;
    void ink_check()
    {
        if (ink_last >= 0 && (int)view.ink != ink_last)
            link->ink_seen((int)view.ink);
        ink_last = (int)view.ink;
    }
    void tick() override
    {
        if (!dpy)
            return;
        files();
        bool changed = false;
        while (XPending(dpy))
        {
            XEvent e;
            XNextEvent(dpy, &e);
            switch (e.type)
            {
            case ButtonPress:
                XSetInputFocus(dpy, wnd, RevertToParent, CurrentTime);   // (the keyboard, once clicked)
                if (e.xbutton.button == Button1)
                    changed |= panel.down(e.xbutton.x, e.xbutton.y);
                else if (e.xbutton.button == Button3)   // a button or key latched
                    changed |= panel.latch(e.xbutton.x, e.xbutton.y);
                else if (e.xbutton.button == Button4 || e.xbutton.button == Button5)
                    changed |= panel.wheel(e.xbutton.x, e.xbutton.y, e.xbutton.button == Button4 ? 1 : -1);
                break;
            case ButtonRelease:
                if (e.xbutton.button == Button1)
                    changed |= panel.up(e.xbutton.x, e.xbutton.y);
                break;
            case MotionNotify:
                changed |= panel.move(e.xmotion.x, e.xmotion.y);
                break;
            case KeyPress:
            case KeyRelease:
            {
                // the computer keyboard (panel.h key): X's keycode - 8 is the PC scan code for the main keys
                if (e.xkey.state & (ControlMask | Mod1Mask))
                    break;   // (the host's)
                if (e.type == KeyRelease && XEventsQueued(dpy, QueuedAfterReading))
                {
                    XEvent nx;
                    XPeekEvent(dpy, &nx);
                    if (nx.type == KeyPress && nx.xkey.keycode == e.xkey.keycode && nx.xkey.time == e.xkey.time)
                        break;   // (auto-repeat: a release and a press at once; the key is still held)
                }
                KeySym ks = XLookupKeysym(&e.xkey, 0);
                int code = ks == XK_Up ? Panel::KEY_UP : ks == XK_Down ? Panel::KEY_DOWN : ks == XK_Left ? Panel::KEY_LEFT
                         : ks == XK_Right ? Panel::KEY_RIGHT : (int)e.xkey.keycode - 8;
                changed |= panel.key(code, e.type == KeyPress);
                break;
            }
            case FocusOut:
                changed |= panel.release_keys();
                break;
            default:
                break;
            }
        }
        if (changed)
            link->input(panel.input);
        link->view(view);
        panel.set_midi(link->midi_count());
        gui_fm1_tick(panel, link);
        ink_check();
        panel.draw(px.data(), view);
        XPutImage(dpy, wnd, gc, img, 0, 0, 0, 0, Panel::W, Panel::H);
        XFlush(dpy);
    }
};
}   // namespace

const char *Gui::api() { return CLAP_WINDOW_API_X11; }
std::unique_ptr<Gui> Gui::make(const clap_host_t *host, GuiLink *link) { return std::make_unique<GuiX11>(host, link); }
