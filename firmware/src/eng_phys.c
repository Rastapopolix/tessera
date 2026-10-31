/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* PHYS: physical models. The DSP is ported to fixed point from DaisySP (Electrosmith, Emilie Gillet;
 * MIT: phys_dsp.c) and Rings' sympathetic strings (Emilie Gillet; MIT: phys_symp.c); this file is the
 * Melodee engine around them.
 *
 * MODEL picks one of four (the values are stored: new ones are appended):
 *   MODAL   a bank of 12 band-pass modes (bells, bars, plates): STRC stretches the partials
 *           (below 0.25 compressed, 0.25..0.3 harmonic, above more and more inharmonic), BRIT
 *           keeps the high modes ringing and opens the strike, DAMP the decay (127 rings on),
 *           POS the strike position (the mode amplitudes follow cos(2 pi pos i), 0..1/4).
 *           Struck by an impulse into a low-pass; with BOW > 0 sustained by random hits (bowed /
 *           blown) at that level instead
 *   STRNG   an extended Karplus-Strong string, plucked by a noise burst one period long: STRC
 *           below ~0.25 a curved bridge (buzzy, sitar-like), above it dispersion (stiff, piano-
 *           like), BRIT the burst's tone and the loop filter, DAMP the decay, POS the pluck
 *           position (a comb on the burst, 0 = none); BOW > 0 again sustains it by random hits
 *   MEMB    a struck drum head: the modal bank with the modes of a circular membrane. HARM blends an
 *           ideal head (inharmonic: toms, timpani) into a loaded one (harmonic overtones: a tabla-like
 *           hand drum), POS goes from the centre (round, only the circular modes) to the rim, BEND
 *           drops the pitch after the strike (up to 12 semitones); BRIT, DAMP as MODAL
 *   SYMP    the plucked string with three sympathetic strings that ring along (a sitar's, a
 *           harp's): CHRD tunes them (OCT 5TH 4TH MAJ MIN SUS 7TH above the note, ROOT: the track's
 *           scale root, its fifth and octave around C3), SYMP how much they take from the string, BUZZ
 *           the curved bridge; BRIT, DAMP as STRNG
 * ACC is the accent at full velocity (brighter, longer, louder strike; velocity scales it), EXC
 * mixes the exciter itself into the output (the mallet / pick). The track's ENV / LFO -> FLT move BRIT.
 *
 * MODEL 4 was DRUM before 1.0: the kit is its own engine now (eng_drum.c); projects and user presets
 * that saved it load as that engine (core.h drum_from_phys).
 *
 * DUST (resonant particles, value 2 before 1.0) was dropped: MODEL 2 is MEMB now. Projects and
 * user presets saved before say so (project.c, upreset.c) and load DUST as MODAL bowed, its nearest
 * kin (random hits into resonators): phys_legacy.
 *
 * Polyphony 3 per part (engine_t.poly): each part's voices 0..2 own a state slot in the pool
 * section (phys_slot: SYMP's, the largest: the string's 512 + 128 sample lines, Q20, and three
 * 256-sample sympathetic lines, 16-bit), 12 slots in all. Voice amplitude: the track's ADSR as for every
 * engine; the presets hold SUS at 127 so the model's own decay is heard and REL damps it after the key.
 * Cost (host, 8 notes asked = 3 voices): see the README and cpu_baseline.txt. */
#include "phys_dsp.c"
#include "phys_symp.c"

enum { PM_MODAL, PM_STRING, PM_MEMB, PM_SYMP, PM_COUNT };
#define PHYS_POLY (MELODEE_RAM_AUDIO ? 2 : 3)   /* (the resident audio arena: four parts of three SYMP voices, 50 KB, pass its 48) */
/* output: model 1.0 -> this (VOICE_FS scale), then a soft knee; per model (a strike of all the modes in
 * phase peaks high but is short) */
static const int32_t PHYS_GAIN[PM_COUNT] = {39500, 16900, 127000, 16900};   /* (+3 dB 2026-10-31: level with FM6,
                                                                              * ORACLE; tests/preset_levels.c) */

