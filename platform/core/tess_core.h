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
/* the build's tracks: 4 as the FM-1, or 8 (CORE_TRACKS 8: the plugin, Andamento) */
uint32_t tess_core_tracks(void);
/* power-on, once: the device's start (its BPM, scales, the default ink), stopped. The web simulator and the plugin
 * then load their demos (tess_core_slot_put, tess_core_slot_load) */
void tess_core_boot(void);

/* ---- audio out. Audio time drives the device's clock, so an offline render plays as real time does. Use one of
 * the two, not both. */
/* n stereo frames (n <= 1024), the device running until they exist. stems: 0, or tess_core_tracks() buffers of n
 * that get each track alone (the FM-1's USB stems: each track's sound after its level, before pan and the bus FX,
 * mono); asked for once, the stems are made from then on (a build with TESS_PLUGIN_IO only; else 0s) */
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
/* per-note expression (a build with TESS_MPE; else nothing, 0): kind TESS_EXPR_PRESSURE or TESS_EXPR_SLIDE (MPE's
 * CC74, a CLAP note's BRIGHTNESS), v 0..127, for the notes that came in on channel ch (-1: every channel) and key (-1:
 * every key of the channel). Queued with the MIDI, so it reaches the note a note-on just before it started. Each note's
 * own AT and SLIDE in the mods (a note not from MIDI keeps the track's). 0 = the queue was full */
enum { TESS_EXPR_PRESSURE, TESS_EXPR_SLIDE };
uint32_t tess_core_note_expr(uint32_t kind, int32_t ch, int32_t key, uint32_t v);
/* the transport: 1 play (from where the song is), 0 stop. Does nothing if it is already so */
void tess_core_transport(int play);
uint32_t tess_core_playing(void);

/* ---- MIDI out (Settings > NOTES OUT; each track on its own channel, GLO > MIDI OUT), as USB MIDI packets: the
 * next one whose frame (tess_core_frames' count) has been rendered. 0 = none. A build with TESS_PLUGIN_IO only */
uint32_t tess_core_midi_out(uint32_t *pkt, uint32_t *frame);
/* note-offs for every note MIDI out still has on (they come out of tess_core_midi_out after the next render) */
void tess_core_notes_off(void);

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
/* the screen's zones (a build whose profile has TESS_HAS_POINTER, the plugin; else none): what a mouse can use, where
 * the page drew it, in screen pixels. A front end finds the first zone under the mouse (the list is in that order) and
 * hands it back to tess_core_zone_do as it got it. KNOB: a value (turn: the wheel, a drag; double-click: its default);
 * ROW: a list's row (click: the cursor there; double-click: there and OCT+; turn: its value, or the list scrolls when
 * the row is the value); KEY: OK, BACK, YES, NO (click); TRACK: HOME's column (click: that track) */
enum { TESS_ZONES = 48 };
enum { TESS_ZONE_KNOB = 1, TESS_ZONE_ROW, TESS_ZONE_KEY, TESS_ZONE_TRACK };
typedef struct {
    uint16_t x, y, w, h;
    uint8_t kind, group;
    int8_t role, vrole;              /* (the core's: the knob it turns, a row's value knob) */
    int16_t row;                     /* (the core's: a row's place, a key's button label, a track) */
    char name[16], value[16];        /* what it shows: a front end's hover */
} tess_zone_t;
enum { TESS_ZA_TURN, TESS_ZA_CLICK, TESS_ZA_DOUBLE };
/* do what the mouse did on zone z: TURN n detents (+ clockwise / up), CLICK, DOUBLE. A zone no longer on the screen
 * is let go. Its steps run a millisecond each as the device plays (a row three away: three detents) */
void tess_core_zone_do(const tess_zone_t *z, uint32_t action, int32_t n);
/* what the FM-1 panel shows */
typedef struct {
    uint16_t screen[TESS_SCREEN * TESS_SCREEN];   /* RGB565 (host order) */
    uint32_t lit_buttons, lit_keys, dim_buttons, dim_keys, breath_buttons, breath_keys;   /* lit_buttons bit 14: PLAY green */
    uint32_t screen_draws, playing, master;   /* master 0..1023 */
    uint32_t ink;                    /* MENU > COLOR's choice now (tess_core_ink_at's i) */
    uint32_t track;                  /* the selected track, 0 .. tess_core_tracks() - 1 (8 tracks: its bank, track & 4) */
    uint32_t zone_count, zone_gen;   /* the screen's zones (above), and a count that changes when they do */
    tess_zone_t zones[TESS_ZONES];
} tess_view_t;
void tess_core_view(tess_view_t *v);
/* the selected track (0 .. tess_core_tracks() - 1), and selecting one as KNOB 1 on TRACKS does (its sound, pages and
 * pattern; 8 tracks: the panel's bank of four with it). The selection happens at the UI's next pass */
