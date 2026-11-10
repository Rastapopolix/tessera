/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* GENERATIVE.md's recipes, played: two of its patches set up as the guide says, from a new project, and rendered through
 * mix_block (the LFOs tick in the render), to show they do what the guide says they do.
 *   The Krell (An envelope that plays itself): T1 MARKOV DENS 100, LENGTH HIT, LFO RATE 35 TRI;
 *     T1 EOC -> T1 CLOCK +100, T1 S&H -> T1 LFO RTE +20, T1 LFO -> T1 GAIN +100. A step on each LFO cycle start and on
 *     nothing else, every cycle its own length.
 *   Long gates: T3 TURING; T1 LFO RATE 20, T2 LFO RATE 25; T1 LFO -> BUS A +50, T2 LFO -> BUS A +50,
 *     BUS A^ -> T3 PLAY +100. Track 3 heard in stretches of different lengths, its pattern not repeating within the
 *     two LFOs' first minutes; with no mod onto PLAY it is always heard. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

#define SEC (FS / CTL)                                  /* blocks a second */
static int32_t out[2 * CTL];
static void one(void) { mix_block(out, CTL); }
static uint32_t steps(uint32_t tr) { return seq_run_step[tr] + 1u; }

static void fresh(void)
{
    uint32_t i;
    ui_power_on();
    for (i = 0; i < NTRK; i++) memset(trk[i].step, 0, sizeof trk[i].step);
    tessera_reset();
    cord_clear();
    song.g[G_BPM] = 120;
    one();
}
static void cord(uint32_t i, uint32_t src, uint32_t strk, uint32_t dst, uint32_t dtrk, int amount, uint8_t uni)
{
    cord_t c = {(uint8_t)src, (uint8_t)strk, (uint8_t)dst, (uint8_t)dtrk, (int8_t)amount, 0, uni};
    cord_set(i, &c);
}
static void play(void) { transport_req = 1; one(); }
static void stop(void) { transport_req = 2; one(); one(); }

static int krell(void)
{
    int bad = 0;
    uint32_t i, s, n = 0, wraps = 0, late = 0, last = 0, gap[64], ngap = 0, lo = ~0u, hi = 0, eoc_was = 0, since = ~0u;
    fresh();
    tessera_param_set(0, TX_MODE, TESS_ENG_MARKOV);
    tessera_param_set(0, TX_MKV_DENS, 100);
    tessera_param_set(0, TX_NL_MODE, 2);                /* LENGTH HIT */
    trk[0].p[P_LRATE] = 35;
    trk[0].p[P_LWAVE] = 1;                              /* TRI */
    cord(0, CS_EOC, 0, CD_CLOCK, 0, 100, CU_GATE);
    cord(1, CS_SH, 0, CD_LFO_RATE, 0, 20, CU_BI);
    cord(2, CS_LFO, 0, CD_GAIN, 0, 100, CU_BI);
    play();
    s = steps(0);
    for (i = 0; i < 120u * SEC; i++) {
        uint32_t e;
        one();
        e = (uint32_t)lfo_eoc_now(&trk[0]);
        if (e && !eoc_was) wraps++, since = 0;
        else if (since != ~0u) since++;
        eoc_was = e;
        if (steps(0) != s) {
            s = steps(0);
            n++;
            if (since > 3u) late++;                     /* (a step more than a few blocks after a cycle start) */
            if (last && ngap < 64u) {
                gap[ngap++] = i - last;
                if (i - last < lo) lo = i - last;
                if (i - last > hi) hi = i - last;
            }
            last = i;
        }
    }
    stop();
    printf("recipes: Krell, 120 s: %u cycles, %u steps, %u not on a cycle start, gaps %.2f .. %.2f s\n", wraps, n, late,
           (double)lo / SEC, (double)hi / SEC);
    bad += check("Krell: a step at each LFO cycle start, and none in between", n >= 10u && late == 0u &&
                 (n == wraps || n + 1u == wraps));
    bad += check("...each cycle its own length (S&H onto its own rate): the gaps spread over 1.5 x or more",
                 ngap >= 8u && hi * 2u >= lo * 3u);
    {
        uint32_t same = 0, k;
        for (k = 1; k < ngap; k++) same += gap[k] == gap[k - 1u];
        bad += check("...and no two in a row the same", same * 4u < ngap);
    }
    return bad;
}

/* track 3's PLAY over secs: the stretches heard and shut, in blocks */
static uint32_t stretches(uint32_t secs, uint32_t *open, uint32_t *shut, uint32_t *nopen, uint32_t *nshut, uint32_t *flips)
{
    uint32_t i, run = 0, was = 2, heard = 0;
    *nopen = *nshut = *flips = 0;
    for (i = 0; i < secs * SEC; i++) {
        uint32_t now;
        one();
        now = (uint32_t)!cord_play_shut(2);
        heard += now;
        if (now != was && was != 2u) {
            if (was && *nopen < 64u) open[(*nopen)++] = run;
            if (!was && *nshut < 64u) shut[(*nshut)++] = run;
            (*flips)++;
            run = 0;
        }
        was = now;
        run++;
    }
    return heard;
}

static int long_gates(void)
{
    int bad = 0;
    uint32_t open[64], shut[64], no, ns, fl, heard, k, lo = ~0u, hi = 0, dist = 0;
    fresh();
    tessera_param_set(2, TX_MODE, TESS_ENG_TURING);
    play();
    heard = stretches(60, open, shut, &no, &ns, &fl);
    bad += check("Long gates: no mod onto PLAY: track 3 heard all the time", heard == 60u * SEC && fl == 0u);
    stop();

    trk[0].p[P_LRATE] = 20;
    trk[1].p[P_LRATE] = 25;
    cord(0, CS_LFO, 0, CD_BUS_A, 0, 50, CU_BI);
    cord(1, CS_LFO, 1, CD_BUS_A, 0, 50, CU_BI);
    cord(2, CS_BUSA, 0, CD_PLAY, 2, 100, CU_GATE);
    play();
    heard = stretches(240, open, shut, &no, &ns, &fl);
    stop();
    for (k = 1; k < no; k++) {                          /* (the first stretch is cut by PLAY) */
        if (open[k] < lo) lo = open[k];
        if (open[k] > hi) hi = open[k];
    }
    for (k = 2; k < no; k++) {
        uint32_t j, again = 0;
        for (j = 1; j < k; j++) again |= open[j] == open[k];   /* (to the block) */
        dist += !again;
    }
    printf("recipes: Long gates, 240 s: %u flips, heard %u %%, stretches heard %.1f .. %.1f s, %u of %u lengths new\n",
           fl, heard * 100u / (240u * SEC), (double)lo / SEC, (double)hi / SEC, dist, no > 2u ? no - 2u : 0u);
    bad += check("...the sum of two LFOs compared: track 3 comes and goes, heard a part of the time", fl >= 16u &&
                 heard > 240u * SEC / 5u && heard < 240u * SEC * 4u / 5u);
    bad += check("...in long stretches of different lengths (the shortest under half the longest, none a blip)",
                 no >= 8u && lo * 2u < hi && lo > SEC / 4u);
    bad += check("...a rhythm that doesn't settle into a loop: most stretch lengths new", dist * 2u > no - 2u);
    return bad;
}

int main(void)
{
    int bad = krell() + long_gates();
    printf(bad ? "recipes: %d FAILED\n" : "recipes: all passed\n", bad);
    return bad != 0;
}
