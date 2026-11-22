/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The window's test, without a host or a window: the core and the Panel (panel.h) together, as the plugin joins them.
 *   - the panel draws; the screen shows the firmware's picture (not black) once it has run;
 *   - a click on PLAY starts the song, and its LED lights; a click on SEQ changes the screen;
 *   - a knob dragged turns its encoder; a key clicked sounds;
 *   - TESSERA's shell (the default): its layout, its menu (a shell, an ink, FM-1 USB), the MIDI lamp;
 *   - GENERATE's strip in it: a click asks for a piece, a hold for before / after, the style's list and arrows, VAR,
 *     the seed's wheel; a piece made through the core shows its seed (none of it in an FM-1 shell or without styles);
 *   - the screen's zones: the mouse over FX's MIX outlines it, with a card under the screen; the wheel there, a drag and a
 *     double-click reach the core as that knob's turns and its default;
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
        if (in.track >= 0) tess_core_track_select((uint32_t)in.track);
        in.track = -1;
        for (int r = 0; r < TESS_NE; r++)
            if (in.enc[r]) { tess_core_enc((uint32_t)r, in.enc[r]); in.enc[r] = 0; }
        for (auto &a : in.zones) tess_core_zone_do(&a.z, a.action, a.n);
        in.zones.clear();
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
    CHECK(p.shell() == 0 && !std::strcmp(Panel::shell_id(0), "tessera"), "TESSERA's shell first, the default");
    p.set_shell(1);   // (the sections below: the FM-1's black shell, the web sim's layout)
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
    CHECK(Panel::shell_count() == 9 && p.shell() == 1, "nine shells: TESSERA, then black (%d)", Panel::shell_count());
    CHECK(p.take_shell_request() == -1, "no shell asked for yet");
    CHECK(Panel::shell_by_id("orange") == 4 && Panel::shell_by_id("nope") == -1, "shells by id");
    const int sx = 36 + 928 - 9 - 2 - 4 * (2 * 9 + 10), sy = 332 + 256 + 5 + 12;   // (swatch 4, ORANGE)
    p.down(sx, sy);
    p.up(sx, sy);
    CHECK(p.shell() == 4 && p.take_shell_request() == 4 && p.take_shell_request() == -1, "a swatch asks once");
    p.down(sx, sy);
    p.up(sx, sy);
    CHECK(p.take_shell_request() == -1, "the shell already shown: nothing asked");
    p.draw(px.data(), v);
    if (!out.empty()) ppm(out + "/07_orange.ppm", px);
    CHECK(px[320 * Panel::W + 12] == 0xFFEE8A30u, "the body is orange (%08X)", px[320 * Panel::W + 12]);
    std::vector<uint16_t> was(v.screen, v.screen + TESS_SCREEN * TESS_SCREEN);
    CHECK(tess_core_ink(Panel::shell_ink(4)) == 1 && tess_core_ink("NO SUCH INK") == 0, "the shell's ink is a palette");
    run(p, 0.2);
    tess_core_view(&v);
    int moved = 0;
    for (int i = 0; i < TESS_SCREEN * TESS_SCREEN; i++) moved += v.screen[i] != was[i];
    CHECK(moved > 50, "the ink changes the screen (%d pixels)", moved);
    tess_core_ink("YELLOW");

    // the inks: MENU > COLOR's 13 as swatches under the buttons; a click asks once; the one the view has is ringed;
    // the name while the mouse is on one
    std::vector<Panel::Ink> inks;
    uint32_t n_inks = tess_core_ink_at(0, nullptr, nullptr);
    for (uint32_t i = 0; i < n_inks; i++)
    {
        const char *name = nullptr;
        Panel::Ink k{};
        tess_core_ink_at(i, &name, k.rgb);
        k.name = name;
        inks.push_back(k);
    }
    CHECK(n_inks == 13 && inks[0].name == "YELLOW" && inks[9].name == "MOSAIC", "MENU > COLOR's 13 (%u)", n_inks);
    CHECK(inks[0].rgb[0] == inks[0].rgb[3] && inks[9].rgb[0] != inks[9].rgb[1], "an ink one colour, MOSAIC four");
    for (int i = 0; i < Panel::shell_count(); i++)
    {
        bool found = false;
        for (auto &k : inks) found |= k.name == Panel::shell_ink(i);
        CHECK(found, "shell %d's ink %s is a choice", i, Panel::shell_ink(i));
    }
    p.set_inks(inks);
    tess_core_view(&v);
    CHECK(v.ink == 0, "the view's ink: YELLOW (%u)", v.ink);
    auto ink_x = [](int i) { return 568 + 400 - 7 - 6 - (12 - i) * (2 * 7 + 6); };
    const int iy = (300 + 332) / 2;
    p.draw(px.data(), v);
    std::vector<uint32_t> plain(px);
    p.move(ink_x(9), iy);   // (MOSAIC: its name in place of INK)
    p.draw(px.data(), v);
    int named = 0;
    for (int y = iy - 8; y < iy + 8; y++)
        for (int x = 568; x < ink_x(0) - 10; x++) named += px[y * Panel::W + x] != plain[y * Panel::W + x];
    CHECK(named > 40, "the hovered ink's name is shown (%d pixels)", named);
    CHECK(px[(iy - 2) * Panel::W + ink_x(9) - 2] != px[(iy + 2) * Panel::W + ink_x(9) + 2], "MOSAIC in quarters");
    CHECK(p.take_ink_request() == -1, "no ink asked for yet");
    p.down(ink_x(8), iy);   // LILAC
    p.up(ink_x(8), iy);
    CHECK(p.take_ink_request() == 8 && p.take_ink_request() == -1, "an ink swatch asks once");
    p.down(ink_x(8), iy);
    p.up(ink_x(8), iy);
    CHECK(p.take_ink_request() == -1, "the ink already picked: nothing asked");
    CHECK(tess_core_ink(inks[8].name.c_str()) == 1, "LILAC set by name");
    run(p, 0.2);
    tess_core_view(&v);
    CHECK(v.ink == 8, "the view has LILAC (%u)", v.ink);
    p.move(10, 10);
    p.draw(px.data(), v);
    uint32_t lilac = 0xFF000000u | inks[8].rgb[0];
    CHECK(px[iy * Panel::W + ink_x(8)] == lilac, "LILAC's swatch (%08X)", px[iy * Panel::W + ink_x(8)]);
    if (!out.empty()) ppm(out + "/08_inks.ppm", px);
    tess_core_ink("YELLOW");
    // 8 tracks: the 1-4 / 5-8 switch beside OPEN / SAVE selects T5 or T1, Tab the other bank; the screen's header shows
    // 5-8 (inverted, before the track dots) while the selected track is in it
    if (tess_core_tracks() == 8)
    {
        p.set_tracks(8);
        auto head_lit = [&]() {   // the header's lit pixels left of the dots (logical x 60..96, y 0..8)
            int n = 0;
            for (int y = 0; y < 18; y++)
                for (int x = 120; x < 192; x++) n += v.screen[y * TESS_SCREEN + x] != 0;
            return n;
        };
        const int BY = 332 + 256 + 5 + 12;   // (KEYBED's bottom + 5, half the switch's height)
        run(p, 0.3);
        tess_core_view(&v);
        int at1 = head_lit();
        p.draw(px.data(), v);
        uint32_t off = px[(BY + 8) * Panel::W + 286 + 6];
        CHECK(v.track < 4, "the song starts on a track of 1-4 (%u)", v.track);
        click(p, 286 + 26, BY);              // 5-8
        run(p, 0.3);
        tess_core_view(&v);
        p.draw(px.data(), v);
        if (!out.empty()) ppm(out + "/08_bank_5_8.ppm", px);
        int at5 = head_lit();
        std::printf("bank: 5-8 selects T%u (core T%u); header pixels %d -> %d\n", v.track + 1, tess_core_track() + 1, at1, at5);
        CHECK(v.track == 4 && tess_core_track() == 4, "5-8 should select T5 (%u)", v.track);
        CHECK(at5 > at1 + 150, "the header should show 5-8 (%d -> %d lit pixels)", at1, at5);
        CHECK(px[(BY + 8) * Panel::W + 286 + 6] != off, "the 5-8 half should light");
        click(p, 234 + 26, BY);              // 1-4
        run(p, 0.3);
        tess_core_view(&v);
        CHECK(v.track == 0 && head_lit() == at1, "1-4 should select T1 and take the marker away (T%u, %d)", v.track + 1, head_lit());
        p.draw(px.data(), v);
        CHECK(p.key(0x0F, true) && !p.key(0x0F, false), "Tab asks for the other bank");
        run(p, 0.3);
        tess_core_view(&v);
        CHECK(v.track == 4, "Tab should select T5 (T%u)", v.track + 1);
        p.draw(px.data(), v);
        CHECK(p.key(0x0F, true), "Tab again");
        run(p, 0.3);
        tess_core_view(&v);
        CHECK(v.track == 0, "Tab again should select T1 (T%u)", v.track + 1);
        // HOME on 5-8: the columns are T5..T8, the selected one outlined at the left (it was off the screen)
        CHECK(p.key(0x0F, true), "Tab to 5-8 for HOME");
        click(p, 582 + 27, 236 + 24);        // HOME (row 2, first)
        run(p, 0.5);
        tess_core_view(&v);
        p.draw(px.data(), v);
        if (!out.empty()) ppm(out + "/09_home_5_8.ppm", px);
        CHECK(v.track == 4 && v.screen[60 * TESS_SCREEN + 0] && v.screen[60 * TESS_SCREEN + 1],
              "HOME on 5-8: T5's column outlined at the left (T%u)", v.track + 1);
        CHECK(p.key(0x0F, true), "Tab back to 1-4");
        run(p, 0.3);
    }

    // TESSERA's shell: flat, its own layout (the button grid wider, the files centred), a menu under the mark
    {
        tess_core_track_select(0);
        run(p, 0.2);
        tess_core_view(&v);
        Panel q;
        q.set_inks(inks);
        q.set_tracks((int)tess_core_tracks());
        q.set_version("0.1-beta2");
        q.draw(px.data(), v);
        if (!out.empty()) ppm(out + "/10_tessera.ppm", px);
        CHECK(px[320 * Panel::W + 12] == 0xFF111111u, "TESSERA's body (%08X)", px[320 * Panel::W + 12]);
        CHECK(px[13 * Panel::W + 37] == 0xFFFFD020u, "the mark in Tessera yellow (%08X)", px[13 * Panel::W + 37]);
        int screen = 0;
        for (int y = 0; y < TESS_SCREEN; y++)
            for (int x = 0; x < TESS_SCREEN; x++)
            {
                uint16_t c565 = v.screen[y * TESS_SCREEN + x];
                uint32_t r = (c565 >> 11 & 31) * 255 / 31, g = (c565 >> 5 & 63) * 255 / 63, b = (c565 & 31) * 255 / 31;
                screen += px[(44 + 8 + y) * Panel::W + 292 + 8 + x] == (0xFF000000u | r << 16 | g << 8 | b);
            }
        CHECK(screen == TESS_SCREEN * TESS_SCREEN, "the screen at 1:1 inside the hairline (%d)", screen);
        q.down(574 + 5 * 66 + 55, 174 + 20);   // GLO, at the grid's right edge (wider than the FM-1's)
        CHECK(q.input.buttons & (1u << 5), "GLO held where TESSERA draws it");
        q.up(0, 0);
        CHECK(!(q.input.buttons & (1u << 5)), "GLO let go");
        CHECK(q.wheel(610, 100, 1) && q.input.enc[3] == 1, "a wheel turns KNOB 1");
        q.input.enc[3] = 0;
        // the MIDI lamp: lit a moment when MIDI comes in
        q.set_midi(7);
        q.draw(px.data(), v);
        std::vector<uint32_t> quiet = px;
        q.set_midi(8);
        q.draw(px.data(), v);
        int lamp = 0;
        for (int y = 16; y < 34; y++)
            for (int x = 200; x < 300; x++) lamp += px[y * Panel::W + x] != quiet[y * Panel::W + x];
        CHECK(lamp >= 30, "the MIDI lamp lights (%d pixels)", lamp);
        // the menu: the mark opens it; SHELL lists the shells, a pick shows it and asks for its ink
        CHECK(!q.menu_shown(), "the menu shut at first");
        q.down(100, 25);
        q.up(100, 25);
        CHECK(q.menu_shown(), "the mark opens the menu");
        q.down(80, 46 + 10 + 18);   // SHELL
        q.up(80, 46 + 10 + 18);
        q.move(400, 46 + 10 + 30 * 4 + 15);
        q.draw(px.data(), v);
        if (!out.empty()) ppm(out + "/11_tessera_shells.ppm", px);
        CHECK(q.take_shell_request() == -1, "no shell asked for yet");
        q.down(400, 46 + 10 + 30 * 4 + 15);   // ORANGE (the fifth)
        q.up(400, 46 + 10 + 30 * 4 + 15);
        CHECK(q.shell() == 4 && q.take_shell_request() == 4 && !q.menu_shown(), "a shell picked from the menu (%d)", q.shell());
        q.draw(px.data(), v);
        CHECK(px[320 * Panel::W + 12] == 0xFFEE8A30u, "the FM-1's orange shell drawn");
        const int tx = 36 + 928 - 9 - 2 - 8 * (2 * 9 + 10) - 14, ty = 332 + 256 + 5 + 12;   // (swatch 0, TESSERA, set 14 apart)
        q.down(tx, ty);
        q.up(tx, ty);
        CHECK(q.shell() == 0 && q.take_shell_request() == 0, "TESSERA again from the FM-1's swatches");
        // INK: the swatches; one picked asks once and shuts the menu
        q.down(100, 25);
        q.up(100, 25);
        q.down(80, 46 + 10 + 36 + 18);   // INK
        q.up(80, 46 + 10 + 36 + 18);
        const int ix = 344 + 18 + 3 * 44 + 16, iy = 46 + 10 + 36 - 10 + 18 + 44 + 16;   // (ink 8: LILAC)
        q.move(ix, iy);
        q.draw(px.data(), v);
        if (!out.empty()) ppm(out + "/12_tessera_inks.ppm", px);
        q.down(ix, iy);
        q.up(ix, iy);
        CHECK(q.take_ink_request() == 8 && q.take_ink_request() == -1 && !q.menu_shown(), "an ink picked from the menu");
        // a click outside, or Esc, shuts it
        q.down(100, 25);
        q.up(100, 25);
        q.down(800, 500);
        CHECK(!q.menu_shown() && !(q.input.keys), "a click outside shuts the menu and plays nothing");
        q.up(800, 500);
        q.down(100, 25);
        q.up(100, 25);
        q.key(0x01, true);
        CHECK(!q.menu_shown(), "Esc shuts the menu");
    }

    // FM-1 USB: TESSERA's menu row (OFF / ON, or a click on the row: the other one), the FM-1 shells' button
    {
        Panel q;
        CHECK(q.take_fm1_request() == -1, "FM-1 USB: nothing asked yet");
        q.down(100, 20);   // (the mark: the menu)
        q.up(100, 20);
        int y = 46 + 10 + 4 * 36 + 2 * 18 + 18;   // (panel.cpp row_y(ROW_FM1): the row's middle)
        q.down(310, y);   // ON (the right one)
        q.up(310, y);
        CHECK(q.take_fm1_request() == 1 && q.take_fm1_request() == -1, "the menu's ON asks once");
        q.set_fm1(true, true, true, "FM-1 CONNECTED");
        q.down(254, y);   // OFF
        q.up(254, y);
        CHECK(q.take_fm1_request() == 0, "the menu's OFF asks for off");
        q.down(90, y);    // the row's name: the other one (on now: off)
        q.up(90, y);
        CHECK(q.take_fm1_request() == 0, "a click on the row while on asks for off");
        q.set_fm1(true, false, false, "IN ANOTHER TESSERA");
        q.down(90, y);
        q.up(90, y);
        CHECK(q.take_fm1_request() == 1, "another Tessera has it: a click asks to take it");
        CHECK(q.menu_shown(), "the menu stays open");
        // KEYS: the computer keyboard Tessera's (ON, the default) or the DAW's (OFF)
        int ky = 46 + 10 + 5 * 36 + 2 * 18 + 22 + 18;   // (panel.cpp row_y(ROW_KEYS): the row's middle)
        CHECK(q.keys_on() && q.take_keys_request() == -1, "KEYS on to start, nothing asked");
        q.down(254, ky);   // OFF
        q.up(254, ky);
        CHECK(!q.keys_on() && q.take_keys_request() == 0 && q.take_keys_request() == -1, "KEYS OFF asks once");
        CHECK(!q.key(0x2C, true) && !q.input.keys, "KEYS OFF: Z left to the DAW");
        q.down(310, ky);   // ON
        q.up(310, ky);
        CHECK(q.keys_on() && q.take_keys_request() == 1, "KEYS ON asks for on");
        CHECK(q.key(0x2C, true) && (q.input.keys & 1u) && q.key(0x2C, false), "KEYS ON: Z plays again");
        q.key(0x01, true);   // (Esc: the menu shut)
        CHECK(!q.menu_shown(), "Esc shuts the menu");
        // a right-click latches a button or key: GLO held while FX is clicked, then let go; Esc lets every latch go
        const int glo_x = 574 + 5 * 66 + 55, fx_x = 574 + 30, by = 174 + 20;
        CHECK(q.latch(glo_x, by) && (q.input.buttons & (1u << 5)), "a right-click latches GLO");
        q.down(fx_x, by);
        CHECK((q.input.buttons & 1u) && (q.input.buttons & (1u << 5)), "FX clicked with GLO latched: both held");
        q.up(fx_x, by);
        CHECK(!(q.input.buttons & 1u) && (q.input.buttons & (1u << 5)), "FX let go, GLO still held");
        CHECK(q.latch(glo_x, by) && !(q.input.buttons & (1u << 5)), "a second right-click lets GLO go");
        q.latch(glo_x, by);
        CHECK(q.down(glo_x, by) && !(q.input.buttons & (1u << 5)), "a click on a latched button lets it go");
        q.up(glo_x, by);
        const int c4x = 36 + 12 + (int)(7.5 * ((928 - 24) / 16)), c4y = 332 + 192;
        CHECK(q.latch(c4x, c4y) && q.input.keys, "a right-click latches a key");
        CHECK(q.key(0x01, true) && !q.input.keys && !q.input.buttons, "Esc lets every latch go");
        CHECK(!q.latch(5, 5), "a right-click on nothing: no change");
        Panel f;
        f.set_tracks(8);
        f.set_shell(1);
        int fy = 0;
        for (int yy = 560; yy < 624 && !fy; yy++)   // (the FM-1 USB button: under the keys, after 5-8)
            if (f.down(400, yy), f.up(400, yy), f.take_fm1_request() == 1)
                fy = yy;
        CHECK(fy > 0, "the FM-1 shells' FM-1 USB button asks for on");
        f.set_fm1(true, true, true, "FM-1 CONNECTED");
        f.down(400, fy);
        f.up(400, fy);
        CHECK(f.take_fm1_request() == 0, "lit, it asks for off");
    }

    // ---- the screen's zones: FX's MIX column under the mouse
    {
        Panel z;
        z.set_shell(1);
        tess_core_buttons(1u << 0);   // FX
        run(z, 0.05);
        tess_core_buttons(0);
        run(z, 0.3);
        tess_core_view(&v);
        z.draw(px.data(), v);
        int mix = -1;
        for (uint32_t i = 0; i < v.zone_count; i++)
            if (v.zones[i].kind == TESS_ZONE_KNOB && v.zones[i].role == 4) mix = (int)i;
        CHECK(mix >= 0, "FX: KNOB 2's column is a zone");
        if (mix >= 0)
        {
            const tess_zone_t m = v.zones[mix];
            const int ox = 292 + 256 / 32, oy = 44 + 256 / 32, mx = ox + m.x + m.w / 2, my = oy + m.y + m.h / 2;
            std::string v0 = m.value;
            z.move(mx, my);
            z.draw(px.data(), v);
            int card = 0;
            for (int yy = 44 + 256; yy < 44 + 256 + 40; yy++)   // (under the screen)
                for (int xx = mx - 80; xx < mx + 80; xx++) card += px[yy * Panel::W + xx] == 0xFFFFD020u;
            CHECK(card > 40, "the mouse over %s: a card with its value under the screen (%d yellow pixels)", m.name, card);
            if (!out.empty()) ppm(out + "/09_zone_hover.ppm", px);
            CHECK(z.wheel(mx, my, 2) && z.input.zones.size() == 1 && z.input.zones[0].action == TESS_ZA_TURN &&
                      z.input.zones[0].n == 2, "the wheel on %s: two detents up for its knob", m.name);
            CHECK(!z.wheel(5, 5, 1), "the wheel off the screen and the knobs: nothing");
            run(z, 0.3);
            tess_core_view(&v);
            std::string v1 = v.zones[mix].value;
            CHECK(v1 != v0, "%s turned by the wheel (%s, then %s)", m.name, v0.c_str(), v1.c_str());
            z.down(mx, my);
            for (int i = 1; i <= 3; i++) z.move(mx, my - 8 * i);
            z.up(mx, my - 24);
            int turned = 0;
            for (auto &a : z.input.zones) turned += a.action == TESS_ZA_TURN ? a.n : 0;
            CHECK(turned == 3, "a drag up 24 px on %s: three detents (%d)", m.name, turned);
            run(z, 0.3);
            tess_core_view(&v);
            z.draw(px.data(), v);
            std::string v2 = v.zones[mix].value;
            CHECK(v2 != v1, "%s turned by the drag (%s, then %s)", m.name, v1.c_str(), v2.c_str());
            z.down(mx, my), z.up(mx, my), z.down(mx, my), z.up(mx, my);
            CHECK(!z.input.zones.empty() && z.input.zones.back().action == TESS_ZA_DOUBLE, "a double-click on %s asks for its default", m.name);
            run(z, 0.3);
            tess_core_view(&v);
            std::printf("zones: %s %s, wheel %s, drag %s, double-click %s\n", m.name, v0.c_str(), v1.c_str(), v2.c_str(),
                        v.zones[mix].value);
            CHECK(std::string(v.zones[mix].value) != v2, "%s back to its default (%s)", m.name, v.zones[mix].value);
        }
    }

    // GENERATE's strip (TESSERA's shell, a build with styles)
    {
        Panel g;
        g.set_inks(inks);
        g.set_tracks((int)tess_core_tracks());
        g.set_version("0.1-beta2");
        std::vector<std::string> names;
        for (uint32_t i = 0; i < tess_core_gen_styles(); i++) names.emplace_back(tess_core_gen_style(i));
        CHECK(names.size() == 15 && names[0] == "AUTUMN", "fifteen styles, AUTUMN first (%d)", (int)names.size());
        tess_core_view(&v);
        g.draw(px.data(), v);
        int strip = 0;
        for (int y = 10; y < 38; y++)
            for (int x = 316; x < 820; x++) strip += px[y * Panel::W + x] == 0xFFFFD020u;
        CHECK(strip == 0, "no strip before the styles are known");
        g.set_gen_styles(names);
        Panel::GenState st;
        st.style = 2;
        st.variation = 100;
        g.set_gen(st);
        g.draw(px.data(), v);
        for (int y = 10; y < 38; y++)
            for (int x = 316; x < 820; x++) strip += px[y * Panel::W + x] == 0xFFFFD020u;
        CHECK(strip > 400, "the strip drawn (%d yellow pixels)", strip);
        // a click: a new piece in WOZZA at VAR 100
        g.down(370, 24);
        for (int i = 0; i < 5; i++) g.draw(px.data(), v);
        g.up(370, 24);
        Panel::GenRequest r = g.take_gen_request();
        CHECK(r.what == Panel::GEN_MAKE && r.style == 2 && r.variation == 100, "a click on GENERATE asks for a piece (%d %d %d)",
              r.what, r.style, r.variation);
        CHECK(g.take_gen_request().what == Panel::GEN_NONE, "asked once");
        // held half a second: before / after, and no piece when let go
        g.down(370, 24);
        for (int i = 0; i < 16; i++) g.draw(px.data(), v);
        r = g.take_gen_request();
        CHECK(r.what == Panel::GEN_SWAP, "GENERATE held half a second: before / after (%d)", r.what);
        g.up(370, 24);
        CHECK(g.take_gen_request().what == Panel::GEN_NONE, "let go after the hold: no new piece");
        // the style: the arrows, the wheel, the list
        g.down(440, 24), g.up(440, 24);
        r = g.take_gen_request();
        CHECK(r.what == Panel::GEN_SET && r.style == 1, "the left arrow: SHORE (%d)", r.style);
        g.wheel(520, 24, -2);
        r = g.take_gen_request();
        CHECK(r.what == Panel::GEN_SET && r.style == 3, "the wheel down twice: ORBIT (%d)", r.style);
        g.down(520, 24), g.up(520, 24);
        g.move(500, 44 + 10 + 7 * 26 + 12);
        g.draw(px.data(), v);
        if (!out.empty()) ppm(out + "/13_generate_list.ppm", px);
        g.down(500, 44 + 10 + 7 * 26 + 12), g.up(500, 44 + 10 + 7 * 26 + 12);
        r = g.take_gen_request();
        CHECK(r.what == Panel::GEN_SET && r.style == 7 && names[7] == "INTERLOCK", "INTERLOCK picked from the list (%d)", r.style);
        g.key(0x01, true);
        // VAR: the wheel, a drag; the seed's wheel: the pieces before
        g.wheel(650, 24, -2);
        r = g.take_gen_request();
        CHECK(r.what == Panel::GEN_SET && r.variation == 90, "VAR down two detents: 90 (%d)", r.variation);
        g.down(650, 24);
        for (int i = 1; i <= 2; i++) g.move(650, 24 + 8 * i);
        g.up(650, 40);
        r = g.take_gen_request();
        CHECK(r.variation == 80, "VAR dragged down 16 px: 80 (%d)", r.variation);
        g.wheel(760, 24, 1);
        r = g.take_gen_request();
        CHECK(r.what == Panel::GEN_RECALL && r.dir == -1, "the seed's wheel up: an older piece (%d %d)", r.what, r.dir);
        // a piece made through the core: its seed shown
        CHECK(tess_core_generate(7, 0x1A2B3C4Du, 80, 0) == 0, "INTERLOCK made");
        run(g, 1.0);
        tess_core_view(&v);
        st.style = 7, st.variation = 80, st.seed = 0x1A2B3C4Du, st.made = 1, st.at = 0;
        g.set_gen(st);
        g.note("HOLD GENERATE FOR BEFORE");
        g.draw(px.data(), v);
        if (!out.empty()) ppm(out + "/13_generate.ppm", px);
        uint32_t sty, sd, var;
        CHECK(tess_core_gen_last(&sty, &sd, &var) && sty == 7 && sd == 0x1A2B3C4Du && var == 80, "the core's last piece");
        CHECK(tess_core_gen_swap() == 0, "before / after through the core");
        // not in an FM-1 shell
        g.set_shell(1);
        g.draw(px.data(), v);
        g.down(370, 24), g.up(370, 24);
        CHECK(g.take_gen_request().what == Panel::GEN_NONE, "no GENERATE in an FM-1 shell");
    }

    std::printf(fails ? "panel test: %d FAILED\n" : "panel test: ok\n", fails);
    return fails ? 1 : 0;
}
