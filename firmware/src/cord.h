/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* Cords (phase 2, the signal bus): up to CORD_MAX connections, each from a source on one track to a destination on
 * another (or the same), with an amount and an offset. They extend Melodee's per-track MOD matrix (mod.c), whose four
 * routes stay as they are. This header: the limits and the destinations a cord can reach (cord.c CORD_DESTS). */
#ifndef CORD_H
#define CORD_H
#include <stdint.h>

#define CORD_MAX 32u              /* the fixed maximum: the saved format and the worst-case cost are sized for it */
#define CORD_LIVE 16u             /* how many are in use now (Theo 2026-10-16); raising it changes no format */

/* How a destination is moved, and when (each one checked against where the firmware reads it):
 *   CK_VOICE  per voice, in mod.c mod_voice: the voice's pitch, cutoff, shape, gain (mod.c's MD_PITCH .. MD_AMP)
 *   CK_BLOCK  a track parameter, modulated in place for the part's block (mod.c mod_begin .. mod_end); only those
 *             read inside it: mix_part after mod_begin, track_render_audio, env_tick, track_lfo_tick, the FX send
 *   CK_DEPTH  the track LFO's depth (mod.c's MD_DEPTH: a gain on the LFO wherever it goes)
 *   CK_STEP   a Tessera setting, in place around the track's generator step (tessera_clock_step)
 *   CK_GLOBAL a project setting of the send effects (SEND FX 1..3, RETURN), for the block (fx.c fx_bus_start): one
 *             for every track, so the cord's destination track means nothing (kept 0)
 *   CK_CORD   another cord's move (M1 AMT .. M16 AMT: its AMOUNT, a fixed cord's OFFSET), for the block (cord.c
 *             cord_tick): the project's, destination track 0 as CK_GLOBAL. One level: an M AMT cord's own move is
 *             never moved
 *   CK_FILL   the project's FILL (seq.c, the steps set to FILL / NOT FILL), held while the move is 50 % or more
 *             (cord.c cord_fill_tick); destination track 0 as CK_GLOBAL
 *   CK_TEMPO  the project's tempo on CLK INT (mix.c seq_tempo_q12, cord.c cord_tempo_tick): a full move doubles it, a
 *             full move down halves it; BPM itself is left alone; destination track 0 as CK_GLOBAL
 *   CK_CC     a MIDI CC out on the destination track's MIDI OUT channel (cord.c cord_cc_tick, issue #374): its number
 *             the cord's own (cord_cc_num, OCT+ held + KNOB 3 on MODS); a track's, as CK_BLOCK (before CK_GLOBAL: the
 *             kinds are not saved, only the codes)
 *   CK_CLOCK  the track's generator clock (cord.c cord_clock_tick, seq.c seq_grid_tick): a track with a live one is
 *             stepped by the cord's edges alone, not by its DIV (always a GATE cord: its OFFSET the threshold; AMOUNT
 *             is the division, every n-th edge, its sign the edge); a track's, before CK_GLOBAL
 *   CK_BUS    a project MIX bus, BUS A or B (cord.c cord_bus_tick): its cords combined by its MODE, then its LAG, each
 *             block; read back as the source BUS A / BUS B one block later; destination track 0 as CK_GLOBAL */
enum { CK_VOICE, CK_BLOCK, CK_DEPTH, CK_STEP, CK_CC, CK_CLOCK, CK_GLOBAL, CK_CORD, CK_FILL, CK_TEMPO, CK_BUS };

