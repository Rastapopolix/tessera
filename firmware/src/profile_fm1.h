/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The FM-1's profile (tess_profile.h): the device */
#define TESS_SCR_W 240        /* ST7789, RGB565 over SPI */
#define TESS_SCR_H 240
#define TESS_SCR_DEPTH 16     /* bits a pixel */
#define TESS_SCR_COUNT 1
#define TESS_N_BUTTONS 14     /* FX SEL ENV LFO EDIT GLO HOME SAVE ARP SEQ PLAY REC OCT- OCT+ */
#define TESS_N_ENCODERS 7     /* SELECT, ALGORITHM, PRESETS, KNOB 1..4 (MASTER is a pot) */
#define TESS_N_KEYS 27
#define TESS_TIER TESS_TIER_TIGHT
#define TESS_HAS_BATTERY 1   /* the FM-1 runs on its battery or USB */
#define TESS_HAS_POINTER 0   /* (its knobs and keys only: no mouse on the screen) */
