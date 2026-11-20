/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Tessera's core API: the one front-end interface every build but the FM-1's own runs Tessera through (the web
 * simulator, the plugin, the Daisy for Andamento, the cross-target tests). Events in (keys, buttons, knobs, MIDI,
 * the transport), audio out, the screen, the project as a .tsp record, the parameters by their central ID (pid.h).
 * platform/core/tess_core.c implements it over the shared build (platform/core/core_unit.c).
 *
 * The firmware keeps its state in file-level globals: one core per process (or per loaded copy of the code). Not
 * thread-safe: the caller serialises every call (on the Daisy, a call from the main loop with the audio interrupt
 * guarded, or from the audio callback itself). */
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- the core */
enum { TESS_CORE_BLOCK = 32 };       /* frames of tess_core_block: Tessera's control block (CTL) */
/* the core's own rate: 44,100 as the FM-1 (48,000 in a build made at MELODEE_FS=48000, as the Daisy's) */
uint32_t tess_core_rate(void);
/* power-on, once: the device's start (its BPM, scales, the default ink), stopped. The web simulator and the plugin
 * then load their demos (tess_core_slot_put, tess_core_slot_load) */
void tess_core_boot(void);

/* ---- audio out. Audio time drives the device's clock, so an offline render plays as real time does. Use one of
 * the two, not both. */
/* n stereo frames (n <= 1024), the device running until they exist. stems: 0, or four buffers of n that get tracks
 * 1..4 alone (the FM-1's USB stems: each track's sound after its level, before pan and the bus FX, mono); asked for
 * once, the stems are made from then on (a build with TESS_PLUGIN_IO only; else 0s) */
void tess_core_render(float *l, float *r, float *const *stems, uint32_t n);
/* TESS_CORE_BLOCK stereo frames, interleaved L R, Q15 in int32 (mix_block's output): for a target that runs in
 * fixed blocks from its audio interrupt (the Daisy) */
void tess_core_block(int32_t *out);
/* the frames rendered so far (wraps) */
uint32_t tess_core_frames(void);

/* ---- events in */
/* keys held, bit k = key k (F3 .. G5; 7 = C4) */
void tess_core_keys(uint32_t mask);
/* buttons held, bit b = label b (B_FX .. B_OCTUP: FX SEL ENV LFO EDIT GLO HOME SAVE ARP SEQ PLAY REC OCT- OCT+) */
void tess_core_buttons(uint32_t mask);
/* an encoder (knob) turned n detents: role 0 SELECT, 1 ALGORITHM, 2 PRESETS, 3..6 KNOB 1..4 */
void tess_core_enc(uint32_t role, int32_t n);
/* MASTER 0..1023 (the FM-1's pot, squared) */
void tess_core_master(uint32_t v);
uint32_t tess_core_master_get(void);
/* a MIDI channel message in, as from USB MIDI: 0 = the queue was full */
uint32_t tess_core_midi(uint32_t status, uint32_t d1, uint32_t d2);
/* the transport: 1 play (from where the song is), 0 stop. Does nothing if it is already so */
void tess_core_transport(int play);
uint32_t tess_core_playing(void);

/* ---- MIDI out (Settings > NOTES OUT; each track on its own channel, GLO > MIDI OUT), as USB MIDI packets: the
 * next one whose frame (tess_core_frames' count) has been rendered. 0 = none. A build with TESS_PLUGIN_IO only */
uint32_t tess_core_midi_out(uint32_t *pkt, uint32_t *frame);

/* ---- the screen and the panel */
enum { TESS_NB = 14, TESS_NE = 7, TESS_KEYS = 27, TESS_SCREEN = 240 };
enum { TESS_PIX_RGB565 = 1 };
/* the FM-1's screen as the UI last drew it: w x h pixels, row by row, in format (RGB565, host order). pixels 0: this
 * build keeps no copy of it (the Daisy: Andamento draws its own screens, platform/andamento) */
