/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* ENTRIES' arrangement grid (entries.c, an 8-track build's; tessera.et_grid, project part TSP_EGRID). On a demo with
 * FORM ABAC (C for two phrases) and ENTRIES at 100: T2 OUT in A, T3 IN in B, T4 OUT in A and B and FADE in C. Over a
 * long play, mid-phrase, each cell is obeyed (OUT out, IN and FADE in), and T4 rises over C's first phrase and falls
 * over its last (A next has it OUT). Every cell AUTO: the project saves no grid
 * part. The grid saves, loads and is the same; the record is written to OUT for the 4-track build to read
 * (tests/core_tracks.sh).
 *   egrid_test DEMO.tsp OUT.tsp      exit 0: passed */
#include "../platform/core/core_unit.c"

#include <stdio.h>

#if CORE_TRACKS != 8
#error "egrid_test.c: build with -DCORE_TRACKS=8"
#endif

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL "); } else printf("ok   "); printf(__VA_ARGS__); printf("\n"); } while (0)
static float l[256], r[256];
static void cell(uint32_t s, uint32_t tr, uint32_t c)
{
    tessera.et_grid[s] = (uint16_t)((tessera.et_grid[s] & ~(3u << (2u * tr))) | c << (2u * tr));
}
static uint32_t has_part(const uint8_t *rec, uint32_t len, uint32_t id)
{
    uint32_t pos = TSP_HEAD, n = rec[8] | (uint32_t)rec[9] << 8, i;
    for (i = 0; i < n && pos + 4u <= len; i++) {
        if (rec[pos] == id) return 1;
        pos += 4u + (rec[pos + 2u] | (uint32_t)rec[pos + 3u] << 8);
    }
    return 0;
}

int main(int argc, char **argv)
{
    static uint8_t buf[TSP_MAX + 64], rec[TSP_MAX + 64];
    FILE *f = argc > 1 ? fopen(argv[1], "rb") : 0;
    size_t n = f ? fread(buf, 1, sizeof buf, f) : 0;
    uint32_t len, bar = 0xFFFFFFFFu, checked[3] = {0}, bad[3] = {0}, rise = 0, rose = 0, fall = 0, fell = 0;
    if (f) fclose(f);
    tess_core_boot();
    if (!n || tess_core_load(buf, (uint32_t)n)) {
        printf("egrid_test: %s did not load\n", argc > 1 ? argv[1] : "(no file)");
        return 1;
    }
    len = tess_core_save(rec, sizeof rec);
    CHECK(len && !has_part(rec, len, TSP_EGRID), "every cell AUTO: no grid part saved");

    tessera.fc_form = 2;                               /* (ABAC) */
    tessera.fc_phrase = 0;                             /* (4 bars a phrase) */
    for (uint32_t k = 0; k < 4u; k++) tessera.fc_rep[k] = k == 3u ? 2u : 1u;
    tessera.et_amount = 100;
    tessera.et_style = 0;                              /* (CUT: the grid's FADE is its own) */
    for (uint32_t k = 0; k < 4u; k++) trk[k].p[P_MUTE] = 0;
    cell(0, 1, ET_G_OUT);
    cell(1, 2, ET_G_IN);
    cell(0, 3, ET_G_OUT);
    cell(1, 3, ET_G_OUT);
    cell(2, 3, ET_G_FADE);
    tess_core_transport(1);
    while (bar_n < 48u) {
        tess_core_render(l, r, 0, 256);
        if (bar_n != bar) {
            uint8_t seq[4], role[TESSERA_TRACKS];
            uint32_t pos, left, warn, out, let;
            bar = bar_n;
            if (bar % 4u != 2u) continue;              /* (mid-phrase: the line's changes done) */
            tessera_chain_get(seq, &pos, &left);
            let = seq[pos & 3u];
            out = tessera_entries_get(&warn, role);
            if (let == 0) checked[0]++, bad[0] += !(out >> 1 & 1u) || !(out >> 3 & 1u);
            if (let == 1) checked[1]++, bad[1] += (out >> 2 & 1u) != 0;
            if (let == 2) checked[2]++, bad[2] += (out >> 3 & 1u) != 0;
        }
        if (et.fin >> 3 & 1u) {                       /* (C's first phrase: T4's level rising) */
            rise++;
            if (entry_gain[3] > 0 && entry_gain[3] < 32768) rose++;
        }
        if (et.fout >> 3 & 1u) {                      /* (C's last: falling) */
            fall++;
            if (entry_gain[3] > 0 && entry_gain[3] < 32768) fell++;
        }
    }
    tess_core_transport(0);
    CHECK(checked[0] && !bad[0], "A: T2 and T4 OUT (out %u of %u mid-phrase looks)", checked[0] - bad[0], checked[0]);
    CHECK(checked[1] && !bad[1], "B: T3 IN (in %u of %u)", checked[1] - bad[1], checked[1]);
    CHECK(checked[2] && !bad[2], "C: T4 FADE plays (in %u of %u)", checked[2] - bad[2], checked[2]);
    CHECK(rise && rose * 2u > rise, "C: a FADE in rises over the phrase (%u of %u blocks part way)", rose, rise);
    CHECK(fall && fell * 2u > fall, "C: ..and falls over its last (%u of %u blocks part way)", fell, fall);

    len = tess_core_save(rec, sizeof rec);
    CHECK(len && has_part(rec, len, TSP_EGRID), "the grid saved (part %u)", (unsigned)TSP_EGRID);
    {
        uint16_t was[3];
        memcpy(was, tessera.et_grid, sizeof was);
        memset(tessera.et_grid, 0, sizeof tessera.et_grid);
        CHECK(!tess_core_load(rec, len) && !memcmp(was, tessera.et_grid, sizeof was), "..and loads the same");
    }
    if (argc > 2 && (f = fopen(argv[2], "wb"))) fwrite(rec, 1, len, f), fclose(f);
    printf(fails ? "egrid_test: %d FAILED\n" : "egrid_test: ok\n", fails);
    return fails != 0;
}
