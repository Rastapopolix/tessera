/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
#ifndef TESSERA_H
#define TESSERA_H

#include <stdint.h>
#include "gen.h"

#define TESSERA_TRACKS 4

/* Engine types per track */
enum {
    TESS_ENG_OFF = 0,     /* Track native step sequencer */
    TESS_ENG_TURING,      /* 16-bit shift register */
    TESS_ENG_EUCLID,      /* Euclidean rhythm generator */
    TESS_ENG_MARKOV,      /* 1st-order Markov pitch matrix */
    TESS_ENG_MORPH,       /* Vector/Morph interpolation */
    TESS_ENG_GRIDS,       /* Grids' drum map: kick, snare, hat */
    TESS_ENG_BOUNCE,      /* a ball in a box: a wall hit plays */
    TESS_ENG_AWAKE,       /* two loops added (norns' Awake) */
    TESS_ENG_POLY,        /* four rhythms advance four steps (EuroPi's Polyrhythmic Sequencer) */
    TESS_ENG_QUAD,        /* a row each of a 4 x 4 grid the tracks share (norns' initenere) */
    TESS_ENG_CONSTEL,     /* a ship catches the stars of a scrolling sky into a loop (norns' Constellation) */
    TESS_ENG_SERIES,      /* Nørgård's infinity series (norns' nørgård) */
    TESS_ENG_CHORD,       /* the chord pad: a progression's chords in the track's scale (QNT CHRD: the others follow) */
    TESS_ENG_ANSWER,      /* the keys' phrase answered with fading variations and echoes (after Zen-1; answer.c) */
    TESS_ENG_COUNT
};
/* CELLS (an elementary automaton, cut 2026-10-20 as too like the others) had 12 in blocks up to version 11: such a
 * track loads as TURING (tess_v11_track); CHORD has 12 from version 12 */
#define TESS_ENG_RETIRED_CELLS 12u

/* The generators' own state lives in gen.h (the names below kept for the adapter and the UI) */
typedef gen_turing_t tessera_turing_t;
typedef gen_euclid_t tessera_euclid_t;
typedef gen_markov_t tessera_markov_t;
typedef gen_morph_t  tessera_morph_t;
typedef gen_grids_t  tessera_grids_t;
typedef gen_bounce_t tessera_bounce_t;
typedef gen_awake_t  tessera_awake_t;
typedef gen_poly_t   tessera_poly_t;
typedef gen_quad_t   tessera_quad_t;
typedef gen_constel_t tessera_constel_t;
typedef gen_series_t tessera_series_t;
typedef gen_chord_t  tessera_chord_t;

/* ANSWER (answer.c): the notes played on the track's keys (up to 8) are its phrase; it answers it, after a gap, with
 * variations (vary.c), echoes, each answer softer until it forgets. The saved part; what plays now is tess_an */
#define TESS_AN_NOTES 8u
#define TESS_AN_AT_MAX 63u                          /* a note's step in the phrase: 0..63 */
enum { TESS_AN_ASPLAYED, TESS_AN_REVERSE, TESS_AN_SHUFFLE, TESS_AN_ORDERS };
enum { TESS_AN_SHORT, TESS_AN_MID, TESS_AN_LONG, TESS_AN_LENS };   /* LEN: the four loops at the top of FADE */
typedef struct {
    uint8_t vary;          /* 0..100: how much an answer changes (0: as played) */
    uint8_t echo;          /* 0..100: the chance a note comes back once more, softer */
    uint8_t fade;          /* 0..100: answers before it forgets, 1 .. 16; 100 never: the four loops */
    uint8_t order;         /* TESS_AN_*: the notes' order in an answer */
    uint8_t shift;         /* 0..7: an answer moved up to this many scale degrees up or down (0: none) */
    uint8_t space;         /* 0..100: the gap before each answer, 0 .. 4 bars */
    uint8_t len;           /* TESS_AN_SHORT .. LONG: the loops' lengths in steps (5 7 11 13, 13 17 23 29, 23 29 31 37) */
    uint8_t n;             /* the phrase's notes, 0..8 (0: nothing to answer yet) */
    uint8_t note[TESS_AN_NOTES], vel[TESS_AN_NOTES], at[TESS_AN_NOTES];   /* each note, its velocity, its step */
} tessera_answer_t;

/* 5. Evolve (per track, engine-independent): phrase-level mutation.
 * LOCK has no storage of its own for Turing: it is 100 - turing.prob. */
typedef struct {
    uint8_t  every;       /* 0 = OFF, 1..5 -> 1/2/4/8/16 bars between mutations */
    uint8_t  amount;      /* 0..100: how much changes at each mutation */
    uint8_t  bar_cnt;     /* bars elapsed since the last mutation */
    uint16_t step_cnt;    /* steps elapsed inside the current bar */
    uint8_t  flash;       /* steps left to show the "just mutated" flash */
    uint8_t  lock;        /* 0..100 deja vu for Markov / Morph (Turing: 100 - prob; Euclid: n/a) */
    uint8_t  lp_pos;      /* next position in the phrase loop */
    uint8_t  pos_now;     /* position of the step being played */
    uint8_t  rec;         /* 2 = armed, 1 = recording this pass, 0 = replaying (Turing: 2 = capture base) */
    uint16_t base_reg;    /* Turing: the register captured at the start of the phrase */
    uint8_t  form;        /* 0 = OFF, 1..6 = AB ABAC ABCB AABA ABCA AABC (see TESS_FORMS) */
    uint8_t  contrast;    /* 0..100: how different a new section is from the one before it */
    uint8_t  sec;         /* current section: 0 = A, 1 = B, 2 = C */
    uint8_t  form_pos;    /* position inside the form */
    uint8_t  visited;     /* bit s: section s exists */
    uint8_t  valid;       /* bit s: section s's recorded phrase is complete (Markov / Morph) */
    int8_t   shift;       /* TWEAK: semitones added to the track's notes; while following, scale degrees from the
                           * followed note (+2 a third, +4 a fifth, -7 an octave down in a seven-note scale) */
    uint8_t  drop;        /* TWEAK: 0..100 chance a hit is dropped, any engine */
    uint8_t  human;       /* TWEAK: 0..100 velocity and gate variation */
    uint8_t  follow;      /* TWEAK: 0 = off, 1..4 = play that track's pitch (a generator's or a STEP track's sounding
                           * note), SHIFT scale degrees away, in this track's scale */
    int8_t   flourish;    /* FORM: semitones added on the last beat of every phrase (EVERY bars), 0 = off */
    uint8_t  branch;      /* TWEAK 2: 0..100 chance a hit plays on track br_to instead (MI Branches' coin) */
    uint8_t  br_to;       /* TWEAK 2: 0 = off, 1..4 = the track a branched hit plays on (never this one) */
    uint8_t  br_mode;     /* TWEAK 2: TESS_BR_COIN, _TOGGLE, _LATCH (see tessera.c tess_branch) */
    uint8_t  br_side;     /* 1 = hits go to br_to now (TOGGLE, LATCH; not saved) */
    uint8_t  logic;       /* TWEAK 2: TESS_LG_OFF, _AND, _OR, _XOR, _NOT against track lg_with's hits */
    uint8_t  lg_with;     /* TWEAK 2: 0 = none, 1..4 = the track LOGIC reads (never this one) */
    uint8_t  cond;        /* TWEAK 2: TESS_COND_OFF .. TESS_COND_NFIRST, the bars a hit may play in */
    uint8_t  cond_bar;    /* bars since PLAY or the section's start, mod 8 (COND; not saved) */
    uint8_t  cond_first;  /* 1 until that first bar ends (COND FIRST; not saved) */
} tessera_evolve_t;

