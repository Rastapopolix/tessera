/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The plugin window: a native child window of the host's (Win32, X11 or Cocoa) showing the Panel. gui_win32.cpp,
 * gui_x11.cpp and gui_mac.mm implement it (gui_none.cpp: none, for a platform without one). All on the host's main
 * thread. */
#pragma once
#include <clap/clap.h>

#include <memory>
#include <string>

#include "fm1_link.h"
#include "panel.h"
#include "projfile.h"

/* the window's instance (tessera_clap.cpp): the latest view its audio thread made, and the panel's input for it */
struct GuiLink
{
    virtual ~GuiLink() = default;
    virtual void view(tess_view_t &v) = 0;
    virtual void input(PanelInput &in) = 0;
    /* the project files (on the host's main thread), each returning what to show under the keys: open a .tsp or an
     * FM-1 backup (its music playing, its slots A..D); save the project as a .tsp; the name to offer for that */
    virtual std::string open_file(const std::string &path_utf8) = 0;
    virtual std::string save_file(const std::string &path_utf8) = 0;
    virtual std::string file_name() = 0;
    /* the shell the window shows (Panel::shell_count), and one picked under the keys: the instance sets its ink and
     * remembers it for the next window */
    virtual int shell() = 0;
    virtual void shell_picked(int i) = 0;
    /* the screen's inks, MENU > COLOR's choices (the swatches under the buttons); one picked there: the instance sets
     * it; the ink the screen shows changed (a pick, a shell's ink, MENU > COLOR): remembered for the next window */
    virtual std::vector<Panel::Ink> inks() = 0;
    virtual void ink_picked(int i) = 0;
    virtual void ink_seen(int i) = 0;
    /* the build's tracks (4 or 8: the window's 1-4 / 5-8 switch) */
    virtual int tracks() = 0;
    /* MIDI in so far (notes and messages from the host, a count that wraps): TESSERA's shell lights its MIDI lamp */
    virtual uint32_t midi_count() { return 0; }
    /* an FM-1 on USB (fm1_link.h): this instance's state, and a click: on (taken from any other Tessera) or off (for
     * every Tessera) */
    virtual Fm1Link::State fm1_state() { return Fm1Link::OFF; }
    virtual void fm1_set(bool on) { (void)on; }
    /* SIZE (TESSERA's menu): the window's size now, as Panel's halves (2, 3, 4: 100, 150, 200 %), and a pick (100, 150
     * or 200): the instance remembers it for every window and asks the host for the new size */
    virtual int size_halves() { return 2; }
    virtual void size_pick(int pct) { (void)pct; }
    /* KEYS (TESSERA's menu): whether the computer keyboard plays Tessera, the same for every window (a setting) */
    virtual bool keys_on() { return projfile::setting_get("keys") != "0"; }
    virtual void keys_set(bool on) { projfile::setting_put("keys", on ? "1" : "0"); }
};

/* each window's tick: a click on FM-1 USB handed on, and its state shown */
inline void gui_fm1_tick(Panel &panel, GuiLink *link)
{
    int f = panel.take_fm1_request();
    if (f >= 0)
        link->fm1_set(f == 1);
    Fm1Link::State s = link->fm1_state();
    panel.set_fm1(s != Fm1Link::OFF, s != Fm1Link::ELSEWHERE, s == Fm1Link::LIVE, fm1_state_text(s));
}

/* each window's tick: SIZE picked in the menu handed on; the size shown */
inline void gui_size_tick(Panel &panel, GuiLink *link)
{
    int pct = panel.take_size_request();
    if (pct > 0)
        link->size_pick(pct);
    panel.set_size_pct(Panel::size_pct(link->size_halves()));
}

/* the plugin's version, for the window's ABOUT (TESS_VERSION, CMakeLists.txt) */
inline const char *tess_version()
{
#ifdef TESS_VERSION_STR
    return TESS_VERSION_STR;
#else
    return "";
#endif
}

class Gui
{
  public:
    virtual ~Gui() = default;
    virtual bool set_parent(const clap_window_t *w) = 0;
    virtual bool show() = 0;
    virtual bool hide() = 0;
    virtual void tick() = 0;   // ~30 times a second (X11: from the host's timer)
    /* the window's size: the panel at n / 2 (2, 3, 4: 100, 150, 200 %); before set_parent, the size it opens at */
    virtual void set_scale(int halves) { (void)halves; }
    static const char *api();  // CLAP_WINDOW_API_WIN32 / _X11 / _COCOA, or nullptr
    static std::unique_ptr<Gui> make(const clap_host_t *host, GuiLink *link);
};
