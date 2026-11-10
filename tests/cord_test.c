/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* Cords (firmware/src/cord.c):
 *   build/host/cord_test tests/cord_dests.txt
 * The destination table: each code at its own place; names short (a cord's chip) and unique; a per-voice or depth
 * destination on one of mod.c's; a parameter destination on a real per-track id (pid.c) with a range, never an
 * action or the device's, and none twice. Then the codes against tests/cord_dests.txt: FROZEN (saved cords keep
 * them), any change fails. Rewrite (CORD_UPDATE=1) only to append. */
#define main hostsim_main
#include "hostsim.c"
#include <sys/wait.h>
#include <unistd.h>
#undef main

static int bad;
static void check(const char *what, int ok)
{
    printf("cord: %-95s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
}

static const char *const KIND[] = {"voice", "block", "depth", "step", "cc", "send", "cord", "fill", "tempo", "bus"};

static cord_t C(uint32_t src, uint32_t st, uint32_t dst, uint32_t dt, int32_t amount, int32_t offset)
{
    cord_t c = {(uint8_t)src, (uint8_t)st, (uint8_t)dst, (uint8_t)dt, (int8_t)amount, (int8_t)offset};
    return c;
}

/* a few seconds of all four tracks playing, every block's peak; 0: something not finite or past full scale */
static double render(uint32_t seconds)
{
    uint32_t f, i, k;
    double sum = 0;
    for (k = 0; k < 4u; k++)
        trk_note_on(&trk[k], 48u + 7u * k, 100);
    for (f = 0; f < seconds * FS; f += CTL) {
        int32_t o[2 * CTL];
        mix_block(o, CTL);
        for (i = 0; i < 2u * CTL; i++) {
            if (o[i] > 0x7FFFFFFF / 2 || o[i] < -0x7FFFFFFF / 2) return -1;
            sum += o[i] < 0 ? -(double)o[i] : (double)o[i];
        }
    }
    for (k = 0; k < 4u; k++)
        trk_note_off(&trk[k], 48u + 7u * k);
    return sum;
}

static int behaviour(void)
{
    int fails = bad;
    cord_t c;
    uint32_t i;
    double dry, wet;
    uint64_t t0, t_dry, t_wet;

    host_tracks_init();
    tessera_reset();
    cord_clear();
    check("none set: nothing to do, and mod_begin stays off for a track without MOD slots",
          cord_count() == 0 && !mod_begin(&trk[1]));
    c = C(CS_OFF, 0, CD_PAN, 1, 0, 50);
    check("refused: past CORD_LIVE, an unknown destination, an amount past 100 %, a track past 4",
          !cord_set(CORD_LIVE, &c) && !(c.dst = CD_N, cord_set(0, &c)) && !(c = C(CS_LFO, 0, CD_PAN, 1, 101, 0), cord_set(0, &c)) &&
          !(c = C(CS_LFO, 4, CD_PAN, 1, 50, 0), cord_set(0, &c)) && cord_count() == 0);

    /* a parameter, in place for the block, back after it */
    c = C(CS_OFF, 0, CD_PAN, 1, 0, 50);
    cord_set(0, &c);
    trk[1].p[P_PAN] = 0;
    mod_begin(&trk[1]);
    i = (uint32_t)trk[1].p[P_PAN];
    mod_end(&trk[1]);
    check("PAN +50 % (no source, the offset): half its range up for the block, back after it",
          i == 63u && trk[1].p[P_PAN] == 0 && cord_count() == 1);   /* (+64 of -64..63: at its top) */

    /* across tracks: track 1's LFO on track 3's LEVEL */
    cord_clear();
    c = C(CS_LFO, 0, CD_LEVEL, 2, -100, 0);
    cord_set(3, &c);
    trk[0].lfo_val = 32767; trk[0].lfo_fade = 32767;
    trk[2].p[P_LEVEL] = 100;
    mod_begin(&trk[2]);
    i = (uint32_t)trk[2].p[P_LEVEL];
    mod_end(&trk[2]);
    check("across tracks: track 1's LFO at its top, -100 % on track 3's LEVEL: down to 0, back to 100",
          i == 0u && trk[2].p[P_LEVEL] == 100);
    check("...and only track 3: track 1 and 2 untouched", !mod_begin(&trk[0]) && !mod_begin(&trk[1]));

    /* polarity: track 1's LFO (POL BI) at its bottom, +50 % on track 3's LEVEL; BI pulls it down, UNI (0) leaves it */
    {
        uint32_t lo, uni, top;
        uint8_t pk[CORD_PK_BYTES];
        cord_clear();
        trk[0].p[P_LPOL] = 0;
        trk[0].lfo_val = -32767;
        trk[2].p[P_LEVEL] = 50;
        c = C(CS_LFO, 0, CD_LEVEL, 2, 50, 0);
        cord_set(0, &c);
        mod_begin(&trk[2]); lo = (uint32_t)trk[2].p[P_LEVEL]; mod_end(&trk[2]);
        c.uni = 1;
        cord_set(0, &c);
        mod_begin(&trk[2]); uni = (uint32_t)trk[2].p[P_LEVEL]; mod_end(&trk[2]);
        trk[0].lfo_val = 32767;
        mod_begin(&trk[2]); top = (uint32_t)trk[2].p[P_LEVEL]; mod_end(&trk[2]);
        check("UNI: a bipolar source at its bottom moves nothing (BI: down), at its top up as BI",
              lo < 50u && uni == 50u && top > 50u);
        trk[0].lfo_fade = 0;                       /* (FADE at its start: the LFO not in yet) */
        mod_begin(&trk[2]); uni = (uint32_t)trk[2].p[P_LEVEL]; mod_end(&trk[2]);
        trk[0].lfo_fade = 16384;
        trk[0].lfo_val = -32767;
        mod_begin(&trk[2]); lo = (uint32_t)trk[2].p[P_LEVEL]; mod_end(&trk[2]);
        trk[0].lfo_fade = 32767;
        check("...UNI with the LFO's FADE at 0: nothing moved; half faded in, at its bottom: nothing (folded before the "
              "FADE, as POL UNI)", uni == 50u && lo == 50u);
        cord_pack(pk);
        cord_clear();
        check("...saved and loaded: UNI kept (src_trk's top bit), the track as it was",
              cord_unpack(pk) && cord_get(0)->uni == 1u && cord_get(0)->src_trk == 0u);
        pk[1] &= 0x7Fu;
        check("...a mod saved before UNI (the bit 0): BI", cord_unpack(pk) && cord_get(0)->uni == 0u);
        pk[6 + 0] = CS_LFO; pk[6 + 2] = CD_RETIRED_CL_RULE; pk[6 + 3] = 1; pk[6 + 4] = 40;   /* (cord 2: to CELLS' RULE) */
        check("...a cord to CELLS' RULE (cut): gone, the others loaded",
              cord_unpack(pk) && !cord_get(1)->src && !cord_get(1)->dst && cord_get(0)->src == CS_LFO);
        trk[0].p[P_LPOL] = 1;                         /* the LFO's own POL UNI: the mod's BI/UNI still decides */
        trk[0].lfo_val = 0;                           /* (POL UNI's bottom) */
        c.uni = 0;
        cord_set(0, &c);
        mod_begin(&trk[2]); lo = (uint32_t)trk[2].p[P_LEVEL]; mod_end(&trk[2]);
        c.uni = 1;
        cord_set(0, &c);
        mod_begin(&trk[2]); uni = (uint32_t)trk[2].p[P_LEVEL]; mod_end(&trk[2]);
        trk[0].lfo_val = 32767;
        mod_begin(&trk[2]); top = (uint32_t)trk[2].p[P_LEVEL]; mod_end(&trk[2]);
        check("...the LFO's POL UNI: BI still pulls down at its bottom, UNI leaves it, its top up",
              lo < 50u && uni == 50u && top > 50u);
        trk[0].p[P_LPOL] = 0;
        c.uni = CU_HOLD + 1;
        check("...a polarity past HOLD: refused", !cord_set(1, &c));
        cord_clear();
        trk[2].p[P_LEVEL] = 100;
    }

    /* mods of mods (CK_CORD): M3 AMT turns mod 3's AMOUNT, a fixed mod's OFFSET; one level */
    {
        uint32_t lv[4];
        uint8_t pk[CORD_PK_BYTES];
        cord_clear();
        trk[0].p[P_LPOL] = 0;
        trk[0].lfo_val = 32767; trk[0].lfo_fade = 32767;
        trk[1].lfo_val = 32767; trk[1].lfo_fade = 32767; trk[1].p[P_LPOL] = 0;
        trk[2].p[P_LEVEL] = 50;
        c = C(CS_LFO, 0, CD_LEVEL, 2, 0, 0);          /* mod 3: T1 LFO -> T3 LEVEL, its amount 0 */
        cord_set(2, &c);
        cord_tick();
        mod_begin(&trk[2]); lv[0] = (uint32_t)trk[2].p[P_LEVEL]; mod_end(&trk[2]);
        c = C(CS_LFO, 1, CD_M1_AMT + 2u, 0, 40, 0);   /* mod 5: T2 LFO -> M3 AMT +40 % */
        cord_set(4, &c);
        cord_tick();
        mod_begin(&trk[2]); lv[1] = (uint32_t)trk[2].p[P_LEVEL]; mod_end(&trk[2]);
        check("mod of mods: mod 3 at 0 moves nothing; M3 AMT +40 % (its source at the top) makes it +40 %, live",
              lv[0] == 50u && cord_move(2) == 40 && lv[1] > 50u && cord_live(cord_get(2)) && cord_count() == 2u);
        trk[1].lfo_val = -32767;
        cord_tick();
        mod_begin(&trk[2]); lv[2] = (uint32_t)trk[2].p[P_LEVEL]; mod_end(&trk[2]);
        check("...its source at the bottom: -40 %, LEVEL down; the set amount itself never changes",
              cord_move(2) == -40 && lv[2] < 50u && cord_get(2)->amount == 0);
        c = C(CS_OFF, 0, CD_M1_AMT + 2u, 0, 0, 100);  /* mod 6: a fixed +100 % more: clamped */
        cord_set(5, &c);
        trk[1].lfo_val = 32767;
        cord_tick();
        check("...two onto one add up, at most 100 %", cord_move(2) == 100);
        c = C(CS_LFO, 0, CD_M1_AMT + 4u, 0, 100, 0);  /* mod 7 onto mod 5 (an M AMT mod): one level, nothing */
        cord_set(6, &c);
        c = C(CS_LFO, 0, CD_M1_AMT + 7u, 0, 100, 0);  /* mod 8 onto itself: nothing */
        cord_set(7, &c);
        cord_tick();
        check("...one level: a mod onto an M AMT mod, or onto itself, turns nothing",
              cord_move(4) == 40 && cord_move(7) == 100 && cord_move(2) == 100);
        c = C(CS_OFF, 0, CD_PAN, 1, 0, 20);           /* mod 9: a fixed PAN +20; mod 10 turns its OFFSET */
        cord_set(8, &c);
        c = C(CS_OFF, 0, CD_M1_AMT + 8u, 0, 0, -50);
        cord_set(9, &c);
        cord_tick();
        check("...a fixed mod: its OFFSET turned (+20 -50: -30)", cord_move(8) == -30);
        check("...the destination track kept 0 (the project's, as a send effect's)", cord_get(4)->dst_trk == 0u);
        cord_pack(pk);
        cord_clear();
        cord_tick();
        check("...saved and loaded: the same mods, the same moves",
              cord_unpack(pk) && (cord_tick(), cord_move(2) == 100) && cord_move(8) == -30 &&
              cord_get(4)->dst == CD_M1_AMT + 2u);
        cord_clear();
        cord_tick();
        check("...cleared: every move back to 0", cord_move(2) == 0 && cord_move(8) == 0);
        trk[2].p[P_LEVEL] = 100;
    }

    /* DIR (CD_SEQ_DIR): the pattern's DIR moved in whole values at each step, stepped, clamped; the stored one kept */
    {
        uint32_t d[4];
        cord_clear();
        tessera_param_set(0, TX_SEQ_DIR, SEQ_FWD);
        trk[0].p[P_LPOL] = 0; trk[0].lfo_fade = 32767;
        check("DIR: no mod, the pattern's", seq_dir_now(&trk[0]) == SEQ_FWD);
        c = C(CS_OFF, 0, CD_SEQ_DIR, 0, 0, 20);        /* a fixed +20 %: one value on, BACK */
        cord_set(0, &c);
        check("...a fixed +20 %: BACK, the pattern's own still FWD", seq_dir_now(&trk[0]) == SEQ_BACK &&
              tessera_param_get(0, TX_SEQ_DIR) == SEQ_FWD);
        c = C(CS_LFO, 0, CD_SEQ_DIR, 0, 40, 0);        /* T1 LFO + (UNI) 40 %: FWD .. PEND */
        c.uni = CU_UNI;
        cord_set(0, &c);
        trk[0].lfo_val = -32767; d[0] = seq_dir_now(&trk[0]);
        trk[0].lfo_val = 0; d[1] = seq_dir_now(&trk[0]);
        trk[0].lfo_val = 32767; d[2] = seq_dir_now(&trk[0]);
        check("...LFO + at 40 %: FWD, BACK, PEND as it rises", d[0] == SEQ_FWD && d[1] == SEQ_BACK && d[2] == SEQ_PEND);
        c.uni = CU_GATE;                               /* a GATE at 20 %: BACK while the LFO is over 0 */
        c.amount = 20;
        cord_set(0, &c);
        trk[0].lfo_val = 16000; cord_tick(); d[0] = seq_dir_now(&trk[0]);
        trk[0].lfo_val = -16000; cord_tick(); d[1] = seq_dir_now(&trk[0]);
        check("...a GATE at 20 %: BACK while open, FWD shut", d[0] == SEQ_BACK && d[1] == SEQ_FWD);
        tessera_param_set(0, TX_SEQ_DIR, SEQ_PONG);
        c = C(CS_OFF, 0, CD_SEQ_DIR, 0, 0, 100);
        cord_set(0, &c);
        d[0] = seq_dir_now(&trk[0]);
        c.offset = -100;
        cord_set(0, &c);
        d[1] = seq_dir_now(&trk[0]);
        check("...from PONG, +100 % stops at DRUNK, -100 % at FWD", d[0] == SEQ_DRUNK && d[1] == SEQ_FWD &&
              tessera_param_get(0, TX_SEQ_DIR) == SEQ_PONG);
        tessera_param_set(0, TX_SEQ_DIR, SEQ_FWD);
        cord_clear();
    }

    /* HOLD (CU_HOLD): the source as it is, sampled when set and at each bar of the destination's track, held between */
    {
        uint32_t lv[4];
        uint8_t pk[CORD_PK_BYTES];
        cord_clear();
        trk[0].p[P_LPOL] = 0; trk[0].lfo_fade = 32767;
        trk[2].p[P_LEVEL] = 50;
        trk[0].lfo_val = 16000;
        c = C(CS_LFO, 0, CD_LEVEL, 2, 40, 0);          /* mod 1: T1 LFO HOLD -> T3 LEVEL +-40 % */
        c.uni = CU_HOLD;
        check("HOLD: set", cord_set(0, &c) && cord_hold_mask() == 1u);
        mod_begin(&trk[2]); lv[0] = (uint32_t)trk[2].p[P_LEVEL]; mod_end(&trk[2]);
        trk[0].lfo_val = -16000; cord_tick();
        mod_begin(&trk[2]); lv[1] = (uint32_t)trk[2].p[P_LEVEL]; mod_end(&trk[2]);
        cord_hold_bar(0); cord_hold_bar(1);            /* (another track's bar: nothing) */
        mod_begin(&trk[2]); lv[2] = (uint32_t)trk[2].p[P_LEVEL]; mod_end(&trk[2]);
        cord_hold_bar(2);
        mod_begin(&trk[2]); lv[3] = (uint32_t)trk[2].p[P_LEVEL]; mod_end(&trk[2]);
        check("...sampled when set, held while the LFO falls, sampled again on T3's bar, both ways (+-)",
              lv[0] > 50u && lv[1] == lv[0] && lv[2] == lv[0] && lv[3] < 50u);
        cord_pack(pk);
        check("...saved as BI (older firmware: a live +- mod), the mask apart", pk[1] == 0x00u && cord_hold_mask() == 1u);
        cord_clear();
        lv[0] = (uint32_t)cord_unpack(pk) && cord_get(0)->uni == CU_BI;
        cord_hold_load(1u);
        check("...loaded back: BI, then HOLD from the mask", lv[0] && cord_get(0)->uni == CU_HOLD);
        c = C(CS_OFF, 0, CD_LEVEL, 2, 0, 30);           /* (no source: never HOLD) */
        cord_set(1, &c);
        cord_hold_load(3u);
        check("...a mod without a source stays as it is", cord_get(1)->uni == CU_BI && cord_hold_mask() == 1u);
        cord_clear();
        check("...cleared: no HOLD", !cord_hold_mask());
        trk[2].p[P_LEVEL] = 100;
    }

    /* GATE (CU_GATE): all while the source is over the threshold (its OFFSET), none under it; FILL held past half */
    {
        uint32_t lv[4];
        uint8_t pk[CORD_PK_BYTES];
        cord_t got[CORD_MAX];
        cord_clear();
        trk[0].p[P_LPOL] = 0; trk[0].lfo_fade = 32767;
        trk[2].p[P_LEVEL] = 50;
        c = C(CS_LFO, 0, CD_LEVEL, 2, 40, 0);          /* mod 1: T1 LFO as a GATE over 0 -> T3 LEVEL +40 % */
        c.uni = CU_GATE;
        check("GATE: set", cord_set(0, &c));
        trk[0].lfo_val = -16000; cord_tick();
        mod_begin(&trk[2]); lv[0] = (uint32_t)trk[2].p[P_LEVEL]; mod_end(&trk[2]);
        trk[0].lfo_val = 16000; cord_tick();
        mod_begin(&trk[2]); lv[1] = (uint32_t)trk[2].p[P_LEVEL]; mod_end(&trk[2]);
        trk[0].lfo_val = 32767; cord_tick();
        mod_begin(&trk[2]); lv[2] = (uint32_t)trk[2].p[P_LEVEL]; mod_end(&trk[2]);
        check("...under the threshold nothing, over it the whole amount, however far over",
              lv[0] == 50u && lv[1] > 50u && lv[1] == lv[2]);
        trk[0].lfo_val = -32767; cord_tick();
        trk[0].lfo_val = 200; cord_tick();
        lv[0] = (uint32_t)cord_gate_on(cord_get(0));
        trk[0].lfo_val = 400; cord_tick();
        lv[1] = (uint32_t)cord_gate_on(cord_get(0));
        trk[0].lfo_val = 200; cord_tick();
        lv[2] = (uint32_t)cord_gate_on(cord_get(0));
        trk[0].lfo_val = -10; cord_tick();
        lv[3] = (uint32_t)cord_gate_on(cord_get(0));
        check("...a step of hysteresis: opens past threshold + 256, closes at the threshold or under",
              !lv[0] && lv[1] && lv[2] && !lv[3]);
        c.offset = 50;                                  /* the threshold +50 %: the LFO at +25 % stays shut */
        cord_set(0, &c);
        trk[0].lfo_val = 8192; cord_tick();
        mod_begin(&trk[2]); lv[0] = (uint32_t)trk[2].p[P_LEVEL]; mod_end(&trk[2]);
        trk[0].lfo_val = 24000; cord_tick();
        mod_begin(&trk[2]); lv[1] = (uint32_t)trk[2].p[P_LEVEL]; mod_end(&trk[2]);
        check("...the OFFSET is the threshold and moves nothing itself", lv[0] == 50u && lv[1] > 50u);
        cord_pack(pk);
        check("...saved in src_trk's bit 6, the track kept", pk[1] == 0x40u && pk[2] == CD_LEVEL);
        cord_clear();
        check("...loaded back a GATE", cord_unpack(pk) && cord_get(0)->uni == CU_GATE && cord_get(0)->src_trk == 0u &&
                                       cord_get(0)->offset == 50);
        pk[1] = 0xC0u;
        check("...both bits (no such polarity): the block refused", !cord_parse(pk, got));
        cord_clear();
        c = C(CS_LFO, 0, CD_FILL, 3, 100, 0);           /* mod 1: T1 LFO as a GATE -> FILL 100 % */
        c.uni = CU_GATE;
        cord_set(0, &c);
        trk[0].lfo_val = 16000; cord_tick();
        lv[0] = seq_fill_mod;
        trk[0].lfo_val = -16000; cord_tick();
        lv[1] = seq_fill_mod;
        check("FILL: held while the GATE is open, the destination track kept 0",
              lv[0] && !lv[1] && cord_get(0)->dst_trk == 0u);
        c.amount = 40;
        cord_set(0, &c);
        trk[0].lfo_val = 16000; cord_tick();
        check("...a move under half: not held", !seq_fill_mod);
        c = C(CS_OFF, 0, CD_FILL, 0, 0, 60);            /* a fixed +60 %: held */
        cord_set(0, &c);
        cord_tick();
        lv[0] = seq_fill_mod;
        cord_clear();
        cord_tick();
        check("...a fixed move past half holds it; cleared, let go", lv[0] && !seq_fill_mod);
        trk[2].p[P_LEVEL] = 100;
    }

    /* per voice: the pitch term, as a MOD slot's */
    cord_clear();
    c = C(CS_OFF, 0, CD_PITCH, 0, 0, 100);
    cord_set(0, &c);
    mod_begin(&trk[0]);
    i = (uint32_t)mod.pit;
    mod_end(&trk[0]);
    check("PITCH +100 %: an octave on every voice (mod.c's units, 16 a semitone)", i == 192u);

    cord_clear();
    c = C(CS_OFF, 0, CD_LFO_DEPTH, 0, 0, -100);
    cord_set(0, &c);
    mod_begin(&trk[0]);
    i = (uint32_t)mod.ldep;
    mod_end(&trk[0]);
    check("LFO DEPTH -100 %: the LFO closed (its gain 0)", i == 0u);

    /* a Tessera setting around the step */
    cord_clear();
    tessera_param_set(0, TX_MODE, TESS_ENG_EUCLID);
    tessera_param_set(0, TX_EUC_STEPS, 16);
    tessera_param_set(0, TX_EUC_PULSES, 4);
    c = C(CS_OFF, 0, CD_EUC_PULSES, 0, 0, 25);
    cord_set(0, &c);
    cord_step_begin(0);
    i = tessera.euclid[0].pulses;
    cord_step_end(0);
    check("EUCLID PULSES +25 % (of 0..32): 4 -> 12 for the step, 4 again after it", i == 12u && tessera.euclid[0].pulses == 4u);
    cord_step_begin(0);
    tessera.euclid[0].pulses = 13;                      /* (as an EVOLVE mutation in the step: one more) */
    cord_step_end(0);
    check("...a mutation in the step kept: one more hit stays one more (5)", tessera.euclid[0].pulses == 5u);
    tessera.euclid[0].pulses = 4;
    c = C(CS_OFF, 0, CD_EUC_PULSES, 0, 0, 100);
    cord_set(0, &c);
    cord_step_begin(0);
    i = tessera.euclid[0].pulses;
    cord_step_end(0);
    check("...past the range: PULSES at most STEPS (16), back to 4", i == 16u && tessera.euclid[0].pulses == 4u);

    /* the generators' CV: Euclid's ramp over its cycle, as gen.c makes it */
    host_tracks_init();
    tessera_reset();
    tessera_param_set(0, TX_MODE, TESS_ENG_EUCLID);
    tessera_param_set(0, TX_EUC_STEPS, 4);
    {
        int32_t seen[4];
        for (i = 0; i < 4u; i++) {
            tessera_clock_step(&trk[0]);
            seen[i] = cord_src(CS_CV, 0);
        }
        check("CV: Euclid's ramp over its 4 steps (0, 1/3, 2/3, all), as the cords read it",
              seen[0] == 0 && seen[1] == 10922 && seen[2] == 21844 && seen[3] == 32767);
    }
    check("CV, GATE, NOTE of a track without a generator: 0", !cord_src(CS_CV, 1) && !cord_src(CS_GATE, 1) &&
          !cord_src(CS_NOTE, 1));
    tessera_stop_all();

    /* sixteen at once, every kind, all four tracks: it plays, nothing breaks, and it is not the dry sound */
    host_tracks_init();
    tessera_reset();
    cord_clear();
    for (i = 0; i < 4u; i++) host_power_on_sound(&trk[i], i);
    t0 = now_ns();
    dry = render(2);
    t_dry = now_ns() - t0;
    {
        static const uint8_t D[16] = {CD_PITCH, CD_CUTOFF, CD_SHAPE, CD_GAIN, CD_LEVEL, CD_PAN, CD_SPREAD, CD_ATTACK,
                                      CD_ENV_FLT, CD_LFO_RATE, CD_LFO_DEPTH, CD_LFO_FLT, CD_FX_SEND, CD_E1, CD_E2, CD_TW_DROP};
        static const uint8_t S[16] = {CS_LFO, CS_ENV, CS_VEL, CS_KEY, CS_RAND, CS_MODW, CS_AT, CS_EXPR, CS_SH, CS_SLEW,
                                      CS_GATE, CS_NOTE, CS_LFO, CS_ENV, CS_SH, CS_GATE};
        for (i = 0; i < 16u; i++) {
            c = C(S[i], i % 4u, D[i], (i + 1u) % 4u, (int32_t)(i * 13u % 201u) - 100, (int32_t)(i * 7u % 41u) - 20);
            cord_set(i, &c);
        }
    }
    check("sixteen cords set, every kind of destination", cord_count() == 16u);
    host_tracks_init();
    for (i = 0; i < 4u; i++) host_power_on_sound(&trk[i], i);
    t0 = now_ns();
    wet = render(2);
    t_wet = now_ns() - t0;
    printf("cord: host cost, four tracks playing: %.1f %% more with sixteen cords (a report, no limit: the device's "
           "cost is measured there)\n", t_dry ? 100.0 * ((double)t_wet - (double)t_dry) / (double)t_dry : 0.0);
    check("...two seconds of four tracks: plays, never past full scale, and differs from no cords",
          dry > 0 && wet > 0 && wet != dry);
    for (i = 0; i < NTRK; i++)
        if (trk[i].p[P_PAN] != TP[P_PAN].def || trk[i].p[P_LEVEL] < 0) wet = -1;
    check("...after it every stored value as it was (PAN, LEVEL: no block's modulation left in)", wet > 0);
    cord_clear();
    return bad - fails;
}

/* the effects (2026-10-28): an insert's MIX / RATE / CHAR and SEND 2 / 3 for the track's block, the send effects'
 * for every track (CK_GLOBAL, its track ignored); ECHO's TIME glides, REVERB's SIZE a sample at a time (ROOM's
 * jumps: a glide measured no better on its short combs) */
/* the MIX buses (CK_BUS, CS_BUSA): each MODE, LAG, the one-block delay, a bus fed into itself, off with no cord */
static int32_t bus_after(uint32_t blocks)
{
    while (blocks--) cord_tick();
    return cord_bus_now(0);
}
static int buses(void)
{
    int fails = bad;
    cord_t c;
    int32_t v[6];
    uint32_t i;
    host_tracks_init();
    tessera_reset();
    check("buses: BUS A / B the sources after MACRO 4, bipolar, the project's (none of a track's)",
          !strcmp(cord_src_name(CS_BUSA), "BUS A") && !strcmp(cord_src_name(CS_BUSB), "BUS B") && cord_bipolar(CS_BUSA, 0) &&
          cord_bipolar(CS_BUSB, 3) && CS_BUSA == CS_MACRO4 + 1u && CS_N == CS_BUSB + 1u);
    check("...a new project: SUM, LAG OFF, both at 0", !cord_bus_get(0) && !cord_bus_get(1) && !cord_bus_get(2) &&
          !cord_bus_get(3) && !cord_bus_now(0) && !cord_bus_now(1));
    cord_tick();
    check("...no cord into them: they stay at 0 (the off position)", !cord_bus_now(0) && !cord_bus_now(1));
    c = C(CS_OFF, 0, CD_BUS_A, 2, 0, 30);               /* fixed +30 % and +50 % into A */
    cord_set(0, &c);
    check("...a bus cord is the project's: its destination track kept 0", cord_get(0)->dst_trk == 0u);
    c = C(CS_OFF, 0, CD_BUS_A, 0, 0, 50);
    cord_set(1, &c);
    check("...nothing moves it before its block", !cord_bus_now(0));
    v[0] = bus_after(1);
    check("SUM: +30 % and +50 % make +80 %", v[0] == 32768 * 30 / 100 + 32768 * 50 / 100);
    c.offset = 90;
    cord_set(1, &c);
    check("...+30 % and +90 %: clamped to 1", bus_after(1) == 32767);
    c.offset = -60;
    cord_set(1, &c);
    check("...+30 % and -60 %: -30 %", bus_after(1) == 32768 * 30 / 100 - 32768 * 60 / 100);
    tessera_param_set(0, TX_BUSA_MODE, CB_MAX);
    check("MAX: the higher of +30 % and -60 %", bus_after(1) == 32768 * 30 / 100);
    tessera_param_set(0, TX_BUSA_MODE, CB_MIN);
    check("MIN: the lower", bus_after(1) == -(32768 * 60) / 100);
    tessera_param_set(0, TX_BUSA_MODE, CB_DIFF);
    check("DIFF: how far apart the first two, |+30 - -60| = 90 %", bus_after(1) == 32768 * 30 / 100 + 32768 * 60 / 100);
    c = C(CS_OFF, 0, CD_BUS_A, 0, 0, 100);              /* a third cord: DIFF reads only the first two */
    cord_set(2, &c);
    check("...a third cord left out", bus_after(1) == 32768 * 30 / 100 + 32768 * 60 / 100);
    cord_clear();
    /* gates as logic: T1 and T2 LFO as GATEs (over 0) at 100 % into bus A */
    c = C(CS_LFO, 0, CD_BUS_A, 0, 100, 0); c.uni = CU_GATE; cord_set(0, &c);
    c = C(CS_LFO, 1, CD_BUS_A, 0, 100, 0); c.uni = CU_GATE; cord_set(1, &c);
    for (i = 0; i < 4u; i++) {                          /* (00 01 10 11) */
        uint32_t m;
        int32_t o[4];
        trk[0].lfo_val = (i & 1u) ? 20000 : -20000;
        trk[1].lfo_val = (i & 2u) ? 20000 : -20000;
        trk[0].lfo_fade = trk[1].lfo_fade = 32767;
        for (m = 0; m < 4u; m++) {
            tessera_param_set(0, TX_BUSA_MODE, (int32_t)m);
            o[m] = bus_after(1) >= 16384;
        }
        v[i] = o[1] | o[2] << 1 | o[3] << 2;            /* (MAX MIN DIFF, as gates past half) */
    }
    check("...two gates: MAX is OR, MIN is AND, DIFF is XOR", v[0] == 0 && v[1] == 5 && v[2] == 5 && v[3] == 3);
    trk[0].lfo_val = trk[1].lfo_val = 0;
    cord_clear();
    /* LAG: a fixed +100 % into A at LAG 10 ms rises over ~14 blocks (1378 a second), and falls as slowly */
    tessera_param_set(0, TX_BUSA_LAG, 1);
    c = C(CS_OFF, 0, CD_BUS_A, 0, 0, 100);
    cord_set(0, &c);
    v[0] = bus_after(1);
    v[1] = bus_after(5);
    v[2] = bus_after(10);
    check("LAG 10 ms: a whole move takes 14 blocks, in even steps", v[0] > 0 && v[0] < 32767 / 10 && v[1] - 6 * v[0] >= 0 && v[1] - 6 * v[0] <= 6 &&
          v[2] == 32767);
    c.offset = 0;
    cord_set(0, &c);
    v[3] = bus_after(7);
    v[4] = bus_after(10);
    check("...its cord at 0: it falls back as slowly, then rests", v[3] > 32767 / 3 && v[3] < 2 * 32767 / 3 && v[4] == 0);
    tessera_param_set(0, TX_BUSA_LAG, 11);              /* 20 s */
    c.offset = 100;
    cord_set(0, &c);
    v[0] = bus_after(1378);
    check("...LAG 20 s: a second rises 1/20", v[0] > 32767 / 20 - 64 && v[0] < 32767 / 20 + 64);
    tessera_param_set(0, TX_BUSA_LAG, 0);
    check("...LAG OFF: at once", bus_after(1) == 32767);
    tessera_param_set(0, TX_BUSA_LAG, 99);
    tessera_param_set(0, TX_BUSA_MODE, -3);
    check("MODE and LAG kept in range (20 s, SUM), any track's the same", tessera_param_get(0, TX_BUSA_LAG) ==
          (int32_t)CB_LAGS - 1 && tessera_param_get(2, TX_BUSA_LAG) == (int32_t)CB_LAGS - 1 &&
          tessera_param_get(0, TX_BUSA_MODE) == CB_SUM);
    tessera_reset();
    check("...a new project: back to SUM, OFF, at rest", !cord_bus_get(1) && !cord_bus_get(0) && !cord_bus_now(0));
    /* the one-block delay: +10 % and BUS A at 100 % into A integrates, a block a step */
    c = C(CS_OFF, 0, CD_BUS_A, 0, 0, 10); cord_set(0, &c);
    c = C(CS_BUSA, 0, CD_BUS_A, 0, 100, 0); cord_set(1, &c);
    v[0] = bus_after(1); v[1] = bus_after(1); v[2] = bus_after(1);
    check("one block late: a bus into itself adds its last value (+10 %, +20 %, +30 %)",
          v[0] == 3276 && v[1] == 2 * 3276 && v[2] == 3 * 3276);
    v[3] = bus_after(100);
    check("...and stops at the clamp, stable", v[3] == 32767 && bus_after(10) == 32767);
    c.amount = -100; cord_set(1, &c);                   /* -100 %: x = 10 % - x, a block each way */
    for (i = 0, v[0] = 40000, v[1] = -40000; i < 200u; i++) {
        int32_t x = bus_after(1);
        if (i > 4u) v[0] = x < v[0] ? x : v[0], v[1] = x > v[1] ? x : v[1];
    }
    v[2] = bus_after(1); v[3] = bus_after(1); v[4] = bus_after(1);
    check("...at -100 %: a swing a block each way round +5 %, bounded by the clamp", v[0] >= -32767 && v[1] <= 32767 &&
          v[1] > v[0] && v[2] == v[4] && v[2] != v[3] && v[2] + v[3] == 3276);
    c.amount = -50; cord_set(1, &c);
    v[0] = bus_after(300);
    check("...at -50 %: settles at 10 / 1.5 = 6.7 %", v[0] > 2150 && v[0] < 2220 && bus_after(1) == v[0]);
    c = C(CS_BUSA, 0, CD_BUS_B, 0, 100, 0); cord_set(2, &c);   /* A into B: B one block behind A */
    v[0] = cord_bus_now(0);
    cord_tick();
    check("A into B: B reads A as it was the block before", cord_bus_now(1) == v[0]);
    c = C(CS_BUSB, 0, CD_PAN, 1, 100, 0); cord_set(3, &c);    /* B onto track 2's PAN: as any source */
    trk[1].p[P_PAN] = 0;
    mod_begin(&trk[1]); v[1] = trk[1].p[P_PAN]; mod_end(&trk[1]);
    check("...a bus moves a track's parameter as any source", v[1] > 0 && trk[1].p[P_PAN] == 0);
    cord_clear();
    return bad - fails;
}

static void fresh_tracks(void);
static int fx_dests(void)
{
    int fails = bad;
    cord_t c;
    int32_t sh, was;
    uint32_t i, ok;
    fx_state_t *f = &fx_state[0];                       /* (track 1's FX 1) */

    fresh_tracks();
    trk[1].p[P_FX2TYPE] = FX_ECHO; trk[1].p[P_FX2MIX] = 0; trk[1].p[P_FX2RATE] = 20;
    c = C(CS_OFF, 0, CD_FX2_MIX, 1, 0, 50);
    cord_set(0, &c);
    check("FX2 MIX +50 % on track 2 (its MIX 0: the effect off): not active without the mod", !fx_active(&trk[1]));
    mod_begin(&trk[1]);
    i = (uint32_t)trk[1].p[P_FX2MIX];
    ok = (uint32_t)fx_active(&trk[1]);
    mod_end(&trk[1]);
    check("...for the block: MIX 64, the effect active; back to 0 after it", i == 64u && ok && !trk[1].p[P_FX2MIX]);
    c = C(CS_OFF, 0, CD_FX2_RATE, 1, 0, -100);
    cord_set(1, &c);
    mod_begin(&trk[1]);
    i = (uint32_t)trk[1].p[P_FX2RATE];
    mod_end(&trk[1]);
    check("FX2 RTE -100 %: RATE (TIME) at 0 for the block, 20 after it", i == 0u && trk[1].p[P_FX2RATE] == 20);
    check("...the knob card's live mark: track 2's FX 2 RATE moved, track 1's not",
          cord_moves(PID_TRACK(P_FX2RATE), 1, &sh) && sh < 0 && !cord_moves(PID_TRACK(P_FX2RATE), 0, &sh));

    /* SEND 3 (FX MODE SPLIT): read after the block's mods (it was before mod_begin) */
    fresh_tracks();
    song.g[G_FXMODE] = 1;
    song.g[G_MFX3TYPE] = FX_ROOM; song.g[G_MFX3MIX] = 127; song.g[G_FXRETURN] = 100;
    trk[3].p[P_FXSEND3] = 0;
    c = C(CS_OFF, 0, CD_FX_SEND3, 3, 0, 100);
    cord_set(0, &c);
    for (i = 0; i < 4u; i++) {
        int32_t o[2 * CTL];
        mix_block(o, CTL);
    }
    check("SEND 3 +100 % on track 4 (its SEND 3 at 0, FX MODE SPLIT): sent at full for the block, 0 stored",
          fx_extra_gain[1] == 65536u && trk[3].p[P_FXSEND3] == 0);

    /* the send effects: for every track, the cord's track kept 0 */
    fresh_tracks();
    song.g[G_MFXTYPE] = FX_ECHO; song.g[G_MFXMIX] = 0; song.g[G_MFXRATE] = 30; song.g[G_FXRETURN] = 100;
    c = C(CS_OFF, 0, CD_SF1_RATE, 2, 0, 50);
    cord_set(0, &c);
    check("SF1 RTE (a send effect's): set on track 3, kept as every track's (its track 0)",
          cord_get(0)->dst_trk == 0u && cord_count() == 1u && !cord_mask[2] && !cord_mask[0] && cord_gmask == 1u);
    fx_bus_active();
    check("...this block: SEND FX 1's RATE 30 + 64, the project's stored 30", fx_bus_g[G_MFXRATE - G_MFXTYPE] == 94 &&
          song.g[G_MFXRATE] == 30);
    check("...SEND FX 1's MIX 0: the send effects off", !fx_bus_active());
    c = C(CS_OFF, 0, CD_SF1_MIX, 0, 0, 100);
    cord_set(1, &c);
    c = C(CS_OFF, 0, CD_FX_RETURN, 0, 0, -30);
    cord_set(2, &c);
    fx_bus_active();
    check("...SF1 MIX +100 %: MIX 127, the send effects on; RETURN -30 %: 100 - 38",
          fx_bus_g[G_MFXMIX - G_MFXTYPE] == 127 && fx_bus_active() && fx_bus_g[G_FXRETURN - G_MFXTYPE] == 62);
    check("...the live mark: SEND FX 1's RATE moved on any track's page",
          cord_moves(PID_GLOBAL(G_MFXRATE), 0, &sh) && cord_moves(PID_GLOBAL(G_MFXRATE), 3, &sh) && sh > 0);
    c = C(CS_LFO, 1, CD_SF1_RATE, 0, 100, 0);
    cord_set(0, &c);
    trk[1].lfo_val = 32767; trk[1].lfo_fade = 32767;
    fx_bus_active();
    was = fx_bus_g[G_MFXRATE - G_MFXTYPE];
    trk[1].lfo_val = -32767;
    fx_bus_active();
    check("...track 2's LFO, +100 %: SEND FX 1's RATE up to 127 at its top, down to 0 at its bottom",
          was == 127 && fx_bus_g[G_MFXRATE - G_MFXTYPE] == 0);
    {
        uint8_t pk[CORD_PK_BYTES];
        cord_pack(pk);
        pk[2 * 6 + 3] = 2;                              /* (cord 3, RETURN, saved with a track: 3) */
        check("...saved and loaded: the send effects' cords as they were, a track saved with one read as 0",
              cord_unpack(pk) && cord_get(0)->dst == CD_SF1_RATE && cord_get(2)->dst == CD_FX_RETURN &&
              cord_get(2)->dst_trk == 0u && cord_gmask == 7u);
    }
    cord_clear();
    check("...cleared: no send effect cords", !cord_gmask);

    /* ECHO's TIME glides: at most 1/4 sample a quarter-rate tick (8 a block), no jump */
    fresh_tracks();
    trk[0].p[P_FXTYPE] = FX_ECHO; trk[0].p[P_FXMIX] = 127; trk[0].p[P_FXRATE] = 0; trk[0].p[P_FXCHAR] = 64;
    trk_note_on(&trk[0], 60, 100);
    for (i = 0; i < 8u; i++) {
        int32_t o[2 * CTL];
        mix_block(o, CTL);
    }
    check("ECHO: at its TIME at once when it starts (110 quarter-rate samples)", f->type == FX_ECHO && f->glide == 110 * 256);
    trk[0].p[P_FXRATE] = 127;
    {
        int32_t o[2 * CTL];
        mix_block(o, CTL);
        check("...TIME to its top: a block later 2 samples on (8 ticks of 1/4), not a jump",
              f->glide == 110 * 256 + 8 * 64);
        for (i = 0; i < 3u * FS / CTL; i++) mix_block(o, CTL);
        check("...three seconds later there (110 + 127 x 31)", f->glide == (110 + 127 * 31) * 256);
    }
    trk_note_off(&trk[0], 60);

    /* REVERB's SIZE: a sample at a time every 4 blocks, there at the start; ROOM's at once */
    fresh_tracks();
    trk[0].p[P_FXTYPE] = FX_ROOM; trk[0].p[P_FXMIX] = 127; trk[0].p[P_FXRATE] = 0;
    trk_note_on(&trk[0], 60, 100);
    {
        int32_t o[2 * CTL];
        uint16_t at;
        for (i = 0; i < 4u; i++) mix_block(o, CTL);
        at = f->rev_span[0];
        check("ROOM: its lines at their SIZE at once (149 x 64 / 191)", f->type == FX_ROOM && at == 149u * 64u / 191u);
        trk[0].p[P_FXRATE] = 127;
        for (i = 0; i < 4u; i++) mix_block(o, CTL);
        check("...SIZE to its top: there a block later", f->rev_span[0] == 149u && f->rev_span[3] == 293u &&
              f->rev_rate == 127);
        trk[0].p[P_FXTYPE] = FX_REVERB; trk[0].p[P_FXRATE] = 0;
        for (i = 0; i < 400u; i++) mix_block(o, CTL);
        at = f->rev_span[0];
        check("REVERB (from ROOM, after its fade): its lines at their SIZE", f->type == FX_REVERB &&
              at == 131u * 64u / 191u && f->rev_rate == 0);
        trk[0].p[P_FXRATE] = 127;
        for (i = 0; i < 8u; i++) mix_block(o, CTL);
        check("...SIZE to its top: 8 blocks later 2 samples longer, not there yet", f->rev_span[0] == at + 2u &&
              f->rev_rate == 0);
        for (i = 0; i < 1200u; i++) mix_block(o, CTL);
        check("...later there, settled", f->rev_span[0] == 131u && f->rev_span[7] == 277u && f->rev_rate == 127);
    }
    trk_note_off(&trk[0], 60);

    /* all of them at once, playing: nothing past full scale, every stored value as it was */
    fresh_tracks();
    song.g[G_FXMODE] = 1;
    was = song.g[G_MFXRATE];
    for (i = 0; i < 3u; i++) {
        trk[i].p[P_FXTYPE] = (int16_t)(FX_ECHO + i); trk[i].p[P_FXMIX] = 60; trk[i].p[P_FXSEND] = 60;
        trk[i].p[P_FXSEND2] = 40; trk[i].p[P_FXSEND3] = 40;
        song.g[G_MFXTYPE + i * FX_PARAMS] = (int16_t)(FX_ECHO + i);
    }
    {
        static const uint8_t D[16] = {CD_FX1_MIX, CD_FX1_RATE, CD_FX1_CHAR, CD_FX2_MIX, CD_FX2_RATE, CD_FX2_CHAR,
                                      CD_FX3_MIX, CD_FX3_RATE, CD_FX3_CHAR, CD_FX_SEND2, CD_FX_SEND3, CD_SF1_RATE,
                                      CD_SF2_RATE, CD_SF3_CHAR, CD_SF1_MIX, CD_FX_RETURN};
        for (i = 0; i < 16u; i++) {
            c = C(i & 1u ? CS_LFO : CS_DRIFT, i % 4u, D[i], i % 3u, (int32_t)(i * 13u % 201u) - 100, 0);
            cord_set(i, &c);
        }
        for (i = 0; i < 4u; i++) { trk[i].p[P_LRATE] = 120; trk[i].lfo_fade = 32767; }
    }
    check("sixteen effect cords at once, on three tracks playing: never past full scale", render(2) > 0);
    ok = 1;
    for (i = 0; i < 3u; i++)
        ok &= trk[i].p[P_FXMIX] == 60 && trk[i].p[P_FXRATE] == TP[P_FXRATE].def && trk[i].p[P_FXSEND2] == 40;
    check("...after it the stored values as they were (MIX, RATE, SEND 2; the project's RATE)",
          ok && song.g[G_MFXRATE] == was);
    cord_clear();
    song.g[G_FXMODE] = 0;
    return bad - fails;
}

/* MOD's routes into cords (cord_adopt): the same move, the sound's go with it, the project's stay, no room: left */
static void fresh_tracks(void)
{
    uint32_t i;
    memset(trk, 0, sizeof trk);                         /* (as fx_performance: every voice, LFO and effect from rest) */
    memset(&song, 0, sizeof song);
    memset(fx_state, 0, sizeof fx_state);
    spread_on = 0;
    host_tracks_init();
    tessera_reset();
    cord_clear();
    for (i = 0; i < 4u; i++) host_power_on_sound(&trk[i], i);
    mod_seed = 0x2545F491u;
}
static void route(uint32_t tr, uint32_t k, int32_t s, int32_t d, int32_t a)
{
    trk[tr].p[P_M1SRC + 3u * k] = (int16_t)s;
    trk[tr].p[P_M1DST + 3u * k] = (int16_t)d;
    trk[tr].p[P_M1AMT + 3u * k] = (int16_t)a;
}
static int routes_off(uint32_t tr)
{
    uint32_t k;
    for (k = 0; k < 12u; k++)
        if (trk[tr].p[P_M1SRC + k]) return 0;
    return 1;
}
/* track 1 a chord of three velocities (each voice its own VEL, KEY, ENV), the others one note */
static double render_chord(uint32_t seconds)
{
    static const uint8_t N[3] = {48, 55, 64}, V[3] = {30, 80, 127};
    uint32_t f, i;
    double sum = 0;
    for (i = 0; i < 3u; i++) trk_note_on(&trk[1], N[i], V[i]);
    trk_note_on(&trk[0], 60, 100);
    for (f = 0; f < seconds * FS; f += CTL) {
        int32_t o[2 * CTL];
        mix_block(o, CTL);
        for (i = 0; i < 2u * CTL; i++) sum += fabs((double)o[i]) * (double)(1u + (f / CTL + i) % 7u);   /* (the order counts; magnitudes:
                                                                                            * > 0 when it sounds) */
    }
    for (i = 0; i < 3u; i++) trk_note_off(&trk[1], N[i]);
    trk_note_off(&trk[0], 60);
    return sum;
}
/* a render from the same start every time: in a child process (the engines keep state of their own across
 * renders); adopt: the routes into cords first. Its sum, or -1. fresh_lpol: track tr's LFO POL */
static int fresh_lpol;
static double render_fresh(uint32_t tr, uint32_t s, uint32_t d, int32_t a, int adopt, const cord_t *cord)
{
    int fd[2];
    double r = -1;
    pid_t child;
    if (pipe(fd)) return -1;
    child = fork();
    if (!child) {
        fresh_tracks();
        trk[tr].p[P_LPOL] = (int16_t)fresh_lpol;
        if (s) route(tr, 2, (int32_t)s, (int32_t)d, a);
        if (adopt && (cord_adopt() || !routes_off(tr) || cord_count() != 1u)) r = -2;
        if (cord) cord_set(0, cord);
        if (r != -2) r = render_chord(1);
        if (write(fd[1], &r, sizeof r) != sizeof r) _exit(1);
        _exit(0);
    }
    close(fd[1]);
    if (read(fd[0], &r, sizeof r) != sizeof r) r = -1;
    close(fd[0]);
    waitpid(child, 0, 0);
    return r;
}
static int adoption(void)
{
    static const struct { uint8_t s, d; int8_t a; const char *what; } R[] = {
        {MS_VEL, MD_CUT, 40, "VEL -> CUTOFF"}, {MS_KEY, MD_PITCH, -23, "KEY -> PITCH"}, {MS_ENV, MD_AMP, 50, "ENV -> GAIN"},
        {MS_RAND, MD_SHP, 63, "RAND -> SHAPE"}, {MS_LFO, MD_CUT, -64, "LFO -> CUTOFF"}, {MS_MODW, MD_DEPTH, 30, "MODW -> DEPTH"},
        {MS_ENV, MD_DEPTH, -40, "ENV -> DEPTH"}, {MS_LFO, MD_PAN, 37, "LFO -> PAN"}, {MS_ENV, MD_RATE, 21, "ENV -> RATE"},
        {MS_LFO, MD_VIB, 45, "LFO -> VIB"}, {MS_VEL, MD_E1 + 4, -50, "VEL -> E5"}};
    int fails = bad, ok;
    int32_t a;
    uint32_t i, n;
    cord_t c;
    double was, now;
    char what[160];

    for (a = -64, ok = 1; a <= 63; a++) {               /* every AMT: the cord's percentage moves by it again */
        if (!a) continue;
        cord_clear();
        route(0, 0, MS_VEL, MD_AMP, a);
        if (cord_adopt() || !routes_off(0) || cord_get(0)->src != MS_VEL || cord_get(0)->dst != CD_GAIN ||
            cord_amt(cord_get(0)->amount) != a || cord_get(0)->src_trk || cord_get(0)->dst_trk) ok = 0;
    }
    check("MOD's route -> a cord on its track, for every AMT (-64..63) the same amount again (cord_amt)", ok);
    was = render_fresh(0, 0, 0, 0, 0, 0);
    check("(two renders from the same start: the same)", was > 0 && was == render_fresh(0, 0, 0, 0, 0, 0));
    for (i = 0; i < sizeof R / sizeof R[0]; i++) {      /* the sound: a route and its cord, sample for sample */
        was = render_fresh(1, R[i].s, R[i].d, R[i].a, 0, 0);
        now = render_fresh(1, R[i].s, R[i].d, R[i].a, 1, 0);
        snprintf(what, sizeof what, "%s on a chord: the cord sounds as the route did, bit for bit", R[i].what);
        check(what, was > 0 && was == now);
    }
    fresh_lpol = 1;                                     /* the LFO at POL UNI: MOD read it 0..1, the cord + does */
    for (i = 0; i < sizeof R / sizeof R[0]; i++) {
        if (R[i].s != MS_LFO) continue;
        was = render_fresh(1, R[i].s, R[i].d, R[i].a, 0, 0);
        now = render_fresh(1, R[i].s, R[i].d, R[i].a, 1, 0);
        snprintf(what, sizeof what, "%s, the LFO at POL UNI: the cord + (one way), as the route sounded", R[i].what);
        check(what, was > 0 && was == now);
    }
    fresh_lpol = 0;
    c = C(CS_VEL, 1, CD_CUTOFF, 1, 60, 0);
    was = render_fresh(0, 0, 0, 0, 0, &c);
    c = C(CS_VEL, 0, CD_CUTOFF, 1, 60, 0);
    now = render_fresh(0, 0, 0, 0, 0, &c);
    check("...VEL on its own track's chord: each voice its own (another track's VEL: its latest, not the same)",
          was > 0 && now > 0 && was != now && was != render_fresh(0, 0, 0, 0, 0, 0));
    fresh_tracks();
    route(2, 0, MS_LFO, MD_PAN, 20);
    route(2, 1, MS_LFO, 0, 30);                         /* (no destination: moves nothing) */
    route(2, 3, MS_VEL, MD_RESERVED7, 30);
    n = cord_adopt();
    check("a project's routes (just loaded) -> cords; ones that move nothing just go", !n && routes_off(2) &&
          cord_count() == 1u && cord_get(0)->dst == CD_PAN);
    cord_sound_load(2);
    (void)cord_adopt();
    check("...another sound on the track: the project's cord stays", cord_count() == 1u);
    route(2, 0, MS_VEL, MD_CUT, 20);                    /* a sound with a route, loaded */
    route(2, 1, MS_ENV, MD_E1 + 2, -10);
    (void)cord_adopt();
    check("a sound's routes -> cords (the next free: 2 and 3)", cord_count() == 3u && cord_get(1)->dst == CD_CUTOFF &&
          cord_get(2)->dst == CD_E3 && cord_get(2)->amount == -16 && routes_off(2));
    c = *cord_get(2);
    c.amount = -50;
    cord_set(2, &c);                                    /* (one edited: the project's now) */
    cord_sound_load(2);
    (void)cord_adopt();
    check("...another sound loaded: the sound's cord goes, the one edited stays", cord_count() == 2u &&
          !cord_get(1)->dst && cord_get(2)->dst == CD_E3 && cord_get(2)->amount == -50);
    route(2, 0, MS_LFO, MD_PAN, 20);                    /* (the same as cord 1: an undo, the same sound again) */
    (void)cord_adopt();
    check("a route there is a cord for already: no second one", cord_count() == 2u && routes_off(2));

    cord_clear();
    for (i = 0; i < CORD_LIVE; i++) {
        c = C(CS_LFO, i % 4u, CD_LEVEL, i % 4u, 10 + (int32_t)i, 0);
        cord_set(i, &c);
    }
    route(3, 1, MS_AT, MD_CUT, 33);
    n = cord_adopt();
    check("sixteen cords in use: the route left, and still there (it plays as before)", n == 1u &&
          trk[3].p[P_M2SRC] == MS_AT && trk[3].p[P_M2AMT] == 33);
    c = C(0, 0, 0, 0, 0, 0);
    cord_set(9, &c);
    n = cord_adopt();
    check("...a cord freed: the route takes it", !n && routes_off(3) && cord_get(9)->src == CS_AT &&
          cord_get(9)->src_trk == 3u && cord_get(9)->dst == CD_CUTOFF);
    cord_clear();
    return bad - fails;
}

/* DRIFT and the default patch (phase 3) */
static int drift_and_defaults(void)
{
    int fails = bad, smooth = 1, bounded = 1;
    int32_t lo[NTRK], hi[NTRK], prev[NTRK];
    uint32_t b, tr, apart = 0, saved = cord_blocks;
    double d;
    for (tr = 0; tr < NTRK; tr++) lo[tr] = 32767, hi[tr] = -32767, prev[tr] = cord_drift(tr);
    for (b = 0; b < 1378u * 120u; b++) {            /* two minutes of blocks */
        cord_tick();
        for (tr = 0; tr < NTRK; tr++) {
            int32_t x = cord_drift(tr);
            if (x < -32767 || x > 32767) bounded = 0;
            if ((x > prev[tr] ? x - prev[tr] : prev[tr] - x) > 16) smooth = 0;   /* (a point 12 s on: a few a block) */
            if (x < lo[tr]) lo[tr] = x;
            if (x > hi[tr]) hi[tr] = x;
            prev[tr] = x;
        }
        if (cord_drift(0) != cord_drift(1)) apart++;
    }
    check("DRIFT: within -1..1, and smooth (at most 16 / 32768 a block: no step to hear)", bounded && smooth);
    for (tr = 0, b = 1; tr < NTRK; tr++)
        if (hi[tr] - lo[tr] < 16384) b = 0;
    check("...it wanders: over two minutes each track's covers at least half its range", b);
    check("...each track its own", apart > 1378u * 100u);
    cord_blocks = saved;

    fresh_tracks();
    {
        cord_t c = {CS_DRIFT, 0, CD_CUTOFF, 0, 12, 0};
        cord_set(0, &c);
    }
    cord_defaults();
    check("a new project's mods: none (automation is opt-in), all 16 free", cord_count() == 0u);
    cord_clear();
    return bad - fails;
}

int main(int argc, char **argv)
{
    const char *golden = argc > 1 ? argv[1] : "tests/cord_dests.txt";
    int update = getenv("CORD_UPDATE") && atoi(getenv("CORD_UPDATE"));
    uint32_t c, k, placed = 1, names = 1, mods = 1, params = 1, twice = 0;
    char what[160];

    host_tracks_init();
    tessera_reset();
    for (c = 0; c < CD_N; c++) {
        const cord_dest_t *d = cord_dest(c);
        if (c == CD_RETIRED_CL_RULE) { placed &= !d; continue; }   /* (CELLS' RULE, cut: a gap) */
        if (!d || d->code != c) { placed = 0; continue; }
        if (!d->name[0] || strlen(d->name) > 7u || !d->full[0]) names = 0;
        for (k = 0; k < c; k++)
            if (cord_dest(k) && !strcmp(cord_dest(k)->name, d->name)) names = 0;
        if (c == CD_OFF) continue;
        if (d->kind == CK_CORD) {                       /* (a mod's move: M1 AMT .. M16 AMT, no parameter) */
            if (d->pid != PID_NONE || d->md || c != CD_M1_AMT + (c - CD_M1_AMT) % 16u || c > CD_M16_AMT) mods = 0;
        } else if (d->kind == CK_FILL) {                /* (the project's FILL: no parameter) */
            if (d->pid != PID_NONE || d->md || c != CD_FILL) mods = 0;
        } else if (d->kind == CK_TEMPO) {               /* (the project's tempo: no parameter) */
            if (d->pid != PID_NONE || d->md || c != CD_TEMPO) mods = 0;
        } else if (d->kind == CK_CC) {                  /* (CC OUT: a MIDI CC, no parameter) */
            if (d->pid != PID_NONE || d->md || c != CD_CC) mods = 0;
        } else if (d->kind == CK_BUS) {                 /* (the MIX buses: no parameter) */
            if (d->pid != PID_NONE || d->md || (c != CD_BUS_A && c != CD_BUS_B)) mods = 0;
        } else if (d->kind == CK_VOICE || d->kind == CK_DEPTH) {
            if (d->pid != PID_NONE || !d->md || d->md >= MD_N || (d->md >= MD_RESERVED6 && d->md <= MD_RESERVED9) ||
                (d->kind == CK_VOICE) != (d->md <= MD_AMP) || (d->kind == CK_DEPTH) != (d->md == MD_DEPTH))
                mods = 0;
        } else {
            const pid_entry_t *e = pid_find(d->pid);
            const param_desc_t *pd = pid_desc(d->pid, 0);
            if (!e || (d->kind == CK_GLOBAL) == !!(e->flags & PID_F_TRACK) || (e->flags & (PID_F_ACTION | PID_F_DEVICE)) || !pd || (pd->max <= pd->min && !(d->pid >= PID_TRACK(P_E0) && d->pid <= PID_TRACK(P_E7))) ||   /* (E1..E8: the engine's) */
                (d->kind == CK_STEP) != (d->pid >= PID_TESS(0)) ||
                (d->kind == CK_GLOBAL) != (d->pid >= PID_GLOBAL(G_MFXTYPE) && d->pid <= PID_GLOBAL(G_FXRETURN))) {
                params = 0;
                printf("    0x%03x %s\n", d->pid, d->name);
            }
            for (k = 1; k < c; k++)
                twice += cord_dest(k) && cord_dest(k)->pid == d->pid;
        }
    }
    check("each destination at its own code, none missing", placed);
    check("names: at most 7 characters (a cord's chip), unique; every one explained", names);
    check("per voice and LFO depth: on mod.c's own destinations (MD_PITCH .. MD_AMP, MD_DEPTH)", mods);
    check("the others: a per-track parameter with a range (pid.c; E1..E8 the engine's; the send effects' the project's), "
          "not an action nor the device's", params);
    check("no parameter reached by two destinations", !twice);
    check("past the end: nothing", !cord_dest(CD_N) && !cord_dest(255));
    snprintf(what, sizeof what, "the limits: room for %u cords, %u in use (Theo 2026-10-16)", CORD_MAX, CORD_LIVE);
    check(what, CORD_MAX == 32u && CORD_LIVE == 16u);

    (void)behaviour();
    (void)fx_dests();
    (void)adoption();
    (void)drift_and_defaults();
    (void)buses();

    {
        FILE *f = fopen(golden, update ? "w" : "r");
        char want[160], got[160];
        uint32_t diffs = 0;
        if (!f) { check("tests/cord_dests.txt opens", 0); return 1; }
        if (update)
            fprintf(f, "# Cord destinations (firmware/src/cord.c): code, name, kind, parameter. FROZEN: saved cords keep the\n"
                       "# codes. Rewrite (CORD_UPDATE=1) only to append.\n");
        else
            while (fgets(want, sizeof want, f) && want[0] == '#') {}
        for (c = 0; c < CD_N; c++) {
            const cord_dest_t *d = cord_dest(c);
            snprintf(got, sizeof got, "%2u %-8s %-5s %s", c, d ? d->name : "?", d ? KIND[d->kind] : "?",
                     d && d->pid != PID_NONE ? pid_find(d->pid)->name : "-");
            if (update) { fprintf(f, "%s\n", got); continue; }
            if (c && !fgets(want, sizeof want, f)) want[0] = 0;
            want[strcspn(want, "\n")] = 0;
            if (strcmp(want, got) && diffs++ < 8) printf("    want: %s\n    got:  %s\n", want, got);
        }
        if (!update && fgets(want, sizeof want, f)) diffs++;
        fclose(f);
        if (update) printf("cord: wrote %u destinations to %s\n", (unsigned)CD_N, golden);
        else check("the destination codes as tests/cord_dests.txt froze them", !diffs);
    }
    printf("cord: %u destinations\n", (unsigned)CD_N);
    if (bad) printf("cord: %d FAILED\n", bad);
    else printf("cord: all passed\n");
    return bad != 0;
}
