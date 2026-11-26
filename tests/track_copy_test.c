/* SPDX-License-Identifier: GPL-3.0-only */
/* A track copied, pasted and moved (platform/core/tess_track.c): built with CORE_TRACKS=8 over the core API's include
 * list, as xtrk_test is. A demo loads; tracks get content and references across the banks; then
 *   - a TRACK paste (T2 onto T6): the sound, steps, settings, its mods (their source tracks kept), T6's MIDI channel;
 *   - a SOUND paste (T1 onto T3): the engine and sound, T3's steps and pattern settings kept;
 *   - swaps (a move): a and b changed places, references renumbered; swapped back, the save byte for byte the same;
 *   - refusals: a broken copy, a track out of range.
 * usage: track_copy_test DEMO.tsp */
#include "../platform/core/core_unit.c"

#if CORE_TRACKS != 8
#error "track_copy_test: build with -DCORE_TRACKS=8"
#endif

static uint8_t buf_in[65536], buf_a[65536], buf_b[65536], cp[65536];
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

static uint32_t a_dest(void)
{
    uint32_t d;
    for (d = 1; d < CD_N; d++)
        if (cord_dest(d) && cord_dest(d)->kind < CK_GLOBAL && !(d >= CD_M1_AMT && d <= CD_M16_AMT)) return d;
    return 0;
}

static uint32_t cords_onto(uint32_t t, uint32_t src_trk)
{
    uint32_t i, n = 0;
    for (i = 0; i < CORD_LIVE; i++) n += tt_cord_onto(&cords[i], t) && cords[i].src_trk == src_trk;
    return n;
}

static uint32_t notes_of(uint32_t t)
{
    uint32_t i, n = 0;
    for (i = 0; i < RECORD_MAX; i++) n += recording[i].vel && (recording[i].owner & REC_OWNER_MASK) / NPAT == t;
    return n;
}

/* the demo with content on T2, T6 and references across the banks */
static void setup(uint32_t n)
{
    uint32_t dst = a_dest(), t;
    tess_core_load(buf_in, n);
    for (t = 0; t < 8u; t++) {
        trk[t].p[P_LEVEL] = (int16_t)(30 + t);
        tessera.midi_ch[t] = (uint8_t)(t + 1u);
        pattern_at(t, 2)->step[4] = (step_t){{(uint8_t)(48u + t)}, 1, ST_NOTE, 0, 0, 90, 0, 0, 0, 0};
        pattern_at(t, 2)->timing[0] = (int16_t)(8 + t);
    }
    tessera.sub_human[1] = 9;
    chain_patterns[3][1] = 5;
    tessera.evolve[0].follow = 2;                       /* T1 follows T2; T7 branches to T2; T3 reads T6 */
    tessera.evolve[6].br_to = 2;
    tessera.evolve[2].lg_with = 6;
    tessera.follow_hold[4] = 2;
    tessera.sd_src[5] = 2;
    tessera.sec_pc = 0x02;
    memset(cords, 0, sizeof cords);
    cords[3] = (cord_t){CS_LFO, 4, (uint8_t)dst, 1, 50, 0, CU_BI};   /* T5's LFO onto T2 */
    cords[5] = (cord_t){CS_RAND, 1, (uint8_t)dst, 6, -20, 10, CU_BI}; /* T2's RAND onto T7 */
    cord_masks();
    {
        recorded_note_t r;
        memset(&r, 0, sizeof r);
        r.vel = 100; r.note = 60; r.step = 0; r.duration = 4096u; r.owner = (uint8_t)(1u * NPAT + trk[1].pattern);
        recording[RECORD_MAX - 2u] = r;
        recording_reindex();
    }
    motion.count = 1;                                   /* a motion event on T1 */
    motion.event[0] = (motion_event_t){(uint8_t)(0u << 6 | 3u), P_LEVEL, 77};
    motion_pattern[0] = trk[0].pattern;
    song.sel = 1;
}

