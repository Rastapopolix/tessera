/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The Daisy Seed's profile (tess_profile.h): the core as Andamento borrows it (platform/daisy). The screen and the
 * controls are the ones the core draws and reads (the FM-1's); Andamento's own two 128 x 64 screens, 16 keys and 8
 * encoders are andamento-fw's, which maps them onto these */
#define TESS_SCR_W 240
#define TESS_SCR_H 240
#define TESS_SCR_DEPTH 16
#define TESS_SCR_COUNT 1
#define TESS_N_BUTTONS 14
#define TESS_N_ENCODERS 7
#define TESS_N_KEYS 27
#define TESS_TIER TESS_TIER_ROOMY   /* 8 MiB QSPI, 64 MiB SDRAM */
