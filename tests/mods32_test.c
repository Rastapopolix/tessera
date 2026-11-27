/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* 32 mods in an 8-track build (cord.h CORD_LIVE 32, Cosmati; the FM-1 keeps 16). Mods 17..32 set, counted and live;
 * mod 20 moves its destination as mod 1 would; mods 17..32 as HOLD (with a track's notes as the clock) and as SCALE
 * onto an M AMT; saved and read back (TSP_MX32 beside TSP_MHOLD / TSP_MVCA / TSP_MHCLK, which keep mods 1..16 as
 * before); a record without mods 17..32's part: none of them HOLD or SCALE; no mod past 16 is reached by an M AMT.
 *   mods32_test      exit 0: passed */
#include "../platform/core/core_unit.c"

#include <stdio.h>

#if CORE_TRACKS != 8
#error "mods32_test.c: build with -DCORE_TRACKS=8"
#endif

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL "); } else printf("ok   "); printf(__VA_ARGS__); printf("\n"); } while (0)
static float l[256], r[256];
static cord_t C(uint32_t src, uint32_t st, uint32_t dst, uint32_t dt, int32_t amount, int32_t offset, uint32_t uni)
{
    cord_t c = {(uint8_t)src, (uint8_t)st, (uint8_t)dst, (uint8_t)dt, (int8_t)amount, (int8_t)offset, (uint8_t)uni};
    return c;
}

int main(void)
{
    static uint8_t rec[TSP_MAX + 64];
    uint32_t len, i;
    cord_t c;
    tess_core_boot();
    tess_core_project_new();
    cord_clear();
    for (i = 0; i < 4u; i++) tess_core_render(l, r, 0, 256);

    CHECK(CORD_LIVE == 32u && CORD_MAMT == 16u, "an 8-track build: %u mods, %u reachable by M AMT", CORD_LIVE, CORD_MAMT);
    c = C(0, 0, CD_PAN, 5, 0, 60, CU_BI);             /* (a fixed mod: T6's PAN, +60 %) */
    CHECK(cord_set(19u, &c) && cord_get(19u)->dst == CD_PAN && cord_count() == 1u, "mod 20 set, and counted");
    c = C(0, 0, CD_PAN, 5, 0, 1, CU_BI);
    CHECK(cord_set(31u, &c) && !cord_set(32u, &c) && cord_count() == 2u, "mod 32 set; past 32 refused");

    /* mod 20 moves as mod 1 does: the same fixed mod in slot 0 or 19, the same move */
    {
        int32_t a, b;
        cord_clear();
        c = C(CS_LFO, 5, CD_LEVEL, 5, 80, 0, CU_BI);
        cord_set(0, &c);
        for (i = 0; i < 4u; i++) tess_core_render(l, r, 0, 256);
        a = cord_move(0);
        cord_clear();
        cord_set(19u, &c);
        for (i = 0; i < 4u; i++) tess_core_render(l, r, 0, 256);
        b = cord_move(19u);
        CHECK(a == b && a == 80, "mod 20's move as mod 1's (%d, %d)", (int)a, (int)b);
    }

    /* HOLD and SCALE past 16, saved apart; mods 1..16's parts as before */
    cord_clear();
    c = C(CS_LFO, 0, CD_PAN, 0, 50, 0, CU_HOLD);
    cord_set(1u, &c);                                  /* (mod 2: HOLD, by the bar) */
    c = C(CS_LFO, 6, CD_PAN, 6, 50, 0, CU_HOLD);
    cord_set(20u, &c);                                 /* (mod 21: HOLD, T3's notes its clock) */
    cord_hold_clk_set(20u, 3u);
    c = C(0, 0, CD_PAN, 7, 0, 40, CU_BI);
    cord_set(2u, &c);                                  /* (mod 3: the one mod 25 scales) */
    c = C(CS_LFO, 7, CD_M1_AMT + 2u, 0, 60, 0, CU_BI);
    cord_set(24u, &c);
    cord_vca_load(1u << 24);
    CHECK(cord_hold_mask() == (1u << 1 | 1u << 20) && cord_hold_clk(20u) == 3u && cord_vca_mask() == 1u << 24,
          "mod 2 and 21 HOLD (21 by T3's notes), mod 25 SCALE onto M3 AMT");
    len = tess_core_save(rec, sizeof rec);
    cord_clear();
    CHECK(!cord_hold_mask() && !cord_vca_mask() && !cord_count(), "cleared");
    CHECK(len && !tess_core_load(rec, len) && cord_hold_mask() == (1u << 1 | 1u << 20) && cord_hold_clk(20u) == 3u &&
          cord_vca_mask() == 1u << 24 && cord_get(24u)->dst == CD_M1_AMT + 2u && cord_get(20u)->src_trk == 6u,
          "saved and read back: HOLD 2, 21 (T3's notes), SCALE 25, the mods as set (hold %08x, scale %08x)",
          cord_hold_mask(), cord_vca_mask());
    {
        uint8_t hc[CORD_HCLK_BYTES];
        CHECK(!cord_hold_clk_pack(hc) && cord_hold_clk_pack_at(hc, 16u) && hc[2] == 3u,
              "mods 1..16's clocks (none by notes: nothing) apart from 17..32's (mod 21's nibble)");
    }

    /* only past 16: TSP_MHOLD / TSP_MVCA stay out; a record without TSP_MX32: none past 16 */
    cord_clear();
    c = C(CS_LFO, 0, CD_PAN, 0, 50, 0, CU_HOLD);
    cord_set(30u, &c);
    len = tess_core_save(rec, sizeof rec);
    cord_clear();
    CHECK(len && !tess_core_load(rec, len) && cord_hold_mask() == 1u << 30, "HOLD on mod 31 alone: saved and read back");
    cord_hold_load(0);
    for (i = 0; i < CORD_LIVE; i++) {                  /* (its HOLD gone: a BI mod again) */
        cord_t k = *cord_get(i);
        if (k.uni == CU_HOLD) k.uni = CU_BI, cord_set(i, &k);
    }
    len = tess_core_save(rec, sizeof rec);
    CHECK(len && !tess_core_load(rec, len) && !cord_hold_mask() && cord_get(30u)->src == CS_LFO,
          "without HOLD past 16: none read, the mod kept");

    /* no M AMT reaches past 16 */
    cord_clear();
    c = C(CS_LFO, 0, CD_PAN, 0, 50, 0, CU_BI);
    cord_set(17u, &c);
    c = C(CS_LFO, 1, CD_M16_AMT, 0, 50, 0, CU_BI);   /* (onto mod 16, empty: nothing to turn) */
    cord_set(18u, &c);
    cord_vca_load(1u << 17);                           /* (SCALE on a mod not onto an M AMT: dropped) */
    CHECK(!(cord_vca_mask() >> 17 & 1u), "SCALE only onto an M AMT: mod 18's dropped");

    printf("mods32 test: %s\n", fails ? "FAILED" : "ok");
    return fails != 0;
}