enum { TESS_BR_COIN, TESS_BR_TOGGLE, TESS_BR_LATCH, TESS_BR_COUNT };
enum { TESS_LG_OFF, TESS_LG_AND, TESS_LG_OR, TESS_LG_XOR, TESS_LG_NOT, TESS_LG_COUNT };
enum { TESS_COND_OFF, TESS_COND_1_2, TESS_COND_2_2, TESS_COND_1_4, TESS_COND_2_4, TESS_COND_3_4, TESS_COND_4_4,
       TESS_COND_1_8, TESS_COND_8_8, TESS_COND_FIRST, TESS_COND_NFIRST, TESS_COND_COUNT };

/* One remembered section (A, B or C) of a track's form */
#define TESS_SV_STEPS 64u                           /* STEP EVOLVE's changes: one a step (NSTEP) */
#define TESS_SV_LOCK0 50u                           /* STEP EVOLVE's LOCK and EVERY (a bar) to start; AMOUNT 0: OFF */
#define TESS_SV_EVERY0 1u
typedef struct {
    uint16_t reg;         /* Turing: base register; Morph: snapshot A; Grids: x, y << 8 */
    uint8_t  p0, p1;      /* Euclid: pulses, rot; Markov: tension; Morph: morph_pos */
    uint8_t  note[32];    /* Markov / Morph phrase: bit 7 = hit, bits 0..6 = note */
    uint8_t  vel[16];     /* 4 bits per step */
} tessera_sec_t;