/* The knobs' travel -> the models' values (2026-10-31, PHYS back made friendlier): STRC, BRIT and DAMP cover only
 * the musical window of their DSP value (Q16 0..1), on a curve of 17 points 8 clicks apart (/256, linear between)
 * with wide flat stretches at the best places: MODAL harmonic over STRC 16..48 (it was 32..36 of the plain 0..1),
 * a free bar's 1 : 2.8 : 5.4 around 84, a tuned bar's 1 : 4 : 9 at the top; the plain string over 24..48, its
 * curved bridge below, its stiffness above. Left out: a dead thud (DAMP 0), the runaway sustain of SYMP at the top
 * of DAMP, partials past a tuned bar's and a string's noisy dispersion (harsh). A knob's middle stays near where it
 * was, the ends move: a project saved before plays a little different */
enum { PX_STRC_M, PX_STRC_S, PX_BRIT_M, PX_BRIT_S, PX_DAMP_M, PX_DAMP_D, PX_DAMP_S, PX_DAMP_Y, PX_NMAP };
static const uint8_t PHYS_MAP[PX_NMAP][17] = {
    {31, 49, 64, 67, 70, 73, 76, 84, 102, 123, 137, 142, 159, 184, 212, 230, 236},     /* MODAL STRC 0.12 .. 0.92 */
    {44, 50, 56, 62, 63, 65, 66, 70, 80, 92, 105, 118, 132, 145, 160, 178, 205},       /* STRNG STRC 0.17 .. 0.80 */
    {15, 29, 42, 56, 70, 84, 97, 111, 125, 139, 152, 166, 180, 194, 207, 221, 235},    /* MODAL MEMB BRIT */
    {30, 44, 58, 71, 85, 99, 113, 126, 140, 154, 168, 181, 195, 209, 223, 236, 250},   /* STRNG SYMP BRIT */
    {46, 57, 68, 79, 90, 101, 112, 123, 133, 144, 155, 166, 177, 188, 199, 210, 220},  /* MODAL DAMP 0.18 .. 0.86 */
    {31, 41, 51, 61, 71, 81, 91, 101, 111, 120, 130, 140, 150, 160, 170, 180, 192},    /* MEMB DAMP 0.12 .. 0.75 */
    {64, 76, 87, 99, 110, 122, 133, 145, 156, 168, 179, 190, 200, 210, 220, 230, 238}, /* STRNG DAMP 0.25 .. 0.93 */
    {64, 73, 82, 91, 100, 109, 118, 127, 136, 145, 153, 161, 169, 177, 185, 193, 200}, /* SYMP DAMP 0.25 .. 0.78 */
};
static const uint8_t PHYS_MAPS[PM_COUNT][3] = {   /* STRC (0xFF: as it is), BRIT, DAMP of each model */
    {PX_STRC_M, PX_BRIT_M, PX_DAMP_M}, {PX_STRC_S, PX_BRIT_S, PX_DAMP_S},
    {0xFF, PX_BRIT_M, PX_DAMP_D}, {0xFF, PX_BRIT_S, PX_DAMP_Y},
};

/* a knob's value x in 1/256 of a click (0 .. 127 * 256: a modulation moves it smoothly) on curve m: Q16 */
static int32_t phys_map(uint32_t m, int32_t x)
{
    const uint8_t *T = PHYS_MAP[m % PX_NMAP];
    uint32_t i, f;
    x = clamp(x, 0, 127 << 8);
    i = (uint32_t)x >> 11;
    f = (uint32_t)x & 2047u;
    return (int32_t)((T[i] * (2048u - f) + T[i + 1u] * f) >> 3);
}

/* the output level by BRIT and DAMP (at knob 0, 32, 64, 96, 128; /32, linear between): each model's loudness
 * within ~3 dB over both (the first 0.1 s of a note, C3 and C5). It moved up to 15 dB: a string dull or short
 * quiet, MEMB bright quiet, SYMP short quiet */
