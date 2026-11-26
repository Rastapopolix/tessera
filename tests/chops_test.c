/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* CHORD HIT's chops (gen.c gen_chord_tick, an 8-track build's; Tessadub in Cosmati, step 6): DOUBLE the "and" and the
 * 16th after it, BUBBLE every 16th but the beat's, the "and" louder; at 8 steps a bar both play OFFBEAT's stab. A
 * section's snapshot keeps them (bit 7, the 2 bits OFFBEAT / STEP for another firmware). The record: its TRACKS part
 * the stand-in, TSP_CHOP the chop; saved and read back. A track's copy, paste and swap carry them.
 *   chops_test DEMO.tsp      exit 0: passed */
#include "../platform/core/core_unit.c"

#include <stdio.h>

#if CORE_TRACKS != 8 || !TESS_CHOPS
#error "chops_test.c: build with -DCORE_TRACKS=8"
#endif

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL "); } else printf("ok   "); printf(__VA_ARGS__); printf("\n"); } while (0)

/* hit's gates over a bar of spb steps (one chord a bar), as a bit per step; vel[] each step's velocity */
static uint32_t bar(uint32_t hit, uint32_t spb, uint8_t *vel)
{
    gen_chord_t g;
    gen_clock_t clk = {0, (uint8_t)spb, 0};
    gen_out_t o;
    uint32_t m = 0;
    memset(&g, 0, sizeof g);
    g.hit = (uint8_t)hit;
    g.bars = 1;
    gen_chord_start(&g);
    for (uint32_t i = 0; i < spb; i++) {
        memset(&o, 0, sizeof o);
        gen_chord_tick(&g, &clk, &o);
        if (o.gate) m |= 1u << i;
        if (vel) vel[i] = o.vel;
    }
    return m;
}

int main(int argc, char **argv)
{
    static uint8_t buf[TSP_MAX + 64], rec[TSP_MAX + 64], clip[TSP_MAX];
    FILE *f = argc > 1 ? fopen(argv[1], "rb") : 0;
    size_t n = f ? fread(buf, 1, sizeof buf, f) : 0;
    uint8_t vel[32];
    uint32_t m, len;
    if (f) fclose(f);

    m = bar(GEN_CH_DOUBLE, 16, vel);
    CHECK(m == 0xCCCCu && vel[2] > vel[3], "DOUBLE, 16 steps: the \"and\" and the 16th after it (%04x), the \"and\" louder",
          (unsigned)m);
    m = bar(GEN_CH_BUBBLE, 16, vel);
    CHECK(m == 0xEEEEu && vel[2] > vel[1] && vel[1] == vel[3], "BUBBLE, 16 steps: every 16th but the beat's (%04x)",
          (unsigned)m);
    CHECK(bar(GEN_CH_DOUBLE, 8, 0) == bar(GEN_CH_OFF, 8, 0) && bar(GEN_CH_BUBBLE, 8, 0) == bar(GEN_CH_OFF, 8, 0),
          "8 steps a bar: both OFFBEAT's stab (%02x)", (unsigned)bar(GEN_CH_OFF, 8, 0));
    CHECK(bar(GEN_CH_OFF, 16, 0) == 0x4444u && bar(GEN_CH_HOLD, 16, 0) == 1u, "OFFBEAT and HOLD as they were");
    {
        gen_chord_t a, b;
        gen_sec_t s;
        memset(&a, 0, sizeof a);
        for (uint32_t h = GEN_CH_HOLD; h < GEN_CH_HITS; h++) {
            a.hit = (uint8_t)h;
            memset(&s, 0, sizeof s);
            gen_chord_save(&a, &s);
            memset(&b, 0, sizeof b);
            gen_chord_load(&b, &s);
            CHECK(b.hit == h && ((s.p0 >> 5) & 3u) == GEN_CH_SAVED(h) && (s.p0 >> 7) == (h >= GEN_CH_DOUBLE),
                  "a section's snapshot: HIT %u back as %u, others read %u", h, b.hit, (s.p0 >> 5) & 3u);
        }
    }

    tess_core_boot();
    if (!n || tess_core_load(buf, (uint32_t)n)) {
        printf("chops_test: %s did not load\n", argc > 1 ? argv[1] : "(no file)");
        return 1;
    }
    tessera.chord[1].hit = GEN_CH_DOUBLE;
    tessera.chord[5].hit = GEN_CH_BUBBLE;
    tessera.chord[2].hit = GEN_CH_STEP;
    len = tess_core_save(rec, sizeof rec);
    tessera.chord[1].hit = tessera.chord[5].hit = tessera.chord[2].hit = GEN_CH_HOLD;
    CHECK(len && !tess_core_load(rec, len) && tessera.chord[1].hit == GEN_CH_DOUBLE &&
          tessera.chord[5].hit == GEN_CH_BUBBLE && tessera.chord[2].hit == GEN_CH_STEP,
          "saved and read back: T2 DOUBLE, T6 BUBBLE, T3 STEP (%u %u %u)", tessera.chord[1].hit, tessera.chord[5].hit,
          tessera.chord[2].hit);
    tsp_seen.at[TSP_CHOP] = 0;                         /* (as a firmware without the part reads it: the stand-ins) */
    tsp_restore(rec);
    CHECK(tessera.chord[1].hit == GEN_CH_OFF && tessera.chord[5].hit == GEN_CH_STEP,
          "without TSP_CHOP: OFFBEAT and STEP, what an older firmware plays (%u %u)", tessera.chord[1].hit,
          tessera.chord[5].hit);
    CHECK(!tess_core_load(rec, len), "  read again");

    /* copy, paste, swap */
    len = tess_core_track_copy(1, clip, sizeof clip);
    CHECK(len && !tess_core_track_paste(3, 0, clip, len) && tessera.chord[3].hit == GEN_CH_DOUBLE,
          "T2 copied onto T4: DOUBLE (%u)", tessera.chord[3].hit);
    CHECK(!tess_core_track_swap(5, 0) && tessera.chord[0].hit == GEN_CH_BUBBLE, "T6 and T1 swapped: T1 BUBBLE (%u)",
          tessera.chord[0].hit);
    tessera.chord[6].hit = GEN_CH_BEAT;
    len = tess_core_track_copy(6, clip, sizeof clip);
    tessera.chord[7].hit = GEN_CH_DOUBLE;
    CHECK(len && !tess_core_track_paste(7, 0, clip, len) && tessera.chord[7].hit == GEN_CH_BEAT,
          "a track without a chop pasted over one: its own HIT (BEAT, %u)", tessera.chord[7].hit);

    printf("chops test: %s\n", fails ? "FAILED" : "ok");
    return fails != 0;
}
