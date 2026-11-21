/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The platform profiles (firmware/src/tess_profile.h): each target's facts agree with the core API's fixed numbers
 * (platform/core/tess_core.h), and core.h picks the profile its build names. Built once per target:
 *   cc -DTESS_TARGET=TESS_T_PLUGIN -Iplatform/core -Ifirmware/src tests/profile_test.c */
#include <stdio.h>
#include <string.h>
#include "core.h"
#include "tess_core.h"

#ifndef TESS_PROFILE_H
#error "core.h includes tess_profile.h"
#endif

int main(void)
{
    int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { printf("FAIL: " __VA_ARGS__); printf("\n"); fails++; } } while (0)
    static const char *NAMES[] = {"?", "FM-1", "plugin", "web sim", "Daisy"};
    printf("profile: %s, screen %d x %d x %d bits (%d), %d buttons, %d encoders, %d keys, tier %d\n",
           NAMES[TESS_TARGET], TESS_SCR_W, TESS_SCR_H, TESS_SCR_DEPTH, TESS_SCR_COUNT, TESS_N_BUTTONS,
           TESS_N_ENCODERS, TESS_N_KEYS, TESS_TIER);
    /* the core draws one 240 x 240 RGB565 screen and reads the FM-1's controls on every target until EXPANDED */
    CHECK(TESS_SCR_W == TESS_SCREEN && TESS_SCR_H == TESS_SCREEN, "the screen: the core API's %d", TESS_SCREEN);
    CHECK(TESS_SCR_DEPTH == 16 && TESS_SCR_COUNT == 1, "one RGB565 screen");
    CHECK(TESS_N_BUTTONS == TESS_NB, "buttons: the core API's %d", TESS_NB);
    CHECK(TESS_N_ENCODERS == TESS_NE, "encoders: the core API's %d", TESS_NE);
    CHECK(TESS_N_KEYS == TESS_KEYS, "keys: the core API's %d", TESS_KEYS);
    CHECK(TESS_TIER == (TESS_TARGET == TESS_T_FM1 ? TESS_TIER_TIGHT : TESS_TARGET == TESS_T_DAISY ? TESS_TIER_ROOMY
                                                                                               : TESS_TIER_DESKTOP),
          "the tier: FM-1 tight, Daisy roomy, plugin and sim desktop");
    printf(fails ? "profile test: %d FAILED\n" : "profile test: ok\n", fails);
    return fails ? 1 : 0;
}
