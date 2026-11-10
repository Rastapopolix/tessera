/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* CLOCK (cord.h CK_CLOCK, cord.c cord_clock_tick, seq.c seq_grid_tick) through the real sequencer: a generator track with
 * a live CLOCK cord steps on the cord's edges alone, not on its DIV. No cord, or an AMOUNT of 0, plays exactly as before;
 * the first step is at PLAY; a gate's edge steps it two blocks on (one after cord_tick); a GATE source counts hits (the
 * track after it in the order steps in the same block, the one before it a block later); AMOUNT is the division, its sign
 * the edge; an edge within a DIV of the last step waits; a cord removed puts the track back on the grid; STOP clears
 * what waits; a STEP track and an ARP track ignore the cord; EOC is a source like any (its bit set by hand here).
 * Steps are counted off seq_run_step, a block at a time: events_block, then cord_tick, as mix_block has them. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

#define DIV_16TH 2u
#define DIV_8TH 1u
static uint32_t blk;
static void one(void)
{
    events_block(CTL);
    cord_tick();
    blk++;
}
static void blocks(uint32_t n) { while (n--) one(); }
static uint32_t steps(uint32_t tr) { return seq_run_step[tr] + 1u; }   /* (0xFFFFFFFF before PLAY: 0) */

static void gen(uint32_t tr, uint32_t mode, uint32_t div)
{
    song.sel = tr;
    tessera_knob(0, mode);
    song.sel = 0;
    trk[tr].p[P_SDIV] = (int16_t)div;
    tessera.seed[tr] = (uint16_t)(5 + tr);
    tessera.rnd[tr] = gen_stream(5u + tr);
}
static void setup(void)
{
    uint32_t i;
    ui_power_on();
    for (i = 0; i < NTRK; i++) {
        memset(trk[i].step, 0, sizeof trk[i].step);
        trk[i].p[P_SLEN] = 16;
        trk[i].p[P_SDIV] = DIV_16TH;
    }
    tessera_reset();
    cord_clear();
    song.g[G_BPM] = 120;
    usb.config = 1;
    mi_r = mi_w = 0; mo_r = mo_w = 0;
    settings_midi_out = MIDI_OUT_ALL; settings_clock_out = 0; settings_drumch = 0;
    song.g[G_ROUTE] = 0;
    gen(0, TESS_ENG_EUCLID, DIV_16TH);
    gen(1, TESS_ENG_EUCLID, DIV_8TH);
    tessera_param_set(1, TX_EUC_STEPS, 8);
    tessera_param_set(1, TX_EUC_PULSES, 8);   /* (a hit every step) */
    tessera.macro[0] = 0;
    blocks(1);
}
static void cord(uint32_t i, uint32_t src, uint32_t strk, uint32_t dst, uint32_t dtrk, int amount, int offset)
{
    cord_t c = {(uint8_t)src, (uint8_t)strk, (uint8_t)dst, (uint8_t)dtrk, (int8_t)amount, (int8_t)offset, CU_BI};
    cord_set(i, &c);
}
static void play(void) { transport_req = 1; blocks(1); }
static void stop(void) { transport_req = 2; blocks(2); }
/* MACRO 1 high for hi blocks every period, from block start (counted from now), n blocks in all; *at: the step count after
 * each block */
static uint32_t pulses(uint32_t n, uint32_t period, uint32_t hi, uint32_t tr, uint32_t *edge_at, uint32_t *step_at)
{
    uint32_t j, e = 0, s = steps(tr), seen = 0;
    for (j = 0; j < n; j++) {
        int on = j >= 10u && ((j - 10u) % period) < hi;
        if (on && !tessera.macro[0] && edge_at && e < 32u) edge_at[e++] = j;
        tessera.macro[0] = on ? 100u : 0u;
        one();
        if (steps(tr) != s) {
            if (step_at && seen < 32u) step_at[seen] = j;
            seen++;
            s = steps(tr);
        }
    }
    return seen;
}