static const uint8_t PHYS_LVL[PM_COUNT][2][5] = {
    {{32, 32, 32, 32, 32}, {48, 36, 32, 32, 32}},        /* MODAL: BRIT, DAMP */
    {{40, 38, 32, 30, 27}, {143, 107, 76, 48, 40}},      /* STRNG */
    {{4, 10, 19, 29, 35}, {45, 38, 32, 32, 32}},         /* MEMB */
    {{45, 40, 32, 29, 25}, {80, 67, 53, 40, 28}},        /* SYMP (and by SYMP: phys_render) */
};
static const uint8_t PHYS_BOWL[2][5] = {                 /* bowed (BOW > 0), by BRIT: MODAL, STRNG (the Dust's */
    {255, 125, 55, 35, 25}, {12, 36, 18, 16, 18},        /* density follows the brightness) */
};
#define PHYS_BUZZ(v) (16384 - (v) * 41)                  /* SYMP's BUZZ: the string's STRC 0.25 .. 0.17 (lower ran away) */
/* a curved bridge (structure below 0.24) pumps energy into a string that rings long, from C4 up: its damping held
 * at 0.72 (structure 0.17) .. 0.84 (0.24), where the sound still dies away (else up to 25 dB above the pluck) */
static int32_t phys_bridge_damp(int32_t strc, int32_t damp)
{
    int32_t dmax = 47186 + (clamp(strc - 11141, 0, 4588) * 219 >> 7);
    if (strc >= 15729)
        return damp;
    return damp < dmax ? damp : dmax;
}
/* the model's STRC, BRIT, DAMP (Q16) from the knobs, BRIT moved by bmod (1/256 of a click): v[0..2] (phys_render,
 * the Mono picture) */
static void phys_body(const int16_t *p, uint32_t md, int32_t bmod, int32_t *v)
{
    const uint8_t *mp = PHYS_MAPS[md % PM_COUNT];
    v[0] = mp[0] == 0xFFu ? p[P_E1] * 516 : phys_map(mp[0], p[P_E1] << 8);
    v[1] = phys_map(mp[1], (p[P_E2] << 8) + bmod);
    v[2] = phys_map(mp[2], p[P_E3] << 8);
}
/* EXC's knock (MODAL, MEMB: the strike's own exciter) faded in over the block it starts in, as a fresh voice's
 * envelope does: a strike on a voice still ringing (a note again, TURING's repeats) starts at the envelope's level,
 * and its first samples clicked (2026-11-01). Only a strike's block */
static __attribute__((noinline)) void phys_knock_in(int32_t *ax, uint32_t n)
{
    uint32_t i;
    int32_t d = n ? 65536 / (int32_t)n : 0, r = 0;
    for (i = 0; i < n; i++, r += d)
        ax[i] = px_m(ax[i], r, 16);
}
static int32_t phys_lvl(const uint8_t *T, int32_t v)     /* v a knob 0..127: Q5 */
{
    uint32_t i = (uint32_t)clamp(v, 0, 127) >> 5, f = (uint32_t)v & 31u;
    return (int32_t)((T[i] * (32u - f) + T[i + 1u] * f) >> 5);
}

typedef struct {
    uint8_t model;               /* PM_* the state belongs to */
    union {
        px_modal_t m;            /* MODAL, MEMB */
        px_string_t s;
        px_symp_t y;
    } u;
} phys_slot_t;

static phys_slot_t *phys_slots(uint32_t part);    /* engines.c eng_state: the part's PHYS_POLY slots */

static const char *const N_PHYS_MODEL[] = {"MODAL", "STRNG", "MEMB", "SYMP"};
static const char *const N_PHYS_CHORD[] = {"OCT", "5TH", "4TH", "MAJ", "MIN", "SUS", "7TH", "ROOT", 0};
/* per model: STRC .. EXC under their own names (phys_desc; range and default as edit[], 0 = edit[]) */
static const param_desc_t PHYS_D_HARM = {"HARM", F_PCT, 0, 127, 64, 0, 0};
static const param_desc_t PHYS_D_BEND = {"BEND", F_PCT, 0, 127, 0, 0, 0};
static const param_desc_t PHYS_D_CHRD = {"CHRD", F_INT, 0, 127, 64, N_PHYS_CHORD, 0};
static const param_desc_t PHYS_D_SYMP = {"SYMP", F_PCT, 0, 127, 20, 0, 0};
static const param_desc_t PHYS_D_BUZZ = {"BUZZ", F_PCT, 0, 127, 0, 0, 0};
static const param_desc_t *const PHYS_DESC[PM_COUNT][8] = {
    [PM_MEMB] = {0, &PHYS_D_HARM, 0, 0, 0, 0, &PHYS_D_BEND, 0},
    [PM_SYMP] = {0, &PHYS_D_CHRD, 0, 0, &PHYS_D_SYMP, 0, &PHYS_D_BUZZ, 0},
};

