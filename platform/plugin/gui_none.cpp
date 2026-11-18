/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* macOS: no plugin window yet (gui.h). The plugin offers none, and plays as in phase 0. */
#include "gui.h"

const char *Gui::api() { return nullptr; }
std::unique_ptr<Gui> Gui::make(const clap_host_t *) { return nullptr; }
