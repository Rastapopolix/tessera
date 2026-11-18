/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* Tessera's core, the shared include list (2 of 2): the FM-1's UI (drawn on whatever screen the HAL gives it), the
 * user presets and the projects (.tsp, project_tsp.c). After core_sources.h and a HAL (the FM-1's in melodee.c;
 * platform/core/host_hal.h everywhere else). With ANN_VER (MELODEE_ACCESS): the screen reader, announce.c first. */
#include "gfx.c"
#include "panel.c"
#include "ui.c"
#include "icons.c"
#include "ui_graph.c"
#include "page_random.c"
#include "ui_draw.c"
#include "ui_menu.c"
#include "ui_input.c"
#include "ui_layer.c"
#ifdef ANN_VER
#include "ui_speak.c"
#endif
#include "upreset.c"
#include "project.c"
#include "fm6_store.c"
