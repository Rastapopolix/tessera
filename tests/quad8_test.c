/* SPDX-License-Identifier: GPL-3.0-only */
/* QUAD's grid in an 8-track build (tessera.c tess_quad_rows): 4 rows (tracks 1..4, as on the FM-1) while no track
 * 5..8 is in QUAD, all 8 once one is. A column on DOWN, EVERY 1: each move takes a cell a row down, the last row's to
 * the first. Built with CORE_TRACKS=8 over platform/core/core_unit.c (tests/core_tracks.sh). */
#include "../platform/core/core_unit.c"

#if CORE_TRACKS != 8
#error "quad8_test: build with -DCORE_TRACKS=8"
#endif

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL "); } else printf("ok   "); printf(__VA_ARGS__); printf("\n"); } while (0)

static void setup(int with5)
{
    uint32_t i;
    for (i = 0; i < 8u; i++) {
        tessera.mode[i] = TESS_ENG_OFF;
        tessera.quad[i].cell[0] = (uint8_t)(i + 1u);   /* (column 1: each row's own mark) */
    }
    tessera.mode[0] = TESS_ENG_QUAD;
    if (with5) tessera.mode[5] = TESS_ENG_QUAD;
    tessera.qcol[0] = GEN_QC_DOWN;                     /* (EVERY: the first, 1 step) */
    tessera.qcol_cnt[0] = 0;
}

int main(void)
{
    uint32_t i, ok;
    tess_core_boot();
    setup(0);
    tess_quad_cols(0);
    for (i = ok = 0; i < 4u; i++) ok += tessera.quad[(i + 1u) % 4u].cell[0] == i + 1u;
    for (i = 4; i < 8u; i++) ok += tessera.quad[i].cell[0] == i + 1u;
    CHECK(ok == 8u, "QUAD on tracks 1..4 only: the column turns down 4 rows, tracks 5..8 untouched");
    setup(1);
    tess_quad_cols(0);
    for (i = ok = 0; i < 8u; i++) ok += tessera.quad[(i + 1u) % 8u].cell[0] == i + 1u;
    CHECK(ok == 8u, "QUAD on track 6 too: the column turns down all 8 rows (track 8's to track 1)");
    for (i = 0; i < 7u; i++) tess_quad_cols(0);
    for (i = ok = 0; i < 8u; i++) ok += tessera.quad[i].cell[0] == i + 1u;
    CHECK(ok == 8u, "..and is back after 8 moves");
    printf(fails ? "quad8_test: %d FAILED\n" : "quad8_test: ok\n", fails);
    return fails != 0;
}