/* Destination codes. FROZEN: saved cords keep them; new ones at the end, a retired one stays as a gap */
enum {
    CD_OFF,
    CD_PITCH, CD_CUTOFF, CD_SHAPE, CD_GAIN,                         /* per voice */
    CD_LEVEL, CD_PAN, CD_SPREAD,
    CD_ATTACK, CD_DECAY, CD_SUSTAIN, CD_RELEASE,
    CD_ENV_FLT, CD_ENV_PIT, CD_ENV_SHP,
    CD_LFO_RATE, CD_LFO_DEPTH, CD_LFO_PIT, CD_LFO_FLT, CD_LFO_SHP, CD_LFO_AMP,
    CD_FX_SEND,
    CD_E1, CD_E2, CD_E3, CD_E4, CD_E5, CD_E6, CD_E7, CD_E8,         /* the track engine's own eight */
    CD_TUR_PROB, CD_TUR_LEN,                                        /* Tessera, at the generator's step */
    CD_EUC_PULSES, CD_EUC_ROT,
    CD_MKV_LEAP, CD_MKV_DENS,
    CD_MRP_POS,
    CD_TW_SHIFT, CD_TW_DROP, CD_TW_HUMAN,
    CD_EV_AMOUNT, CD_FM_CONTRAST,
    CD_GRD_X, CD_GRD_Y, CD_GRD_DENS,
    CD_BNC_SPEED, CD_BNC_ANGLE, CD_AWK_PROB,
    CD_TW_BRANCH, CD_QD_PROB,
    CD_CS_Y, CD_CS_SIZE, CD_CS_DENS,
    CD_SR_START, CD_RETIRED_CL_RULE,                                /* (CELLS' RULE, cut: a gap, no destination) */
    CD_CS_AUTO,
    CD_CH_PROG,
    CD_GRD_KICK, CD_GRD_SNR, CD_GRD_HAT, CD_GRD_TOMS,
    CD_AN_VARY,
    CD_SUB_HUMAN,                 /* TIMING's HUMAN TIME */
    CD_FX1_MIX, CD_FX1_RATE, CD_FX1_CHAR,                           /* the track's inserts, FX 1..3 */
    CD_FX2_MIX, CD_FX2_RATE, CD_FX2_CHAR,
    CD_FX3_MIX, CD_FX3_RATE, CD_FX3_CHAR,
    CD_FX_SEND2, CD_FX_SEND3,
    CD_SF1_MIX, CD_SF1_RATE, CD_SF1_CHAR,                           /* the send effects (CK_GLOBAL) */
    CD_SF2_MIX, CD_SF2_RATE, CD_SF2_CHAR,
    CD_SF3_MIX, CD_SF3_RATE, CD_SF3_CHAR,
    CD_FX_RETURN,
    CD_M1_AMT,                                                      /* the cords' moves (CK_CORD): M1 AMT .. M16 AMT */
    CD_M16_AMT = CD_M1_AMT + 15,
    CD_FILL,                                                        /* the project's FILL (CK_FILL) */
    CD_TEMPO,                                                       /* the project's tempo (CK_TEMPO), issue #359 */
    CD_CC,                                                          /* a MIDI CC out (CK_CC), issue #374 */
    CD_SUB_SPLIT,                                                   /* TIMING's SPLIT */
    CD_SEQ_DIR,                                                     /* the STEP pattern's DIR (seq_dir.c) */
    CD_BUS_A, CD_BUS_B,                                             /* the project's MIX buses (CK_BUS) */
    CD_CLOCK = 109,                                                 /* the track's generator clock (CK_CLOCK); 107, 108: MOD DEPTH, PLAY */
    CD_N
};

/* The device build (MELODEE_ACCESS 1: no help cards) leaves the destinations' sentences out of flash (5 KB), as pid.c
 * leaves its help texts out; the host builds (tests, docs/PARAMETERS.md) keep them */
#if defined(MELODEE_ACCESS) && MELODEE_ACCESS
#define CD_FULL_ON 1
#define CD_FULL(f)
#else
#define CD_FULL_ON 0
#define CD_FULL(f) , f
#endif
typedef struct {
    uint8_t code, kind;
    uint8_t md;                   /* CK_VOICE / CK_DEPTH: mod.c's MD_* it shares; else 0 */
    uint16_t pid;                 /* CK_BLOCK / CK_STEP / CK_GLOBAL: the parameter (pid.h); else PID_NONE */
    const char *name;             /* as a cord shows it: <= 7 characters */
#if !CD_FULL_ON
    const char *full;             /* for the help and the reference (host builds: tests, docs/PARAMETERS.md) */
#endif
} cord_dest_t;

const cord_dest_t *cord_dest(uint32_t code);        /* 0: no such destination (a gap, past the end) */

/* Source codes. FROZEN as the destinations. A source is a track's (its LFO, its latest note ..): one value per
 * track and block, so a cord crosses tracks (on its own track's voice destinations, ENV VEL KEY RAND: each voice its
 * own, as MOD's routes). Values Q15: bipolar -32768..32767 (LFO, KEY, RAND, S&H, SLEW, NOTE, DRIFT, SLOTH, OCHD), else 0..32767
 * (CV too) */
