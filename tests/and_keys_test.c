/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Andamento's keys and their lights (platform/andamento/and_keys.c) on the PC, through the virtual prototype's own
 * build (web/andamento/sim.c, at 48 kHz) driven as the page drives it.
 *   and_keys_test        (run by tests/run_tests.sh; exit 1 on a failed check)
 * Checks:
 *   modes     FUNC + key 9..12 picks SCALE, CHROM, STEP, MUTE; the lights show the choice while FUNC is held
 *   SCALE     the bottom-left key plays the root at C4, the next keys the scale's degrees; FUNC + UP an octave up
 *   CHROM     the bottom row C D E F G A B C, the top row their sharps; the keys above E and B silent and dark
 *   STEP      a key sets its step with the track's last note, again clears it; the step playing lit white
 *   MUTE      a top-row key mutes its track (lit red) and again unmutes it; a bottom-row key selects the track
 *   ENERGY    turning it shows its level across the keys
 *   activity  a demo playing: its generators' notes flash keys of the selected track (the note feed) */
#include "../web/andamento/sim.c"

#include <stdio.h>

static uint32_t nfail;
#define CHECK(c, ...) do { if (!(c)) { nfail++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

static void run_ms(uint32_t ms) { while (ms--) web_ms(); }
static void tap(uint32_t k) { web_key(k, 1); run_ms(30); web_key(k, 0); run_ms(30); }
static void func_key(uint32_t k) { web_button(AB_FUNC, 1); web_key(k, 1); web_key(k, 0); web_button(AB_FUNC, 0); }
static uint32_t lum(uint32_t k) { const uint8_t *c = web_led_keys() + 3u * k; return (uint32_t)c[0] + c[1] + c[2]; }

/* the notes the track sounds now (gated voices), as a bit mask over notes 0..127 */
static int sounding(uint32_t tr, uint32_t note)
{
    uint32_t v;
    for (v = 0; v < NVOICE; v++)
        if (trk[tr].v[v].gate && trk[tr].v[v].note == note)
            return 1;
    return 0;
}
static int held_sounds(uint32_t k, uint32_t note)
{
    int r;
    web_key(k, 1);
    run_ms(20);
    r = sounding(song.sel, note);
    web_key(k, 0);
    run_ms(40);
    return r;
}

