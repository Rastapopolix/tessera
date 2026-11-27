/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* CHORD MOVE BY NOTES (tessera.c tess_chord_move, an 8-track build's TESS_CH_NOTES; Cosmati). A strike's four notes
 * moved each by its own offset: in semitones as set; in scale degrees each one in key and as many scale steps away;
 * a fifth note takes the fourth's; 0 0 0 0 leaves the chord as it is. Played: with WHEN LAST only the bar's last
 * strike moves. Saved and read back (TSP_CHOFS; the CHMOVE part keeps +1 ST in its place, which the FM-1 plays), a
 * record without the part: none; a track copy carries them.
 *   chordofs_test [OUT.tsp]      exit 0: passed; OUT.tsp: a record with NOTES (core_tracks.sh loads it at 4 tracks) */
#define CORE_OUT_HOOKS 1
#include "../platform/core/core_unit.c"

#include <stdio.h>

#if CORE_TRACKS != 8 || !TESS_CH_NOTES
#error "chordofs_test.c: build with -DCORE_TRACKS=8"
#endif

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL "); } else printf("ok   "); printf(__VA_ARGS__); printf("\n"); } while (0)
static float l[256], r[256];
static void blocks(uint32_t n) { for (uint32_t i = 0; i < n; i++) tess_core_render(l, r, 0, 256); }
static void ofs(int a, int b, int c, int d, int deg)
{
    tessera_param_set(0, TX_CH_OFS1, a); tessera_param_set(0, TX_CH_OFS2, b);
    tessera_param_set(0, TX_CH_OFS3, c); tessera_param_set(0, TX_CH_OFS4, d);
    tessera_param_set(0, TX_CH_ODEG, deg);
}
/* the strikes of 2 bars: each strike's lowest note and its step */
#define MAXN 512
static struct { uint32_t step; uint8_t note; } logn[MAXN];
static uint32_t nlog;
static void hook(const track_t *t, uint32_t note, uint32_t vel)
{
    if (t != &trk[0] || !vel || nlog >= MAXN) return;
    logn[nlog].step = seq_run_step[0], logn[nlog].note = (uint8_t)note;
    nlog++;
}

int main(int argc, char **argv)
{
    static uint8_t rec[TSP_MAX + 64], tc[TSP_MAX + 64];
    const track_t *t = &trk[0];
    uint8_t out[CHORD_MAX], src[4] = {48, 52, 55, 59};
    uint32_t n, len, i;
    int ok;
    tess_core_boot();
    tess_core_project_new();
    tessera_param_set(0, TX_MODE, TESS_ENG_CHORD);
    blocks(4);

    CHECK(TESS_CB_NOTES == 12u && str_eq(pid_desc(PID_TESS(TX_CH_BY), 0)->names[TESS_CB_NOTES], "NOTES"),
          "MOVE BY's last value: NOTES");
    tessera_param_set(0, TX_CH_BY, TESS_CB_NOTES);
    ofs(0, 0, 2, -1, 0);
    memcpy(out, src, 4);
    n = tess_chord_move(t, 0, &tessera.chord[0], out, 4);
    CHECK(n == 4u && out[0] == 48 && out[1] == 52 && out[2] == 57 && out[3] == 58,
          "ST 0 0 +2 -1: 48 52 57 58 (%u %u %u %u)", out[0], out[1], out[2], out[3]);
    ofs(0, 0, 2, -1, 1);
    memcpy(out, src, 4);
    n = tess_chord_move(t, 0, &tessera.chord[0], out, 4);
    ok = out[0] == 48 && out[1] == 52 && out[2] == (uint8_t)tess_scale_steps(t, 55, 2) &&
         out[3] == (uint8_t)tess_scale_steps(t, 59, -1);
    for (i = 2; i < 4u; i++) ok &= tessera_snap_plain(t, out[i]) == out[i];
    CHECK(ok && out[2] > 55 && out[3] < 59, "DEG 0 0 +2 -1: the top two two degrees up and one down, in key (%u %u)",
          out[2], out[3]);
    ofs(0, 0, 0, 0, 0);
    memcpy(out, src, 4);
    tess_chord_move(t, 0, &tessera.chord[0], out, 4);
    CHECK(!memcmp(out, src, 4), "0 0 0 0: the chord as it is");
    {
        uint8_t five[5] = {48, 52, 55, 59, 62};
        ofs(1, 1, 1, 3, 0);
        tess_chord_move(t, 0, &tessera.chord[0], five, 5);
        CHECK(five[3] == 62 && five[4] == 65, "a fifth note takes the fourth's offset (+3: %u)", five[4]);
    }
    ofs(-12, 0, 0, 0, 0);
    out[0] = 5;
    tess_chord_move(t, 0, &tessera.chord[0], out, 1);
    CHECK(out[0] == 0, "kept to 0..127 (%u)", out[0]);

    /* played: WHEN LAST, the bar's last strike moved */
    {
        uint32_t base[MAXN], nb, moved = 0, others = 0, spb;
        out_note_hook = hook;
        tessera_param_set(0, TX_CH_WHEN, TESS_CW_OFF);
        tessera_param_set(0, TX_CH_HIT, GEN_CH_STEP);
        ofs(12, 12, 12, 12, 0);
        spb = tess_steps_per_bar(&trk[0]);
        nlog = 0;
        tess_core_transport(1);
        for (i = 0; i < 64u && !(song.playing && seq_run_step[0] < 2u); i++) blocks(1);   /* (from PLAY again) */
        nlog = 0;
        for (i = 0; i < 100000u && (seq_run_step[0] == 0xFFFFFFFFu || seq_run_step[0] < 2u * spb); i++) blocks(1);
        tess_core_transport(0);
        blocks(8);
        nb = nlog;
        for (i = 0; i < nb; i++) base[i] = logn[i].note | logn[i].step << 8;
        tessera_param_set(0, TX_CH_WHEN, TESS_CW_LAST);
        nlog = 0;
        tess_core_transport(1);
        for (i = 0; i < 64u && !(song.playing && seq_run_step[0] < 2u); i++) blocks(1);   /* (from PLAY again) */
        nlog = 0;
        for (i = 0; i < 100000u && (seq_run_step[0] == 0xFFFFFFFFu || seq_run_step[0] < 2u * spb); i++) blocks(1);
        tess_core_transport(0);
        blocks(8);
        for (i = 0; i < nlog && i < nb; i++) {
            if ((base[i] & 255u) + 12u == logn[i].note) moved++;
            else if ((base[i] & 255u) == logn[i].note) others++;
        }
        CHECK(nb > 8u && nlog == nb && moved && others && moved + others == nb,
              "WHEN LAST, +12 each: %u notes an octave up on the last strikes, %u as before", moved, others);
        out_note_hook = 0;
    }

    /* saved and read back; the CHMOVE part keeps +1 ST; none: none */
    tessera_param_set(0, TX_CH_WHEN, TESS_CW_LAST);
    tessera_param_set(0, TX_CH_BY, TESS_CB_NOTES);
    ofs(0, 4, -3, 7, 1);
    len = tess_core_save(rec, sizeof rec);
    if (argc > 1) {
        FILE *f = fopen(argv[1], "wb");
        if (f) fwrite(rec, 1, len, f), fclose(f);
    }
    tessera_param_set(0, TX_CH_BY, TESS_CB_UP2);
    ofs(0, 0, 0, 0, 0);
    CHECK(len && !tess_core_load(rec, len) && tessera.ch_by[0] == TESS_CB_NOTES && tessera.ch_odeg[0] == 1u &&
          tessera.ch_ofs[0][1] == 4 && tessera.ch_ofs[0][2] == -3 && tessera.ch_ofs[0][3] == 7,
          "saved and read back: NOTES, DEG, 0 4 -3 7");
    n = tess_core_track_copy(0, tc, sizeof tc);
    CHECK(n && !tess_core_track_paste(5, TESS_PASTE_TRACK, tc, n) && tessera.ch_by[5] == TESS_CB_NOTES &&
          tessera.ch_ofs[5][3] == 7 && tessera.ch_odeg[5] == 1u, "a track copy carries them (T1 onto T6)");
    len = tess_core_save(rec, sizeof rec);
    CHECK(len && !tess_core_load(rec, len) && tessera.ch_by[5] == TESS_CB_NOTES && tessera.ch_ofs[5][2] == -3,
          "T6 (kept in XTRK) saved and read back too");
    tessera_param_set(0, TX_CH_BY, TESS_CB_UP2);
    tessera_param_set(5, TX_CH_BY, TESS_CB_UP2);
    for (i = 0; i < TESSERA_TRACKS; i++) memset(tessera.ch_ofs[i], 0, 4), tessera.ch_odeg[i] = 0;
    len = tess_core_save(rec, sizeof rec);
    tessera.ch_ofs[2][0] = 5;
    CHECK(len && !tess_core_load(rec, len) && !tessera.ch_ofs[2][0] && tessera.ch_by[0] == TESS_CB_UP2,
          "a record with none: offsets 0, BY as set");

    printf("chordofs test: %s\n", fails ? "FAILED" : "ok");
    return fails != 0;
}
