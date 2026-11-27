/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* MIX buses C and D (cord.c, an 8-track build's TESS_BUS_CD; Cosmati). Two fixed mods into BUS C at MAX give the larger;
 * at SUM their sum; BUS D apart from C; the sources BUS C, D read them back (a mod from BUS C moves PAN as the bus);
 * MODE and LAG kept in range; saved and read back (TSP_BUSCD, the mods at their places); the record's TSP_CORDS holds
 * no BUS C, D code (they go in XTRK, which the FM-1 keeps unread: it opens the project and plays without them).
 *   buscd_test [OUT.tsp]      exit 0: passed; OUT.tsp: the record with BUS C, D (core_tracks.sh loads it at 4 tracks) */
#include "../platform/core/core_unit.c"

#include <stdio.h>

#if CORE_TRACKS != 8 || !TESS_BUS_CD
#error "buscd_test.c: build with -DCORE_TRACKS=8"
#endif

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL "); } else printf("ok   "); printf(__VA_ARGS__); printf("\n"); } while (0)
static float l[256], r[256];
static cord_t C(uint32_t src, uint32_t st, uint32_t dst, uint32_t dt, int32_t amount, int32_t offset)
{
    cord_t c = {(uint8_t)src, (uint8_t)st, (uint8_t)dst, (uint8_t)dt, (int8_t)amount, (int8_t)offset, 0};
    return c;
}
static void blocks(uint32_t n) { for (uint32_t i = 0; i < n; i++) tess_core_render(l, r, 0, 256); }
/* the record's part id: its bytes (0: none) */
static const uint8_t *part(const uint8_t *raw, uint32_t id, uint32_t *len)
{
    uint32_t pos = TSP_HEAD, n = raw[8] | (uint32_t)raw[9] << 8, i;
    for (i = 0; i < n; i++) {
        uint32_t plen = raw[pos + 2u] | (uint32_t)raw[pos + 3u] << 8;
        if (raw[pos] == id) { *len = plen; return raw + pos + 4u; }
        pos += 4u + plen;
    }
    return 0;
}

int main(int argc, char **argv)
{
    static uint8_t rec[TSP_MAX + 64];
    uint32_t len, i, plen = 0;
    cord_t c;
    const uint8_t *p;
    int ok;
    tess_core_boot();
    tess_core_project_new();
    cord_clear();
    blocks(4);

    CHECK(CORD_NBUS == 4u && str_eq(cord_src_name(CS_BUSC), "BUS C") && cord_dest(CD_BUS_D) &&
          str_eq(cord_dest(CD_BUS_D)->name, "BUS D"), "four buses: BUS C, D sources and destinations");
    c = C(0, 0, CD_BUS_C, 0, 0, 30);
    cord_set(4u, &c);
    c = C(0, 0, CD_BUS_C, 0, 0, 50);
    cord_set(9u, &c);
    c = C(0, 0, CD_BUS_D, 0, 0, -40);
    cord_set(20u, &c);
    tessera_param_set(0, TX_BUSC_MODE, CB_MAX);
    blocks(4);
    CHECK(cord_bus_now(2) > 16000 && cord_bus_now(2) < 16500, "BUS C at MAX: the larger, 50 %% (%d)", (int)cord_bus_now(2));
    tessera_param_set(0, TX_BUSC_MODE, CB_SUM);
    blocks(4);
    CHECK(cord_bus_now(2) > 26000 && cord_bus_now(2) < 26500, "BUS C at SUM: 80 %% (%d)", (int)cord_bus_now(2));
    CHECK(cord_bus_now(3) < -13000 && cord_bus_now(3) > -13300 && !cord_bus_now(0) && !cord_bus_now(1),
          "BUS D apart: -40 %% (%d); A and B untouched", (int)cord_bus_now(3));
    CHECK(cord_src(CS_BUSC, 0) == cord_bus_now(2) && cord_src(CS_BUSD, 5) == cord_bus_now(3) && cord_bipolar(CS_BUSC, 0) &&
          CS_PROJECT(CS_BUSC) && CS_PROJECT(CS_BUSD), "the sources BUS C, D read them, bipolar, the project's");
    c = C(CS_BUSC, 0, CD_PAN, 6, 100, 0);              /* (BUS C onto T7's PAN) */
    cord_set(30u, &c);
    blocks(4);
    CHECK(cord_count() == 4u, "a mod from BUS C onto T7's PAN (mod 31): live");

    /* MODE, LAG in range */
    tessera_param_set(0, TX_BUSD_LAG, 99);
    tessera_param_set(0, TX_BUSD_MODE, -2);
    CHECK(tessera_param_get(0, TX_BUSD_LAG) == (int32_t)CB_LAGS - 1 && tessera_param_get(0, TX_BUSD_MODE) == CB_SUM &&
          tessera_param_get(3, TX_BUSD_LAG) == (int32_t)CB_LAGS - 1, "MODE and LAG kept in range, any track's the same");

    /* saved and read back */
    tessera_param_set(0, TX_BUSC_MODE, CB_DIFF);
    tessera_param_set(0, TX_BUSC_LAG, 3);
    len = tess_core_save(rec, sizeof rec);
    cord_clear();
    for (i = 4; i < 8u; i++) cord_bus_set(i, 0);
    CHECK(len && !tess_core_load(rec, len) && cord_bus_get(4) == CB_DIFF && cord_bus_get(5) == 3u &&
          cord_bus_get(7) == CB_LAGS - 1u && cord_get(4u)->dst == CD_BUS_C && cord_get(9u)->offset == 50 &&
          cord_get(20u)->dst == CD_BUS_D && cord_get(30u)->src == CS_BUSC && cord_get(30u)->dst_trk == 6u,
          "saved and read back: C at DIFF, LAG 3, D's LAG, the four mods at their places");
    if (argc > 1) {                                    /* (for the 4-track build to load) */
        FILE *f = fopen(argv[1], "wb");
        if (f) fwrite(rec, 1, len, f), fclose(f);
    }
    p = part(rec, TSP_CORDS, &plen);
    for (i = 0, ok = p && plen == CORD_PK_BYTES; ok && i < CORD_MAX; i++)
        ok = p[i * 6u] != CS_BUSC && p[i * 6u] != CS_BUSD && p[i * 6u + 2u] != CD_BUS_C && p[i * 6u + 2u] != CD_BUS_D;
    CHECK(ok, "the record's TSP_CORDS: no BUS C, D code (the FM-1 opens it, without those mods)");
    CHECK(part(rec, TSP_BUSCD, &plen) && plen == 4u, "BUS C, D's settings in their own part");
    for (i = 4; i < 8u; i++) cord_bus_set(i, 0);
    cord_clear();
    len = tess_core_save(rec, sizeof rec);
    CHECK(len && !part(rec, TSP_BUSCD, &plen), "nothing set: no part");

    printf("buscd test: %s\n", fails ? "FAILED" : "ok");
    return fails != 0;
}
