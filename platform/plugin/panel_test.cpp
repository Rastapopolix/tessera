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

    // the computer keyboard, as the web simulator maps it: Z the first white key, 1 FX, a held key and a mouse hold
    // apart, up arrow SELECT; repeats and other keys ignored
    CHECK(p.key(0x2C, true) && (p.input.keys & 1u) && !p.key(0x2C, true), "Z holds the first key (a repeat: no change)");
    CHECK(p.key(0x02, true) && (p.input.buttons & 1u), "1 holds FX");
    p.down(36 + 12 + (int)(7.5 * ((928 - 24) / 16)), 332 + 192);   // (the mouse holds C4 meanwhile)
    uint32_t c4 = p.input.keys & ~1u;
    CHECK(p.key(0x2C, false) && !(p.input.keys & 1u) && (p.input.keys & c4) == c4 && c4, "Z let go; the mouse's key still held");
    p.up(0, 0);
    CHECK(p.key(0x02, false) && !(p.input.buttons & 1u), "1 let go: FX up");
    p.input.enc[0] = 0;
    CHECK(p.key(Panel::KEY_UP, true) && p.input.enc[0] == 1, "up arrow turns SELECT");
    p.input.enc[0] = 0;
    CHECK(!p.key(0x39, true), "space: not a panel key");
    CHECK(p.key(0x10, true) && p.release_keys() && !p.input.keys, "focus lost: every key up");

    // OPEN and SAVE under the keys: a click asks once (the window answers with its dialog), the outcome is shown
    CHECK(p.take_file_request() == Panel::FILE_NONE, "no file asked for yet");
    p.down(60 + 36, 588 + 5 + 12);
    p.up(60 + 36, 588 + 5 + 12);
    CHECK(p.take_file_request() == Panel::FILE_OPEN && p.take_file_request() == Panel::FILE_NONE, "OPEN asks once");
    p.down(142 + 36, 588 + 5 + 12);
    p.up(142 + 36, 588 + 5 + 12);
    CHECK(p.take_file_request() == Panel::FILE_SAVE, "SAVE asks");
    std::vector<uint32_t> before_note = px;
    p.draw(before_note.data(), v);
    p.note("Saved Undertow");
    p.draw(px.data(), v);
    if (!out.empty()) ppm(out + "/06_note.ppm", px);
    int differ = 0;
    for (int y = 588; y < Panel::H; y++)
        for (int x = 230; x < 700; x++) differ += px[y * Panel::W + x] != before_note[y * Panel::W + x];
    CHECK(differ > 100, "the note is drawn beside the buttons (%d pixels)", differ);

    // the shells: a swatch under the keys shows that shell at once and asks for its ink; the ink changes the screen
    CHECK(Panel::shell_count() == 8 && p.shell() == 0, "eight shells, black first (%d)", Panel::shell_count());
    CHECK(p.take_shell_request() == -1, "no shell asked for yet");
    CHECK(Panel::shell_by_id("orange") == 3 && Panel::shell_by_id("nope") == -1, "shells by id");
    const int sx = 36 + 928 - 9 - 2 - 4 * (2 * 9 + 10), sy = 332 + 256 + 5 + 12;   // (swatch 3, ORANGE)
    p.down(sx, sy);
    p.up(sx, sy);
    CHECK(p.shell() == 3 && p.take_shell_request() == 3 && p.take_shell_request() == -1, "a swatch asks once");
    p.down(sx, sy);
    p.up(sx, sy);
    CHECK(p.take_shell_request() == -1, "the shell already shown: nothing asked");
    p.draw(px.data(), v);
    if (!out.empty()) ppm(out + "/07_orange.ppm", px);
    CHECK(px[320 * Panel::W + 12] == 0xFFEE8A30u, "the body is orange (%08X)", px[320 * Panel::W + 12]);
    std::vector<uint16_t> was(v.screen, v.screen + TESS_SCREEN * TESS_SCREEN);
    CHECK(tess_core_ink(Panel::shell_ink(3)) == 1 && tess_core_ink("NO SUCH INK") == 0, "the shell's ink is a palette");
    run(p, 0.2);
    tess_core_view(&v);
    int moved = 0;
    for (int i = 0; i < TESS_SCREEN * TESS_SCREEN; i++) moved += v.screen[i] != was[i];
    CHECK(moved > 50, "the ink changes the screen (%d pixels)", moved);
    tess_core_ink("YELLOW");

    std::printf(fails ? "panel test: %d FAILED\n" : "panel test: ok\n", fails);
    return fails ? 1 : 0;
}