enum {
    CS_OFF,
    CS_LFO, CS_ENV, CS_VEL, CS_KEY, CS_RAND, CS_MODW, CS_AT, CS_EXPR, CS_SH, CS_SLEW,   /* mod.c's MS_LFO .. MS_SLEW */
    CS_GATE,                      /* the track's generator: 1 while its note sounds (Tessera) */
    CS_NOTE,                      /* the generator's last note, as KEY (C4 = 0, an octave = 0.19) */
    CS_CV,                        /* the generator's CV (gen.c): Turing its register, Euclid and Morph a ramp, Markov the
                                   * degree, Grids the kick's chance, Bounce the ball's height, Awake A's value; 0..1 */
    CS_DRIFT,                     /* a slow wander of the track's own, a new point every 12 s or so */
    CS_TORPOR, CS_APATHY, CS_INERTIA,   /* the track's SLOTH (organic.c): chaos, ~17 min, ~1 min, ~3 min a turn */
    CS_OCHD1, CS_OCHD2, CS_OCHD3, CS_OCHD4, CS_OCHD5, CS_OCHD6, CS_OCHD7, CS_OCHD8,   /* its OCHD: 64 s .. 2.4 s */
    CS_ENERGY,                    /* the project's tide (ARC, energy.c), the same for every track: -1 sparse .. 1 full */
    CS_MACRO1, CS_MACRO2, CS_MACRO3, CS_MACRO4,   /* GLO > PERF's knobs, the project's: 0..1 */
    CS_BUSA, CS_BUSB,             /* the project's MIX buses as their cords make them, one block late: -1..1 */
    CS_EOC,                       /* the track's LFO cycle start (end of cycle) as a gate: 1 from a wrap of the LFO to an
                                   * eighth of the cycle, else 0; a TRIG / phase reset is not a wrap. Added after the
                                   * project's sources: a track's source all the same (CS_PROJECT) */
    CS_N
};
#define CS_PROJECT(s) ((s) >= CS_ENERGY && (s) != CS_EOC)   /* the project's sources (no track), not a track's */

typedef struct {                  /* one cord, as saved (6 bytes: uni rides in src_trk's top two bits) */
    uint8_t src, src_trk;         /* CS_*, 0..3 */
    uint8_t dst, dst_trk;         /* CD_*, 0..3 */
    int8_t amount;                /* -100..100 %: at a full source, the destination's whole range (pitch: an octave) */
    int8_t offset;                /* -100..100 %: added whatever the source (CS_OFF: a fixed setting) */
    uint8_t uni;                  /* CU_*: how the source is read */
} cord_t;
/* a cord's uni: CU_BI the source as it is (+-), CU_UNI a bipolar source folded to 0..1 (+: one way from the knob),
 * CU_GATE the source as a gate: all (1) while it is over the threshold OFFSET (-100..100 % of its range), else 0 (the
 * OFFSET then moves nothing), CU_HOLD the source as it is (as BI), sampled once a bar of the destination's track and
 * held to the next (cord_hold_bar; saved as BI, the project's part TSP_MHOLD marks it: older firmware plays it live) */
enum { CU_BI, CU_UNI, CU_GATE, CU_HOLD };

/* MIX buses (CK_BUS, CS_BUSA): each bus's MODE, how its cords combine: SUM (clamped to -1..1), MAX, MIN, DIFF (|a - b|
 * of its first two cords); and LAG, a slew on the result, the time a whole 0..1 move takes (OFF none) */
enum { CB_SUM, CB_MAX, CB_MIN, CB_DIFF, CB_MODES };
#define CB_LAGS 12u               /* LAG: OFF, 10 ms .. 20 s (pid.c TX_N_BUSLAG; saved: append only) */
/* setting k: 0 BUS A MODE, 1 its LAG, 2 BUS B MODE, 3 its LAG (TX_BUSA_MODE ..); set: clamped to its range */
uint32_t cord_bus_get(uint32_t k);
void cord_bus_set(uint32_t k, int32_t v);
int32_t cord_bus_now(uint32_t b);                   /* bus b's value now (Q15), as the source BUS A / B reads it */

const char *cord_src_name(uint32_t code);           /* <= 7 characters; 0: no such source */
int cord_set(uint32_t i, const cord_t *c);          /* i < CORD_LIVE; out of range: refused (0), nothing changed */
/* CC OUT: cord i's CC number (0..119; 1, the mod wheel, until set), and setting it (cord_reset: all 1 again) */
uint32_t cord_cc_num(uint32_t i);
void cord_cc_set(uint32_t i, uint32_t n);
const cord_t *cord_get(uint32_t i);
void cord_hold_bar(uint32_t tr);                    /* seq.c: track tr's bar begins (its HOLD mods sample) */
uint32_t cord_hold_mask(void);                      /* bit i: cord i is HOLD (the project's part TSP_MHOLD) */
void cord_hold_load(uint32_t m);                    /* .. and back, after cord_unpack */
void cord_clear(void);                              /* every cord off */
int32_t cord_move(uint32_t i);                      /* cord i's move as its M AMT cords make it now: -100..100 % */
uint32_t cord_count(void);                          /* cords doing something (a destination and an amount or offset) */
uint32_t cord_adopt(void);
void cord_tick(void);                               /* an audio block (mix.c mix_block): DRIFT's clock, SLOTH, OCHD */
void cord_defaults(void);                           /* a new project's mods (project.c): the default patch */
void cord_fx_bus(int16_t *g);                       /* fx.c fx_bus_start: the send effects' settings as moved now */
void cord_sound_load(uint32_t tr);                  /* a sound loads on track tr (ui.c load_begin) */                          /* MOD's routes into cords (main loop): the routes left, no room */

#endif
