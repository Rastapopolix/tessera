/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The plugin window on Linux (gui.h): an X11 child window of the host's. CLAP hosts run the plugin's GUI on their
 * own loop: the host's timer (timer-support) calls tick() ~30 times a second, which reads the X events and draws. */
#include "gui.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <clap/ext/timer-support.h>

#include <cstdlib>
#include <vector>

namespace
{
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

    GuiX11(const clap_host_t *h, GuiLink *l) : host(h), link(l) {}
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
        XSelectInput(dpy, wnd, ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
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
    void tick() override
    {
        if (!dpy)
            return;
        bool changed = false;
        while (XPending(dpy))
        {
            XEvent e;
            XNextEvent(dpy, &e);
            switch (e.type)
            {
            case ButtonPress:
                if (e.xbutton.button == Button1)
                    changed |= panel.down(e.xbutton.x, e.xbutton.y);
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
            default:
                break;
            }
        }
        if (changed)
            link->input(panel.input);
        link->view(view);
        panel.draw(px.data(), view);
        XPutImage(dpy, wnd, gc, img, 0, 0, 0, 0, Panel::W, Panel::H);
        XFlush(dpy);
    }
};
}   // namespace

const char *Gui::api() { return CLAP_WINDOW_API_X11; }
std::unique_ptr<Gui> Gui::make(const clap_host_t *host, GuiLink *link) { return std::make_unique<GuiX11>(host, link); }
