/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The target's profile: its facts in one header, picked by TESS_TARGET (the FM-1 when a build names none). Each
 * target's build names itself: tools/build.py none (the FM-1), platform/plugin/core_defs.txt TESS_T_PLUGIN,
 * web/sim/build.sh TESS_T_SIM, platform/daisy/Makefile TESS_T_DAISY.
 *
 * Facts describe the hardware (screen, controls) and nothing else sets them; page layouts are to read them in place
 * of the FM-1's numbers (TESS_SCR_W, not 240), so a wider screen (the plugin's EXPANDED mode, the FM-2) is a profile,
 * not a fork. The tier says how much room the target has (code space, RAM) for the features a tight build leaves out.
 *
 * TESS_HAS_BATTERY is read (no battery: no header icon, BATTERY setting, BATTERY LOW or POWER lines); the rest
 * are not yet (every build is as before). The core API's fixed
 * numbers (platform/core/tess_core.h TESS_SCREEN, TESS_NB, TESS_NE, TESS_KEYS) must agree: tests/profile_test.c. */
#ifndef TESS_PROFILE_H
#define TESS_PROFILE_H

#define TESS_T_FM1 1
#define TESS_T_PLUGIN 2
#define TESS_T_SIM 3
#define TESS_T_DAISY 4

#define TESS_TIER_TIGHT 1     /* under 1 MiB of code: today's device build */
#define TESS_TIER_ROOMY 2     /* 1..8 MiB, small RAM: the Daisy (Andamento), an FM-2 of the same family */
#define TESS_TIER_DESKTOP 3   /* no limit: the plugin, the web simulator */

#ifndef TESS_TARGET
#define TESS_TARGET TESS_T_FM1
#endif

#if TESS_TARGET == TESS_T_FM1
#include "profile_fm1.h"
#elif TESS_TARGET == TESS_T_PLUGIN
#include "profile_plugin.h"
#elif TESS_TARGET == TESS_T_SIM
#include "profile_sim.h"
#elif TESS_TARGET == TESS_T_DAISY
#include "profile_daisy.h"
#else
#error "TESS_TARGET: TESS_T_FM1, TESS_T_PLUGIN, TESS_T_SIM or TESS_T_DAISY"
#endif

#if !defined(TESS_SCR_W) || !defined(TESS_SCR_H) || !defined(TESS_SCR_DEPTH) || !defined(TESS_SCR_COUNT) || \
    !defined(TESS_N_BUTTONS) || !defined(TESS_N_ENCODERS) || !defined(TESS_N_KEYS) || !defined(TESS_TIER) || \
    !defined(TESS_HAS_BATTERY)
#error "a profile defines every fact: TESS_SCR_W, _H, _DEPTH, _COUNT, TESS_N_BUTTONS, _ENCODERS, _KEYS, TESS_TIER, TESS_HAS_BATTERY"
#endif

#endif