/* Master Tessera Engine State */
typedef struct {
    uint8_t          mode[TESSERA_TRACKS];
    tessera_turing_t turing[TESSERA_TRACKS];
    tessera_euclid_t euclid[TESSERA_TRACKS];
    tessera_markov_t markov[TESSERA_TRACKS];
    tessera_morph_t  morph[TESSERA_TRACKS];
    tessera_grids_t  grids[TESSERA_TRACKS];
    tessera_bounce_t bounce[TESSERA_TRACKS];
    tessera_awake_t  awake[TESSERA_TRACKS];
    tessera_poly_t   poly[TESSERA_TRACKS];
    tessera_quad_t   quad[TESSERA_TRACKS];    /* a row each of QUAD's grid (whatever the track's MODE) */
    uint8_t          qcol[4];                 /* QUAD's columns: MOVE (GEN_QC_*) | EVERY (0..6) << 2 (saved a column
                                               * a track record) */
    uint8_t          qcol_cnt[4];             /* the steps since a column last moved (not saved) */
    uint8_t          qcol_sel;                /* the column OCT+ held on GEN edits, 0..3 (the page's, not saved) */
    tessera_constel_t constel[TESSERA_TRACKS];
    tessera_series_t series[TESSERA_TRACKS];
    tessera_chord_t  chord[TESSERA_TRACKS];
    tessera_answer_t answer[TESSERA_TRACKS];
    uint16_t         chord_pc[TESSERA_TRACKS]; /* CHORD: the pitch classes of the chord it plays (bit 0 C), 0 none yet
                                               * (QNT CHRD's: tessera_chord_pcs; not saved) */
    tessera_evolve_t evolve[TESSERA_TRACKS];
    tessera_sec_t    sec[TESSERA_TRACKS][3];
    uint8_t          last_note[TESSERA_TRACKS];
    uint8_t          follow_mem[TESSERA_TRACKS];   /* FOLLOW of a STEP track: its last sounding note + 1, 0 none yet */
    /* FOLLOW's HOLD (TWEAK, OCT+ held, KNOB 4): 0 off (the followed note on each of this track's own hits, as before),
     * 1..4 that track's hits sample the followed note and this track plays it until the next one: a sample and hold.
     * Not in the packed block: the record's HOLD part */
    uint8_t          follow_hold[TESSERA_TRACKS];
    uint8_t          hold_mem[TESSERA_TRACKS];     /* HOLD: the sampled note + 1, 0 none yet (not saved) */
    /* FOLLOW's HOW and RHYTHM (TWEAK, OCT+ held, KNOBs 2 and 3): TESS_FH_*, TESS_FR_*; 0 (SAME, OWN) as before. Not in
     * the packed block: the record's FMODE part */
    uint8_t          follow_how[TESSERA_TRACKS];
    uint8_t          follow_rhy[TESSERA_TRACKS];
    uint8_t          f_anchor[TESSERA_TRACKS];     /* CONTRA: the first note followed + 1, 0 none yet (not saved) */
    uint8_t          f_seen[TESSERA_TRACKS];       /* RHYTHM: the source's f_cnt when this track last looked */
    uint8_t          f_cnt[TESSERA_TRACKS];        /* the notes this track has played since PLAY, mod 256 */
    uint8_t          f_hist[TESSERA_TRACKS][16];   /* CANON: its last 16 notes + 1 (slot f_cnt % 16), 0 none */
    uint8_t          active_gate[TESSERA_TRACKS];
    uint8_t          hit[TESSERA_TRACKS];     /* its last step played on its own voice (LOGIC; seq.c sets a STEP track's) */
    uint8_t          br_in[TESSERA_TRACKS];   /* its seq_notes hold notes another track BRANCHed onto it */
    uint8_t          org_speed[TESSERA_TRACKS];   /* SLOTH SPEED 0..100 (organic.c), 50: the module's speeds */
    uint8_t          org_rate[TESSERA_TRACKS];    /* OCHD RATE 0..100, 50: 64 s .. 2.4 s a turn */
    int16_t          cv[TESSERA_TRACKS];      /* the generator's CV of its last step (gen.c; the cords' source CV) */
    uint16_t         seed[TESSERA_TRACKS];    /* SEED 1..128 (saved by 1.1.3-1.1.5: up to 9999), 0 = not picked yet
                                               * (tessera.c tess_seed_fresh) */
    uint32_t         rnd[TESSERA_TRACKS];     /* each track's random stream, from its seed (gen.c gen_stream) */
    uint16_t         roll[TESSERA_TRACKS];    /* ROLL turned while playing: the seed for the next bar line, 0 none
                                               * (tessera.c tessera_seed_turn; not saved) */
    uint8_t          nl_mode[TESSERA_TRACKS]; /* NOTE LENGTH (note_len.c): 0 STEP (the GATE), 1 FIXED, 2 HIT */
    uint8_t          nl_len[TESSERA_TRACKS];  /* FIXED's length: 0..5 = 1/32 1/16 1/8 1/4 1/2 1 bar */
    uint8_t          nl_keys[TESSERA_TRACKS]; /* FIXED: 1..4 notes held at once */
    /* STEP EVOLVE (a STEP track's EVOLVE page; vary.c): its steps played with changes that come and go */
    uint8_t          sv_lock[TESSERA_TRACKS];   /* 0..100: the chance a change is a step put back as written */
    uint8_t          sv_every[TESSERA_TRACKS];  /* 0 OFF, 1..5: 1 2 4 8 16 bars between rounds of changes */
    uint8_t          sv_amount[TESSERA_TRACKS]; /* 0..100: changes a round, 1 a quarter; 0 none (as written) */
    uint8_t          sv_how[TESSERA_TRACKS];    /* HOW: TESS_SV_DRIFT (as before), _ORNAMENT, _SERIAL; not in the packed
                                                 * block: the record's HOW part */
    /* SUB (the TIMING page; seq.c seq_sub.c): a generator's notes again within their step */
    uint8_t          sub_ratch[TESSERA_TRACKS];  /* 0..7: RATCH x1..x8, the hits a step (0 x1: as before) */
    uint8_t          sub_chance[TESSERA_TRACKS]; /* 0..100: the chance a step ratchets (100: every step) */
    uint8_t          sub_shape[TESSERA_TRACKS];  /* SUB_EVEN / ACCEL / DECEL: where a ratchet's hits fall, any track's
                                                  * (STEP ones too); not in the packed block: the record's SUB part */
    uint8_t          sub_human[TESSERA_TRACKS];  /* 0..TESS_HUMAN_MS: HUMAN TIME, a step up to this many ms late, any
                                                  * track's (STEP ones too); not in the packed block: the HUMAN part */
    uint8_t          sub_split[TESSERA_TRACKS];  /* 0..100: SPLIT, the chance a step plays at half its division (0: off), */
    uint8_t          sub_split_how[TESSERA_TRACKS]; /* TESS_SPLIT_*: any track's; not in the packed block: the SPLIT part */
    uint8_t          acc_limit[TESSERA_TRACKS];  /* 1..TESS_ACC_LIMIT: the pitch accumulator's reach in degrees from the
                                                  * written note, a STEP track's (step_acc.c; 0 reads as 7) */
    uint8_t          acc_mode[TESSERA_TRACKS];   /* TESS_ACC_WRAP / PING / CLAMP: what it does at LIMIT; both not in the
                                                  * packed block: the record's ACC part */
    /* MIDI OUT (midi_out.c; GLO > MIDI OUT): each track's channel. 0 AUTO (ROUT, a drum track the DRUM channel, as
     * before), 1..16 that channel, TESS_MIDI_OFF none; not in the packed block: the record's MIDI part */
    uint8_t          midi_ch[TESSERA_TRACKS];
    /* MIDI OUT's TRANSPOSE (OCT+ held on GLO > MIDI OUT): each track's notes go out this many semitones up (down when
     * negative), the sound itself unchanged; 0 as before. Not in the packed block: the record's TRANSP part */
    int8_t           midi_tr[TESSERA_TRACKS];
    /* SOUND (OCT- held on GLO > MIDI OUT): 1 OFF, the track plays no voices of its own (voice.c trk_note_on); its notes
     * still go out over MIDI and its GATE / NOTE / CV still feed the mods: a MIDI track, or a lane of modulation. 0 ON,
     * as before. Not in the packed block: the record's SOUND part */
    uint8_t          sound_off[TESSERA_TRACKS];
    /* KEY LINK (ui.c scale_share; SCL's sheet), the project's: 1 one ROOT for every melodic track, shared as SCL and
     * QNT always are; 0 each track its own ROOT (as before). On in a new project; a record without its part: off */
    uint8_t          key_link;
    /* STEP as seed (step_seed.c; OCT+ held on GEN, TURING, EUCLID, MARKOV and MORPH): the record's SEED part (SOURCE,
     * KEEP) and GRAV part (GRAVITY), not the packed block */
    uint8_t          sd_src[TESSERA_TRACKS];     /* SOURCE: 0 OFF, 1..4 that track's STEP pattern */
    uint8_t          sd_keep[TESSERA_TRACKS];    /* KEEP: TESS_SD_FREE, _RHYTHM, _PITCH */
    uint8_t          sd_grav[TESSERA_TRACKS];    /* GRAVITY: 0..100 % of steps played as written, 0 off */
    /* FORM's DEVELOP (OCT+ held on FORM, KNOB 1): the record's DEV part, not the packed block */
    uint8_t          fm_dev[TESSERA_TRACKS];     /* 0..100: how far a section's return varies, 0 exact (as before) */
    /* CHORD's BREATH (HIT HOLD): its voices on slow cycles of their own, one now and then sitting out */
    uint8_t          ch_breath[TESSERA_TRACKS];  /* 0..100: 0 off (as before) */
    /* ENERGY (energy.c; the ENERGY page), the project's, not a track's: its slow tide and its rests */
    uint8_t          en_arc;                     /* 0..100: how far the tide swings, 0 off */
    uint8_t          en_rests;                   /* 0..100: how often the notes rest, 0 off */
    int8_t           en_level;                   /* -100..100: the energy the tide swings round, 0 as set (part 2) */
    uint8_t          en_depth[TESSERA_TRACKS];   /* 0..100: a track's share of ENERGY, 100 all (as before), 0 an anchor */
    /* Form chaining (form_chain.c; the CHAIN page under GLO), the project's FORM on the bar clock, and each track's part
     * in it (OCT+ held on FORM): the record's CHAIN part, not the packed block. FORM OFF: nothing changes */
    uint8_t          fc_form;                    /* 0 OFF, 1..6 AB .. AABC (TESS_FORMS) */
    uint8_t          fc_phrase;                  /* 0, 1, 2: a phrase of 4, 8 (default), 16 bars */
    uint8_t          fc_wander;                  /* 0..100: the chance the next letter is drawn, 0 as written */
    uint8_t          fc_rep[4];                  /* 1..8: the phrases each place of the FORM holds (default 1) */
    uint8_t          fc_follow[TESSERA_TRACKS];  /* 0 not set (FOLLOW where the track's own FORM is OFF, else OWN),
                                                  * 1 FOLLOW, 2 OWN */
    uint8_t          fc_skip[TESSERA_TRACKS];    /* 0..100: the chance the track holds its section a phrase longer */
    /* KEY B, KEY C (the KEY page, SCL's KEY row): semitones every following track's notes move by in the CHAIN's B / C
     * section, -12..12, 0 the saved key (A always is). The record's FKEY part (written only when one is not 0) */
    int8_t           fc_key[2];
    /* Entries (entries.c; the ENTRIES page under GLO): tracks out and back in on the CHAIN's phrase lines. The record's
     * ENTRY part. ENTRIES 0: nothing changes */
    uint8_t          et_amount;                  /* 0..100: how many tracks may be out at most (of those that may), 0 off */
    uint8_t          et_dwell;                   /* 1..8: the phrases a track stays in or out at least (default 2) */
    uint8_t          et_style;                   /* 0 CUT, 1 FADE (default) */
    uint8_t          et_fade;                    /* 1..4: a FADE's bars (default 2) */
    uint8_t          et_role[TESSERA_TRACKS];    /* 0 AUTO, 1 BASS, 2 RHYTHM, 3 HARMONY, 4 MELODY, 5 ANCHOR (never out) */
    /* The piece planner (planner.c; the PIECE page under GLO): the piece's LENGTH and SHAPE, steering ENERGY, Entries and
     * the CHAIN. The record's PLAN part. LENGTH FREE: nothing changes */
    uint8_t          pl_length;                  /* 0 FREE, 1..21 minutes (planner.c PL_MIN: 1 .. 240) */
    uint8_t          pl_shape;                   /* 0 ARCH .. 9 USER */
    uint8_t          pl_peak;                    /* 0..100: ARCH's, PLATEAU's, VALLEY's centre, % of the length (62) */
    uint8_t          pl_count;                   /* 2..8: WAVES' swells, BUILDS' builds, STEPS' terraces (3) */
    uint8_t          pl_end;                     /* 0 STOP, 1 LOOP, 2 NEW, 3 FREE */
    uint8_t          pl_user[8];                 /* 0..100: USER's levels, evenly along the piece */
    uint8_t          macro[4];                   /* 0..100: PERF's MACRO 1..4 (cord.c CS_MACRO1..4) */
    uint8_t          pat_gen[TESSERA_TRACKS];    /* PAT GEN: 1 each pattern keeps its own generator setup (pat_gen.c) */
    uint8_t          midi_dmap[TESSERA_TRACKS];  /* MIDI OUT's DRUM MAP: TESS_DM_GM (as before), _ROW (midi_out.c) */
} tessera_state_t;

