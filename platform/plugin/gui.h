/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The plugin window: a native child window of the host's (Win32 or X11) showing the Panel. gui_win32.cpp and
 * gui_x11.cpp implement it; on macOS there is no window yet (gui_none.cpp). All on the host's main thread. */
#pragma once
#include <clap/clap.h>

#include <memory>

#include "panel.h"

/* the window's instance (tessera_clap.cpp): the latest view its audio thread made, and the panel's input for it */
struct GuiLink
{
    virtual ~GuiLink() = default;
    virtual void view(tess_view_t &v) = 0;
    virtual void input(PanelInput &in) = 0;
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
