/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* ATLAS's genre banks (gen.c gen_grids_map, gen_atlas_banks.h; an 8-track build's, project part TSP_ABANK). Each bank
 * is a map of its own (no two the same, none Grids'); X goes from the skeleton to the fullest (no hit at the left that
 * is not also at the right); the snare lane plays the bank's note (RIM, CLAP or SD); BANK ALL plays as before. BANK
 * saves only when a track's is not ALL, loads the same, goes with a track copied and swapped; a 4-track build reads
 * the record (tests/core_tracks.sh).
 *   abank_test OUT.tsp                    the checks; exit 0: passed
 *   abank_test wav BANK SECS_PER OUT.wav  a listen: BANK's five feels (Y) in turn, each at X left, middle and right
 *                                         (two bars each), T1 the DRUM 808 kit alone at the bank's tempo */
#include "../platform/core/core_unit.c"

#include <stdio.h>
#include <stdlib.h>

#if CORE_TRACKS != 8 || !TESS_ATLAS_BANKS
#error "abank_test.c: build with -DCORE_TRACKS=8"
#endif

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL "); } else printf("ok   "); printf(__VA_ARGS__); printf("\n"); } while (0)
static uint32_t has_part(const uint8_t *rec, uint32_t len, uint32_t id)
{
    uint32_t pos = TSP_HEAD, n = rec[8] | (uint32_t)rec[9] << 8, i;
    for (i = 0; i < n && pos + 4u <= len; i++) {
        if (rec[pos] == id) return 1;
        pos += 4u + (rec[pos + 2u] | (uint32_t)rec[pos + 3u] << 8);
    }
    return 0;
}
/* the 16 steps' hits of part p at bank, x, y (DENS 100, every part's whole share) as a mask */
static uint32_t hits(uint32_t bank, uint32_t x, uint32_t y, uint32_t p)
{
    gen_grids_t g;
    uint32_t m = 0;
    memset(&g, 0, sizeof g);
    g.bank = (uint8_t)bank, g.x = (uint8_t)x, g.y = (uint8_t)y, g.dens = 100;
    for (uint32_t s = 0; s < 16u; s++) m |= (uint32_t)gen_grids_hit(&g, s, 16u, p, 0, 0) << s;
    return m;
}

static const uint8_t BPM[GEN_AB_N] = {120, 72, 78, 128, 90, 172, 124, 70, 130, 120, 132, 140, 92, 118};

static void wav_out(const char *path, const int16_t *pcm, uint32_t frames)
{
    FILE *f = fopen(path, "wb");
    uint32_t bytes = frames * 4u, rate = FS, brate = FS * 4u;
    if (!f) return;
    fwrite("RIFF", 1, 4, f); bytes += 36u; fwrite(&bytes, 4, 1, f); bytes -= 36u;
    fwrite("WAVEfmt ", 1, 8, f);
    fwrite(&(uint32_t){16}, 4, 1, f); fwrite(&(uint16_t){1}, 2, 1, f); fwrite(&(uint16_t){2}, 2, 1, f);
    fwrite(&rate, 4, 1, f); fwrite(&brate, 4, 1, f); fwrite(&(uint16_t){4}, 2, 1, f); fwrite(&(uint16_t){16}, 2, 1, f);
    fwrite("data", 1, 4, f); fwrite(&bytes, 4, 1, f);
    fwrite(pcm, 4, frames, f);
    fclose(f);
}

