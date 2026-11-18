/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The plugin window: a native child window of the host's (Win32 or X11) showing the Panel. gui_win32.cpp and
 * gui_x11.cpp implement it; on macOS there is no window yet (gui_none.cpp). All on the host's main thread. */
#pragma once
#include <clap/clap.h>

#include <memory>

#include "panel.h"

/* tessera_clap.cpp: the latest view the audio thread made, and the panel's input for the audio thread */
void tess_gui_view(tess_view_t &v);
void tess_gui_input(PanelInput &in);

class Gui
{
  public:
    virtual ~Gui() = default;
    virtual bool set_parent(const clap_window_t *w) = 0;
    virtual bool show() = 0;
    virtual bool hide() = 0;
    virtual void tick() = 0;   // ~30 times a second (X11: from the host's timer)
    static const char *api();  // CLAP_WINDOW_API_WIN32 / _X11, or nullptr
    static std::unique_ptr<Gui> make(const clap_host_t *host);
};