extern tessera_state_t tessera;
/* STEP EVOLVE's changes as they play (not saved; cleared with `tessera` by tessera_init). In .pool: static RAM is
 * nearly full */
typedef struct {
    uint8_t  flash[TESSERA_TRACKS];             /* steps left to show a round just made */
    uint16_t steps[TESSERA_TRACKS];             /* steps since the last round */
    uint32_t rnd[TESSERA_TRACKS];               /* its random stream */
    uint8_t  mut[TESSERA_TRACKS][TESS_SV_STEPS];   /* each step's change (vary.c), 0 as written */
    uint8_t  ser[TESSERA_TRACKS];               /* HOW SERIAL: the round's transform (TESS_SV_SER_*), 0 none */
    uint8_t  ser_arg[TESSERA_TRACKS];           /* its argument: ROTATION's steps, AUGMENTATION's half (0 the first) */
    /* GRAVITY on EVOLVE (tessera.c tess_gv_round): a generator track's home and its state after the last round */
    uint8_t   gv_mode[TESSERA_TRACKS];          /* the MODE home is for, + 1; 0 none yet */
    gen_sec_t gv_home[TESSERA_TRACKS], gv_last[TESSERA_TRACKS];
    uint8_t   gv_haw[TESSERA_TRACKS][16], gv_law[TESSERA_TRACKS][16];   /* (Awake's values: home's, the last round's) */
    /* FORM's DEVELOP (tessera.c tess_form_develop): a return as developed, so leaving it keeps the section unchanged */
    uint8_t   dv_on;                            /* tracks (bits) playing a developed return */
    gen_sec_t dv_last[TESSERA_TRACKS];
    uint8_t   dv_law[TESSERA_TRACKS][16];
} tessera_sv_t;
enum { TESS_SV_SER_NONE, TESS_SV_SER_RETRO, TESS_SV_SER_INV, TESS_SV_SER_ROT, TESS_SV_SER_AUG };
extern tessera_sv_t tess_sv;
/* ANSWER as it plays (answer.c; not saved, cleared by tessera_init). In .pool */
enum { TESS_AN_IDLE, TESS_AN_HEAR, TESS_AN_GAP, TESS_AN_PLAY, TESS_AN_LOOP };
#define TESS_AN_ECHOES 3u
typedef struct {
    uint8_t  phase[TESSERA_TRACKS];             /* TESS_AN_IDLE .. LOOP */
    uint8_t  ans[TESSERA_TRACKS];               /* the answers played since the phrase (its FADE count) */
    int8_t   shift[TESSERA_TRACKS];             /* this answer's SHIFT, scale degrees */
    uint8_t  slot[TESSERA_TRACKS];              /* the phrase's note just played + 1, 0 none (the picture) */
    uint16_t hclk[TESSERA_TRACKS];              /* listening: steps since the phrase's first note */
    uint8_t  hlate[TESSERA_TRACKS];             /* the first note was past its step's middle: on the next step */
    uint32_t cnt[TESSERA_TRACKS];               /* steps in this phase (listening: since the last note) */
    uint32_t rnd[TESSERA_TRACKS];               /* its random stream (from the SEED at PLAY) */
    uint8_t  mut[TESSERA_TRACKS][TESS_AN_NOTES];   /* each place's change (vary.c) */
    uint8_t  ord[TESSERA_TRACKS][TESS_AN_NOTES];   /* the note each place plays (ORDER) */
    uint8_t  e_note[TESSERA_TRACKS][TESS_AN_ECHOES], e_vel[TESSERA_TRACKS][TESS_AN_ECHOES];
    uint8_t  e_left[TESSERA_TRACKS][TESS_AN_ECHOES];   /* steps until the echo plays, 0 none */
} tessera_an_t;
extern tessera_an_t tess_an;