static int listen(uint32_t bank, const char *path)
{
    static const uint8_t XS[3] = {0, 128, 255};
    uint32_t frames = 0, cap = FS * 600u, bar = 0xFFFFFFFFu, seg = 0;
    int16_t *pcm = malloc(cap * 4u);
    float l[256], r[256];
    if (!pcm || bank >= GEN_AB_N) return 1;
    tess_core_boot();
    tess_core_project_new();
    for (uint32_t k = 0; k < NTRK; k++) trk[k].p[P_MUTE] = 1;
    engine_load_of(&trk[0], ENGI_DRUM, 0);              /* (the 808 kit) */
    trk[0].engine = trk[0].eng_req;
    trk[0].p[P_MUTE] = 0; trk[0].p[P_SDIV] = 2;         /* (16ths) */
    trk[0].p[P_FXSEND] = 10; trk[0].p[P_FXSEND2] = bank == GEN_AB_DUB ? 25 : 0;
    song.g[G_BPM] = BPM[bank];
    tessera_param_set(0, TX_MODE, TESS_ENG_GRIDS);
    tessera_param_set(0, TX_GRD_BANK, (int32_t)bank);
    tessera_param_set(0, TX_GRD_DENS, 60);
    tessera_param_set(0, TX_EV_LOCK, 100);              /* (CHAOS 0) */
    tess_core_transport(1);
    while (seg < 15u && frames + 256u < cap) {
        if (bar_n != bar) {
            bar = bar_n;
            if (bar && bar % 2u == 0u) seg++;
            if (seg >= 15u) break;
            tessera_param_set(0, TX_GRD_Y, (int32_t)((seg / 3u) * 255u / 4u));
            tessera_param_set(0, TX_GRD_X, XS[seg % 3u]);
        }
        tess_core_render(l, r, 0, 256);
        for (uint32_t i = 0; i < 256u; i++, frames++) {
            float a = l[i] * 32767.0f, b = r[i] * 32767.0f;
            pcm[frames * 2u] = (int16_t)(a > 32767.0f ? 32767 : a < -32768.0f ? -32768 : a);
            pcm[frames * 2u + 1u] = (int16_t)(b > 32767.0f ? 32767 : b < -32768.0f ? -32768 : b);
        }
    }
    tess_core_transport(0);
    wav_out(path, pcm, frames);
    printf("abank_test: %s, %u s at %u BPM -> %s\n", TX_N_GRBANK[bank], frames / FS, BPM[bank], path);
    free(pcm);
    return 0;
}