int main(void)
{
    uint32_t k, i;
    track_t *t;
    web_boot();
    song.sel = 0;
    t = TSEL;
    run_ms(100);

    /* the modes, and the lights while FUNC is held */
    func_key(9);
    CHECK(and_keys.mode == AK_CHROM, "FUNC + key 10: CHROM (mode %u)", and_keys.mode);
    web_button(AB_FUNC, 1);
    CHECK(lum(9) > 3u * lum(8) && lum(10) > 0 && lum(12) == 0, "FUNC held: the mode keys, CHROM brightest");
    CHECK(lum(0) > lum(1) && lum(1) > 0 && lum(NTRK) == 0, "FUNC held: the tracks, the selected brightest");
    web_button(AB_FUNC, 0);
    func_key(8);
    CHECK(and_keys.mode == AK_SCALE, "FUNC + key 9: SCALE");

    /* SCALE: degrees from the root at C4 */
    if (drum_track(t)) {
        printf("track 1 is a drum track: SCALE and CHROM not checked\n");
    } else {
        uint32_t root = scale_degree_map(t, 0, 0, 1), second = scale_degree_map(t, 1, 0, 1);
        CHECK(held_sounds(0, root), "SCALE: key 1 plays the root %u", root);
        CHECK(held_sounds(1, second), "SCALE: key 2 plays the second degree %u", second);
        CHECK(held_sounds(8, scale_degree_map(t, 8, 0, 1)), "SCALE: key 9 plays degree 9");
        web_button(AB_FUNC, 1);
        web_button(AB_UP, 1);
        web_button(AB_UP, 0);
        web_button(AB_FUNC, 0);
        CHECK(song.octave == 1, "FUNC + UP: an octave up (%d)", song.octave);
        CHECK(held_sounds(0, root + 12u), "SCALE: an octave up, key 1 plays %u", root + 12u);
        song.octave = 0;
        and_keys_lights();
        CHECK(lum(0) > lum(1) && lum(1) > 0, "SCALE: the root lit brighter than the other degrees");

        /* CHROM (CHRD off: a chord would snap its notes to the scale) */
        t->p[P_CHRD] = 0;
        func_key(9);
        {
            static const uint8_t SEMI[16] = {0, 2, 4, 5, 7, 9, 11, 12, 1, 3, 0, 6, 8, 10, 0, 13};
            for (k = 0; k < 16u; k++) {
                if (k == 10u || k == 14u)
                    continue;
                CHECK(held_sounds(k, 60u + SEMI[k] + (uint32_t)t->p[P_TRANS]), "CHROM: key %u plays C4 + %u", k + 1u, SEMI[k]);
            }
            web_key(10, 1);
            run_ms(20);
            for (i = 0; i < 128u && !sounding(song.sel, i); i++)
                ;
            web_key(10, 0);
            run_ms(40);
            CHECK(i == 128u, "CHROM: key 11 (above E) silent");
            and_keys_lights();
            CHECK(lum(10) == 0 && lum(14) == 0, "CHROM: the keys above E and B dark");
        }
    }

    /* STEP: set, clear, the playhead */
    func_key(10);
    CHECK(and_keys.mode == AK_STEP, "FUNC + key 11: STEP");
    tessera_mode_set(song.sel, TESS_ENG_OFF);
    for (i = 0; i < NSTEP; i++) {
        memset(&t->step[i], 0, sizeof t->step[i]);
        t->step[i].time = ST_REST;
    }
    t->p[P_SLEN] = 16;
    tap(4);
    CHECK(ak_step_on(&t->step[4]), "STEP: key 5 sets step 5");
    CHECK(drum_track(t) || t->step[4].note[0] == and_keys.step_note[song.sel] || !and_keys.step_note[song.sel],
          "STEP: the step takes the track's last note");
    and_keys_lights();
    CHECK(lum(4) > lum(5), "STEP: step 5 lit");
    tap(4);
    CHECK(!ak_step_on(&t->step[4]), "STEP: key 5 again clears it");
    tap(0);
    web_button(AB_PLAY, 1);
    web_button(AB_PLAY, 0);
    run_ms(300);
    {
        const uint8_t *c;
        and_keys_lights();
        c = web_led_keys() + 3u * (t->seq_idx % 16u);
        CHECK(song.playing && c[0] > 100 && c[1] > 100 && c[2] > 100, "STEP: the step playing (%u) lit white", t->seq_idx);
    }
    web_button(AB_STOP, 1);
    web_button(AB_STOP, 0);
    run_ms(50);

    /* MUTE */
    func_key(11);
    CHECK(and_keys.mode == AK_MUTE, "FUNC + key 12: MUTE");
    tap(9);
    CHECK(pid_get(0x028, 1) == 1, "MUTE: key 10 mutes track 2");
    and_keys_lights();
    {
        const uint8_t *c = web_led_keys() + 3u * 9u;
        CHECK(c[0] > 0 && !c[1] && !c[2], "MUTE: track 2 lit red");
    }
    tap(9);
    CHECK(pid_get(0x028, 1) == 0, "MUTE: key 10 again unmutes it");
    tap(2);
    CHECK(song.sel == 2, "MUTE: key 3 selects track 3");
    song.sel = 0;

    /* ENERGY across the keys */
    pid_set(0x26c, 0, 0);
    web_enc(8, 25);                                  /* (+50) */
    and_keys_lights();
    CHECK(lum(0) > 100 && lum(15) < 30, "ENERGY: its level across the keys (%d)", pid_get(0x26c, 0));

    /* activity: the demo's generators flash the keys */
    func_key(8);
    web_demo(0);
    song.sel = 1;                                    /* (STILL TIDE's AWAKE line; track 1 is a low chord pad) */
    {
        uint32_t seen = 0;
        for (i = 0; i < 400u; i++) {
            run_ms(25);
            and_keys_lights();
            for (k = 0; k < 16u; k++)
                if (and_keys.flash_ms[k] && fm1_ms - and_keys.flash_ms[k] < 30u)
                    seen |= 1u << k;
        }
        CHECK(seen != 0, "activity: the demo's notes flash keys of track 2");
        printf("activity: keys flashed %04x in 10 s of STILL TIDE, track 2\n", seen);
    }

    printf(nfail ? "and_keys: %u failed\n" : "and_keys: all checks passed\n", nfail);
    return nfail ? 1 : 0;
}
