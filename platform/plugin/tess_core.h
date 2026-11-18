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
/* n stereo frames (n <= 1024) at tess_core_rate(), the device running until they exist. stems: 0, or four buffers of n
 * that get tracks 1..4 alone (the FM-1's USB stems: each track's sound after its level, before pan and the bus FX,
 * mono); asked for once, the stems are made from then on */
void tess_core_render(float *l, float *r, float *const *stems, uint32_t n);
/* the core frames rendered so far (wraps) */
uint32_t tess_core_frames(void);
/* MIDI OUT (Settings > NOTES OUT; each track on its own channel, GLO > MIDI OUT), as USB MIDI packets: the next one
 * whose frame (tess_core_frames' count) has been rendered. 0 = none */
uint32_t tess_core_midi_out(uint32_t *pkt, uint32_t *frame);
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

/* ---- the panel (the plugin window): as web/sim/sim.c's page interface */
enum { TESS_NB = 14, TESS_NE = 7, TESS_KEYS = 27, TESS_SCREEN = 240 };
/* buttons held, bit b = label b (B_FX .. B_OCTUP: FX SEL ENV LFO EDIT GLO HOME SAVE ARP SEQ PLAY REC OCT- OCT+) */
void tess_core_buttons(uint32_t mask);
/* keys held, bit k = key k (F3 .. G5) */
void tess_core_keys(uint32_t mask);
/* an encoder turned n detents: role 0 SELECT, 1 ALGORITHM, 2 PRESETS, 3..6 KNOB 1..4 */
void tess_core_enc(uint32_t role, int32_t n);
/* MASTER 0..1023 */
void tess_core_master(uint32_t v);
/* what the panel shows */
typedef struct {
    uint16_t screen[TESS_SCREEN * TESS_SCREEN];   /* RGB565 (host order) */
    uint32_t lit_buttons, lit_keys, dim_buttons, dim_keys, breath_buttons, breath_keys;   /* lit_buttons bit 14: PLAY green */
    uint32_t screen_draws, playing, master;   /* master 0..1023 */
} tess_view_t;
void tess_core_view(tess_view_t *v);

/* ---- the DAW (phase 2) */
/* each host block while the DAW plays, before its audio, and once when it stops: the DAW's transport. playing: its PLAY; bpm: its tempo; beats: its position
 * at the block's start (quarter notes from its zero). With CLK INT, Tessera follows it: PLAY starts every track where it
 * would be after playing from the DAW's zero (Song Position), its beat position drives the clock (fractional tempos,
 * tempo changes, no drift), a jump (a loop, a locate) seeks again; STOP stops. has_pos 0: a transport with no position
 * (start and stop only, Tessera's own BPM). */
void tess_core_host(int playing, int has_pos, double bpm, double beats);
/* PERF's MACRO 1..4 (k 0..3), 0..100: the project's, cord sources */
uint32_t tess_core_macro(uint32_t k);
void tess_core_macro_set(uint32_t k, uint32_t v);
/* MASTER, 0..1023 */
uint32_t tess_core_master_get(void);
/* where the song is, in quarter notes from its top (the bar and beat counters); the BPM shown */
double tess_core_position(void);
uint32_t tess_core_bpm(void);

#ifdef __cplusplus
}
#endif
