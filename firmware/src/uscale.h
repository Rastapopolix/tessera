/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
#pragma once
#include <stdint.h>                                 /* (settings_persist.c includes it on its own too) */
/* USER scales: four 12-note scales of the player's own, each a mask as the built-in ones (scales.h SCALE_MASK: bit 0 the
 * root, bit i the note i semitones above it), picked on the keys on SCL > SCALES (scale_picker.c uscale_keys). They are
 * the device's (the settings record: favorites row 8 bytes 8..15, settings_persist.c) and go with a project that plays
 * one (its USCALE part, project_tsp.c), so it plays the same on any FM-1. uscale_sel picks one for the whole project,
 * as TUNING does (tune.h), SCL kept underneath (older firmware plays SCL); picking one turns TUNING OFF and back. seq.c
 * scale_mask reads it, so the keys, every generator, QNT, chords and FOLLOW play it as a built-in mask */
#define USCALE_SLOTS 4u
#define USCALE_DEF 0xAB5u                           /* a slot never set: MAJ */
#define USCALE_ID (SCALE_TOTAL + TUNE_SLOTS)        /* SCL's picker: USCALE_ID + k the slot k, in the USER family */
static uint16_t uscale_mask[USCALE_SLOTS] = {USCALE_DEF, USCALE_DEF, USCALE_DEF, USCALE_DEF};
static uint8_t uscale_sel;                          /* 0 none, 1..USCALE_SLOTS that slot (the project's) */
static const char *const USCALE_NAME[USCALE_SLOTS] = {"USER1", "USER2", "USER3", "USER4"};
static const char *const USCALE_TITLE[USCALE_SLOTS] = {"User 1 keys edit", "User 2 keys edit", "User 3 keys edit",
                                                       "User 4 keys edit"};

static int uscale_ok(uint32_t m) { return (m & 1u) && m <= 0xFFFu; }   /* the root in, 12 notes at most */
static uint32_t uscale_count(uint32_t m)
{
    uint32_t n = 0;
    for (; m; m >>= 1) n += m & 1u;
    return n;
}