uint32_t tess_core_track(void);
void tess_core_track_select(uint32_t t);

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
/* the device's MIDI and tuning setup (ROUT, DRUM's channel, NOTES OUT, TUNE), which a project load leaves alone: for a
 * front end to keep with its own state (the plugin's, in the DAW project). save: its length, 0 = cap too small;
 * load: 0 done, 1 not a setup this core reads */
uint32_t tess_core_setup_save(uint8_t *out, uint32_t cap);
uint32_t tess_core_setup_load(const uint8_t *in, uint32_t len);

/* ---- parameters by central ID (firmware/src/pid.h: a track's P_* 0x000.., the project's G_* 0x100.., Tessera's TX_*
 * 0x200..). track 0..3 for a per-track one (else ignored) */
typedef struct {
    const char *name;                /* "TURING LEN", "LFO RATE" */
    int32_t min, max, def;
    uint32_t per_track;              /* 1: one value per track */
    uint32_t settable;               /* 1: tess_core_param_set sets it (0: a device setting, or no range) */
    int32_t span_min, span_max;      /* the widest range it can have: an engine's knob over every engine (ENGINE 1..8
                                      * change range with the track's engine), else min..max */
} tess_param_t;
/* the n-th parameter in ID order (n < tess_core_param_count): its ID; info filled. PID 0xFFFF past the end */
uint32_t tess_core_param_count(void);
uint32_t tess_core_param_at(uint32_t n, uint32_t track, tess_param_t *info);
/* 1 and info filled if pid is a parameter (actions such as LOAD are not) */
uint32_t tess_core_param_info(uint32_t pid, uint32_t track, tess_param_t *info);
int32_t tess_core_param_get(uint32_t pid, uint32_t track);
/* the stored value, clamped, as a project load sets it: 0 not settable (an action, a device setting) */
uint32_t tess_core_param_set(uint32_t pid, uint32_t track, int32_t v);
/* the value v as the FM-1's screen shows it ("SAW", "1/16", "250ms", "42%"), into out (cap bytes, terminated): its
 * length, 0 if pid is not a parameter */
uint32_t tess_core_param_text(uint32_t pid, uint32_t track, int32_t v, char *out, uint32_t cap);

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
/* the Mono look's ink by name, as MENU > COLOR sets it ("YELLOW", "HOT PINK": tools/gen_ui_palettes.py MONO_INK): 1 if
 * set, 0 for an unknown name or a build without the Mono look. A device setting: not in the project */
uint32_t tess_core_ink(const char *name);
/* MENU > COLOR's choices: how many (0 without the Mono look); for i below that, choice i's name and its four track
 * colours as 0xRRGGBB (alike for one ink; MOSAIC, SUNSET, OCEAN, NEON differ). name or rgb may be 0 */
uint32_t tess_core_ink_at(uint32_t i, const char **name, uint32_t rgb[4]);

/* ---- GENERATE (a build with TESS_GENERATE: firmware/src/generate.c): a whole piece in one press, from a style and a
 * seed. The piece is an ordinary project (every setting one a player could make), from a new project: the same style,
 * seed and variation give the same .tsp. Playing, it plays on; stopped, it stays stopped */
enum { TESS_GEN_KEEP_TEMPO = 1 };    /* tess_core_generate's flags: the tempo as it is (a DAW or MIDI clock drives) */
/* the styles: how many (0: a build without GENERATE), and style s's name ("AUTUMN", "WOZZA" ..) */
uint32_t tess_core_gen_styles(void);
const char *tess_core_gen_style(uint32_t s);
/* a new piece: style, seed (0 reads as 1), variation 0..100 (how wide each pick is in its range; 100 as the styles were
 * tuned). The project before it is kept for tess_core_gen_swap. 0 made, 1 refused */
uint32_t tess_core_generate(uint32_t style, uint32_t seed, uint32_t variation, uint32_t flags);
/* before / after: the project before the last piece and what is there now change places. 0 done, 1 nothing kept */
uint32_t tess_core_gen_swap(void);
/* the last piece's style, seed and variation (each may be 0): 1, or 0 none yet */
uint32_t tess_core_gen_last(uint32_t *style, uint32_t *seed, uint32_t *variation);

#ifdef __cplusplus
}
#endif
