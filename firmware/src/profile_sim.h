/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The web simulator's profile (tess_profile.h): the FM-1 in the browser (web/sim) */
#define TESS_SCR_W 240
#define TESS_SCR_H 240
#define TESS_SCR_DEPTH 16
#define TESS_SCR_COUNT 1
#define TESS_N_BUTTONS 14
#define TESS_N_ENCODERS 7
#define TESS_N_KEYS 27
#define TESS_TIER TESS_TIER_DESKTOP
#define TESS_HAS_BATTERY 1   /* (shows the FM-1's, as the device would) */
#define TESS_HAS_POINTER 0   /* (not yet: the page would read the core's zones as the plugin does) */
#define TESS_HOSTED 0        /* (the FM-1 in a page: its settings as the FM-1's) */