void tessera_init(void);
void tessera_clock_step(track_t *t);
void tessera_markov_recalc(uint32_t tr);
void tessera_knob(uint32_t k, int32_t s);
void tessera_mode_set(uint32_t tr, uint32_t m);   /* SEQ MODE (IRQs off) */
void tessera_ensure_init(void);
int32_t tessera_snap_note(const track_t *t, int32_t n);
int32_t tessera_pitch_turn(const track_t *t, int32_t n, int32_t k, int fine);   /* NOTES KNOB 2: by scale notes */
uint32_t tessera_euclid_pitch(const track_t *t, uint32_t step);
uint32_t tessera_sig(uint32_t tr);
void tessera_kill(track_t *t);
void tessera_mutate(uint32_t tr);
void tessera_seed_turn(uint32_t tr, int32_t s);    /* ROLL (EVOLVE page): the next seed, on the bar line */
void tessera_evolve_knob(uint32_t k, int32_t s);
void tessera_ev_layer_knob(uint32_t k, int32_t s);
void tessera_tw_layer_knob(uint32_t k, int32_t s);
void tessera_hold_hit(uint32_t tr);
void tessera_fm_layer_knob(uint32_t k, int32_t s);   /* OCT+ held on FORM: KNOB 1 DEVELOP */
void tessera_en_layer_knob(uint32_t k, int32_t s);   /* OCT+ held on ENERGY: KNOB 1 the track's DEPTH */   /* OCT+ held on a generator's EVOLVE: KNOB 1 GRAVITY */
uint32_t tessera_every_bars(uint32_t every);
int  tessera_lock_get(uint32_t tr);            /* 0..100, or -1 when LOCK is not available for the mode */
void tessera_phrase(uint32_t tr, uint32_t *bar, uint32_t *of, int *flash); /* of == 0: EVERY is OFF */
void tessera_stop_all(void);
void tessera_tweak_knob(uint32_t k, int32_t s);
void tessera_form_knob(uint32_t k, int32_t s);
void tessera_tweak2_knob(uint32_t k, int32_t s);    /* TWEAK 2 page: BRANCH TO LOGIC COND */
void tessera_len_knob(uint32_t k, int32_t s);       /* LENGTH page: MODE LEN KEYS (KNOB 4: PATTERN's GATE) */
void tessera_len_tick(track_t *t, uint32_t n);      /* seq.c, each block: FIXED's held notes end on time */
void tessera_len_stop(track_t *t);                  /* the held notes end now (panic, a pattern change) */
void organic_play(void);                            /* seq.c, PLAY: SLOTH and OCHD from each track's SEED */
void fchain_play(void);                             /* seq.c, PLAY: form chaining from the top (form_chain.c) */
uint32_t tessera_chain_get(uint8_t seq[4], uint32_t *pos, uint32_t *left);   /* the project FORM: letters, place, left */
int32_t tessera_key_now(uint32_t tr);   /* KEY B / C: the semitones track tr's notes move by now (form_chain.c) */
void tessera_chain_knob(uint32_t k, int32_t s, int layer);   /* the CHAIN page: KNOB k turned s (layer: OCT+ held) */
void entries_play(void);                            /* seq.c, PLAY: Entries from the top, everything in (entries.c) */
void entries_tick(uint32_t n);                      /* seq.c, each block before the steps: the lines, the fades */
int entries_hand_in(uint32_t tr);                   /* GLO's mute key: an out track back in (1 if it was out) */
void mute_press(uint32_t tr, int all);              /* GLO's mute key / UNMUTE ALL: now or on MENU MUTES' line */
uint32_t mute_waiting(void);                        /* the mute keys waiting for their line (blinking) */
uint32_t tessera_entries_get(uint32_t *warn, uint8_t role[4]);   /* the tracks out, those about to change, the roles */
void tessera_entries_knob(uint32_t k, int32_t s, int layer);   /* the ENTRIES page: KNOB k turned s (layer: OCT+ held) */
void planner_play(void);                            /* seq.c, PLAY: the planned piece from the top (planner.c) */
int tessera_piece_get(uint32_t *x, uint32_t *left); /* the piece: where (Q16), seconds left; 1 while planned */
int32_t tessera_piece_shape(uint32_t x);            /* the SHAPE at x (Q16): Q15 0 sparse .. 32767 full */
void tessera_piece_knob(uint32_t k, int32_t s, int layer);   /* the PIECE page: KNOB k (layer 1 OCT+, 2 OCT-) */
void energy_play(void);                             /* seq.c, PLAY: ENERGY's tide and rests from the SEEDs (energy.c) */
int32_t energy_now(void);                           /* the tide now, Q15 -32767 sparse .. 32767 full (cord source) */
int energy_resting(void);                           /* 1 while the notes rest */
const char *tessera_cond_name(uint32_t cond);
uint32_t tessera_form_get(uint32_t tr, uint8_t seq[4], uint32_t *pos);   /* returns the form's length, 0 = OFF */
const char *tessera_form_name(uint32_t form);
#define TESSERA_PK_BYTES 730u                       /* the saved state, version 19 (tessera.c: TESS_PK_TRACK x TESSERA_TRACKS,
                                                     * then the project's TESS_PK_TAIL) */
