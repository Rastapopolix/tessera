/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The plugin's profile (tess_profile.h): the FM-1's panel, drawn in the DAW's window (platform/plugin). Its screen is
 * the FM-1's until EXPANDED mode lays the pages out wider */
#define TESS_SCR_W 240
#define TESS_SCR_H 240
#define TESS_SCR_DEPTH 16
#define TESS_SCR_COUNT 1
#define TESS_N_BUTTONS 14     /* the FM-1's, by mouse and computer keyboard */
#define TESS_N_ENCODERS 7
#define TESS_N_KEYS 27
#define TESS_TIER TESS_TIER_DESKTOP
#define TESS_HAS_BATTERY 0   /* (a computer: no battery icon, BATTERY setting or POWER lines) */