int main(int argc, char **argv)
{
    static uint8_t rec[TSP_MAX + 64], cp[TSP_MAX];
    uint32_t len, b, c, x, y, p, same = 0, grow = 0, cells = 0;
    FILE *f;
    if (argc > 4 && !strcmp(argv[1], "wav")) return listen((uint32_t)atoi(argv[2]), argv[4]);

    for (b = 1; b < GEN_AB_N; b++)                     /* (each bank its own map, none Grids') */
        for (c = 0; c < b; c++) {
            uint32_t diff = 0;
            for (x = 0; x < 256u; x += 64u)
                for (y = 0; y < 256u; y += 64u)
                    for (p = 0; p < 3u; p++) diff |= hits(b, x, y, p) != hits(c, x, y, p);
            same += !diff;
        }
    CHECK(!same, "every bank a map of its own (%u pairs the same)", same);
    for (b = 1; b < GEN_AB_N; b++)                     /* (X: the left's hits are the right's too) */
        for (y = 0; y < 5u; y++)
            for (p = 0; p < 3u; p++, cells++) {
                uint32_t lo = hits(b, 0, y * 64u, p), hi = hits(b, 255, y * 64u, p);
                grow += (lo & ~hi) == 0;
            }
    CHECK(grow == cells, "X goes from skeleton to fullest (%u of %u lanes)", grow, cells);
    CHECK(hits(GEN_AB_REGGAE, 0, 0, 0) == 1u << 8 && hits(GEN_AB_REGGAE, 0, 0, 1) == 1u << 8,
          "REGGAE's one drop: kick and rim on beat 3 alone at the left");
    CHECK(hits(GEN_AB_TECHNO, 0, 0, 0) == 0x1111u, "TECHNO's first feel: four on the floor");
    {
        gen_grids_t g;
        memset(&g, 0, sizeof g);
        g.bank = GEN_AB_REGGAE;
        CHECK(gen_grids_note(&g, 1) == 37u, "REGGAE's snare lane plays the rim");
        g.bank = GEN_AB_HOUSE;
        CHECK(gen_grids_note(&g, 1) == 39u, "HOUSE's the clap");
        g.bank = GEN_AB_ALL;
        CHECK(gen_grids_note(&g, 1) == 38u && gen_grids_note(&g, 0) == 36u, "ALL as before");
    }
    {
        gen_grids_t g;
        uint32_t d = 0;
        memset(&g, 0, sizeof g);
        for (x = 0; x < 256u; x += 37u)
            for (y = 0; y < 256u; y += 37u)
                for (p = 0; p < 3u; p++)
                    for (c = 0; c < 32u; c++) {
                        uint32_t i = x >> 6, j = y >> 6, fx = (x << 2) & 0xFFu, fy = (y << 2) & 0xFFu, o = p * 32u + c;
                        uint32_t e = gen_grids_mix(gen_grids_mix(GEN_GRIDS_NODE[i * 5u + j][o], GEN_GRIDS_NODE[(i + 1u) * 5u + j][o], fx),
                                                   gen_grids_mix(GEN_GRIDS_NODE[i * 5u + j + 1u][o], GEN_GRIDS_NODE[(i + 1u) * 5u + j + 1u][o], fx), fy);
                        g.x = (uint8_t)x, g.y = (uint8_t)y;
                        d += gen_grids_level(&g, c, p) != e;
                    }
        CHECK(!d, "BANK ALL reads Grids' map unchanged");
    }
    CHECK(pid_find(PID_TESS(TX_GRD_BANK)) != 0, "ATLAS BANK has its id (0x%x), range and help", PID_TESS(TX_GRD_BANK));

    tess_core_boot();
    tess_core_project_new();
    tessera_param_set(0, TX_MODE, TESS_ENG_GRIDS);
    len = tess_core_save(rec, sizeof rec);
    CHECK(len && !has_part(rec, len, TSP_ABANK), "every BANK ALL: no bank part saved");
    tessera_param_set(0, TX_GRD_BANK, GEN_AB_DUB);
    tessera_param_set(5, TX_GRD_BANK, GEN_AB_AFRO);
    CHECK(tessera_param_get(0, TX_GRD_BANK) == GEN_AB_DUB, "BANK set and read");
    tessera_param_set(1, TX_GRD_BANK, 99);
    CHECK(tessera_param_get(1, TX_GRD_BANK) == GEN_AB_N - 1, "BANK kept in range");
    tessera_param_set(1, TX_GRD_BANK, 0);
    len = tess_core_save(rec, sizeof rec);
    CHECK(len && has_part(rec, len, TSP_ABANK), "the banks saved (part %u)", (unsigned)TSP_ABANK);
    for (c = 0; c < NTRK; c++) tessera.grids[c].bank = 0;
    CHECK(!tess_core_load(rec, len) && tessera.grids[0].bank == GEN_AB_DUB && tessera.grids[5].bank == GEN_AB_AFRO &&
          !tessera.grids[1].bank, "..and load the same");
    c = tess_core_track_copy(0, cp, sizeof cp);
    CHECK(c && !tess_core_track_paste(3, TESS_PASTE_TRACK, cp, c) && tessera.grids[3].bank == GEN_AB_DUB,
          "a track pasted takes its BANK");
    CHECK(!tess_core_track_swap(3, 5) && tessera.grids[3].bank == GEN_AB_AFRO && tessera.grids[5].bank == GEN_AB_DUB,
          "..and a swap moves it");
    len = tess_core_save(rec, sizeof rec);
    if (argc > 1 && (f = fopen(argv[1], "wb"))) fwrite(rec, 1, len, f), fclose(f);
    printf(fails ? "abank_test: %d FAILED\n" : "abank_test: ok\n", fails);
    return fails != 0;
}
