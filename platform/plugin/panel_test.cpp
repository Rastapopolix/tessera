/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The window's test, without a host or a window: the core and the Panel (panel.h) together, as the plugin joins them.
 *   - the panel draws; the screen shows the firmware's picture (not black) once it has run;
 *   - a click on PLAY starts the song, and its LED lights; a click on SEQ changes the screen;
 *   - a knob dragged turns its encoder; a key clicked sounds;
 *   - writes the window as PPM pictures (argument: a folder) to look at.
 * usage: tessera-panel-test [out-folder] */
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "panel.h"

static int fails = 0;
#define CHECK(c, ...)                        \
    do                                       \
    {                                        \
        if (!(c))                            \
        {                                    \
            std::printf("FAIL: " __VA_ARGS__); \
            std::printf("\n");               \
            fails++;                         \
        }                                    \
    } while (0)

static float L[1024], R[1024];
static double run(Panel &p, double seconds)   // as the plugin's audio thread: input in, render, the view out
{
    double e = 0;
    PanelInput &in = p.input;
    if (in.changed)
    {
        tess_core_buttons(in.buttons);
        tess_core_keys(in.keys);
        if (in.master >= 0) tess_core_master((uint32_t)in.master);
        in.master = -1;
        for (int r = 0; r < TESS_NE; r++)
            if (in.enc[r]) { tess_core_enc((uint32_t)r, in.enc[r]); in.enc[r] = 0; }
        in.changed = false;
    }
    for (int n = (int)(seconds * tess_core_rate() / 441); n > 0; n--)
    {
        tess_core_render(L, R, nullptr, 441);
        for (int k = 0; k < 441; k++) e += (double)L[k] * L[k];
    }
    return e;
}
static void click(Panel &p, int x, int y)
{
    p.down(x, y);
    run(p, 0.12);
    p.up(x, y);
    run(p, 0.12);
}
static void ppm(const std::string &path, const std::vector<uint32_t> &px)
{
    FILE *f = std::fopen(path.c_str(), "wb");
    if (!f) return;
    std::fprintf(f, "P6\n%d %d\n255\n", Panel::W, Panel::H);
    for (uint32_t c : px)
    {
        unsigned char rgb[3] = {(unsigned char)(c >> 16), (unsigned char)(c >> 8), (unsigned char)c};
        std::fwrite(rgb, 1, 3, f);
    }
    std::fclose(f);
}
static uint64_t screen_hash(const tess_view_t &v)
{
    uint64_t h = 1469598103934665603ull;
    for (uint16_t s : v.screen) h = (h ^ s) * 1099511628211ull;
    return h;
}

int main(int argc, char **argv)
{
    std::string out = argc > 1 ? argv[1] : "";
    static tess_view_t v;
    std::vector<uint32_t> px(Panel::W * Panel::H);
    Panel p;
    tess_core_boot();
    p.input.changed = true;
    run(p, 1.0);
    tess_core_view(&v);
    p.draw(px.data(), v);
    int lit = 0;
    for (uint16_t s : v.screen) lit += s != 0;
    std::printf("home: %d of %d screen pixels lit, playing %u\n", lit, TESS_SCREEN * TESS_SCREEN, v.playing);
    CHECK(lit > 500, "the screen should show the firmware's picture");
    CHECK(px[0] != 0 && px[(Panel::H / 2) * Panel::W + Panel::W / 2] != 0, "the panel draws");
    if (!out.empty()) ppm(out + "/01_home.ppm", px);

    // PLAY (row 2, fifth: x 582 + 4 * 63, y 236)
    click(p, 582 + 4 * 63 + 27, 236 + 24);
    run(p, 2.0);
    tess_core_view(&v);
    std::printf("PLAY: playing %u, lit buttons %#x\n", v.playing, v.lit_buttons);
    CHECK(v.playing, "PLAY should start the song");
    p.draw(px.data(), v);
    if (!out.empty()) ppm(out + "/02_playing.ppm", px);

    // SEQ (row 2, fourth): another page
    uint64_t before = screen_hash(v);
    click(p, 582 + 3 * 63 + 27, 236 + 24);
    run(p, 0.5);
    tess_core_view(&v);
    CHECK(screen_hash(v) != before, "SEQ should change the screen");
    p.draw(px.data(), v);
    if (!out.empty()) ppm(out + "/03_seq.ppm", px);

    // KNOB 1 dragged up: its encoder turns (the screen's value changes)
    before = screen_hash(v);
    p.down(610, 100);
    for (int i = 1; i <= 6; i++) p.move(610, 100 - 8 * i);
    p.up(610, 52);
    run(p, 0.5);
    tess_core_view(&v);
    CHECK(screen_hash(v) != before, "KNOB 1 should change the screen");
    p.draw(px.data(), v);
    if (!out.empty()) ppm(out + "/04_knob1.ppm", px);

    // PLAY again stops; then a key held sounds
    click(p, 582 + 4 * 63 + 27, 236 + 24);
    double quiet = run(p, 6.0);
    quiet = run(p, 0.5);
    p.down(36 + 12 + (int)(7.5 * ((928 - 24) / 16)), 332 + 192);   // the eighth white key (C4)
    double loud = run(p, 0.5);
    tess_core_view(&v);
    p.draw(px.data(), v);
    if (!out.empty()) ppm(out + "/05_key.ppm", px);
    p.up(0, 0);
    run(p, 0.1);
    std::printf("key: energy %.4f stopped, %.4f with the key held; keys held %#x\n", quiet, loud, p.input.keys);
    CHECK(loud > quiet * 4 + 1e-4, "a key held should sound");

    std::printf(fails ? "panel test: %d FAILED\n" : "panel test: ok\n", fails);
    return fails ? 1 : 0;
}
