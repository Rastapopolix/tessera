/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Andamento's home screen (platform/andamento/and_home.c) on the PC, through the virtual prototype's own build
 * (web/andamento/sim.c, at 48 kHz) driven as the page drives it.
 *   and_home_test [dir]  (run by tests/run_tests.sh; exit 1 on a failed check; with a dir, the frames as dir/ppm/home_*.ppm
 *                        in the panel's yellow, as tests/oled_render.c writes them, for tests/oled_render.py)
 * Checks:
 *   start     Andamento starts on HOME, no page button lit; a page button leaves it, NO comes back from any page
 *   DEPTH     encoders A..D turn tracks 1..4's ENERGY DEPTH, a push resets it; E..H the external tracks 5..8's
 *   lanes     8 lanes of 16 px, each with its meter; the selected one lit (5..8 too)
 *   FREE      a demo playing with ARC set: ENERGY's history grows from the left, now a disc at its leading edge;
 *             once full it scrolls, now at the right edge; a rest shows REST in the header and is marked under the arc
 *   piece     a LENGTH set: the SHAPE across the screen, what was played over it up to the play head, the time left */
#include "../web/andamento/sim.c"

#include <stdio.h>

static uint32_t nfail;
#define CHECK(c, ...) do { if (!(c)) { nfail++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

static const char *dir;
static void run_ms(uint32_t ms) { while (ms--) web_ms(); }
static void btn(uint32_t b) { web_button(b, 1); web_button(b, 0); }
static const uint8_t *scr(void) { return web_screen(); }
/* the brightest level in a box of the frame */
static uint32_t peak(int32_t x, int32_t y, int32_t w, int32_t h)
{
    const uint8_t *f = scr();
    uint32_t m = 0;
    int32_t i, j;
    for (j = y; j < y + h; j++)
        for (i = x; i < x + w; i++)
            if (f[j * AND_W + i] > m)
                m = f[j * AND_W + i];
    return m;
}
static uint32_t set_columns(void)
{
    uint32_t i, n = 0;
    for (i = 0; i < AH_X; i++)
        n += (and_home.f[i] & AH_SET) != 0;
    return n;
}
static void dump(const char *name)
{
    char p[256];
    FILE *f;
    const uint8_t *fb = scr();
    uint32_t i;
    if (!dir)
        return;
    snprintf(p, sizeof p, "%s/ppm/%s.ppm", dir, name);
    if (!(f = fopen(p, "wb")))
        return;
    fprintf(f, "P6\n%d %d\n255\n", AND_W, AND_H);
    for (i = 0; i < AND_W * AND_H; i++) {
        uint32_t l = fb[i] & 15u;
        fputc((int)(255u * l / 15u), f);
        fputc((int)(208u * l / 15u), f);
        fputc((int)(32u * l / 15u), f);
    }
    fclose(f);
}

int main(int argc, char **argv)
{
    uint32_t i;
    dir = argc > 1 ? argv[1] : 0;
    web_boot();
    run_ms(100);

    /* start on HOME; a page and back */
    CHECK(and_ui.page == AND_HOME, "starts on HOME (page %u)", and_ui.page);
    CHECK((web_led_buttons() & 0x3Fu) == 0, "HOME: no page button lit (%x)", web_led_buttons());
    dump("home_stopped");
    btn(AB_GEN);
    CHECK(and_ui.page == 0, "GEN opens GEN");
    btn(AB_PIECE);
    btn(AB_PIECE);
    btn(AB_NO);
    CHECK(and_ui.page == AND_HOME, "NO comes back to HOME");
    btn(AB_UP);
    CHECK(and_ui.page == AND_HOME, "UP on HOME stays on HOME");

    /* DEPTH on encoders A..H */
    pid_set(AND_PID_DEPTH, 0, 100);
    pid_set(AND_PID_DEPTH, 2, 100);
    web_enc(0, -10);
    run_ms(5);                                       /* (the header's moment begins at the next millisecond) */
    CHECK(pid_get(AND_PID_DEPTH, 0) == 90, "encoder A: track 1's DEPTH 90 (%d)", pid_get(AND_PID_DEPTH, 0));
    CHECK(pid_get(AND_PID_DEPTH, 2) == 100, "encoder A leaves track 3 (%d)", pid_get(AND_PID_DEPTH, 2));
    CHECK(and_ui.hot == 0 && and_ui.hot_pid == AND_PID_DEPTH, "the header shows track 1's DEPTH turned");
    dump("home_depth");
    web_push(0);
    CHECK(pid_get(AND_PID_DEPTH, 0) == 100, "encoder A pushed: DEPTH back to 100");
    web_enc(4, -5);
    CHECK(ae_get(AE_DEPTH, 0) == 95 && pid_get(AND_PID_DEPTH, 0) == 100, "encoder E: track 5's DEPTH 95 (%d)",
          ae_get(AE_DEPTH, 0));
    web_push(4);
    CHECK(ae_get(AE_DEPTH, 0) == 100, "encoder E pushed: track 5's DEPTH back to 100");
    CHECK(and_ui.page == AND_HOME, "encoders E..H: HOME stays");

    /* the lanes */
    run_ms(1300);
    song.sel = 1;
    CHECK(peak(17, AH_LANE_Y, 15, 9) == 15, "lane 2 selected: its number lit");
    CHECK(peak(1, AH_LANE_Y + 1, 15, 8) < 15, "lane 1 not selected");
    for (i = 0; i < AND_TRACKS; i++)
        CHECK(peak((int32_t)i * 16 + 2, AH_LANE_Y + 10, 13, 13) > 3u, "lane %u: a meter", i + 1u);
    and_select(5);
    CHECK(peak(16 * 5 + 1, AH_LANE_Y, 15, 9) == 15 && peak(17, AH_LANE_Y, 15, 9) < 15, "lane 6 selected: its number lit");
    and_select(0);

    /* FREE: the demo, ARC and RESTS set */
    web_demo(0);
    pid_set(0x280, 0, 0);                            /* (LENGTH FREE) */
    pid_set(0x259, 0, 100);                          /* ARC */
    pid_set(0x25a, 0, 100);                          /* RESTS */
    for (i = 0; i < 40u; i++) {                      /* (30 s, the screen drawn as the page does) */
        run_ms(750);
        scr();
    }
    CHECK(song.playing, "the demo plays");
    CHECK(set_columns() >= 18u && set_columns() <= 22u, "FREE: a column every 1.5 s (%u in 30 s)", set_columns());
    CHECK((and_home.f[0] & AH_SET) && !(and_home.f[AH_X - 1u] & AH_SET), "FREE: grows from the left");
    CHECK(peak((int32_t)ah_now_x() - 1, 10, 3, 27) == 15, "FREE: now a disc at the leading edge");
    dump("home_free");
    energy.rest_next = 1;                            /* (a rest now, rather than in minutes) */
    run_ms(2000);
    scr();
    CHECK(energy_resting(), "a rest");
    CHECK(peak(2, 1, 40, 7) == 15 && peak(3, 0, 40, 1) == 0, "a rest: REST inverted in the header");
    run_ms(1600);
    scr();
    CHECK(and_home.f[ah_now_x()] & AH_REST, "a rest: marked in the history");
    CHECK(peak((int32_t)ah_now_x() - 3, 35, 4, 2) == 12, "a rest: marked under the arc");
    dump("home_rest");
    for (i = 0; i < 240u; i++) {                     /* (3 minutes more: full, then scrolling) */
        run_ms(750);
        scr();
    }
    CHECK(and_home.last_x == AH_X && set_columns() == AH_X, "FREE: after 3.5 min full and scrolling (%u)", set_columns());
    CHECK(peak(AH_X - 3, 10, 3, 27) == 15, "FREE: scrolling, now at the right edge");
    dump("home_free_full");

    /* a planned piece: 1 minute */
    btn(AB_STOP);
    run_ms(100);
    pid_set(0x280, 0, 1);                            /* LENGTH 1 min */
    pid_set(0x25a, 0, 0);
    scr();
    dump("home_piece_stopped");
    btn(AB_PLAY);
    for (i = 0; i < 40u; i++) {
        run_ms(750);
        scr();
    }
    {
        uint32_t at = 0, left = 0, n = set_columns();
        int on = tessera_piece_get(&at, &left);
        CHECK(on && left >= 25u && left <= 35u, "piece: about 30 s left (%u)", left);
        CHECK(n >= 55u && n <= 72u, "piece: half the columns played (%u)", n);
        CHECK(!(and_home.f[AH_X - 1u] & AH_SET), "piece: none ahead of the play head");
    }
    dump("home_piece");

    printf(nfail ? "and_home: %u failed\n" : "and_home: all checks passed\n", nfail);
    return nfail ? 1 : 0;
}