static const param_desc_t *phys_desc(const track_t *t, uint32_t k)
{
    return k < 8u ? PHYS_DESC[(uint32_t)t->p[P_E0] % PM_COUNT][k] : 0;
}

/* SYMP: the sympathetic strings above the note, cents (CHRD's names) */
static const int16_t PHYS_CHORDS[7][PX_NSYMP] = {
    {-1200, 1200, 2400}, {700, 1200, 1900}, {500, 1200, 1700}, {400, 700, 1200},
    {300, 700, 1200}, {200, 700, 1400}, {400, 700, 1000},
};
static const int8_t PHYS_DETUNE[PX_NSYMP] = {1, -1, 0};   /* 1/16 semitone (6 cents): the strings beat a little */

/* a project or user preset saved before 1.0 (MODEL 2 was DUST): its E values as today's */
static void phys_legacy(int16_t *e)
{
    if (e[0] == 2) {                                     /* DUST -> MODAL bowed (random hits into resonators) */
        e[0] = PM_MODAL;
        e[6] = 64;
    }
}

static phys_slot_t *phys_slot_of(track_t *t, voice_t *v)
{
    uint32_t i;
    if (t < &trk[0] || t >= &trk[NPART])
        return 0;
    i = (uint32_t)(v - t->v);
    return i < PHYS_POLY ? &phys_slots((uint32_t)(t - trk))[i] : 0;
}

/* a clean state for model md */
static void phys_reset(phys_slot_t *S, uint32_t md, uint32_t seed)
{
    uint32_t i, *w = (uint32_t *)&S->u;
    for (i = 0; i < sizeof S->u / 4u; i++)
        w[i] = 0;
    S->model = (uint8_t)md;
    seed |= 1u;
    switch (md) {
    case PM_STRING:
    case PM_SYMP:                                        /* (the main string is the union's px_string_t) */
        S->u.s.rng = seed;
        S->u.s.src = 1u << 30;
        break;
    default:
        S->u.m.rng = seed;
        break;
    }
}

static void phys_note_on(track_t *t, voice_t *v)
{
    phys_slot_t *S = phys_slot_of(t, v);
    uint32_t md = (uint32_t)t->p[P_E0] % PM_COUNT;
    if (!S)
        return;
    if ((!v->env && !v->env_out) || S->model != md)     /* a fresh voice (not a retrigger): from rest */
        phys_reset(S, md, rng());
    switch (md) {
    case PM_MODAL:
    case PM_MEMB:
        S->u.m.trig = 1;                                 /* a retrigger strikes the ringing body again */
        break;
    default:                                             /* STRNG, SYMP */
        S->u.s.trig = 1;
        break;
    }
}

/* SYMP: the sympathetic strings' pitches (cycles Q32) by CHRD: a chord above the note, or ROOT */
static __attribute__((noinline)) void phys_symp_pitches(const track_t *t, const vmod_t *m, uint32_t *f)
{
    uint32_t c = (uint32_t)t->p[P_E1] >> 4, k;          /* 0..7 */
    for (k = 0; k < PX_NSYMP; k++) {
        int32_t p16 = PHYS_DETUNE[k];
        p16 += c < 7u ? m->pitch16 + ((PHYS_CHORDS[c][k] * 41) >> 8)   /* cents -> 1/16 semitones (0.16) */
                      : (48 + t->p[P_ROOT] + (k == 1u ? 7 : k == 2u ? 12 : 0)) * 16;
        f[k] = tuned_pitch_inc(clamp(p16, 0, 2047));
    }
}

