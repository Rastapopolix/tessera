/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Tessera without the FM-1: the whole firmware (sound, sequencer, generators, the FM-1's UI, the projects) on the
 * host HAL, with the core API (tess_core.h) over it. One unity build, the same two include lists as the host tests
 * (firmware/src/core_sources.h, core_ui_sources.h): the web simulator (web/sim/sim.c), the plugin
 * (platform/plugin) and the Daisy (platform/daisy/tessera_core.c) build this file, each with its own options:
 *
 *   MELODEE_FS                     the tables' rate (tools/gen_tables.py): 44,100, the Daisy 48,000
 *   CORE_KEEP_ATTRIBUTES           keep the firmware's placements (section, aligned, noinline): the Daisy's linker
 *                                  script puts .audio_text in ITCM. Without it they are dropped, as on the host tests
 *   CORE_BIG                       where the large host buffers go (the Daisy: its SDRAM)
 *   HOST_HAL_SCREEN 0              no copy of the FM-1's screen (host_hal.h)
 *   TESS_CORE_UI 0                 the FM-1's UI never runs (its input and frames): a front end with its own UI
 *   TESS_IRQ_OFF / ON, TESS_ENTROPY   the audio interrupt's guard, a fresh seed (core_sources.h)
 *   TESS_HOST_CLOCK, TESS_PLUGIN_IO   the plugin's DAW clock, its track outputs and MIDI out (mix.c, usb.c)
 *   TESS_CORE_BOOT_HOOK()          run in tess_core_boot before the screen is set up (the simulator's demos)
 *   TESS_GENERATE 1                GENERATE (generate.c): a whole piece from a style and a seed (the plugin) */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#ifndef TRK_START_TESS
#define TRK_START_TESS 1                          /* the device's power-on start (engines.c), not the tests' */
#endif
#ifndef CORE_BIG
#define CORE_BIG
#endif
#ifndef CORE_KEEP_ATTRIBUTES
#define __attribute__(x)                          /* (the FM-1's sections: not every host linker takes them) */
#endif
#ifndef SMP_USER_XIP
static uint32_t host_slots[3u * 0x14000u / 4u] CORE_BIG;   /* USR1..3 (zero: empty), as the flash at 0xA0000 */
#define SMP_USER_XIP(k) ((const uint8_t *)host_slots + (k) * SMP_USER_SIZE)
#endif
#include "core_sources.h"                         /* the core: the first shared list */
#include "host_hal.h"                             /* the FM-1's hardware, simulated */
#include "core_ui_sources.h"                      /* the UI and the projects: the second shared list */
#include "host_boot.h"                            /* main.c melodee_init (ui_power_on) */
#ifndef CORE_KEEP_ATTRIBUTES
#undef __attribute__
#endif
#include "tess_core.c"