#define TESSERA_PK_BYTES_V18 602u                   /* version 18 (before ANSWER) */
#define TESSERA_PK_BYTES_V17 600u                   /* version 17 (before ENERGY's ARC and RESTS) */
#define TESSERA_PK_BYTES_V16 596u                   /* version 16 (before CHORD's BREATH) */
#define TESSERA_PK_BYTES_V15 588u                   /* version 15 (before SUB) */
#define TESSERA_PK_BYTES_V14 584u                   /* version 14 (before Grids' PART4) */
#define TESSERA_PK_BYTES_V13 572u                   /* version 13 (before STEP EVOLVE) */
#define TESSERA_PK_BYTES_V12 540u                   /* version 12 (before Grids' KICK, SNR, HAT, TOMS, Chord's USER) */
#define TESSERA_PK_BYTES_V11 520u                   /* version 11 (before CHORD) */
#define TESSERA_PK_BYTES_V10 516u                   /* version 10 (before CONSTELLATION's LEN and FILL) */
#define TESSERA_PK_BYTES_V9 508u                    /* version 9 (before QUAD's columns, RANGE and LINK to several) */
#define TESSERA_PK_BYTES_V8 480u                    /* version 8 (before SERIES and CELLS) */
#define TESSERA_PK_BYTES_V7 392u                    /* version 7 (before CONSTELLATION) */
#define TESSERA_PK_BYTES_V6 380u                    /* version 6 (before NOTE LENGTH) */
#define TESSERA_PK_BYTES_V5 352u                    /* version 5 (before QUAD) */
#define TESSERA_PK_BYTES_V4 320u                    /* version 4 (before BRANCH, LOGIC, COND, SLOTH, OCHD) */
#define TESSERA_PK_BYTES_V3 204u                    /* version 3 (before BOUNCE, AWAKE, POLY): older projects and backups */
#define TESSERA_PK_BYTES_V2 188u                    /* version 2 (before GRIDS) */
#define TESSERA_PK_BYTES_V1 180u                    /* version 1 (before SEED) */
uint32_t tessera_pack(uint8_t *out);                /* live state -> TESSERA_PK_BYTES */
#define TESS_PG_BYTES 40u                           /* PAT GEN: a pattern's generator setup (pat_gen.c) */
void tessera_pg_store(uint32_t tr, uint32_t b);
void tessera_pg_load(uint32_t tr, uint32_t b);
void tessera_pg_copy(uint32_t tr, uint32_t from, uint32_t to);
int tessera_pg_check(uint32_t tr, const uint8_t *slot);
uint32_t tessera_pg_mode(uint32_t tr, uint32_t b);
const uint8_t *tessera_pg_slots(uint32_t tr);
void tessera_pg_put(uint32_t tr, const uint8_t *slots);
int tessera_unpack(const uint8_t *in);              /* 1 loaded; 0 refused (range check), nothing changed */
int tessera_unpack_ver(uint32_t ver, const uint8_t *in);   /* the same for a block of version ver (1..19) */
int tessera_check(uint32_t ver, const uint8_t *in);        /* the same checks, nothing loaded: 1 it would load */
uint32_t tessera_pk_bytes(uint32_t ver);            /* a version's block length, 0 none */
void tessera_track_up(uint32_t ver, const uint8_t *in, uint8_t *out);   /* one track's record, version ver -> 19 */
void tessera_reset(void);                           /* every track OFF, defaults */
uint32_t tessera_ratchet(uint32_t tr);              /* SUB: the hits this step plays (1..8), its CHANCE drawn */
/* Tessera's parameters by number (pid.h: PID_TESS(x)). FROZEN: cords and saved projects keep these numbers, so an id
 * never changes meaning; new ones go at the end, a retired one stays as a gap. Ranges and help: pid.c TX_DESC */
