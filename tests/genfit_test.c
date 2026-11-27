/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* A generator's LANE and RANGE (tessera.c tess_fit_*, an 8-track build's GEN sheet). T1, a DRUM track playing TURING
 * at SPAN 3 OCT, PROB 0 (a loop), every note it starts logged (CORE_OUT_HOOKS): with LANE ALL its notes pick several
 * drums; with LANE SD every note is the snare's (38), on the same steps. On a melodic track (ORACLE) the same TURING:
 * RANGE 1 OCT keeps every note within 12 semitones of the lowest, each the same pitch class as before (folded by
 * octaves, in key), on the same steps; RANGE OFF as before. LANE on a melodic track: no effect. Saved and read back; a
 * record without the part reads ALL, OFF.
 *   genfit_test      exit 0: passed */
#define CORE_OUT_HOOKS 1
#include "../platform/core/core_unit.c"

#include <stdio.h>

#if CORE_TRACKS != 8 || !TESS_GEN_FIT
#error "genfit_test.c: build with -DCORE_TRACKS=8"
#endif

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL "); } else printf("ok   "); printf(__VA_ARGS__); printf("\n"); } while (0)
static float l[256], r[256];

#define MAXN 1024
static struct { uint32_t step; uint8_t note; } logn[MAXN];
static uint32_t nlog;
static void hook(const track_t *t, uint32_t note, uint32_t vel)
{
    if (t != &trk[0] || !vel || nlog >= MAXN) return;
    logn[nlog].step = seq_run_step[0];
    logn[nlog].note = (uint8_t)note;
    nlog++;
}
/* T1 set up afresh (engine, TURING at a fixed loop), LANE and RANGE as given, played 8 bars from PLAY */
static uint32_t play(uint32_t eng, uint32_t lane, uint32_t range)
{
    uint32_t i, k;
    tess_core_transport(0);
    for (i = 0; i < 4u; i++) tess_core_render(l, r, 0, 256);
    tess_core_project_new();
    for (k = 0; k < NTRK; k++) trk[k].p[P_MUTE] = 1;
    engine_load_of(&trk[0], eng, 0);
    trk[0].engine = trk[0].eng_req;
    trk[0].p[P_MUTE] = 0; trk[0].p[P_SDIV] = 2;         /* (16ths) */
    tessera_param_set(0, TX_MODE, TESS_ENG_TURING);
    tessera.turing[0].reg = 0xB5A3u;                    /* (a fixed register, PROB 0: the same loop every time) */
    tessera.turing[0].len = 16, tessera.turing[0].prob = 0, tessera.turing[0].oct_span = 3;
    tessera_param_set(0, TX_GN_LANE, (int32_t)lane);
    tessera_param_set(0, TX_GN_RANGE, (int32_t)range);
    for (i = 0; i < 4u; i++) tess_core_render(l, r, 0, 256);
    nlog = 0;
    tess_core_transport(1);
    for (i = 0; i < 64u && !song.playing; i++) tess_core_render(l, r, 0, 256);
    for (i = 0; i < 200000u && (seq_run_step[0] == 0xFFFFFFFFu || seq_run_step[0] < 8u * 16u); i++)
        tess_core_render(l, r, 0, 256);
    tess_core_transport(0);
    for (i = 0; i < 4u; i++) tess_core_render(l, r, 0, 256);
    return nlog;
}

