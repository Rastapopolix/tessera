/* SPDX-License-Identifier: GPL-3.0-only */
/* XTRK (project_tsp.c): an 8-track build's tracks 5..8 in a .tsp. Built with CORE_TRACKS=8 over the core API's
 * include list (platform/core/core_unit.c), as tsp_cross is. A demo loads (no XTRK: tracks 5..8 as a new project's);
 * tracks 5..8 get settings, steps, a timed note and a cord, tracks 1..4 references past track 4; the save holds XTRK,
 * loads back to the same tracks and saves byte for byte the same. The record is written to OUT for the 4-track build
 * to read (tests/core_tracks.sh).
 * usage: xtrk_test DEMO.tsp OUT.tsp */
#include "../platform/core/core_unit.c"

#if CORE_TRACKS != 8
#error "xtrk_test: build with -DCORE_TRACKS=8"
#endif

static uint8_t buf_in[65536], buf_a[65536], buf_b[65536];
static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL "); } else printf("ok   "); printf(__VA_ARGS__); printf("\n"); } while (0)

static uint32_t read_file(const char *path, uint8_t *b, uint32_t cap)
{
    FILE *f = fopen(path, "rb");
    size_t n;
    if (!f) return 0;
    n = fread(b, 1, cap, f);
    fclose(f);
    return (uint32_t)n;
}

/* the record's part id's offset (0: none) */
static uint32_t part_at(const uint8_t *raw, uint32_t len, uint32_t id)
{
    uint32_t n = raw[8] | (uint32_t)raw[9] << 8, pos = 12u, i;   /* (TSP_HEAD) */
    (void)len;
    for (i = 0; i < n; i++) {
        uint32_t plen = raw[pos + 2u] | (uint32_t)raw[pos + 3u] << 8;
        if (raw[pos] == id) return pos + 4u;
        pos += 4u + plen;
    }
    return 0;
}

static uint32_t a_dest(void)
{
    uint32_t d;
    for (d = 1; d < CD_N; d++)
        if (cord_dest(d) && cord_dest(d)->kind < CK_GLOBAL) return d;
    return 0;
}

int main(int argc, char **argv)
{
    uint32_t n, a, b, t, ci = CORD_LIVE - 1u, dst;
    if (argc != 3) { fprintf(stderr, "usage: xtrk_test DEMO.tsp OUT.tsp\n"); return 3; }
    n = read_file(argv[1], buf_in, sizeof buf_in);
    if (!n) { fprintf(stderr, "xtrk_test: can't read %s\n", argv[1]); return 3; }
    tess_core_boot();
    trk[5].p[P_LEVEL] = 3;                                   /* (a value the load must replace) */
    CHECK(!tess_core_load(buf_in, n), "the demo loads");
    CHECK(trk[4].eng_req == TRK_DEF[4][0] && trk[5].p[P_LEVEL] != 3, "no XTRK: tracks 5..8 as a new project's");

    for (t = 4; t < 8u; t++) {                               /* tracks 5..8: settings, steps, the Tessera object's */
        trk[t].p[P_LEVEL] = (int16_t)(40 + t);
        pattern_at(t, 3)->step[5] = (step_t){{(uint8_t)(50u + t)}, 1, ST_NOTE, 0, 0, 90, 0, 0, 0, 0};
        pattern_at(t, 3)->timing[0] = 12;
        tessera.sub_human[t] = (uint8_t)t;
        tessera.midi_tr[t] = -3;
        pattern_dir[t][2] = 1;
        chain_patterns[1][t] = 2;
    }
    trk[6].step[0] = (step_t){{62}, 1, ST_NOTE, SF_RECORDED, 0, 100, 0, 0, 0, 0};
    pattern_commit(&trk[6]);
    {
        recorded_note_t r;
        memset(&r, 0, sizeof r);
        r.vel = 100; r.note = 62; r.step = 0; r.duration = 4096u; r.owner = (uint8_t)(6u * NPAT + trk[6].pattern);
        recording[RECORD_MAX - 1u] = r;
        recording_reindex();
    }
    tessera.evolve[0].follow = 6;                            /* tracks 1..4's references past track 4 */
    tessera.evolve[1].br_to = 7;
    tessera.follow_hold[2] = 8;
    tessera.sd_src[3] = 5;
    tessera.sec_pc = 0x31;
    tessera.quad[0].link = (uint8_t)(1u << 1 | 1u << 5);  /* QUAD LINK: track 1 to 2 and 6, track 6 to 7 */
    tessera.quad[5].link = (uint8_t)(1u << 6);
    dst = a_dest();
    cords[ci] = (cord_t){0};
    cords[ci].src = CS_LFO; cords[ci].src_trk = 5; cords[ci].dst = (uint8_t)dst; cords[ci].dst_trk = 1;
    cords[ci].amount = 40;
    cord_masks();
    song.sel = 5;

    a = tess_core_save(buf_a, sizeof buf_a);
    CHECK(a > 0u, "the save (%u B)", a);
    CHECK(a && part_at(buf_a, a, 0xC0u), "it holds XTRK (0xC0)");
    {
        uint32_t c = part_at(buf_a, a, 37u /* TSP_CORDS */ );
        (void)c;
    }

    CHECK(!tess_core_load(buf_in, n), "the demo again (tracks 5..8 reset)");
    CHECK(trk[5].p[P_LEVEL] != 45 && tessera.evolve[0].follow != 6, "..its tracks 5..8 and references");
    CHECK(!tess_core_load(buf_a, a), "the 8-track record loads");
    CHECK(trk[5].p[P_LEVEL] == 45 && trk[7].p[P_LEVEL] == 47, "..tracks 5..8's settings");
    CHECK(pattern_at(5, 3)->step[5].note[0] == 55 && pattern_at(5, 3)->step[5].time == ST_NOTE &&
          pattern_at(5, 3)->timing[0] == 12, "..their steps and timing");
    CHECK(tessera.sub_human[6] == 6 && tessera.midi_tr[7] == -3 && pattern_dir[4][2] == 1 && chain_patterns[1][5] == 2,
          "..their Tessera settings, DIR, SONG column");
    CHECK(tessera.evolve[0].follow == 6 && tessera.evolve[1].br_to == 7 && tessera.follow_hold[2] == 8 &&
          tessera.sd_src[3] == 5 && tessera.sec_pc == 0x31 && song.sel == 5, "..tracks 1..4's references past 4");
    CHECK(tessera.quad[0].link == (1u << 1 | 1u << 5) && tessera.quad[5].link == (1u << 6),
          "..QUAD LINK across the banks (track 1: %02x, track 6: %02x)", tessera.quad[0].link, tessera.quad[5].link);
    CHECK(cords[ci].src == CS_LFO && cords[ci].src_trk == 5 && cords[ci].dst == dst && cords[ci].amount == 40,
          "..the cord from track 6");
    {
        uint32_t i, k = 0;
        for (i = 0; i < RECORD_MAX; i++) k += recording[i].vel && (recording[i].owner & REC_OWNER_MASK) / NPAT == 6u;
        CHECK(k == 1u, "..track 7's timed note (%u)", k);
    }
    b = tess_core_save(buf_b, sizeof buf_b);
    CHECK(a == b && !memcmp(buf_a, buf_b, a), "saved again: byte for byte the same (%u, %u B)", a, b);
    {
        FILE *f = fopen(argv[2], "wb");
        CHECK(f && fwrite(buf_a, 1, a, f) == a && !fclose(f), "written to %s", argv[2]);
    }
    printf(fails ? "xtrk_test: %d FAILED\n" : "xtrk_test: ok\n", fails);
    return fails != 0;
}
