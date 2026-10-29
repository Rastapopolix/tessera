/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* ENERGY, its first part: the project's slow tide and its rests, after Zen-1's conductor (aceyprime/zen-1, GPL-3.0-only,
 * its arc and rests, themselves after Charles Vestal's AMB-1); our own code.
 *   ARC   0..100: how far a slow tide swings, a sine 3-6 min end to end (each turn a new length), from full (+1: the
 *         tracks as set) to sparse (-1: up to 70 % more of a generator's hits left out, up to 16 softer). 0 off.
 *   RESTS 0..100: how often the notes rest, 8-18 s each: at 100 about every 4 min, at 1 about every 20 (sooner while
 *         the tide is sparse). A rest leaves out the generator tracks' hits, except CHORD's: a held chord and its
 *         BREATH go on. 0 off.
 * Part 2 (ENERGY_2): LEVEL -100..+100 the energy the tide swings round (0 as set); DEPTH a track's share of it (100 all,
 *   0 an anchor that neither the tide, LEVEL nor RESTS move). Above 0 (full) a generator's hits take back up to all of
 *   its TWEAK DROP and play up to 12 louder; EVOLVE's AMOUNT goes x0.5 (-1) .. x1.5 (+1). LEVEL 0, ARC 0: nothing changes.
 * ENERGY (LEVEL and the tide) is a cord source (cord.c CS_ENERGY), the same for every track. Both run while the transport runs,
 * an audio block at a time (cord_tick); PLAY starts them from the project's SEEDs, so a piece replays. Project-wide
 * settings: tessera.en_arc, en_rests (the Tessera block's tail since TS18). Included by tessera.c. */

#define EN_BPS 1378u                                  /* audio blocks a second (44100 / 32: CTL), about */
static struct {
    uint32_t ph, per;                                 /* the tide: blocks into this turn, the turn's length */
    uint32_t rest_left, rest_next;                    /* blocks left of a rest; to the next */
    uint32_t rnd;                                     /* its random stream */
    int32_t e;                                        /* the tide now, Q15: -32767 sparse .. 32767 full */
} energy;

static uint32_t en_rnd(uint32_t n)                    /* 0..n-1 */
{
    uint32_t k = energy.rnd += 0x9E3779B9u;
    k = (k ^ (k >> 16)) * 0x7FEB352Du;
    k = (k ^ (k >> 15)) * 0x846CA68Bu;
    return n ? (k ^ (k >> 16)) % n : 0u;
}
/* ENERGY now, Q15: LEVEL plus the tide, -32767 sparse .. 32767 full */
static int32_t en_all(void)
{
    int32_t e = energy.e + (int32_t)tessera.en_level * 32767 / 100;
    return e < -32767 ? -32767 : e > 32767 ? 32767 : e;
}
/* the energy track tr feels: ENERGY times its DEPTH */
static int32_t en_track(uint32_t tr)
{
    return tr < TESSERA_TRACKS ? en_all() * (int32_t)tessera.en_depth[tr] / 100 : en_all();
}
static uint32_t en_sparse(void) { int32_t e = en_all(); return e < 0 ? (uint32_t)-e : 0u; }   /* 0..32767 */
static void en_rest_draw(void)                        /* the time to the next rest, from RESTS and the tide */
{
    uint32_t r = tessera.en_rests ? tessera.en_rests : 1u, s = 240u + (100u - r) * 48u / 5u;   /* 240 .. 1190 s */
    s = s * (50u + en_rnd(101u)) / 100u;              /* (half .. one and a half) */
    s -= s * 2u * (en_sparse() >> 7) / 1280u;         /* (sparse: up to 40 % sooner) */
    energy.rest_next = s * EN_BPS;
}
static void en_turn(void) { energy.per = (180u + en_rnd(181u)) * EN_BPS; }   /* a tide 3-6 min */
void energy_play(void)                                /* seq.c seq_start: from the project's SEEDs */
{
    uint32_t tr, h = 0x454E4552u;
    for (tr = 0; tr < TESSERA_TRACKS; tr++) h = (h ^ tessera.seed[tr]) * 16777619u;
    energy.rnd = h;
    energy.ph = 0;
    energy.e = 0;
    energy.rest_left = 0;
    en_turn();
    en_rest_draw();
}
/* ph of per as 0..n-1 (n up to 65536, ph < per), in 32 bits: the device has no 64-bit divide */
static uint32_t en_frac(uint32_t ph, uint32_t per, uint32_t n)
{
    while (per >= 65536u) { ph >>= 1; per >>= 1; }
    return per ? ph * n / per : 0u;
}
/* a turn's sine, Q15, from blocks into it (a parabola a half: within 6 % of a sine, for a tide plenty) */
static int32_t en_sin(uint32_t ph, uint32_t per)
{
    uint32_t x = en_frac(ph, per, 65536u), h = x & 32767u;                  /* (a half: 0..32767) */
    int32_t v = (int32_t)(h * (32768u - h) >> 13);                          /* 4 h (1 - h), Q15 */
    v = v > 32767 ? 32767 : v;
    return x & 32768u ? -v : v;
}
static void energy_tick(void)                         /* an audio block (cord_tick) */
{
    if (!song.playing) return;
    if (++energy.ph >= energy.per) {
        energy.ph = 0;
        en_turn();
    }
    energy.e = tessera.en_arc ? en_sin(energy.ph, energy.per) * (int32_t)tessera.en_arc / 100 : 0;
    if (!tessera.en_rests)
        energy.rest_left = 0;
    else if (energy.rest_left)
        energy.rest_left--;
    else if (!energy.rest_next || !--energy.rest_next) {
        energy.rest_left = (8u + en_rnd(11u)) * EN_BPS;   /* 8-18 s */
        en_rest_draw();
    }
}
static void energy_rests_on(void) { en_rest_draw(); }   /* RESTS from 0: the next one counted from now */
int32_t energy_now(void) { return en_all(); }          /* the ENERGY cord source */
int energy_resting(void) { return energy.rest_left != 0u; }
/* track tr's TWEAK DROP as ENERGY leaves it: above 0, up to all of it taken back (no hit added) */
static uint32_t energy_drop(uint32_t tr, uint32_t drop)
{
    int32_t e = en_track(tr);
    return e > 0 ? drop - (drop * (uint32_t)e + 16383u) / 32767u : drop;
}
/* EVOLVE's AMOUNT as ENERGY scales it: x0.5 at -1 .. x1.5 at +1, at most 100 */
static uint32_t energy_amount(uint32_t tr, uint32_t amt)
{
    int32_t e = en_track(tr);
    if (!e) return amt;
    amt = amt * (uint32_t)(65534 + e) / 65534u;
    return amt > 100u ? 100u : amt;
}

/* a generator's hit of track tr (not CHORD's): 1 kept, 0 left out (a rest, the sparse tide); *vel softened. At ARC 0
 * and no rest it draws nothing: the track plays as it would */
static int energy_hit(uint32_t tr, uint8_t *vel)
{
    int32_t e;
    uint32_t s;
    if (tr < TESSERA_TRACKS && !tessera.en_depth[tr]) return 1;   /* (an anchor) */
    if (energy.rest_left) return 0;
    e = en_track(tr);
    if (e > 0) {                                      /* full: up to 12 louder */
        if (vel) {
            int32_t v = (int32_t)*vel + e * 12 / 32767;
            *vel = (uint8_t)(v > 127 ? 127 : v);
        }
        return 1;
    }
    if (!(s = (uint32_t)-e)) return 1;
    if (en_rnd(32768u) < s * 7u / 10u) return 0;      /* (up to 70 %) */
    if (vel) {
        int32_t v = (int32_t)*vel - (int32_t)(s * 16u / 32767u);
        *vel = (uint8_t)(v < 1 ? 1 : v);
    }
    return 1;
}