int main(void)
{
    static uint8_t rec[TSP_MAX + 64];
    static struct { uint32_t step; uint8_t note; } base[MAXN];
    uint32_t nb, i, kinds = 0, lo = 127, hi = 0, len, ok;
    tess_core_boot();
    out_note_hook = hook;

    /* a DRUM track: LANE */
    nb = play(ENGI_DRUM, 0, 0);
    memcpy(base, logn, sizeof base);
    for (i = 0; i < nb; i++) kinds |= 1u << (drum_lane(base[i].note) & 31u);
    CHECK(nb > 16u && __builtin_popcount(kinds) >= 2, "DRUM, LANE ALL: %u notes over %d drums", nb,
          __builtin_popcount(kinds));
    CHECK(play(ENGI_DRUM, 0, 0) == nb && !memcmp(base, logn, nb * sizeof base[0]), "  replays the same from PLAY");
    play(ENGI_DRUM, 2, 0);                              /* (SD) */
    for (i = 0, ok = nlog == nb; ok && i < nb; i++) ok = logn[i].note == 38u && logn[i].step == base[i].step;
    CHECK(ok, "DRUM, LANE SD: every note the snare's (38), on the same steps (%u notes)", nlog);
    play(ENGI_DRUM, 1, 0);                              /* (BD) */
    for (i = 0, ok = nlog == nb; ok && i < nb; i++) ok = logn[i].note == 36u;
    CHECK(ok, "DRUM, LANE BD: every note the kick's (36)");

    /* a melodic track: RANGE */
    nb = play(ENGI_ORACLE, 0, 0);
    memcpy(base, logn, sizeof base);
    for (i = 0; i < nb; i++) lo = base[i].note < lo ? base[i].note : lo, hi = base[i].note > hi ? base[i].note : hi;
    CHECK(nb > 16u && hi - lo > 12u, "ORACLE, RANGE OFF: %u notes over %u semitones", nb, hi - lo);
    play(ENGI_ORACLE, 0, 1);
    lo = 127, hi = 0;
    for (i = 0, ok = nlog == nb; ok && i < nb; i++) {
        ok = logn[i].note % 12u == base[i].note % 12u && logn[i].step == base[i].step;
        lo = logn[i].note < lo ? logn[i].note : lo, hi = logn[i].note > hi ? logn[i].note : hi;
    }
    CHECK(ok && hi - lo < 12u, "ORACLE, RANGE 1 OCT: within %u semitones, each note's pitch class and step kept", hi - lo);
    play(ENGI_ORACLE, 0, 2);
    lo = 127, hi = 0;
    for (i = 0; i < nlog; i++) lo = logn[i].note < lo ? logn[i].note : lo, hi = logn[i].note > hi ? logn[i].note : hi;
    CHECK(nlog == nb && hi - lo < 24u, "ORACLE, RANGE 2 OCT: within %u semitones", hi - lo);
    play(ENGI_ORACLE, 3, 0);
    CHECK(nlog == nb && !memcmp(base, logn, nb * sizeof base[0]), "ORACLE, LANE CP: no effect on a melodic track");
    play(ENGI_ORACLE, 0, 0);
    CHECK(nlog == nb && !memcmp(base, logn, nb * sizeof base[0]), "RANGE OFF again: as before");

    /* saved and read back; none: ALL, OFF */
    tessera.gn_lane[5] = 7, tessera.gn_range[5] = 3;
    len = tess_core_save(rec, sizeof rec);
    tessera.gn_lane[5] = tessera.gn_range[5] = 0;
    CHECK(len && !tess_core_load(rec, len) && tessera.gn_lane[5] == 7u && tessera.gn_range[5] == 3u,
          "saved and read back: T6 LANE RS, RANGE 3 OCT");
    memset(tessera.gn_lane, 0, sizeof tessera.gn_lane);
    memset(tessera.gn_range, 0, sizeof tessera.gn_range);
    len = tess_core_save(rec, sizeof rec);
    tessera.gn_lane[2] = 4;
    CHECK(len && !tess_core_load(rec, len) && !tessera.gn_lane[2], "a record with none: ALL, OFF everywhere");
    {   /* a track copied with them, pasted onto another */
        static uint8_t tc[TSP_MAX + 64];
        uint32_t n;
        tessera.gn_lane[0] = 2, tessera.gn_range[0] = 1;
        n = tess_core_track_copy(0, tc, sizeof tc);
        CHECK(n && !tess_core_track_paste(3, TESS_PASTE_TRACK, tc, n) && tessera.gn_lane[3] == 2u && tessera.gn_range[3] == 1u,
              "a track copy carries them (T1 onto T4)");
    }

    printf("genfit test: %s\n", fails ? "FAILED" : "ok");
    return fails != 0;
}
