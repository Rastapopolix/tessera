/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The plugin window: a native child window of the host's (Win32 or X11) showing the Panel. gui_win32.cpp and
 * gui_x11.cpp implement it; on macOS there is no window yet (gui_none.cpp). All on the host's main thread. */
#pragma once
#include <clap/clap.h>

#include <memory>
#include <string>

#include "panel.h"

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
};

class Gui
{
  public:
    virtual ~Gui() = default;
    virtual bool set_parent(const clap_window_t *w) = 0;
    virtual bool show() = 0;
    virtual bool hide() = 0;
    virtual void tick() = 0;   // ~30 times a second (X11: from the host's timer)
    static const char *api();  // CLAP_WINDOW_API_WIN32 / _X11, or nullptr
    static std::unique_ptr<Gui> make(const clap_host_t *host, GuiLink *link);
};
