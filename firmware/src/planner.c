/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* The piece planner (long form, slice 3): a LENGTH for the whole piece and the SHAPE it takes. The PIECE page (GLO):
 * LENGTH FREE (the default: nothing planned) or 1 minute to 4 hours; SHAPE ARCH, RISE, FALL, PLATEAU, VALLEY, WAVES,
 * BUILDS, STEPS, FLAT or USER; PEAK (where ARCH, PLATEAU and VALLEY have their centre, a share of the length) or COUNT
 * (WAVES' swells, BUILDS' builds, STEPS' terraces, 2-8); END STOP, LOOP, NEW or FREE. OCT+ held: USER's points 1-4,
 * OCT- held: 5-8 (eight levels evenly along the piece).
 *   The shape, 0 sparse .. 1 full along the piece, is read where the music is: in phrases (the CHAIN's PHRASE) from
 *   PLAY, at the tempo now, so a tempo change moves the plan with it. It steers what slices 1 and 2 do, adding no system
 *   of its own: ENERGY's tide is the shape while a length is set (ARC how far it swings, LEVEL and RESTS as before);
 *   Entries keeps out as many tracks as the shape is low (of what ENTRIES allows: at 0 no track moves), so a piece
 *   layers in, thins and fills with it, and layers out at its end, one track a phrase between sections; the CHAIN's new
 *   sections depart further near the top (up to +20 CONTRAST) and the last 15 % of the piece (12 phrases at most) is
 *   A, its home. Every shape
 *   but RISE and FALL begins and ends sparse (the intro, the outro).
 *   At the end, on a phrase line: STOP lets the tracks fade out and the tails ring, then stops; LOOP plays the same piece
 *   again from the top; NEW draws new SEEDs and plays a new one; FREE carries on unplanned (the tide back) until STOP.
 *   Included by tessera.c after form_chain.c, before entries.c. */

enum { PL_ARCH, PL_RISE, PL_FALL, PL_PLATEAU, PL_VALLEY, PL_WAVES, PL_BUILDS, PL_STEPS, PL_FLAT, PL_USER, PL_SHAPES };
enum { PL_END_STOP, PL_END_LOOP, PL_END_NEW, PL_END_FREE, PL_ENDS };
#define PL_LENGTHS 22u
static const uint16_t PL_MIN[PL_LENGTHS] = {0, 1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 20, 25, 30, 40, 45, 60, 75, 90, 120, 180, 240};
#define PL_POINTS 8u                                  /* USER: levels at 0, 1/7 .. 1 of the piece */
#define PL_HOME_MAX 12u                               /* the last 15 % is A, at most 12 phrases */

static void entries_all_out(void);                   /* (entries.c) */
static struct {
    uint8_t done;                                     /* this PLAY's piece is over, END FREE: unplanned from here */
    uint8_t ending;                                   /* END STOP: the tails ringing */
    uint32_t stop_in;                                 /* blocks to the stop */
} plan;

static uint32_t pl_secs(void) { return tessera.pl_length < PL_LENGTHS ? PL_MIN[tessera.pl_length] * 60u : 0u; }
/* a piece is planned now: a LENGTH set, and this PLAY's not over */
static int pl_on(void) { return pl_secs() && !plan.done; }
static uint32_t pl_beats(void) { uint32_t b = beat_samples_set(); return pl_secs() * (uint32_t)FS / (b ? b : 1u); }   /* (at the tempo now:
                                                                * BPM's, a TEMPO mod plays the piece faster or slower, never jumps it) */

/* where beat b (since PLAY) is in the piece, Q16: 0 .. 65536 its end (and past it, 65536) */
static uint32_t pl_at(uint32_t b)
{
    uint32_t n = pl_beats();
    if (!n) return 0;
    if (b >= n) return 65536u;
    return en_frac(b, n, 65536u);
}
static uint32_t pl_phrase_at(uint32_t p) { return pl_at((p * 2u + 1u) * fc_bars() * 2u); }   /* (a phrase's middle) */

/* ----------------------------------------------------------------- the shapes --- */
static int32_t pl_smooth(int32_t t)                   /* Q15 0..32767: eased (3t^2 - 2t^3) */
{
    t = t < 0 ? 0 : t > 32767 ? 32767 : t;
    return (int32_t)((uint32_t)(t * t >> 15) * (uint32_t)(98301 - 2 * t) >> 15);   /* (32 bits: no 64-bit divide here) */
}
static int32_t pl_ramp(uint32_t x, uint32_t a, uint32_t b)   /* Q16 x from a to b as Q15 0..32767, eased */
{
    if (x <= a) return 0;
    if (x >= b || b <= a) return 32767;
    return pl_smooth((int32_t)((x - a) * 32767u / (b - a)));
}
static int32_t pl_env(uint32_t x)                     /* the intro and the outro: up over 12 %, down over the last 12 % */
{
    int32_t u = pl_ramp(x, 0, 7864u), d = pl_ramp(65536u - x, 0, 7864u);
    return u < d ? u : d;
}
static int32_t pl_mix(int32_t a, int32_t b, int32_t t) { return a + (b - a) * t / 32767; }
static uint32_t pl_peak_x(void)                       /* PEAK as Q16, kept inside 10..90 % */
{
    uint32_t p = tessera.pl_peak < 10u ? 10u : tessera.pl_peak > 90u ? 90u : tessera.pl_peak;
    return p * 65536u / 100u;
}
static uint32_t pl_count(void) { return tessera.pl_count < 2u ? 2u : tessera.pl_count > 8u ? 8u : tessera.pl_count; }

/* the SHAPE at x (Q16 0..65536): Q15, 0 sparse .. 32767 full */
static int32_t pl_shape(uint32_t x)
{
    uint32_t pk = pl_peak_x(), n = pl_count(), u, a, b;
    int32_t y;
    if (x > 65536u) x = 65536u;
    switch (tessera.pl_shape) {
    case PL_RISE: return pl_mix(3277, 32767, pl_ramp(x, 0, 65536u));
    case PL_FALL: return pl_mix(32767, 3277, pl_ramp(x, 0, 65536u));
    case PL_PLATEAU:
        a = pk > 16384u + 6554u ? pk - 16384u : 6554u;            /* (the hold: PEAK +- 25 %, inside 10..90 %) */
        b = pk + 16384u < 58982u ? pk + 16384u : 58982u;
        return x < a ? pl_ramp(x, 0, a) : x > b ? 32767 - pl_ramp(x, b, 65536u) : 32767;
    case PL_VALLEY:                                   /* full, down to a quarter round PEAK (+- 20 %), full again */
        a = x > pk ? x - pk : pk - x;
        y = a < 13107u ? 32767 - 24575 * (32767 - pl_ramp(a, 0, 13107u)) / 32767 : 32767;
        return y * pl_env(x) / 32767;
    case PL_WAVES:                                    /* COUNT swells over the intro and outro */
        u = (x * n) & 0xFFFFu;
        y = (int32_t)(u * (65536u - u) >> 15);           /* 4u(1-u), Q15 */
        y = y > 32767 ? 32767 : y;
        return pl_mix(9830, 32767, y) * pl_env(x) / 32767;
    case PL_BUILDS:                                   /* COUNT rises, each dropped at once */
        u = (x * n) & 0xFFFFu;
        return pl_mix(8192, 32767, (int32_t)(u >> 1)) * pl_env(x) / 32767;
    case PL_STEPS: {                                  /* the ARCH in COUNT terraces */
        int32_t arch = x < pk ? pl_ramp(x, 0, pk) : 32767 - pl_ramp(x, pk, 65536u);
        int32_t q = (arch * (int32_t)n + 16383) / 32767;
        return (q < 1 ? 1 : q) * 32767 / (int32_t)n;
    }
    case PL_FLAT: return pl_env(x) * 4 / 5;
    case PL_USER: {
        uint32_t seg = x * (PL_POINTS - 1u), i = seg >> 16;
        int32_t v0, v1;
        if (i >= PL_POINTS - 1u) return (int32_t)tessera.pl_user[PL_POINTS - 1u] * 32767 / 100;
        v0 = (int32_t)tessera.pl_user[i] * 32767 / 100;
        v1 = (int32_t)tessera.pl_user[i + 1u] * 32767 / 100;
        return pl_mix(v0, v1, (int32_t)((seg & 0xFFFFu) >> 1));
    }
    default:                                          /* ARCH: up to PEAK, down to the end */
        return x < pk ? pl_ramp(x, 0, pk) : 32767 - pl_ramp(x, pk, 65536u);
    }
}

/* ------------------------------------------------------ what it steers --- */
/* energy.c energy_tick: the shape where the music is now, as ENERGY's tide (-32767 .. 32767), 1 while planned */
static int planner_energy(int32_t *e)
{
    if (!pl_on() || !song.playing) return 0;
    *e = 2 * pl_shape(pl_at(bar_n * 4u + beat_n)) - 32767;
    return 1;
}
/* form_chain.c fc_step: the CHAIN at phrase p is home for good (the piece's last 15 %, 12 phrases at most) */
static int planner_home(uint32_t p)
{
    uint32_t ph = fc_bars() * 4u, all = ph ? pl_beats() / ph : 0u, home = all * 15u / 100u;
    if (!pl_on()) return 0;
    if (home > PL_HOME_MAX) home = PL_HOME_MAX;       /* (a long piece: not a coda of a quarter hour on one section) */
    return p + home >= all;
}
/* form_chain.c: a new section at phrase p departs further near the top: up to +20 CONTRAST */
static uint32_t planner_contrast(uint32_t p) { return pl_on() ? (uint32_t)pl_shape(pl_phrase_at(p)) * 20u / 32767u : 0u; }
/* entries.c: of `most` tracks Entries may take out, how many the shape wants out at phrase p */
static uint32_t planner_out(uint32_t p, uint32_t most) { return (uint32_t)((32767 - pl_shape(pl_phrase_at(p))) * (int32_t)most + 16383) / 32767u; }

/* seq.c seq_start */
void planner_play(void) { memset(&plan, 0, sizeof plan); }

/* each block (entries_tick): the end, on the phrase line at or past the LENGTH */
static void planner_tick(uint32_t n)
{
    uint32_t b = bar_n, bars = fc_bars(), tr;
    if (!song.playing || !pl_secs() || plan.done) return;
    if (plan.ending) {                                /* (END STOP: the fades and tails, then the stop) */
        if (plan.stop_in > n) plan.stop_in -= n;
        else {
            plan.done = 1;
            transport_req = 2;
        }
        return;
    }
    if (b % bars || beat_n || pl_at(b * 4u) < 65536u) return;
    switch (tessera.pl_end) {
    case PL_END_LOOP: transport_req = 3; plan.done = 1; break;    /* (the same SEEDs: the same piece) */
    case PL_END_NEW:
        for (tr = 0; tr < TESSERA_TRACKS; tr++) {
            uint32_t h = (tessera.seed[tr] + 0x9E37u * (tr + 1u)) * 2654435761u;
            tessera.seed[tr] = (uint16_t)(h >> 16 ? h >> 16 : 1u);
        }
        transport_req = 3;
        plan.done = 1;
        break;
    case PL_END_FREE: plan.done = 1; break;
    default:                                          /* STOP: everything out (its FADE), 2 bars more of tails */
        plan.ending = 1;
        plan.stop_in = ((tessera.et_style ? tessera.et_fade : 0u) + 2u) * 4u * seq_beat_samples();
        entries_all_out();
        break;
    }
}

/* the PIECE page: where the piece is (Q16, 0 when FREE or stopped), the seconds left, 1 while planned */
int tessera_piece_get(uint32_t *x, uint32_t *left)
{
    uint32_t s = pl_secs(), beats = bar_n * 4u + beat_n, el;
    *x = 0;
    *left = s;
    if (!s) return 0;
    if (!song.playing) return 1;
    if (plan.done) { *x = 65536u; *left = 0; return 0; }
    *x = pl_at(beats);
    el = beats * (beat_samples_set() / 441u) / 100u;  /* (seconds played, at the tempo set) */
    *left = el < s ? s - el : 0u;
    return 1;
}
int32_t tessera_piece_shape(uint32_t x) { return pl_shape(x); }   /* the PIECE picture: the curve */
