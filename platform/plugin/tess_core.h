/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Tessera's core for the plugin: the web simulator's firmware build (web/sim/sim.c) with a small C interface.
 * The firmware keeps its state in file-level globals, so there is one core per process: the plugin allows one
 * instance at a time. Not thread-safe: the caller serialises every call. */
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* the core's own rate (44,100, as the FM-1) */
uint32_t tess_core_rate(void);
/* power-on, once per process: STILL TIDE loaded in slot A (and KOTO GARDEN in B), stopped */
void tess_core_boot(void);
/* n stereo frames (n <= 1024) at tess_core_rate(), the device running until they exist */
void tess_core_render(float *l, float *r, uint32_t n);
/* a MIDI channel message in, as from USB MIDI: 0 = the queue was full */
uint32_t tess_core_midi(uint32_t status, uint32_t d1, uint32_t d2);
/* the transport: 1 play (from where the song is), 0 stop. Does nothing if it is already so */
void tess_core_transport(int play);
uint32_t tess_core_playing(void);
/* the music as it is now, as a .tsp record (TSP1, what SAVE > PROJECT writes): its length, 0 = failed/too big */
uint32_t tess_core_save(uint8_t *out, uint32_t cap);
/* a .tsp record in: 0 loaded, 1 not a record this firmware reads */
uint32_t tess_core_load(const uint8_t *in, uint32_t len);
/* the largest record */
uint32_t tess_core_tsp_max(void);

#ifdef __cplusplus
}
#endif