enum {
    TX_MODE,                                        /* STEP (off) or a generator */
    TX_TUR_LEN, TX_TUR_PROB, TX_TUR_SPAN,
    TX_EUC_STEPS, TX_EUC_PULSES, TX_EUC_ROT,
    TX_MKV_LEAP, TX_MKV_DENS, TX_MKV_SPAN,
    TX_MRP_POS, TX_MRP_SNPA, TX_MRP_SNPB,
    TX_EV_LOCK, TX_EV_EVERY, TX_EV_AMOUNT, TX_EV_SEED,
    TX_FM_FORM, TX_FM_CONTRAST, TX_FM_FLOURISH,
    TX_TW_SHIFT, TX_TW_DROP, TX_TW_HUMAN, TX_TW_FOLLOW,
    TX_GRD_X, TX_GRD_Y, TX_GRD_DENS,
    TX_BNC_SPEED, TX_BNC_ANGLE, TX_BNC_BALLS,
    TX_AWK_LENA, TX_AWK_LENB, TX_AWK_PROB,
    TX_POL_DIV1, TX_POL_DIV2, TX_POL_DIV3, TX_POL_DIV4, TX_POL_SPAN,
    TX_TW_BRANCH, TX_TW_BR_TO, TX_TW_BR_MODE, TX_TW_LOGIC, TX_TW_LG_WITH, TX_TW_COND,
    TX_ORG_SPEED, TX_ORG_RATE,
    TX_QD_DIR, TX_QD_PROB, TX_QD_LINK,
    TX_NL_MODE, TX_NL_LEN, TX_NL_KEYS,
    TX_CS_Y, TX_CS_DENS, TX_CS_SIZE, TX_CS_AUTO,
    TX_SR_START, TX_SR_INTV, TX_SR_LOOP,
    TX_CL_RULE_RETIRED, TX_CL_TAP_RETIRED, TX_CL_SPAN_RETIRED,   /* (CELLS's, cut 2026-10-20: gaps, no parameter) */
    TX_QD_RANGE, TX_QD_MOVE1, TX_QD_MOVE2, TX_QD_MOVE3, TX_QD_MOVE4, TX_QD_EVERY1, TX_QD_EVERY2, TX_QD_EVERY3, TX_QD_EVERY4,
    TX_CS_LEN, TX_CS_FILL,
    TX_CH_PROG, TX_CH_BARS, TX_CH_HIT,
    TX_CH_OCT,
    TX_GRD_KICK, TX_GRD_SNR, TX_GRD_HAT, TX_GRD_TOMS,
    TX_CH_ULEN,
    TX_SV_LOCK, TX_SV_EVERY, TX_SV_AMOUNT,
    TX_GRD_PART4,
    TX_SUB_RATCH, TX_SUB_CHANCE,
    TX_CH_BREATH,
    TX_EN_ARC, TX_EN_RESTS,                         /* (the project's: one value, whichever track) */
    TX_AN_VARY, TX_AN_ECHO, TX_AN_FADE, TX_AN_ORDER, TX_AN_SHIFT, TX_AN_SPACE, TX_AN_LEN,
    TX_SUB_SHAPE,                                   /* (0x262: SUB's SHAPE, any track's) */
    TX_MIDI_CH,                                     /* (0x263: MIDI OUT's channel, any track's) */
    TX_KEY_LINK,                                    /* (0x264: KEY LINK, the project's: one value, whichever track) */
    TX_SD_SOURCE, TX_SD_KEEP,                       /* (0x265, 0x266: STEP as seed's SOURCE and KEEP, any track's) */
    TX_SUB_HUMAN,                                   /* (0x267: TIMING's HUMAN TIME, any track's) */
    TX_ACC_LIMIT, TX_ACC_MODE,                      /* (0x268, 0x269: the pitch accumulator's LIMIT and MODE, a STEP
                                                     * track's) */
    TX_SD_GRAVITY,                                  /* (0x26A: STEP as seed's GRAVITY, any track's) */
    TX_SV_HOW,                                      /* (0x26B: STEP EVOLVE's HOW, a STEP track's) */
    TX_EN_LEVEL,                                    /* (0x26C: ENERGY's LEVEL, the project's: one value, whichever track) */
    TX_EN_DEPTH,                                    /* (0x26D: ENERGY's DEPTH, any track's) */
    TX_FM_DEVELOP,                                  /* (0x26E: FORM's DEVELOP, a generator track's) */
    TX_MIDI_TR,                                     /* (0x26F: MIDI OUT's TRANSPOSE, any track's) */
    TX_TUNE,                                        /* (0x270: the user tuning, the project's: tune.h tune_sel) */
    TX_SOUND,                                       /* (0x271: SOUND ON / OFF, any track's: OFF plays no voices) */
    TX_FC_FORM, TX_FC_PHRASE, TX_FC_WANDER,         /* (0x272..0x274: form chaining, the project's: one value, whichever
                                                     * track) */
    TX_FC_REP1, TX_FC_REP2, TX_FC_REP3, TX_FC_REP4, /* (0x275..0x278: the repeats of the chain's four places) */
    TX_FC_FOLLOW, TX_FC_SKIP,                       /* (0x279, 0x27A: FOLLOW / OWN and SKIP, a generator track's) */
    TX_ET_AMOUNT, TX_ET_DWELL, TX_ET_STYLE, TX_ET_FADE,   /* (0x27B..0x27E: Entries, the project's: one value, whichever
                                                     * track) */
    TX_ET_ROLE,                                     /* (0x27F: Entries' ROLE, any track's) */
    TX_PL_LENGTH, TX_PL_SHAPE, TX_PL_PEAK, TX_PL_COUNT, TX_PL_END,   /* (0x280..0x284: the piece planner, the project's) */
    TX_PL_USER1, TX_PL_USER2, TX_PL_USER3, TX_PL_USER4,             /* (0x285..0x28C: SHAPE USER's eight levels) */
    TX_PL_USER5, TX_PL_USER6, TX_PL_USER7, TX_PL_USER8,
    TX_MACRO1, TX_MACRO2, TX_MACRO3, TX_MACRO4,     /* (0x28D..0x290: PERF's four MACROs, the project's: cord sources) */
    TX_TW_HOLD,                                     /* (0x291: FOLLOW's HOLD, a generator track's: OFF, T1..T4) */
    TX_FC_KEYB, TX_FC_KEYC,                         /* (0x292, 0x293: the CHAIN's B and C sections' key, the project's) */
    TX_SUB_SPLIT, TX_SUB_SPLIT_HOW,                 /* (0x294, 0x295: TIMING's SPLIT and HOW, any track's) */
    TX_TW_FHOW, TX_TW_FRHY,                         /* (0x296, 0x297: FOLLOW's HOW and RHYTHM, a generator track's) */
    TX_PAT_GEN,                                     /* (0x298: PAT GEN, any track's: each pattern its own generator) */
    TX_MIDI_DMAP,                                   /* (0x299: MIDI OUT's DRUM MAP, any track's: midi_out.c) */
    TX_COUNT
};
_Static_assert(TX_TW_HOLD == 0x91 && TX_FC_KEYC == 0x93 && TX_SUB_SPLIT_HOW == 0x95 && TX_TW_FRHY == 0x97 &&
               TX_PAT_GEN == 0x98 && TX_MIDI_DMAP == 0x99,
               "parameter ids are stored: never renumbered");
/* TX_TW_FHOW: the source's note as it is, the nearest chord tone, mirrored, a bass, or CANON 1..8 notes back */
enum { TESS_FH_SAME, TESS_FH_CHORD, TESS_FH_CONTRA, TESS_FH_BASS, TESS_FH_CANON, TESS_FH_COUNT = TESS_FH_CANON + 8 };
enum { TESS_FR_OWN, TESS_FR_SOURCE, TESS_FR_BOTH, TESS_FR_COUNT };   /* TX_TW_FRHY */
enum { TESS_DM_GM, TESS_DM_ROW, TESS_DM_COUNT };   /* TX_MIDI_DMAP: General MIDI's drum notes, or one a track in a row */
enum { TESS_SPLIT_DOUBLE, TESS_SPLIT_PUSH, TESS_SPLIT_MIX, TESS_SPLIT_HOWS };   /* TX_SUB_SPLIT_HOW */
#define TESS_HUMAN_MS 60u                           /* TX_SUB_HUMAN: a step up to 60 ms late */
#define TESS_ACC_LIMIT 14u                          /* TX_ACC_LIMIT: up to two octaves of a 7-note scale */
#define TESS_ACC_LIMIT_DEF 7u
enum { TESS_ACC_WRAP, TESS_ACC_PING, TESS_ACC_CLAMP, TESS_ACC_MODES };   /* TX_ACC_MODE */
enum { TESS_SD_FREE, TESS_SD_RHYTHM, TESS_SD_PITCH, TESS_SD_KEEPS };   /* TX_SD_KEEP */
enum { TESS_SV_DRIFT, TESS_SV_ORNAMENT, TESS_SV_SERIAL, TESS_SV_HOWS };   /* TX_SV_HOW */
void tessera_sd_play(void);                         /* seq.c, PLAY: STEP as seed's loops from their sources again */
void tessera_sd_layer_knob(uint32_t k, int32_t s);  /* TURING / EUCLID / MARKOV / MORPH on GEN, OCT+ held: KNOB 1
                                                     * SOURCE, 2 KEEP, 3 GRAVITY */