int main(int argc, char **argv)
{
    uint32_t n, len, a, b, dst;
    if (argc != 2) { fprintf(stderr, "usage: track_copy_test DEMO.tsp\n"); return 3; }
    n = read_file(argv[1], buf_in, sizeof buf_in);
    if (!n) { fprintf(stderr, "track_copy_test: can't read %s\n", argv[1]); return 3; }
    tess_core_boot();
    dst = a_dest();

    /* TRACK: T2 onto T6 */
    setup(n);
    tessera.chord[1].uq[0] = CH_MAJ7;                 /* (CHORD USER's quality, ENTRIES' grid: T2 IN in A, OUT in C) */
    tessera.et_grid[0] |= 1u << 2;
    tessera.et_grid[2] |= 2u << 2;
    len = tess_core_track_copy(1, cp, sizeof cp);
    CHECK(len > 0u && len <= tess_core_tsp_max(), "T2 copied (%u B)", len);
    CHECK(!tess_core_track_copy(1, cp, 20), "..not into 20 B");
    CHECK(!tess_core_track_paste(5, TESS_PASTE_TRACK, cp, len), "T2 pasted onto T6");
    CHECK(trk[5].eng_req == trk[1].eng_req && trk[5].p[P_LEVEL] == 31 && trk[5].p[P_E0] == trk[1].p[P_E0],
          "..its sound (engine %u, LEVEL %d)", trk[5].eng_req, trk[5].p[P_LEVEL]);
    CHECK(pattern_at(5, 2)->step[4].note[0] == 49 && pattern_at(5, 2)->timing[0] == 9 && chain_patterns[3][5] == 5 &&
              tessera.sub_human[5] == 9,
          "..its steps, timing, SONG column, settings");
    CHECK(tessera.midi_ch[5] == 6, "..T6's MIDI channel kept (%u)", tessera.midi_ch[5]);
    CHECK(tessera.chord[5].uq[0] == CH_MAJ7 && (tessera.et_grid[0] >> 10 & 3u) == 1u && (tessera.et_grid[2] >> 10 & 3u) == 2u,
          "..its CHORD USER qualities and ENTRIES' grid cells");
    CHECK(cords_onto(5, 4) == 1u && cords_onto(1, 4) == 1u, "..the mod from T5 onto it, T2's kept too");
    CHECK(cords[5].dst_trk == 6 && cords[5].src_trk == 1, "..a mod from T2 elsewhere untouched");
    CHECK(notes_of(5) == 1u && notes_of(1) == 1u, "..its timed note (T6 %u, T2 %u)", notes_of(5), notes_of(1));
    CHECK((tessera.sec_pc >> 5 & 1u) && (tessera.sec_pc >> 1 & 1u), "..SECTION PC");
    CHECK(!tess_core_track_paste(5, TESS_PASTE_TRACK, cp, len) && cords_onto(5, 4) == 1u,
          "pasted again: one mod onto it still");

    /* SOUND: T2 onto T3 */
    setup(n);
    {
        int16_t slen = trk[2].p[P_SLEN];
        uint8_t note = pattern_at(2, 2)->step[4].note[0];
        len = tess_core_track_copy(1, cp, sizeof cp);
        trk[2].p[P_E0] = (int16_t)(trk[1].p[P_E0] + 1);
        CHECK(!tess_core_track_paste(2, TESS_PASTE_SOUND, cp, len), "T2's sound pasted onto T3");
        CHECK(trk[2].eng_req == trk[1].eng_req && trk[2].p[P_E0] == trk[1].p[P_E0], "..the engine and its parameters");
        CHECK(trk[2].p[P_SLEN] == slen && pattern_at(2, 2)->step[4].note[0] == note && tessera.sub_human[2] != 9,
              "..T3's steps, LEN and settings kept");
        CHECK(cords_onto(2, 4) == 0u && notes_of(2) == 0u, "..no mods or timed notes come with a sound");
    }

    /* SWAP: T2 and T6, then back */
    setup(n);
    tessera.chord[1].uq[2] = CH_SUS4;                 /* (T2: a USER quality, FADE in B) */
    tessera.et_grid[1] |= 3u << 2;
    a = tess_core_save(buf_a, sizeof buf_a);
    {
        uint8_t e2 = trk[1].eng_req, e6 = trk[5].eng_req;
        CHECK(!tess_core_track_swap(1, 5), "T2 and T6 swapped");
        CHECK(tessera.chord[5].uq[2] == CH_SUS4 && tessera.chord[1].uq[2] == 0 && (tessera.et_grid[1] >> 10 & 3u) == 3u &&
                  !(tessera.et_grid[1] >> 2 & 3u),
              "..their CHORD USER qualities and grid cells");
        CHECK(trk[5].eng_req == e2 && trk[1].eng_req == e6 && trk[5].p[P_LEVEL] == 31 && trk[1].p[P_LEVEL] == 35,
              "..their sounds");
        CHECK(tessera.evolve[0].follow == 6 && tessera.evolve[6].br_to == 6 && tessera.evolve[2].lg_with == 2 &&
                  tessera.follow_hold[4] == 6 && tessera.sd_src[1] == 6,
              "..the references renumbered (FOLLOW %u, BRANCH %u, LOGIC %u, HOLD %u, SOURCE %u)",
              tessera.evolve[0].follow, tessera.evolve[6].br_to, tessera.evolve[2].lg_with, tessera.follow_hold[4],
              tessera.sd_src[1]);
        CHECK(cords[3].dst_trk == 5 && cords[5].src_trk == 5 && cords[5].dst_trk == 6, "..the mods renumbered");
        CHECK(notes_of(5) == 1u && notes_of(1) == 0u && (tessera.sec_pc >> 5 & 1u) && song.sel == 5,
              "..its timed note, SECTION PC, the selection");
        CHECK(!tess_core_track_swap(5, 1), "swapped back");
        b = tess_core_save(buf_b, sizeof buf_b);
        CHECK(a > 0u && a == b && !memcmp(buf_a, buf_b, a), "..the save byte for byte as before (%u, %u B)", a, b);
    }
    CHECK(!tess_core_track_swap(0, 2) && motion.count == 1u && motion.event[0].place >> 6 == 2u,
          "T1 and T3 swapped: T1's motion on T3");
    CHECK(!tess_core_track_swap(2, 0), "..and back");
    b = tess_core_save(buf_b, sizeof buf_b);
    CHECK(a == b && !memcmp(buf_a, buf_b, a), "..the save byte for byte as before");

    /* refusals */
    len = tess_core_track_copy(1, cp, sizeof cp);
    CHECK(tess_core_track_paste(8, TESS_PASTE_TRACK, cp, len) && tess_core_track_swap(0, 8), "no track 9");
    cp[4] = 99;
    CHECK(tess_core_track_paste(2, TESS_PASTE_TRACK, cp, len), "another version: refused");
    cp[4] = 1;
    CHECK(tess_core_track_paste(2, TESS_PASTE_TRACK, cp, len - 1u), "cut short: refused");
    cp[TT_HEAD] = 255;                                  /* (the body's engine: none such) */
    b = tess_core_save(buf_b, sizeof buf_b);
    CHECK(tess_core_track_paste(2, TESS_PASTE_TRACK, cp, len) && tess_core_track_paste(2, TESS_PASTE_SOUND, cp, len),
          "no such engine: refused");
    a = tess_core_save(buf_a, sizeof buf_a);
    CHECK(a == b && !memcmp(buf_a, buf_b, a), "..and nothing changed");
    (void)dst;

    printf(fails ? "track_copy_test: %d FAILED\n" : "track_copy_test: ok\n", fails);
    return fails != 0;
}
