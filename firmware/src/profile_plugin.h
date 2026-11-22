/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The plugin's profile (tess_profile.h): the FM-1's panel, drawn in the DAW's window (platform/plugin). Its screen is
 * the FM-1's; a second, TRACKS (480 x 240: ui_tracks.c, tess_core_tracks_view), beside it in EXPANDED mode */
#define TESS_SCR_W 240
#define TESS_SCR_H 240
#define TESS_SCR_DEPTH 16
#define TESS_SCR_COUNT 2     /* the FM-1's, and TRACKS (EXPANDED mode) */
#define TESS_N_BUTTONS 14     /* the FM-1's, by mouse and computer keyboard */
#define TESS_N_ENCODERS 7
#define TESS_N_KEYS 27
#define TESS_TIER TESS_TIER_DESKTOP
#define TESS_HAS_BATTERY 0   /* (a computer: no battery icon, BATTERY setting or POWER lines) */
#define TESS_HAS_POINTER 1   /* a mouse on the screen: its values, lists and keys as zones (ui_zones.c) */
#define TESS_HOSTED 1        /* inside a DAW: MENU without the device's rows (SCREEN OFF, PANEL GAMMA, CLOCK OUT, USB,
                              * SIM CONTROL, CALIBRATION) or KNOBS and LARGE VALUE (the mouse turns the dials) */