typedef struct {
    uint32_t w, h, format, draws;    /* draws: pixels sent so far (changes when the screen does) */
    const uint16_t *pixels;
} tess_screen_t;
void tess_core_screen(tess_screen_t *s);
/* what the FM-1 panel shows */
typedef struct {
    uint16_t screen[TESS_SCREEN * TESS_SCREEN];   /* RGB565 (host order) */
    uint32_t lit_buttons, lit_keys, dim_buttons, dim_keys, breath_buttons, breath_keys;   /* lit_buttons bit 14: PLAY green */
    uint32_t screen_draws, playing, master;   /* master 0..1023 */
} tess_view_t;
void tess_core_view(tess_view_t *v);

/* ---- the project, as a .tsp record (TSP1: byte for byte what SAVE > PROJECT writes on the FM-1) */
/* the music as it is now: its length, 0 = failed/too big */
uint32_t tess_core_save(uint8_t *out, uint32_t cap);
/* a record in, as SAVE > PROJECT loads one: 0 loaded, 1 not a record this firmware reads */
uint32_t tess_core_load(const uint8_t *in, uint32_t len);
/* the largest record */
uint32_t tess_core_tsp_max(void);
/* the project slots A..D (0..3), as the FM-1 keeps them (no flash here: they last as long as the core) */
uint32_t tess_core_slot_get(uint32_t slot, uint8_t *out, uint32_t cap);   /* its length, 0 = empty */
uint32_t tess_core_slot_put(uint32_t slot, const uint8_t *in, uint32_t len);   /* 0 done, 1 not a record */
void tess_core_slot_load(uint32_t slot);                                /* as SAVE > PROJECT > LOAD */

/* ---- parameters by central ID (firmware/src/pid.h: a track's P_* 0x000.., the project's G_* 0x100.., Tessera's TX_*
 * 0x200..). track 0..3 for a per-track one (else ignored) */
typedef struct {
    const char *name;                /* "TURING LEN", "LFO RATE" */
    int32_t min, max, def;
    uint32_t per_track;              /* 1: one value per track */
} tess_param_t;
/* the n-th parameter in ID order (n < tess_core_param_count): its ID; info filled. PID 0xFFFF past the end */
uint32_t tess_core_param_count(void);
uint32_t tess_core_param_at(uint32_t n, uint32_t track, tess_param_t *info);
/* 1 and info filled if pid is a parameter (actions such as LOAD are not) */
uint32_t tess_core_param_info(uint32_t pid, uint32_t track, tess_param_t *info);
int32_t tess_core_param_get(uint32_t pid, uint32_t track);
/* the stored value, clamped, as a project load sets it: 0 not settable (an action, a device setting) */
uint32_t tess_core_param_set(uint32_t pid, uint32_t track, int32_t v);

/* ---- a DAW's transport (a build with TESS_HOST_CLOCK). Each host block while the DAW plays, before its audio, and
 * once when it stops. playing: its PLAY; bpm: its tempo; beats: its position at the block's start (quarter notes
 * from its zero). With CLK INT, Tessera follows it: PLAY starts every track where it would be after playing from the
 * DAW's zero (Song Position), its beat position drives the clock (fractional tempos, tempo changes, no drift), a jump
 * (a loop, a locate) seeks again; STOP stops. has_pos 0: a transport with no position (start and stop only, Tessera's
 * own BPM). */
void tess_core_host(int playing, int has_pos, double bpm, double beats);
/* PERF's MACRO 1..4 (k 0..3), 0..100: the project's, cord sources */
uint32_t tess_core_macro(uint32_t k);
void tess_core_macro_set(uint32_t k, uint32_t v);
/* where the song is, in quarter notes from its top (the bar and beat counters); the BPM shown */
double tess_core_position(void);
uint32_t tess_core_bpm(void);

#ifdef __cplusplus
}
#endif