static void phys_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    const int16_t *p = t->p;
    phys_slot_t *S = phys_slot_of(t, v);
    uint32_t md = (uint32_t)p[P_E0] % PM_COUNT, i, f0 = m->inc;
    int32_t y[CTL], ax[CTL], bd[3], pos = p[P_E4];      /* bd: STRC, BRIT, DAMP (Q16 0..1) */
    int32_t exc = p[P_E7] * (md == PM_MODAL ? 1032 : md == PM_MEMB ? 516 : 258);   /* exc Q15 (a strike's click is
                                                                                    * short: x4, x2, else unheard; x16
                                                                                    * and x8 clicked, 2026-11-01) */
    int32_t bow = p[P_E6] ? 9830 + p[P_E6] * 129 : 0;   /* BOW: Dust at 0.15 .. 0.4 (DaisySP's sustain: 1; below
                                                         * 0.15 all but silent) */
    int32_t acc = clamp(p[P_E5] * v->vel * 4, 0, 65536);   /* ACC at full velocity, Q16 */
    int32_t g = (PHYS_GAIN[md] * phys_lvl(PHYS_LVL[md][0], p[P_E2]) >> 5) * phys_lvl(PHYS_LVL[md][1], p[P_E3]) >> 5;
    if (!S)
        return;
    phys_body(p, md, m->cutoff, bd);                     /* (ENV / LFO -> FLT: a click 516 of cutoff * 2) */
    if (bow && md <= PM_STRING)                         /* bowed: by BRIT, and 6 dB an octave from C4 (its */
        g = px_m(g * phys_lvl(PHYS_BOWL[md], p[P_E2]) >> 5,   /* Dust's low-pass follows the note), 2 at most */
                 (int32_t)px_exp2(clamp((960 - m->pitch16) * (md ? 170 : 341), -2 << 16, 2 << 16)), 16);   /* (STRNG 3) */
    if (md == PM_SYMP)                                   /* the strings ringing along add up to 6 dB: 1 at SYMP 60 */
        g = px_m(g, (256 - pos) * 334, 16);
    if (n > CTL)
        n = CTL;
    if (S->model != md)                                 /* MODEL changed under a sounding note */
        phys_reset(S, md, rng());
    switch (md) {
    case PM_MODAL: {
        px_modal_blk_t B;
        px_modal_block(&B, &S->u.m, f0, bd[0], bd[1], bd[2], acc, pos * 129, bow);   /* pos 0..1/4 */
        px_modal_run(&B, &S->u.m, y, ax, n);
        if (B.strike && exc)
            phys_knock_in(ax, n);
        break;
    }
    case PM_MEMB: {
        px_modal_blk_t B;
        px_memb_block(&B, &S->u.m, f0, bd[0], bd[1], bd[2], acc, pos * 516, p[P_E6] * 6192);   /* BEND 0..12 st */
        px_modal_run(&B, &S->u.m, y, ax, n);
        if (B.strike && exc)
            phys_knock_in(ax, n);
        break;
    }
    case PM_STRING: {
        px_string_blk_t B;
        px_string_block(&B, &S->u.s, f0, bd[0], bd[1], phys_bridge_damp(bd[0], bd[2]), acc, bow, pos * 516);   /* 0..1/2
                                                                                                                 * period */
        px_string_excite(&B, &S->u.s, ax, n);
        px_string_run(&B, &S->u.s, ax, y, n);
        break;
    }
    default: {                                           /* SYMP */
        px_string_blk_t B;
        uint32_t f[PX_NSYMP];
        int32_t bz = PHYS_BUZZ(p[P_E6]);
        phys_symp_pitches(t, m, f);
        px_string_block(&B, &S->u.y.s, f0, bz, bd[1], phys_bridge_damp(bz, bd[2]), acc, 0, 0);
        px_string_excite(&B, &S->u.y.s, ax, n);
        px_string_run(&B, &S->u.y.s, ax, y, n);
        px_symp_block(&S->u.y, f, bd[1], bd[2], pos * 516);
        px_symp_run(&S->u.y, y, y, n);
        break;
    }
    }
    for (i = 0; i < n; i++) {
        int32_t s = px_m(y[i] + px_m(ax[i], exc, 15), g, 20), a = s < 0 ? -s : s;
        if (a > 24000) {                                 /* soft knee: only peaks saturate */
            a = 24000 + (softclip(clamp(a - 24000, 0, 1 << 20) * 2) >> 1);
            s = s < 0 ? -a : a;
        }
        out[i] += voice_amp(s, m, i) << 1;
    }
}

/* MODAL partials by STRC (PHYS_MAP): 6 compressed 1 : 1.96 : 2.89 (a gong's cluster), 16 .. 48 harmonic, 84
 * 1 : 2.81 : 5.43 (a free bar, a chime: 1 : 2.76 : 5.40), 104 1 : 3.40 : 7.2, 124 1 : 3.99 : 8.9 (a tuned bar: 1 : 4 : 10).
 * The first nine kept their places (2026-10-31: retuned for the new knob curves), the rest appended; each within ~3 dB
 * of the others over C3 .. C5 (the first 0.1 s) */
