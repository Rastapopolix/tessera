/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* FOLLOW RHYTHM KICK (tessera.c tess_follow_gate, an 8-track build's; Tessadub in Cosmati, step 6). T1 ATLAS, T2
 * FOLLOWing it: with RHYTHM KICK, T2 plays only on the steps where T1's kick (part 0, note 36) played, and on fewer steps
 * than with SOURCE (every step T1 hits). Saved: the FMODE part and XTRK keep SOURCE (what an older or 4-track firmware
 * reads), TSP_FKICK brings KICK back; read without it, SOURCE. A copied track keeps KICK.
 *   fkick_test DEMO.tsp      exit 0: passed */
#define CORE_OUT_HOOKS 1
#include "../platform/core/core_unit.c"

#include <stdio.h>
#include <stdlib.h>

#if CORE_TRACKS != 8 || !TESS_FR_KICKS
#error "fkick_test.c: build with -DCORE_TRACKS=8"
#endif

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL "); } else printf("ok   "); printf(__VA_ARGS__); printf("\n"); } while (0)
static float l[256], r[256];
static uint32_t kick[512], hit1[512], f2[512];      /* (bits per step: T1 kicked, T1 hit, T2 played) */

static void hook(const track_t *t, uint32_t note, uint32_t vel)
{
    uint32_t k = (uint32_t)(t - trk), s;
    if (!vel || seq_run_step[k] == 0xFFFFFFFFu) return;
    s = seq_run_step[k];
    if (s >= 512u * 32u) return;
    if (k == 0) {
        hit1[s / 32u] |= 1u << (s % 32u);
        if (note == 36u) kick[s / 32u] |= 1u << (s % 32u);
    } else if (k == 1)
        f2[s / 32u] |= 1u << (s % 32u);
}
/* 8 bars: T2's steps, and those not on a kick (or a hit) of T1 */
static void play(const uint8_t *buf, uint32_t n, uint32_t rhy, uint32_t *on, uint32_t *off_kick, uint32_t *off_hit)
{
    uint32_t i;
    tess_core_transport(0);
    for (i = 0; i < 4u; i++) tess_core_render(l, r, 0, 256);
    tess_core_load(buf, n);
    for (i = 0; i < 4u; i++) tess_core_render(l, r, 0, 256);
    tessera_param_set(0, TX_MODE, TESS_ENG_GRIDS);
    tessera_param_set(1, TX_MODE, TESS_ENG_TURING);
    tessera.evolve[1].follow = 1;
    tessera.follow_rhy[1] = (uint8_t)rhy;
    trk[0].p[P_MUTE] = trk[1].p[P_MUTE] = 0;
    trk[1].p[P_SDIV] = trk[0].p[P_SDIV];               /* (the same steps, so the two count alike) */
    tessera.et_amount = 0;                             /* (ENTRIES off: nobody taken out) */
    memset(kick, 0, sizeof kick), memset(hit1, 0, sizeof hit1), memset(f2, 0, sizeof f2);
    tess_core_transport(1);
    for (i = 0; i < 64u && !song.playing; i++) tess_core_render(l, r, 0, 256);
    for (i = 0; i < 200000u && (seq_run_step[1] == 0xFFFFFFFFu || seq_run_step[1] < 8u * tess_steps_per_bar(&trk[1])); i++)
        tess_core_render(l, r, 0, 256);
    tess_core_transport(0);
    for (i = 0; i < 4u; i++) tess_core_render(l, r, 0, 256);
    if (getenv("FK_DEBUG")) {
        for (i = 0; i < 64u; i++) printf("%c", f2[i / 32u] >> (i % 32u) & 1u ? (kick[i / 32u] >> (i % 32u) & 1u ? 'K' : 'x') : kick[i / 32u] >> (i % 32u) & 1u ? 'k' : hit1[i / 32u] >> (i % 32u) & 1u ? 'h' : '.');
        printf("  (rhy %u)\n", rhy);
    }
    *on = *off_kick = *off_hit = 0;
    for (i = 0; i < 512u; i++) {
        *on += (uint32_t)__builtin_popcount(f2[i]);
        *off_kick += (uint32_t)__builtin_popcount(f2[i] & ~kick[i]);
        *off_hit += (uint32_t)__builtin_popcount(f2[i] & ~hit1[i]);
    }
}

int main(int argc, char **argv)
{
    static uint8_t buf[TSP_MAX + 64], rec[TSP_MAX + 64], clip[TSP_MAX];
    FILE *f = argc > 1 ? fopen(argv[1], "rb") : 0;
    size_t n = f ? fread(buf, 1, sizeof buf, f) : 0;
    uint32_t on_k, offk_k, offh_k, on_s, offk_s, offh_s, len, kicks = 0, hits = 0, i;
    if (f) fclose(f);
    tess_core_boot();
    if (!n || tess_core_load(buf, (uint32_t)n)) {
        printf("fkick_test: %s did not load\n", argc > 1 ? argv[1] : "(no file)");
        return 1;
    }
    out_note_hook = hook;
    play(buf, (uint32_t)n, TESS_FR_SOURCE, &on_s, &offk_s, &offh_s);
    for (i = 0; i < 512u; i++) kicks += (uint32_t)__builtin_popcount(kick[i]), hits += (uint32_t)__builtin_popcount(hit1[i]);
    CHECK(kicks && hits > kicks, "T1 ATLAS: %u kicks among %u hits in 8 bars", kicks, hits);
    CHECK(on_s && !offh_s && offk_s, "SOURCE: T2 on %u steps, each a T1 hit, %u of them not a kick", on_s, offk_s);
    play(buf, (uint32_t)n, TESS_FR_KICK, &on_k, &offk_k, &offh_k);
    CHECK(on_k && !offk_k && on_k < on_s, "KICK: T2 on %u steps, each a T1 kick (fewer than SOURCE's %u)", on_k, on_s);

    /* saved */
    tessera.follow_rhy[1] = TESS_FR_KICK;              /* (T2: the FMODE part) */
    tessera.follow_rhy[5] = TESS_FR_KICK;              /* (T6: XTRK) */
    tessera.follow_rhy[2] = TESS_FR_BOTH;
    len = tess_core_save(rec, sizeof rec);
    tessera.follow_rhy[1] = tessera.follow_rhy[5] = tessera.follow_rhy[2] = 0;
    CHECK(len && !tess_core_load(rec, len) && tessera.follow_rhy[1] == TESS_FR_KICK &&
          tessera.follow_rhy[5] == TESS_FR_KICK && tessera.follow_rhy[2] == TESS_FR_BOTH,
          "saved and read back: T2, T6 KICK, T3 BOTH (%u %u %u)", tessera.follow_rhy[1], tessera.follow_rhy[5],
          tessera.follow_rhy[2]);
    tsp_seen.at[TSP_FKICK] = 0;                        /* (read as a firmware without the part) */
    tsp_restore(rec);
    CHECK(tessera.follow_rhy[1] == TESS_FR_SOURCE && tessera.follow_rhy[5] == TESS_FR_SOURCE,
          "without TSP_FKICK: SOURCE, in the FMODE part and XTRK (%u %u)", tessera.follow_rhy[1], tessera.follow_rhy[5]);
    CHECK(!tess_core_load(rec, len), "  read again");
    len = tess_core_track_copy(5, clip, sizeof clip);
    CHECK(len && !tess_core_track_paste(6, 0, clip, len) && tessera.follow_rhy[6] == TESS_FR_KICK,
          "T6 copied onto T7: KICK (%u)", tessera.follow_rhy[6]);

    printf("fkick test: %s\n", fails ? "FAILED" : "ok");
    return fails != 0;
}
