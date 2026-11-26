/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The dub FORMs (entries.c dub_form_apply, an 8-track build's; Tessadub in Cosmati, step 3) and the grid's DROP column
 * (tess_core.c core_egrid). Roles T1 BASS, T2 RHYTHM, T3 HARMONY, T4 MELODY, T5 ANCHOR. Each FORM writes only the
 * CHAIN's and ENTRIES' settings and the grid, the same every time; VERSION played: in B (the DROP) T3 and T4 are out
 * and T1 T2 T5 in, in C (the break) T1 is out and T2 T3 in. A letter's head clicked past FADE gives the DROP column
 * (the spine in, the rest out), then AUTO. The project saves and reads back the same grid.
 *   dubform_test DEMO.tsp      exit 0: passed */
#include "../platform/core/core_unit.c"

#include <stdio.h>

#if CORE_TRACKS != 8
#error "dubform_test.c: build with -DCORE_TRACKS=8"
#endif

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL "); } else printf("ok   "); printf(__VA_ARGS__); printf("\n"); } while (0)
static float l[256], r[256];
static uint32_t cell(uint32_t s, uint32_t tr) { return tessera.et_grid[s] >> (2u * tr) & 3u; }
static void roles(void)
{
    static const uint8_t R[8] = {ET_BASS, ET_RHYTHM, ET_HARMONY, ET_MELODY, ET_ANCHOR, ET_MELODY, ET_HARMONY, ET_RHYTHM};
    for (uint32_t tr = 0; tr < TESSERA_TRACKS; tr++) tessera.et_role[tr] = R[tr], trk[tr].p[P_MUTE] = 0;
}
static void head(uint32_t s) { core_egrid((int32_t)(0x100u + s), TESS_ZA_CLICK); }   /* (a click on letter s's head) */

int main(int argc, char **argv)
{
    static uint8_t buf[TSP_MAX + 64], rec[TSP_MAX + 64];
    FILE *f = argc > 1 ? fopen(argv[1], "rb") : 0;
    size_t n = f ? fread(buf, 1, sizeof buf, f) : 0;
    uint32_t i, k, bar = 0xFFFFFFFFu, seen[3] = {0}, bad[3] = {0}, len;
    uint16_t g1[3];
    if (f) fclose(f);
    tess_core_boot();
    if (!n || tess_core_load(buf, (uint32_t)n)) {
        printf("dubform_test: %s did not load\n", argc > 1 ? argv[1] : "(no file)");
        return 1;
    }
    roles();
    CHECK(dub_form_count() == 5u && !strcmp(dub_form_name(0), "VERSION") && !strcmp(dub_form_name(4), "STEPPERS") &&
          !dub_form_name(5)[0] && !dub_form_apply(5), "five FORMs, VERSION .. STEPPERS; none past them");
    for (i = 0; i < dub_form_count(); i++) {
        CHECK(dub_form_apply(i) && tessera.fc_form >= 1u && tessera.fc_form <= 6u && tessera.fc_wander == 0u &&
              tessera.et_amount && tessera.et_dwell >= 1u && tessera.et_dwell <= 8u,
              "%s: FORM %u, PHRASE %u, ENTRIES %u, DWELL %u, WANDER 0", dub_form_name(i), tessera.fc_form,
              tessera.fc_phrase, tessera.et_amount, tessera.et_dwell);
        for (k = 0; k < 3u; k++) g1[k] = tessera.et_grid[k];
        dub_form_apply(i);
        CHECK(!memcmp(g1, tessera.et_grid, sizeof g1), "  the same pick, the same grid");
        CHECK(cell(0, 4) == ET_G_IN || cell(0, 4) == ET_G_AUTO, "  the ANCHOR never out in A");
    }
    dub_form_apply(0);
    CHECK(tessera.fc_form == 2u && tessera.fc_phrase == 1u && cell(1, 2) == ET_G_OUT && cell(1, 3) == ET_G_OUT &&
          cell(1, 0) == ET_G_IN && cell(1, 1) == ET_G_IN && cell(2, 0) == ET_G_OUT && cell(2, 2) == ET_G_IN,
          "VERSION: ABAC, 8 bars; B the DROP (bass and drums), C the bass out");

    /* played: B and C obeyed mid-phrase */
    tess_core_transport(1);
    while (bar_n < 48u) {
        tess_core_render(l, r, 0, 256);
        if (bar_n != bar) {
            uint8_t seq[4], role[TESSERA_TRACKS];
            uint32_t pos, left, warn, out, let;
            bar = bar_n;
            if (bar % 8u != 4u) continue;
            tessera_chain_get(seq, &pos, &left);
            let = seq[pos & 3u];
            out = tessera_entries_get(&warn, role);
            if (let < 3u) seen[let]++;
            if (let == 1) bad[1] += !(out >> 2 & 1u) || !(out >> 3 & 1u) || (out & 0x13u);
            if (let == 2) bad[2] += !(out & 1u) || (out >> 1 & 1u) || (out >> 2 & 1u);
        }
    }
    tess_core_transport(0);
    CHECK(seen[1] && !bad[1], "VERSION played: B out T3 T4, in T1 T2 T5 (%u of %u bars wrong)", bad[1], seen[1]);
    CHECK(seen[2] && !bad[2], "VERSION played: C out T1, in T2 T3 (%u of %u bars wrong)", bad[2], seen[2]);

    /* the head's cycle: AUTO IN OUT FADE DROP AUTO */
    tessera.et_grid[0] = 0;
    head(0), head(0), head(0);
    CHECK(cell(0, 0) == ET_G_FADE && cell(0, 7) == ET_G_FADE, "head: three clicks, all FADE");
    head(0);
    CHECK(tessera.et_grid[0] == dub_drop_column() && cell(0, 4) == ET_G_IN && cell(0, 2) == ET_G_OUT,
          "  a fourth: DROP, the ANCHOR in, the rest out (%04x)", tessera.et_grid[0]);
    head(0);
    CHECK(tessera.et_grid[0] == 0u, "  a fifth: AUTO again");
    tessera.et_role[4] = ET_MELODY;
    CHECK((dub_drop_column() & 3u) == ET_G_IN && (dub_drop_column() >> 2 & 3u) == ET_G_IN &&
          (dub_drop_column() >> 4 & 3u) == ET_G_OUT, "no ANCHOR: the DROP column keeps BASS and RHYTHM");

    /* saved and read back */
    dub_form_apply(2);
    for (k = 0; k < 3u; k++) g1[k] = tessera.et_grid[k];
    len = tess_core_save(rec, sizeof rec);
    tessera.et_grid[0] = tessera.et_grid[1] = tessera.et_grid[2] = 0;
    CHECK(len && !tess_core_load(rec, len) && !memcmp(g1, tessera.et_grid, sizeof g1) && tessera.fc_form == 2u,
          "ROCKERS saves and reads back the same");

    printf("dubform test: %s\n", fails ? "FAILED" : "ok");
    return fails != 0;
}