#define TESS_MIDI_OFF 17u                           /* TX_MIDI_CH: 0 AUTO, 1..16, 17 OFF */
#define TESS_MIDI_TR 48                             /* TX_MIDI_TR: -48..48 semitones, four octaves either way */
int32_t tessera_param_get(uint32_t tr, uint32_t x);           /* the value its page shows */
void tessera_param_poke(uint32_t tr, uint32_t x, int32_t v); /* written as it is (cord.c, around a step) */
void tessera_param_set(uint32_t tr, uint32_t x, int32_t v);  /* clamped, with what its knob does besides (a MODE
                                                               * change releases the notes, STEPS keeps PULSES ..) */
/* the UI's view of a track's generators (read only; tr < TESSERA_TRACKS) */
const gen_turing_t *tessera_turing(uint32_t tr);
const gen_euclid_t *tessera_euclid(uint32_t tr);
const gen_markov_t *tessera_markov(uint32_t tr);
const gen_morph_t *tessera_morph(uint32_t tr);
const gen_grids_t *tessera_grids(uint32_t tr);
const gen_bounce_t *tessera_bounce(uint32_t tr);
const gen_awake_t *tessera_awake(uint32_t tr);
void tessera_awake_select(uint32_t tr, uint32_t step);   /* the step KNOB 1 (A) and KNOB 2 (B) set (the same again, or
                                                        * step 16: none) */
const gen_poly_t *tessera_poly(uint32_t tr);
const gen_quad_t *tessera_quad(uint32_t tr);
void tessera_quad_cols_knob(uint32_t k, int32_t s);  /* OCT+ held on GEN in QUAD: COL, MOVE, EVERY, RANGE */
uint32_t tessera_quad_every(uint32_t c);            /* column c's EVERY in steps */
const gen_constel_t *tessera_constel(uint32_t tr);
uint32_t tessera_cs_speed(uint32_t tr);                 /* Constellation's SPEED: the track's DIV, 0 the slowest .. 7 */
#define TESS_CS_SPEEDS 8u                               /* (4BAR .. 1/32, the straight DIVs) */
void tessera_constel_layer_knob(uint32_t k, int32_t s);   /* OCT+ held on GEN in CONSTELLATION: LOCK, LEN, FILL, SIZE */
void tessera_grids_layer_knob(uint32_t k, int32_t s);      /* OCT+ held on GEN in GRIDS: KICK, SNR, HAT, TOMS */
const char *tessera_grids_part4_name(uint32_t tr);         /* the fourth part's sound: TOM, CLAP, HATOP, RIM, BELL */
int tessera_grids_part4_note(uint32_t tr, uint32_t note);  /* OCT+ held on GEN in GRIDS, a key: note's sound picked (1) */
uint32_t tessera_chord_user_key(uint32_t tr, uint32_t note, int first);   /* CHORD USER: a key's chord (0 full) */
/* STEP EVOLVE (step_evolve.c): a STEP track's EVOLVE page evolves its written steps; they stay as written until kept */
int tessera_step_on(uint32_t tr);                        /* a STEP track (no generator, no ARP): its EVOLVE page */
const step_t *tessera_step_vary(track_t *t, const step_t *s, step_t *out);   /* seq.c: the step as it plays */
uint32_t tessera_step_varied(uint32_t tr);               /* steps playing a change now */
uint32_t tessera_sd_loop(uint32_t tr, uint8_t *now, uint8_t *home, uint32_t *at);   /* TURING / MORPH from a SOURCE: its loop now and as written (the pictures) */
int tessera_step_now(uint32_t tr, uint32_t i, step_t *out);   /* step i as it plays now: 1 changed (the pictures) */
uint32_t tessera_step_serial(uint32_t tr, uint32_t *arg);   /* SERIAL's transform now (TESS_SV_SER_*), its argument */
void tessera_step_clear(uint32_t tr);                    /* the steps as written again (KNOB 4 CLEAR) */
void tessera_sv_layer_knob(uint32_t k, int32_t s);       /* its page, OCT+ held: KNOB 1 HOW */
uint32_t tessera_step_keep(uint32_t tr);                 /* REC held: the changes become the steps; returns how many */
void tessera_chord_ulen_knob(int32_t s);                   /* OCT+ held on GEN in CHORD USER, KNOB 4: LEN */
void tessera_chord_oct_knob(int32_t s);                    /* OCT+ held on GEN in CHORD, KNOB 1: OCT */
void tessera_chord_breath_knob(int32_t s);                 /* OCT+ held on GEN in CHORD (not USER), KNOB 4: BREATH */
const gen_series_t *tessera_series(uint32_t tr);
const tessera_answer_t *tessera_answer(uint32_t tr);
void tessera_answer_hear(track_t *t, uint32_t note, uint32_t vel);   /* a key / MIDI note on the track, on its step */
void tessera_answer_hear_at(track_t *t, uint32_t note, uint32_t vel, uint32_t late);   /* seq.c: late, played past
                                                     * the middle of the step playing: on the next, as REC places it */
void tessera_answer_layer_knob(uint32_t k, int32_t s);  /* OCT+ held on GEN in ANSWER: ORDER, SHIFT, SPACE, LEN */
uint32_t tessera_answer_count(uint32_t tr);             /* the answers FADE gives (1..16), 0: never forgets (loops) */
const gen_chord_t *tessera_chord(uint32_t tr);
uint32_t tessera_chord_pcs(void);     /* QNT CHRD: the chord the first CHORD track plays (pitch classes, bit 0 C), 0 none */
uint32_t tessera_chord_snap(uint32_t pcs, int32_t n);   /* n moved to the nearest note of pcs (a tie: down), 0..127 */
uint32_t tessera_chord_at(uint32_t tr, uint32_t i, char *name);   /* chord i of its progression: name (12 B) -> pitch
                                                                 * classes (bit 0 C) | its root's << 12 */

#endif
