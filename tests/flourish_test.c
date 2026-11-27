/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* FLOURISH (seq_sub.c fl_step, an 8-track build's; Tessadub in Cosmati, step 5). On a demo, the busiest track is played
 * 16 bars without a FLOURISH, then again with each move, every note it starts logged with its step (CORE_OUT_HOOKS).
 * The piece replays the same from PLAY, so the two runs differ only on the flourish steps: the last step of every 2nd
 * bar (HEAT 1), or the last two (HEAT 2). DROP: none there; DOUBLE: twice as many there; OCTAVE: each an octave up
 * (BASS: down); ECHO: the track thrown there and only there. Off the flourish steps: the same notes. Saved and read back;
 * a 4-track build's record (the part left out) reads as none.
 *   flourish_test DEMO.tsp      exit 0: passed */
#define CORE_OUT_HOOKS 1
#include "../platform/core/core_unit.c"

#include <stdio.h>

#if CORE_TRACKS != 8 || !TESS_FLOURISH
#error "flourish_test.c: build with -DCORE_TRACKS=8"
#endif

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL "); } else printf("ok   "); printf(__VA_ARGS__); printf("\n"); } while (0)
static float l[256], r[256];

#define MAXN 4096
static struct { uint32_t step; uint8_t note; } logn[MAXN];
static uint32_t nlog, who, thrown_on, thrown_off;
static void hook(const track_t *t, uint32_t note, uint32_t vel)
{
    if ((uint32_t)(t - trk) != who || !vel || nlog >= MAXN) return;
    logn[nlog].step = seq_run_step[who];
    logn[nlog].note = (uint8_t)note;
    nlog++;
}
static int fl_spot(uint32_t step, uint32_t spb, uint32_t n)   /* a flourish step: the last n of every 2nd bar */
{
    return (step / spb) % 2u == 1u && step % spb >= spb - n;
}
/* the track's notes over 16 bars from PLAY, into log; thrown counts on / off the flourish steps */
static const uint8_t *proj;
static uint32_t projn;
static uint8_t fl_set[3];                              /* (WHERE, WHAT, HEAT for the next play) */
static uint32_t play(uint32_t spb, uint32_t n)
{
    uint32_t i;
    tess_core_transport(0);
    for (i = 0; i < 4u; i++) tess_core_render(l, r, 0, 256);
    tess_core_load(proj, projn);                       /* (from the same start every time: EVOLVE and the rest moved on) */
    for (i = 0; i < 4u; i++) tess_core_render(l, r, 0, 256);
    tessera.fl_where[who] = fl_set[0], tessera.fl_what[who] = fl_set[1], tessera.fl_heat[who] = fl_set[2];
    nlog = thrown_on = thrown_off = 0;
    tess_core_transport(1);
    for (i = 0; i < 64u && !song.playing; i++) {       /* (PLAY taken in) */
        tess_core_render(l, r, 0, 256);
        if (!song.playing && i == 8u) tess_core_transport(1);
    }
    for (i = 0; i < 200000u && (seq_run_step[who] == 0xFFFFFFFFu || seq_run_step[who] < 16u * spb); i++) {
        tess_core_render(l, r, 0, 256);
        if (seq_run_step[who] != 0xFFFFFFFFu && (fl_throw >> who & 1u)) {
            if (fl_spot(seq_run_step[who], spb, n)) thrown_on++;
            else thrown_off++;
        }
    }
    tess_core_transport(0);
    for (i = 0; i < 4u; i++) tess_core_render(l, r, 0, 256);
    return nlog;
}

int main(int argc, char **argv)
{
    static uint8_t buf[TSP_MAX + 64], rec[TSP_MAX + 64];
    static struct { uint32_t step; uint8_t note; } base[MAXN];
    FILE *f = argc > 1 ? fopen(argv[1], "rb") : 0;
    size_t n = f ? fread(buf, 1, sizeof buf, f) : 0;
    uint32_t nb, i, k, spb, best = 0, most = 0, len, drums;
    if (f) fclose(f);
    tess_core_boot();
    if (!n || tess_core_load(buf, (uint32_t)n)) {
        printf("flourish_test: %s did not load\n", argc > 1 ? argv[1] : "(no file)");
        return 1;
    }
    out_note_hook = hook;
    proj = buf, projn = (uint32_t)n;
    for (who = 0; who < 4u; who++) {                   /* (the busiest of tracks 1..4) */
        uint32_t c = play(tess_steps_per_bar(&trk[who]) ? tess_steps_per_bar(&trk[who]) : 16u, 1);
        if (c > most) most = c, best = who;
    }
    who = best;
    spb = tess_steps_per_bar(&trk[who]);
    nb = play(spb, 1);
    memcpy(base, logn, sizeof base);
    drums = drum_track(&trk[who]) || tessera.mode[who] == TESS_ENG_GRIDS || tessera_role(who) == ET_RHYTHM;
    CHECK(nb > 16u && spb, "T%u: %u notes in 16 bars, %u steps a bar%s", who + 1u, nb, spb, drums ? ", drums" : "");
    CHECK(play(spb, 1) == nb && !memcmp(base, logn, nb * sizeof base[0]), "  replays the same from PLAY");

    for (uint32_t heat = 0; heat < 2u; heat++) {
        uint32_t m = heat ? 2u : 1u, on0 = 0, off_same = 1, on1 = 0, j;
        for (i = 0; i < nb; i++) on0 += fl_spot(base[i].step, spb, m);
        fl_set[0] = 1, fl_set[2] = (uint8_t)heat;

        fl_set[1] = FL_DROP;
        play(spb, m);
        for (i = on1 = 0; i < nlog; i++) on1 += fl_spot(logn[i].step, spb, m);
        CHECK(on0 && !on1 && nlog == nb - on0, "HEAT %u DROP: none of the %u notes on the flourish steps, the rest the same "
              "(%u of %u)", heat + 1u, on0, nlog, nb - on0);

        fl_set[1] = FL_DOUBLE;
        play(spb, m);
        for (i = on1 = 0; i < nlog; i++) on1 += fl_spot(logn[i].step, spb, m);
        for (i = j = 0; i < nb; i++)                   /* (off the spots: the same notes, in order) */
            if (!fl_spot(base[i].step, spb, m)) {
                while (j < nlog && fl_spot(logn[j].step, spb, m)) j++;
                if (j >= nlog || logn[j].note != base[i].note || logn[j].step != base[i].step) off_same = 0;
                j++;
            }
        CHECK(on1 >= 2u * on0 && off_same, "HEAT %u DOUBLE: %u notes there (%u before), the rest the same", heat + 1u, on1,
              on0);

        fl_set[1] = FL_OCTAVE;
        play(spb, m);
        if (drums) {                                   /* (drums: a roll, as DOUBLE) */
            for (i = on1 = 0; i < nlog; i++) on1 += fl_spot(logn[i].step, spb, m);
            CHECK(on1 >= 2u * on0, "HEAT %u OCTAVE on drums: a roll there (%u notes, %u before)", heat + 1u, on1, on0);
        } else {
            int up = tessera_role(who) == ET_BASS ? -12 : 12, ok = nlog == nb;
            for (i = 0; ok && i < nb; i++)
                ok = logn[i].note == base[i].note + (fl_spot(base[i].step, spb, m) ? up : 0);
            CHECK(ok, "HEAT %u OCTAVE: the flourish steps' notes %+d, the rest the same", heat + 1u, up);
        }

        fl_set[1] = FL_ECHO;
        play(spb, m);
        CHECK(thrown_on && !thrown_off && nlog == nb, "HEAT %u ECHO: thrown on the flourish steps only (%u blocks on, %u "
              "off), the notes the same", heat + 1u, thrown_on, thrown_off);
    }
    fl_set[0] = 0;
    play(spb, 1);
    CHECK(nlog == nb && !memcmp(base, logn, nb * sizeof base[0]), "WHERE OFF: as before");
    for (k = 0; k < 4u; k++)                           /* (OCTAVE on a track that is not drums: up, BASS down) */
        if (k != who && !drum_track(&trk[k]) && tessera.mode[k] != TESS_ENG_GRIDS && tessera_role(k) != ET_RHYTHM) {
            uint32_t sp2, o0, ok = 1, up;
            who = k;
            sp2 = tess_steps_per_bar(&trk[k]);
            fl_set[0] = 0;
            o0 = play(sp2, 2);
            memcpy(base, logn, sizeof base);
            fl_set[0] = 1, fl_set[1] = FL_OCTAVE, fl_set[2] = 1;
            play(sp2, 2);
            up = tessera_role(k) == ET_BASS ? (uint32_t)-12 : 12u;
            for (i = 0, ok = nlog == o0; ok && i < o0; i++)
                ok = logn[i].note == (uint8_t)(base[i].note + (fl_spot(base[i].step, sp2, 2) ? up : 0u));
            CHECK(ok && o0, "T%u (not drums) OCTAVE, HEAT 2: its flourish steps' notes %+d, the rest the same (%u notes)",
                  k + 1u, (int)up, o0);
            fl_set[0] = 0;
            who = best;
            break;
        }

    /* saved and read back; none: OFF */
    tessera.fl_where[who] = 3, tessera.fl_what[who] = FL_OCTAVE, tessera.fl_heat[who] = 2;
    len = tess_core_save(rec, sizeof rec);
    tessera.fl_where[who] = tessera.fl_what[who] = tessera.fl_heat[who] = 0;
    CHECK(len && !tess_core_load(rec, len) && tessera.fl_where[who] == 3u && tessera.fl_what[who] == FL_OCTAVE &&
          tessera.fl_heat[who] == 2u, "saved and read back: BAR 8, OCTAVE, HEAT 3");
    for (k = 0; k < TESSERA_TRACKS; k++) tessera.fl_where[k] = 0;
    len = tess_core_save(rec, sizeof rec);
    tessera.fl_where[1] = 2;
    CHECK(len && !tess_core_load(rec, len) && !tessera.fl_where[1], "a record with none: OFF everywhere");
    CHECK(!tess_core_load(buf, (uint32_t)n) && !tessera.fl_where[who], "the demo (no part): OFF");

    printf("flourish test: %s\n", fails ? "FAILED" : "ok");
    return fails != 0;
}