int main(void)
{
    int bad = 0;
    uint32_t base, i, e[32], s[32], n, ok, period = seq_div_samples(DIV_16TH) / CTL;

    /* the ground: no cord, a CLOCK cord at AMOUNT 0 and a deleted one play the same, on the grid */
    setup(); play(); blocks(1500);
    base = steps(0);
    setup(); cord(0, CS_MACRO1, 0, CD_CLOCK, 0, 0, 50); play(); blocks(1500);
    bad += check("no cord / AMOUNT 0: the DIV clock as before (same steps)", steps(0) == base && base > 8u);
    setup(); cord(0, CS_MACRO1, 0, CD_CLOCK, 0, 100, 50); cord(0, 0, 0, CD_OFF, 0, 0, 0); play(); blocks(1500);
    bad += check("a deleted cord: the same", steps(0) == base);

    /* replaced: the DIV clock is gone; the first step at PLAY, the rest on the edges, two blocks on */
    setup(); cord(0, CS_MACRO1, 0, CD_CLOCK, 0, 100, 50); play();
    bad += check("CLOCK: the first step is at PLAY", steps(0) == 1u);
    blocks(period);                                     /* (an edge sooner than a DIV waits: tested below) */
    n = pulses(1500, 400, 3, 0, e, s);
    bad += check("...no step from DIV; one per rising edge of the gate", n == 4u && steps(0) == 5u);
    ok = n == 4u;
    for (i = 0; i < n && ok; i++) ok &= s[i] == e[i] + 1u;
    bad += check("...each a block after cord_tick saw it (macro set, events, cord_tick, then the step)", ok);
    bad += check("...another track (on the grid) is unaffected", steps(1) > 3u);

    /* the division: every n-th edge, the sign the edge */
    setup(); cord(0, CS_MACRO1, 0, CD_CLOCK, 0, 50, 50); play();
    n = pulses(2800, 400, 3, 0, 0, 0);
    bad += check("AMOUNT 50 %: every second edge (7 edges: 3 steps and the first)", n == 3u);
    setup(); cord(0, CS_MACRO1, 0, CD_CLOCK, 0, 33, 50); play();
    n = pulses(2800, 400, 3, 0, 0, 0);
    bad += check("AMOUNT 33 %: every third edge", n == 2u);
    setup(); cord(0, CS_MACRO1, 0, CD_CLOCK, 0, 10, 50); play();
    n = pulses(4800, 400, 3, 0, 0, 0);
    bad += check("AMOUNT 10 %: every tenth edge (11 edges: 1 step)", n == 1u);
    setup(); cord(0, CS_MACRO1, 0, CD_CLOCK, 0, -100, 50); play(); blocks(period);
    n = pulses(1500, 400, 3, 0, e, s);
    ok = n == 4u;
    for (i = 0; i < n && ok; i++) ok &= s[i] == e[i] + 3u + 1u;   /* (the gate closes 3 blocks after it opens) */
    bad += check("AMOUNT -100 %: the gate closing", ok);

    /* the refractory time: DIV long since the last step; an edge sooner waits (one deep) */
    setup(); cord(0, CS_MACRO1, 0, CD_CLOCK, 0, 100, 50); play();
    blocks(period * 2u);
    tessera.macro[0] = 100; blocks(3); tessera.macro[0] = 0;
    n = steps(0);
    blocks(10);
    tessera.macro[0] = 100; blocks(3); tessera.macro[0] = 0;
    bad += check("an edge in a step's first DIV: the step waits (not yet)", steps(0) == n);
    blocks(period);
    bad += check("...and plays when DIV is up, once", steps(0) == n + 1u);

    /* a GATE source counts hits */
    setup(); cord(0, CS_GATE, 1, CD_CLOCK, 0, 100, 0); play();
    {
        uint32_t hits = tessera.f_cnt[1], hb[32], sb[32], hn = 0, sn = 0, st = steps(0);
        for (i = 0; i < 1500u; i++) {
            one();
            if (tessera.f_cnt[1] != hits) { hits = tessera.f_cnt[1]; if (hn < 32u) hb[hn] = i; hn++; }
            if (steps(0) != st) { st = steps(0); if (sn < 32u) sb[sn] = i; sn++; }
        }
        ok = hn > 3u && sn == hn;
        for (i = 0; i < sn && i < hn && ok; i++) ok &= sb[i] == hb[i] + 1u;
        bad += check("GATE source: a step for each hit; a track before it in the order steps a block later", ok);
    }
    setup(); gen(2, TESS_ENG_EUCLID, DIV_16TH); trk[2].p[P_SDIV] = DIV_16TH;
    cord(0, CS_GATE, 1, CD_CLOCK, 2, 100, 0); play();
    {
        uint32_t hits = tessera.f_cnt[1], hb[32], sb[32], hn = 0, sn = 0, st = steps(2);
        for (i = 0; i < 1500u; i++) {
            one();
            if (tessera.f_cnt[1] != hits) { hits = tessera.f_cnt[1]; if (hn < 32u) hb[hn] = i; hn++; }
            if (steps(2) != st) { st = steps(2); if (sn < 32u) sb[sn] = i; sn++; }
        }
        ok = hn > 3u && sn == hn;
        for (i = 0; i < sn && i < hn && ok; i++) ok &= sb[i] == hb[i];
        bad += check("...a track after it steps in the same block (no flam for a FOLLOW)", ok);
    }
    setup(); cord(0, CS_GATE, 1, CD_CLOCK, 0, 50, 0); play(); blocks(1500);
    bad += check("...AMOUNT 50 %: every second hit", steps(0) - 1u == tessera.f_cnt[1] / 2u);

    /* EOC: a source like any (its bit set by hand: the LFO ticks in the render) */
    setup(); cord(0, CS_EOC, 2, CD_CLOCK, 0, 100, 50); play();
    for (i = 0; i < 1200u; i++) {
        if (i % 300u == 10u) lfo_eoc |= 4u;
        if (i % 300u == 60u) lfo_eoc &= (uint8_t)~4u;
        one();
    }
    bad += check("EOC: the pulses of track 3's LFO step track 1 (4 and the first)", steps(0) == 5u);
    lfo_eoc = 0;

    /* STOP clears what waits; a gate high at PLAY is no edge; the step at PLAY */
    setup(); cord(0, CS_MACRO1, 0, CD_CLOCK, 0, 100, 50); play(); blocks(2u * period);
    tessera.macro[0] = 100; blocks(3); transport_req = 2; blocks(1);   /* (the edge, then STOP at once) */
    stop(); tessera.macro[0] = 0; blocks(5);
    play(); blocks(period * 3u);
    bad += check("STOP: an edge that waited is gone; PLAY steps once, then nothing", steps(0) == 1u);
    stop(); tessera.macro[0] = 100; blocks(5);
    play(); blocks(period * 3u);
    bad += check("a gate already open at PLAY is not an edge", steps(0) == 1u);
    tessera.macro[0] = 0;

    /* the cord taken away while playing: back on the grid; put on: it waits */
    setup(); play(); blocks(60);
    cord(0, CS_MACRO1, 0, CD_CLOCK, 0, 100, 50);
    n = steps(0);
    blocks(period * 6u);
    bad += check("a cord put on while playing: DIV no longer steps it", steps(0) <= n + 1u);
    cord(0, 0, 0, CD_OFF, 0, 0, 0);
    blocks(period * 6u);
    bad += check("...taken away: back on the grid (steps again)", steps(0) >= n + 4u);

    /* STEP tracks and the OFF mode ignore it; a muted clocked track steps on */
    setup(); tessera.mode[0] = TESS_ENG_OFF; trk[0].p[P_SDIV] = DIV_16TH;
    cord(0, CS_MACRO1, 0, CD_CLOCK, 0, 100, 50); play(); blocks(1500);
    bad += check("a STEP track ignores a CLOCK cord (the grid)", steps(0) > 8u);

    /* seek: a clocked track is left alone */
    setup(); cord(0, CS_MACRO1, 0, CD_CLOCK, 0, 100, 50);
    transport_req = 1; blocks(1);
    bad += check("a clocked track after PLAY: step 1 only", steps(0) == 1u);

    /* the destination table */
    bad += check("CD_CLOCK is 109, a track's (not the project's), named CLOCK",
                 CD_CLOCK == 109 && cord_dest(CD_CLOCK) && cord_dest(CD_CLOCK)->kind == CK_CLOCK && cord_dest(CD_CLOCK)->kind < CK_GLOBAL &&
                 !strcmp(cord_dest(CD_CLOCK)->name, "CLOCK"));

    printf("%s\n", bad ? "CLOCK TEST FAILED" : "CLOCK tests passed");
    return bad != 0;
}