static const preset_t PHYS_PRESETS[] = {
    /* name, {MODEL, STRC, BRIT, DAMP, POS, ACC, BOW, EXC}, {A D S R}, fenv, mono */
    {"BELL TREE", {PM_MODAL, 84, 104, 106, 12, 100, 0, 10}, {0, 100, 127, 96}, 0, 0},
    {"MARIMBA", {PM_MODAL, 124, 40, 40, 24, 90, 0, 40}, {0, 90, 127, 52}, 0, 0},
    {"PLUCK", {PM_STRING, 36, 72, 60, 40, 96, 0, 0}, {0, 100, 127, 58}, 0, 0},
    {"BOWED METAL", {PM_MODAL, 104, 96, 112, 10, 80, 70, 0}, {60, 90, 127, 90}, 0, 0},
    {"KALIMBA", {PM_MODAL, 30, 20, 56, 100, 90, 0, 56}, {0, 100, 127, 60}, 0, 0},
    /* MEMB: {MODEL, HARM, BRIT, DAMP, POS, ACC, BEND, EXC} */
    {"HAND DRUM", {PM_MEMB, 112, 76, 56, 44, 100, 14, 24}, {0, 100, 127, 64}, 0, 0},
    {"TOMS", {PM_MEMB, 0, 52, 50, 30, 100, 40, 30}, {0, 100, 127, 70}, 0, 0},
    /* SYMP: {MODEL, CHRD, BRIT, DAMP, SYMP, ACC, BUZZ, EXC} */
    {"DRONE STRING", {PM_SYMP, 127, 80, 100, 64, 100, 92, 0}, {0, 100, 127, 90}, 0, 0},
    {"HARP", {PM_SYMP, 8, 62, 74, 60, 90, 0, 0}, {0, 100, 127, 80}, 0, 0},
    {"VIBES", {PM_MODAL, 124, 72, 100, 20, 80, 0, 12}, {0, 100, 127, 90}, 0, 0},
    {"GONG", {PM_MODAL, 6, 70, 118, 40, 100, 0, 16}, {0, 100, 127, 100}, 0, 0},
    {"SINGING BOWL", {PM_MODAL, 76, 64, 122, 30, 80, 40, 0}, {70, 100, 127, 100}, 0, 0},
    {"DULCIMER", {PM_STRING, 96, 90, 84, 24, 100, 0, 10}, {0, 100, 127, 70}, 0, 0},
    {"SITAR", {PM_STRING, 4, 80, 96, 30, 100, 0, 0}, {0, 100, 127, 80}, 0, 0},
    {"BOWED STRING", {PM_STRING, 40, 40, 90, 40, 90, 64, 0}, {50, 100, 127, 80}, 0, 0},
    {"TIMPANI", {PM_MEMB, 40, 40, 100, 20, 110, 4, 10}, {0, 100, 127, 90}, 0, 0},
    {"SYMPATHY", {PM_SYMP, 20, 50, 112, 90, 60, 0, 0}, {40, 100, 127, 100}, 0, 0},
};

static const engine_t ENG_PHYS = {
    .name = "PHYS",
    .page_title = {"BODY", "EXCT"},
    .edit = {
        {"MODEL", F_ENUM, 0, PM_COUNT - 1, 0, N_PHYS_MODEL, 0},
        {"STRC", F_PCT, 0, 127, 64, 0, 0},
        {"BRIT", F_PCT, 0, 127, 80, 0, 0},
        {"DAMP", F_PCT, 0, 127, 80, 0, 0},
        {"POS", F_PCT, 0, 127, 20, 0, 0},
        {"ACC", F_PCT, 0, 127, 90, 0, 0},
        {"BOW", F_PCT, 0, 127, 0, 0, 0},
        {"EXC", F_PCT, 0, 127, 0, 0, 0},
    },
    .presets = PHYS_PRESETS,
    .npresets = NELEM(PHYS_PRESETS),
    .note_on = phys_note_on,
    .render = phys_render,
    .knob = {P_E1, P_E2, P_E3, P_E4},
    .poly = PHYS_POLY,
    .desc = phys_desc,
};
