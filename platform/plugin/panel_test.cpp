/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The window's test, without a host or a window: the core and the Panel (panel.h) together, as the plugin joins them.
 *   - the panel draws; the screen shows the firmware's picture (not black) once it has run;
 *   - a click on PLAY starts the song, and its LED lights; a click on SEQ changes the screen;
 *   - a knob dragged turns its encoder; a key clicked sounds;
 *   - TESSERA's shell (the default): its layout, its menu (a shell, an ink, FM-1 USB, SIZE), the MIDI lamp; the window
 *     at 150 and 200 % (Panel::scale);
 *   - GENERATE's strip in it: a click asks for a piece, a hold for before / after, the style's list and arrows, VAR,
 *     the seed's wheel; a piece made through the core shows its seed (none of it in an FM-1 shell or without styles);
 *   - the screen's zones: the mouse over FX's MIX outlines it, with a card under the screen; the wheel there, a drag and a
 *     double-click reach the core as that knob's turns and its default;
 *   - EXPANDED (MODE in TESSERA's menu): 1500 wide, SIZE 100 / 125, TRACKS beside the screen as the core draws it, a
 *     click on a row selects that track, the wheel on a level turns it, the keys across the window, no bank switch; an
 *     FM-1 shell is FM-1 mode; 125 % scales;
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
        tess_core_key_words(in.key_words ? 1 : 0);
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
        for (auto &a : in.tzones) tess_core_tracks_do(&a.z, a.action, a.n);
        in.tzones.clear();
        for (auto &a : in.typed) tess_core_zone_type(&a.z, a.text);
        in.typed.clear();
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
static void ppm(const std::string &path, const std::vector<uint32_t> &px, int w = Panel::W)
{
    FILE *f = std::fopen(path.c_str(), "wb");
    if (!f) return;
    std::fprintf(f, "P6\n%d %d\n255\n", w, Panel::H);
    for (size_t i = 0; i < (size_t)w * Panel::H && i < px.size(); i++)
    {
        uint32_t c = px[i];
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

    // MASTER dragged fast while the view lags (the audio thread takes each value a block or two later, as in the
    // Windows app): every step counts, from where the drag left it, not from the last view
    {
        tess_core_view(&v);
        p.draw(px.data(), v);
        int m0 = (int)v.master, last = -1;
        int mx, my;
        p.knob_at_xy(0, mx, my);
        p.down(mx, my);
        for (int i = 1; i <= 10; i++)
        {
            p.move(mx, my - 3 * i);   // (3 px: a step of 16)
            if (p.input.master >= 0) last = p.input.master;
            p.input.master = -1;          // handed on; the view not yet updated
            p.draw(px.data(), v);
        }
        p.up(mx, my - 30);
        int want = m0 + 160 > 1023 ? 1023 : m0 + 160;
        CHECK(last == want, "MASTER dragged 30 px up from %d: %d, not %d", m0, want, last);
        CHECK(std::abs(p.knob_angle(0) - (-135 + 270.0 * want / 1023)) < 0.01, "MASTER's pointer where the drag left it");
        tess_core_master((uint32_t)m0);   // (as it was, for what follows)
        p.input.changed = false;
    }

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
    CHECK(!p.key(0x39, true), "space: not a panel key (in a DAW)");
    CHECK(!p.shortcut('O') && p.take_file_request() == Panel::FILE_NONE, "Ctrl+O in a DAW: the DAW's");
    CHECK(p.key(0x01, true) && (p.input.buttons >> 12 & 1u), "Esc holds OCT- (BACK)");
    CHECK(p.key(0x01, false) && !(p.input.buttons >> 12 & 1u), "Esc let go");
    CHECK(p.key(0x1C, true) && (p.input.buttons >> 13 & 1u), "Enter holds OCT+ (OK)");
    CHECK(p.key(0x1C, false) && !(p.input.buttons >> 13 & 1u), "Enter let go");
    p.set_standalone(true);
    CHECK(p.key(0x39, true) && (p.input.buttons >> 10 & 1u), "the standalone app: space holds PLAY");
    CHECK(p.key(0x39, false) && !(p.input.buttons >> 10 & 1u), "space let go");
    CHECK(p.shortcut('S') && p.take_file_request() == Panel::FILE_SAVE, "the standalone app: Ctrl+S asks for SAVE");
    p.set_standalone(false);
    {   // a click on nothing (the body) while a picker, sheet or help card is open: away (tess_core_away); else nothing
        tess_view_t cv;
        tess_core_view(&cv);
        cv.closable = 1;
        p.draw(px.data(), cv);
        p.input.away = false;
        CHECK(p.down(280, 315) && p.input.away, "a click on the body with a picker open: away");
        p.up(280, 315);
        p.input.away = false;
        cv.closable = 0;
        p.draw(px.data(), cv);
        CHECK(!p.down(280, 315) && !p.input.away, "nothing open: a click on the body does nothing");
        p.up(280, 315);
    }
    CHECK(p.key(0x10, true) && p.release_keys() && !p.input.keys, "focus lost: every key up");

    // OPEN and SAVE under the keys: a click asks once (the window answers with its dialog), the outcome is shown
    CHECK(p.take_file_request() == Panel::FILE_NONE, "no file asked for yet");
    p.down(60 + 36, 588 + 5 + 12);
    p.up(60 + 36, 588 + 5 + 12);
    CHECK(p.take_file_request() == Panel::FILE_OPEN && p.take_file_request() == Panel::FILE_NONE, "OPEN asks once");
    p.down(142 + 36, 588 + 5 + 12);
    p.up(142 + 36, 588 + 5 + 12);
    CHECK(p.take_file_request() == Panel::FILE_SAVE, "SAVE asks");
    p.down(224 + 36, 588 + 5 + 12);
    p.up(224 + 36, 588 + 5 + 12);
    CHECK(p.take_file_request() == Panel::FILE_MIDI, "MIDI asks");
    p.down(224 + 36, 588 + 5 + 12);   // MIDI dragged off (G4): no ask on the press or a small wobble, a drag past 8 px
    CHECK(p.take_file_request() == Panel::FILE_NONE, "MIDI pressed: nothing asked yet (a click or a drag)");
    p.move(224 + 39, 588 + 5 + 13);
    CHECK(p.peek_file_request() == Panel::FILE_NONE, "MIDI: a wobble of 4 px is still a click");
    p.move(224 + 36, 588 + 5 + 2);
    CHECK(p.take_file_request() == Panel::FILE_MIDI_DRAG, "MIDI dragged 10 px: the clip's drag asked for");
    p.up(-1, -1);
    CHECK(p.take_file_request() == Panel::FILE_NONE, "the drag's window lets go: no save as well");
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
    CHECK(Panel::shell_count() == 10 && p.shell() == 1, "ten shells: TESSERA, the FM-1's, its DAY (%d)", Panel::shell_count());
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
        uint32_t off = px[(BY + 8) * Panel::W + 368 + 6];
        CHECK(v.track < 4, "the song starts on a track of 1-4 (%u)", v.track);
        click(p, 368 + 26, BY);              // 5-8
        run(p, 0.3);
        tess_core_view(&v);
        p.draw(px.data(), v);
        if (!out.empty()) ppm(out + "/08_bank_5_8.ppm", px);
        int at5 = head_lit();
        std::printf("bank: 5-8 selects T%u (core T%u); header pixels %d -> %d\n", v.track + 1, tess_core_track() + 1, at1, at5);
        CHECK(v.track == 4 && tess_core_track() == 4, "5-8 should select T5 (%u)", v.track);
        CHECK(at5 > at1 + 150, "the header should show 5-8 (%d -> %d lit pixels)", at1, at5);
        CHECK(px[(BY + 8) * Panel::W + 368 + 6] != off, "the 5-8 half should light");
        click(p, 316 + 26, BY);              // 1-4
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
        q.down(566 + 1 * 84 + 37, 236 + 20);   // MIX, the second row's second (TESSERA's own buttons)
        CHECK(q.input.buttons & TESS_BTN_SECTION(TESS_SEC_MIX), "MIX held where TESSERA draws it");
        q.up(0, 0);
        CHECK(!(q.input.buttons & TESS_BTN_SECTION(TESS_SEC_MIX)), "MIX let go");
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
        q.move(400, 46 + 10 + 30 + 15);
        q.draw(px.data(), v);
        if (!out.empty()) ppm(out + "/11_tessera_shells.ppm", px);
        CHECK(q.take_shell_request() == -1, "no shell asked for yet");
        q.down(400, 46 + 10 + 30 + 15);   // DAY (the second of NIGHT, DAY)
        q.up(400, 46 + 10 + 30 + 15);
        CHECK(q.day() && q.native() && q.take_shell_request() == Panel::shell_count() - 1 && !q.menu_shown(),
              "SKIN: DAY picked from the menu (%d)", q.shell());
        q.draw(px.data(), v);
        if (!out.empty()) ppm(out + "/11_tessera_day.ppm", px);
        CHECK(px[320 * Panel::W + 12] == 0xFFEDE8DAu, "the DAY skin's paper body (%08X)", px[320 * Panel::W + 12]);
        q.down(q.skin_switch_x() + 8, 24);   // the moon, left of the version
        q.up(q.skin_switch_x() + 8, 24);
        CHECK(!q.day() && q.shell() == 0 && q.take_shell_request() == 0, "the moon: NIGHT again");
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
        // typing: a character goes to NAME only while the view says NAME is open, and isn't a key then
        CHECK(!q.text('a') && q.input.text.empty(), "not typing: a character is a key's, not NAME's");
        {
            tess_view_t tv2 = v;
            tv2.typing = 1;
            q.draw(px.data(), tv2);
            CHECK(q.text('a') && q.input.text.size() == 1 && q.input.text[0] == 'a', "NAME open: a character typed into it");
            q.input.text.clear();
            q.draw(px.data(), v);
        }
        // INK's row shows the ink as its swatch does: a four-ink palette (MOSAIC) in quarters, not its first ink alone
        {
            int mos = -1;
            for (int j = 0; j < (int)inks.size() && mos < 0; j++)
                if (inks[j].rgb[0] != inks[j].rgb[1] && inks[j].rgb[1] != inks[j].rgb[2]) mos = j;
            CHECK(mos >= 0, "a four-ink palette among the inks");
            if (mos >= 0)
            {
                q.down(100, 25), q.up(100, 25);
                q.down(80, 46 + 10 + 36 + 18), q.up(80, 46 + 10 + 36 + 18);   // INK
                const int mx2 = 344 + 18 + mos % 5 * 44 + 16, my2 = 46 + 10 + 36 - 10 + 18 + mos / 5 * 44 + 16;
                q.down(mx2, my2), q.up(mx2, my2);
                q.take_ink_request();
                q.down(100, 25), q.up(100, 25);   // (the menu again: INK's row)
                q.draw(px.data(), v);
                int seen = 0;
                for (int i = 0; i < 4; i++)
                {
                    bool here = false;
                    for (int y = 92; y < 92 + 36 && !here; y++)
                        for (int x = 20; x < 340 && !here; x++) here = px[y * Panel::W + x] == (0xFF000000u | inks[mos].rgb[i]);
                    seen += here;
                }
                CHECK(seen == 4, "INK's row: %s's four inks in quarters (%d of 4)", inks[mos].name.c_str(), seen);
                if (!out.empty()) ppm(out + "/12b_tessera_ink_row.ppm", px);
                q.key(0x01, true);
            }
        }
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
        // SIZE: 100, 150 or 200 picked in the menu (asked once, the menu shut); the pixels at 150 and 200 %
        CHECK(q.take_size_request() == -1, "no size asked for yet");
        q.down(100, 25), q.up(100, 25);
        CHECK(q.menu_shown(), "the menu again");
        q.down(300, 200), q.up(300, 200);   // (SIZE's row, its right end: 200)
        CHECK(q.take_size_request() == 200 && !q.menu_shown(), "SIZE 200 picked from the menu");
        q.set_size_pct(200);
        q.down(100, 25), q.up(100, 25);
        q.draw(px.data(), v);
        if (!out.empty()) ppm(out + "/14_tessera_size.ppm", px);   // (the menu: SIZE 200 lit)
        q.down(247, 200), q.up(247, 200);
        CHECK(q.take_size_request() == 150, "SIZE 150 picked from the menu");
        q.down(100, 25), q.up(100, 25);
        q.down(300, 200), q.up(300, 200);
        CHECK(q.take_size_request() == -1, "SIZE 200 again: nothing to ask (it is 200 already)");
        {   // a frame dragged to 117 %: the row shows 117 lit, left of the presets; 75 picked from it
            q.set_size_pct(117);
            q.down(100, 25), q.up(100, 25);
            int x75 = -1, x117 = -1;
            for (int x = 300; x > 40 && (x75 < 0 || x117 < 0); x--)
            {
                int pct = q.size_seg_at(x);
                if (pct == 75 && x75 < 0) x75 = x;
                if (pct == 117 && x117 < 0) x117 = x;
            }
            CHECK(x75 > 0 && x117 > 0 && x117 < x75, "SIZE's row: 75 and, left of it, 117 (%d, %d)", x75, x117);
            q.down(x117, 200), q.up(x117, 200);
            CHECK(q.take_size_request() == -1 && !q.menu_shown(), "117 (the size now): nothing to ask");
            q.down(100, 25), q.up(100, 25);
            q.down(x75, 200), q.up(x75, 200);
            CHECK(q.take_size_request() == 75, "SIZE 75 picked from the menu");
            q.set_size_pct(200);
        }
        q.key(0x01, true);
        q.draw(px.data(), v);
        std::vector<uint32_t> big((size_t)Panel::W * Panel::H * 4), mid((size_t)Panel::W * Panel::H * 9 / 4);
        Panel::scale(px.data(), big.data(), Panel::W, 200);
        Panel::scale(px.data(), mid.data(), Panel::W, 150);
        {   // 75 % and 117 %: every window pixel a 1:1 pixel or a mean of them (no stray colour from past the edge)
            std::vector<uint32_t> small((size_t)(Panel::W * 3 / 4) * (Panel::H * 3 / 4), 0x12345678u);
            std::vector<uint32_t> odd((size_t)(Panel::W * 117 / 100) * (Panel::H * 117 / 100), 0x12345678u);
            Panel::scale(px.data(), small.data(), Panel::W, 75);
            Panel::scale(px.data(), odd.data(), Panel::W, 117);
            int bad = 0;
            for (uint32_t c : small) bad += c >> 24 != 0xFFu;
            for (uint32_t c : odd) bad += c >> 24 != 0xFFu;
            CHECK(bad == 0, "75 %% and 117 %%: every pixel drawn (%d not)", bad);
        }
        int same = 0, even = 0;
        for (int y = 0; y < Panel::H; y += 7)
            for (int x = 0; x < Panel::W; x += 7)
            {
                uint32_t c = px[y * Panel::W + x];
                same += big[(2 * y) * 2 * Panel::W + 2 * x] == c && big[(2 * y + 1) * 2 * Panel::W + 2 * x + 1] == c;
                even += mid[(3 * y / 2) * (3 * Panel::W / 2) + 3 * x / 2] == c || (x & 1) || (y & 1);
            }
        CHECK(same == ((Panel::H + 6) / 7) * ((Panel::W + 6) / 7), "200 %%: every pixel doubled");
        CHECK(even == ((Panel::H + 6) / 7) * ((Panel::W + 6) / 7), "150 %%: an even pixel lands whole");
        if (!out.empty())
        {
            FILE *f = std::fopen((out + "/15_tessera_150.ppm").c_str(), "wb");
            if (f)
            {
                std::fprintf(f, "P6\n%d %d\n255\n", Panel::W * 3 / 2, Panel::H * 3 / 2);
                for (uint32_t c : mid)
                {
                    unsigned char rgb[3] = {(unsigned char)(c >> 16), (unsigned char)(c >> 8), (unsigned char)c};
                    std::fwrite(rgb, 1, 3, f);
                }
                std::fclose(f);
            }
        }
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
        // HOVER HELP: OFF to start; ON asks once
        int hy = ky + 36;   // (row_y(ROW_HOVER)'s middle)
        CHECK(!q.hover_help() && q.take_hover_request() == -1, "HOVER HELP off to start, nothing asked");
        q.down(310, hy);   // ON
        q.up(310, hy);
        CHECK(q.hover_help() && q.take_hover_request() == 1 && q.take_hover_request() == -1, "HOVER HELP ON asks once");
        q.down(254, hy);   // OFF
        q.up(254, hy);
        CHECK(!q.hover_help() && q.take_hover_request() == 0, "HOVER HELP OFF asks for off");
        // OUTPUTS: STEREO to start; MONO asks once (the ports change from the DAW's next load)
        // TRACK: its list (copy, paste, paste the sound, move up, down), each a request for the selected track; the
        // keys (Ctrl+C ..) the same; T1 can't move up
        int ty = 46 + 10 + 9 * 36 + 2 * 18 + 22 + 18;   // (row_y(ROW_TRACK)'s middle)
        CHECK(q.take_track_request().what == Panel::TR_NONE, "TRACK: nothing asked to start");
        q.down(100, ty), q.up(100, ty);   // (the row: its list)
        int ly0 = ty - 18 - 10 + 10;      // (the list's first row's top)
        q.down(400, ly0 + 15), q.up(400, ly0 + 15);
        Panel::TrackRequest tr = q.take_track_request();
        CHECK(tr.what == Panel::TR_COPY && !q.menu_shown(), "TRACK > COPY asked (%d), the menu shut", tr.what);
        q.down(100, 25), q.up(100, 25);
        q.down(100, ty), q.up(100, ty);
        q.down(400, ly0 + 3 * 30 + 15), q.up(400, ly0 + 3 * 30 + 15);
        CHECK(q.take_track_request().what == Panel::TR_NONE && q.menu_shown(), "MOVE T1 UP: nothing (no place to go)");
        q.move(400, ly0 + 2 * 30 + 15);
        q.draw(px.data(), v);
        if (!out.empty()) ppm(out + "/14b_tessera_track.ppm", px);   // (the menu: TRACK's list, PASTE SOUND under the mouse)
        q.down(400, ly0 + 2 * 30 + 15), q.up(400, ly0 + 2 * 30 + 15);
        CHECK(q.take_track_request().what == Panel::TR_PASTE_SOUND, "PASTE SOUND asked");
        CHECK(q.shortcut('V') && q.take_track_request().what == Panel::TR_PASTE && q.shortcut('_') &&
                  q.take_track_request().what == Panel::TR_DOWN && q.shortcut('C') && q.take_track_request().what == Panel::TR_COPY,
              "Ctrl+V, Alt+Down, Ctrl+C: their requests");
        CHECK(!q.shortcut('X'), "Ctrl+X: not Cosmati's");
        q.down(100, 25), q.up(100, 25);
        int oy = 46 + 10 + 10 * 36 + 2 * 18 + 22 + 18;   // (row_y(ROW_OUTS)'s middle)
        CHECK(q.take_outs_request() == -1, "OUTPUTS: nothing asked to start");
        q.down(200, oy);   // MONO
        q.up(200, oy);
        CHECK(q.take_outs_request() == 1 && q.take_outs_request() == -1, "OUTPUTS MONO asks once");
        q.down(300, oy);   // STEREO
        q.up(300, oy);
        CHECK(q.take_outs_request() == 0, "OUTPUTS STEREO asks for stereo");
        // LISTEN: < OFF >, the channel whose notes steer (listen.h)
        int ly = 46 + 10 + 7 * 36 + 2 * 18 + 22 + 18;   // (row_y(ROW_LISTEN)'s middle)
        auto lclick = [&](int x) { q.down(x, ly), q.up(x, ly); return q.take_listen_request(); };
        CHECK(q.listen() == 0 && q.take_listen_request() == -1, "LISTEN OFF to start, nothing asked");
        CHECK(lclick(198) == -1, "< at OFF: nothing");
        CHECK(lclick(306) == 1 && q.listen() == 1, "> : channel 1");
        CHECK(lclick(306) == 2 && lclick(198) == 1, "> then <: 2, then 1");
        CHECK(lclick(252) == 0 && q.listen() == 0, "its value: OFF");
        CHECK(lclick(252) == 1, "its value again: back to the last channel (1)");
        q.set_listen(16);
        CHECK(lclick(306) == -1 && q.listen() == 16, "> at 16: nothing");
        // SPLIT, the row under: 48 to start, < and > an octave, 12..108
        int sy = ly + 36;   // (row_y(ROW_SPLIT)'s middle)
        auto sclick = [&](int x) { q.down(x, sy), q.up(x, sy); return q.take_split_request(); };
        CHECK(q.split() == 48 && q.take_split_request() == -1, "SPLIT 48 to start, nothing asked");
        CHECK(sclick(198) == 36 && sclick(306) == 48 && sclick(306) == 60, "< 36, > 48, > 60");
        CHECK(sclick(252) == -1 && q.split() == 60, "its value: nothing");
        q.set_split(12);
        CHECK(sclick(198) == -1 && q.split() == 12, "< at 12: nothing");
        q.set_split(108);
        CHECK(sclick(306) == -1 && q.split() == 108, "> at 108: nothing");
        q.set_split(48);
        q.draw(px.data(), v);
        if (!out.empty()) ppm(out + "/15_tessera_listen.ppm", px);   // (the menu: LISTEN on CH 16)
        CHECK(q.menu_shown(), "the menu stays open");
        q.key(0x01, true);   // (Esc: the menu shut)
        CHECK(!q.menu_shown(), "Esc shuts the menu");
        // a right-click latches a button or key (a section: the FM-1 button it stands for, PIECE's GLO): GLO held
        // while FX is clicked, then let go; Esc lets every latch go
        const int glo_x = 566 + 30, gy = 236 + 20, fx_x = 566 + 3 * 84 + 20, by = 174 + 20;   // (PIECE: row 2's first)
        const uint32_t fx_b = TESS_BTN_SECTION(TESS_SEC_FX);
        CHECK(q.latch(glo_x, gy) && (q.input.buttons & (1u << 5)), "a right-click on PIECE latches GLO");
        q.down(fx_x, by);
        CHECK((q.input.buttons & fx_b) && (q.input.buttons & (1u << 5)), "FX clicked with GLO latched: both held");
        q.up(fx_x, by);
        CHECK(!(q.input.buttons & fx_b) && (q.input.buttons & (1u << 5)), "FX let go, GLO still held");
        CHECK(q.latch(glo_x, gy) && !(q.input.buttons & (1u << 5)), "a second right-click lets GLO go");
        q.latch(glo_x, gy);
        CHECK(q.down(glo_x, gy) && !(q.input.buttons & (1u << 5)), "a click on a latched button lets it go");
        q.up(glo_x, gy);
        const int c4x = 36 + 12 + (int)(7.5 * ((928 - 24) / 16)), c4y = 332 + 192;
        CHECK(q.latch(c4x, c4y) && q.input.keys, "a right-click latches a key");
        CHECK(q.key(0x01, true) && !q.input.keys && !q.input.buttons, "Esc lets every latch go");
        CHECK(!q.latch(5, 5), "a right-click on nothing: no change");
        Panel f;
        f.set_tracks(8);
        f.set_shell(1);
        int fy = 0;
        for (int yy = 560; yy < 624 && !fy; yy++)   // (the FM-1 USB button: under the keys, after 5-8)
            if (f.down(480, yy), f.up(480, yy), f.take_fm1_request() == 1)
                fy = yy;
        CHECK(fy > 0, "the FM-1 shells' FM-1 USB button asks for on");
        f.set_fm1(true, true, true, "FM-1 CONNECTED");
        f.down(480, fy);
        f.up(480, fy);
        CHECK(f.take_fm1_request() == 0, "lit, it asks for off");

        // TESSERA's own BACK, SHIFT, OK and OCT- / OCT+: the screen names them (key words), an FM-1 shell or an FM-1
        // on USB keeps OCT- / OCT+; SHIFT holds OCT-'s layer and is never BACK; OCT- / OCT+ step the octave anywhere,
        // BACK / OK on Stage no longer do
        CHECK(q.input.key_words && !f.input.key_words, "key words: TESSERA's shell on, an FM-1 shell off");
        q.set_fm1(true, true, true, "FM-1 CONNECTED");
        CHECK(!q.input.key_words, "an FM-1 on USB playing it: OCT- / OCT+ as ever");
        q.set_fm1(false, false, false, "");
        CHECK(q.input.key_words, "and back to BACK / OK / SHIFT");
        q.move(50 + 33, 258 + 17);   // the mouse over SHIFT: what it does, in a card over it
        q.draw(px.data(), v);
        if (!out.empty()) ppm(out + "/10b_tessera_shift_tip.ppm", px);
        int tip = 0;
        for (int yy = 258 - 40; yy < 258; yy++)
            for (int xx = 20; xx < 400; xx++) tip += px[yy * Panel::W + xx] == 0xFFFFD020u;
        CHECK(tip > 100, "SHIFT's card over it (%d yellow pixels)", tip);
        q.down(50 + 33, 258 + 17);
        q.up(0, 0);
        CHECK(q.input.buttons == 1u << 16, "SHIFT clicked: latched (%#x)", q.input.buttons);
        q.down(50 + 33, 258 + 17);
        q.up(0, 0);
        CHECK(q.input.buttons == 0, "SHIFT clicked again: let go (%#x)", q.input.buttons);
        q.down(36 + 30, 332 + 128 + 5 + 23);   // (OCT-: under OCT+, at the keys' left)
        CHECK(q.input.buttons == 1u << 17, "OCT- (the octave) held (%#x)", q.input.buttons);
        q.up(0, 0);
        q.down(118 + 28, 258 + 17);
        CHECK(q.input.buttons == 1u << 12, "BACK is the FM-1's OCT- (%#x)", q.input.buttons);
        q.up(0, 0);
        q.down(180 + 25, 258 + 17);
        CHECK(q.input.buttons == 1u << 13, "OK is the FM-1's OCT+ (%#x)", q.input.buttons);
        q.up(0, 0);
        // the computer keyboard: Shift SHIFT (held only), [ ] BACK / OK, Page Up / Page Down OCT+ / OCT-
        q.key(0x2A, true);
        CHECK(q.input.buttons == 1u << 16, "Shift: SHIFT held (%#x)", q.input.buttons);
        q.key(0x2A, false);
        CHECK(q.input.buttons == 0, "Shift let go: SHIFT too, never latched (%#x)", q.input.buttons);
        q.key(0x36, true);
        CHECK(q.input.buttons == 1u << 16, "right Shift: SHIFT (%#x)", q.input.buttons);
        q.key(0x36, false);
        q.key(0x1A, true);
        CHECK(q.input.buttons == 1u << 12, "[: BACK (%#x)", q.input.buttons);
        q.key(0x1A, false);
        q.key(0x1B, true);
        CHECK(q.input.buttons == 1u << 13, "]: OK (%#x)", q.input.buttons);
        q.key(0x1B, false);
        q.key(Panel::KEY_PGUP, true);
        CHECK(q.input.buttons == 1u << 18, "Page Up: OCT+ (%#x)", q.input.buttons);
        q.key(Panel::KEY_PGUP, false);
        q.key(Panel::KEY_PGDN, true);
        CHECK(q.input.buttons == 1u << 17, "Page Down: OCT- (%#x)", q.input.buttons);
        q.key(Panel::KEY_PGDN, false);
        q.key(0x3B, true);
        CHECK(q.input.buttons == 1u << 19, "F1: ? (%#x)", q.input.buttons);
        q.key(0x3B, false);
        CHECK(q.input.buttons == 0, "every key up (%#x)", q.input.buttons);
        int shot_n = 0;
        auto shot = [&](double t) {
            run(q, t);
            tess_core_view(&v);
            if (getenv("SHOTS"))
            {
                std::vector<uint32_t> sp(TESS_SCREEN * Panel::H, 0xFF000000u);
                for (int i = 0; i < TESS_SCREEN * TESS_SCREEN; i++)
                {
                    uint16_t c5 = v.screen[i];
                    sp[i] = 0xFF000000u | (c5 >> 11 & 31) * 255 / 31 << 16 | (c5 >> 5 & 63) * 255 / 63 << 8 | (c5 & 31) * 255 / 31;
                }
                ppm(std::string(getenv("SHOTS")) + "/s" + std::to_string(shot_n++) + ".ppm", sp, TESS_SCREEN);
            }
            return std::vector<uint16_t>(v.screen, v.screen + TESS_SCREEN * TESS_SCREEN);
        };
        auto press = [&](uint32_t b, double t) {
            tess_core_buttons(b);
            run(q, t);
            tess_core_buttons(0);
        };
        run(q, 0.05);           // (the clicks' input taken first: run hands it in once)
        tess_core_key_words(1);
        press(1u << 6, 0.05);   // HOME
        std::vector<uint16_t> home = shot(0.4);
        const uint32_t octs = 3u << 12 | 3u << 17;   // (Stage: the OCT LEDs show the octave; here OCT- / OCT+'s)
        CHECK(!(v.lit_buttons & octs), "Stage at octave 0: no OCT LED (%#x)", v.lit_buttons);
        press(1u << 13, 0.05);   // OK on Stage: nothing (the FM-1: an octave up)
        CHECK(shot(0.4) == home && !(v.lit_buttons & octs), "OK tapped on Stage: no octave (%#x)", v.lit_buttons);
        press(1u << 18, 0.05);   // OCT+: an octave up, its LED lit
        shot(0.4);
        CHECK((v.lit_buttons & octs) == 1u << 18, "OCT+ on Stage: an octave up, OCT+ lit (%#x)", v.lit_buttons);
        press(1u << 17, 0.05);
        shot(0.4);
        CHECK(!(v.lit_buttons & octs), "OCT- on Stage: back to 0 (%#x)", v.lit_buttons);
        press(1u << 0, 0.05);   // FX
        std::vector<uint16_t> fx = shot(0.4);
        press(1u << 16, 0.05);  // SHIFT tapped: never BACK
        CHECK(shot(0.4) == fx, "SHIFT tapped on FX: still FX");
        std::vector<uint16_t> mixer;   // (a page with an OCT- layer: GLO's pages, SELECT on till OCT- held shows one)
        press(1u << 5, 0.05);   // GLO
        for (int pg = 0; pg < 12; pg++)
        {
            mixer = shot(1.0);
            tess_core_buttons(1u << 12);
            std::vector<uint16_t> layer = shot(0.4);
            tess_core_buttons(0);
            run(q, 0.3);
            if (layer != mixer) break;
            tess_core_enc(0, 1);
        }
        press(1u << 16, 0.05);  // SHIFT tapped: never BACK
        CHECK(shot(0.4) == mixer, "SHIFT tapped on a page: still there");
        tess_core_buttons(1u << 16);
        std::vector<uint16_t> shifted = shot(0.4);
        tess_core_buttons(0);
        run(q, 0.2);
        tess_core_buttons(1u << 12);
        std::vector<uint16_t> held = shot(0.4);
        tess_core_buttons(0);
        CHECK(shifted == held && shifted != mixer, "SHIFT held shows what OCT- held shows (the page's second layer)");
        CHECK(shot(0.4) == mixer, "OCT- let go after a hold: still the page");
        // help by the mouse: a right-click on FX's KNOB 2 column opens its card, kept without ARP; a click closes it and
        // does nothing more; ? opens KNOB 1's and closes it again
        press(1u << 0, 0.05);   // FX
        fx = shot(0.6);
        {
            int mix = -1;
            for (uint32_t i = 0; i < v.zone_count; i++)
                if (v.zones[i].kind == TESS_ZONE_KNOB && v.zones[i].role == 4) mix = (int)i;
            CHECK(mix >= 0, "FX: KNOB 2's column is a zone (help)");
            if (mix >= 0)
            {
                const tess_zone_t m = v.zones[mix];
                std::string before = m.value;
                const int mx = 292 + 8 + m.x + m.w / 2, my = 44 + 8 + m.y + m.h / 2;
                q.draw(px.data(), v);
                CHECK(q.latch(mx, my) && q.input.zones.empty() && q.ctx_rows().size() == 9 && q.ctx_rows()[0] == "HELP" &&
                          q.ctx_rows()[5] == "TYPE A VALUE",
                      "a right-click on KNOB 2's column: its menu, HELP first (G2)");
                q.draw(px.data(), v);
                if (!out.empty()) ppm(out + "/10f_tessera_value_menu.ppm", px);
                CHECK(q.down(mx + 20, my + 4 + 7) && q.input.zones.size() == 1 && q.input.zones[0].action == TESS_ZA_HELP,
                      "..its HELP row asks for its help");
                q.up(mx + 20, my + 11);
                q.input.buttons = 0;
                std::vector<uint16_t> card = shot(0.3);
                CHECK(card != fx, "its help card over FX");
                CHECK(shot(1.0) == card, "the card stays without ARP held");
                q.draw(px.data(), v);
                if (!out.empty()) ppm(out + "/10e_tessera_help.ppm", px);
                tess_zone_t any = v.zones[0];
                for (uint32_t i = 0; i < v.zone_count; i++)
                    if (v.zones[i].kind == TESS_ZONE_KNOB) any = v.zones[i];
                tess_core_zone_do(&any, TESS_ZA_CLICK, 0);   // (a click anywhere on the screen)
                CHECK(shot(0.4) == fx, "a click closes it");
                CHECK(v.zones[mix].value == before, "and changes nothing (%s)", v.zones[mix].value);
                press(1u << 19, 0.05);   // ?
                CHECK(shot(0.4) != fx, "? opens KNOB 1's card");
                press(1u << 19, 0.05);
                CHECK(shot(0.4) == fx, "? again closes it");
            }
        }

        // the keys' jobs (tess_view_t.key_short / key_long): GLO held, MIXER's keys (M / S, ALL HLD FIL TAP); SEQ held,
        // the patterns; HOME: the keys play, no labels; the labels drawn over the keys, a key's meaning in a card
        press(1u << 6, 0.05);   // HOME
        shot(0.3);
        int labelled = 0;
        for (int k = 0; k < TESS_KEYS; k++) labelled += v.key_short[k][0] != 0;
        CHECK(!labelled, "HOME: the keys play, no labels (%d)", labelled);
        tess_core_buttons(1u << 5);   // GLO held
        shot(0.4);
        CHECK(!std::strcmp(v.key_short[1], "M1") && !std::strcmp(v.key_long[1], "T1 MUTE"), "GLO: F#3 is T1's MUTE (%s, %s)",
              v.key_short[1], v.key_long[1]);
        CHECK(!std::strcmp(v.key_short[0], "S1") && !std::strcmp(v.key_short[7], "ALL") && !std::strcmp(v.key_short[11], "FIL"),
              "GLO: F3 T1's SOLO, C4 ALL, E4 FILL (%s %s %s)", v.key_short[0], v.key_short[7], v.key_short[11]);
        q.draw(px.data(), v);
        if (!out.empty()) ppm(out + "/10c_tessera_glo_keys.ppm", px);
        {
            int lab = 0;   // (dim yellow over F#3, the first black key)
            for (int yy = 332; yy < 342; yy++)
                for (int xx = 36; xx < 300; xx++) lab += px[yy * Panel::W + xx] != 0xFF111111u && px[yy * Panel::W + xx] != 0xFF3B3007u;
            CHECK(lab > 10, "the labels drawn over the keys (%d pixels)", lab);
        }
        tess_core_buttons(0);
        shot(0.4);   // (GLO let go: its page, MIXER, whose keys are GLO's)
        CHECK(!std::strcmp(v.key_long[3], "T2 MUTE"), "MIXER: G#3 T2's MUTE (%s)", v.key_long[3]);
        press(1u << 6, 0.05);   // HOME
        run(q, 0.3);
        tess_core_buttons(1u << 9);   // SEQ held: the patterns
        shot(0.4);
        CHECK(!std::strcmp(v.key_short[0], "P1") && !std::strcmp(v.key_long[12], "PATTERN 8") && !v.key_short[1][0],
              "SEQ held: F3 P1, F4 PATTERN 8, F#3 nothing (%s %s)", v.key_short[0], v.key_long[12]);
        q.draw(px.data(), v);
        q.move(36 + 76 + 12 + (int)(0.5 * ((928 - 76 - 24) / 16)), 332 + 192);   // over F3 (right of OCT+ / OCT-): PATTERN 1
        q.draw(px.data(), v);
        if (!out.empty()) ppm(out + "/10d_tessera_seq_keys.ppm", px);
        {
            int card = 0;
            for (int yy = 400; yy < 470; yy++)
                for (int xx = 20; xx < 360; xx++) card += px[yy * Panel::W + xx] == 0xFFFFD020u;
            CHECK(card > 100, "PATTERN 1 in a card over F3 (%d)", card);
        }
        q.move(5, 5);
        tess_core_buttons(0);
        run(q, 0.2);
        tess_core_key_words(0);
        run(q, 0.2);
    }

    // ---- the screen's zones: FX's MIX column under the mouse
    {
        Panel z;
        z.set_shell(0);   // (COSMATI's own shell: the screen where the FM-1's is)
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
            for (int yy = 44 + 256; yy < 44 + 256 + 40; yy++)   // (HOVER HELP OFF: none)
                for (int xx = mx - 80; xx < mx + 80; xx++) card += px[yy * Panel::W + xx] == 0xFFFFD020u;
            CHECK(card == 0, "HOVER HELP OFF: no card under the screen (%d yellow pixels)", card);
            z.set_hover_help(true);
            z.draw(px.data(), v);
            card = 0;
            for (int yy = 44 + 256; yy < 44 + 256 + 40; yy++)   // (under the screen)
                for (int xx = mx - 80; xx < mx + 80; xx++) card += px[yy * Panel::W + xx] == 0xFFFFD020u;
            CHECK(card > 40, "the mouse over %s: a card with its value under the screen (%d yellow pixels)", m.name, card);
            if (!out.empty()) ppm(out + "/09_zone_hover.ppm", px);
            CHECK(z.wheel(mx, my, 2) && z.input.zones.size() == 1 && z.input.zones[0].action == TESS_ZA_WHEEL &&
                      z.input.zones[0].n == 2, "the wheel on %s: two notches up (its knob: two detents)", m.name);
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
            // G1: Shift, a drag a detent every 32 px, never sped up; Ctrl / Cmd-click, a box to type the value in
            z.input.zones.clear();
            z.set_mods(false, true);
            z.down(mx, my);
            for (int i = 1; i <= 4; i++) z.move(mx, my - 16 * i);
            z.up(mx, my - 64);
            turned = 0;
            for (auto &a : z.input.zones) turned += a.action == TESS_ZA_TURN_FINE ? a.n : 0;
            CHECK(turned == 2, "Shift-drag up 64 px: two fine detents (%d)", turned);
            CHECK(z.wheel(mx, my, 1) && z.input.zones.back().action == TESS_ZA_TURN_FINE, "Shift-wheel: a fine detent");
            run(z, 0.3);
            tess_core_view(&v);
            z.draw(px.data(), v);
            z.set_mods(true, false);
            CHECK(z.down(mx, my) && z.entry_open() && z.typing(), "Ctrl-click on %s: its box, typing", m.name);
            z.up(mx, my);
            z.set_mods(false, false);
            z.draw(px.data(), v);
            if (!out.empty()) ppm(out + "/09b_zone_type.ppm", px);
            for (char ch : std::string("7x")) z.text((uint32_t)ch);
            z.text(TESS_TEXT_BACK);
            z.text('5');
            CHECK(z.text(TESS_TEXT_OK) && !z.entry_open() && z.input.typed.size() == 1 && !std::strcmp(z.input.typed[0].text, "75"),
                  "\"7x\", Backspace, \"5\", Enter: \"75\" handed on (%s)", z.input.typed.empty() ? "-" : z.input.typed[0].text);
            run(z, 0.3);
            tess_core_view(&v);
            CHECK(std::string(v.zones[mix].value) == "75%", "%s typed: 75%% (%s)", m.name, v.zones[mix].value);
            z.set_mods(true, false);
            z.down(mx, my), z.up(mx, my);
            z.set_mods(false, false);
            z.text('1');
            CHECK(z.text(TESS_TEXT_CANCEL) && !z.entry_open() && z.input.typed.empty(), "Esc: the box shut, nothing set");
            z.set_mods(true, false);
            z.down(mx, my), z.up(mx, my);
            z.set_mods(false, false);
            CHECK(z.down(5, 5) && !z.entry_open(), "a click away shuts it");
            z.up(5, 5);
            // G2: the value's menu: COPY VALUE, the value turned, PASTE VALUE puts it back; RANDOM; a track's menu
            auto row_y = [&](int r) { return my + 4 + r * 28 + 10; };
            run(z, 0.3);
            tess_core_view(&v);
            z.draw(px.data(), v);
            std::string kept = v.zones[mix].value;
            z.latch(mx, my), z.down(mx + 20, row_y(2)), z.up(mx + 20, row_y(2));   // COPY VALUE
            z.latch(mx, my);
            CHECK(z.ctx_rows().size() == 9, "the menu again");
            z.down(5, 5), z.up(5, 5);
            CHECK(z.ctx_rows().empty() && z.input.typed.empty(), "a click away: shut, nothing done");
            z.wheel(mx, my, 6);
            run(z, 0.3);
            tess_core_view(&v);
            z.draw(px.data(), v);
            CHECK(std::string(v.zones[mix].value) != kept, "%s turned (%s, was %s)", m.name, v.zones[mix].value, kept.c_str());
            z.latch(mx, my), z.down(mx + 20, row_y(3)), z.up(mx + 20, row_y(3));   // PASTE VALUE
            run(z, 0.3);
            tess_core_view(&v);
            z.draw(px.data(), v);
            CHECK(std::string(v.zones[mix].value) == kept, "COPY VALUE, PASTE VALUE: %s back (%s)", kept.c_str(), v.zones[mix].value);
            z.latch(mx, my), z.down(mx + 20, row_y(4)), z.up(mx + 20, row_y(4));   // RANDOM
            CHECK(!z.input.zones.empty() && z.input.zones.back().action == TESS_ZA_RANDOM, "RANDOM asked for");
            run(z, 0.3);
            tess_core_view(&v);
            z.draw(px.data(), v);
            // G6: LOCK the value; DICE THE PAGE leaves it, PUT BACK undoes the roll; UNLOCK
            {
                std::string held = v.zones[mix].value;
                std::vector<std::string> before;
                for (uint32_t i = 0; i < v.zone_count; i++) before.push_back(v.zones[i].value);
                z.latch(mx, my);
                CHECK(z.ctx_rows()[6] == "LOCK" && z.ctx_rows()[8] == "PUT BACK", "G6: the value's menu: LOCK, DICE, PUT BACK");
                CHECK(!z.ctx_row_on(8), "PUT BACK: nothing to put back yet");
                z.down(mx + 20, row_y(6)), z.up(mx + 20, row_y(6));
                CHECK(!z.input.zones.empty() && z.input.zones.back().action == TESS_ZA_LOCK, "LOCK asked for");
                run(z, 0.3);
                tess_core_view(&v);
                CHECK(v.locked >> mix & 1u, "%s LOCKed (its bit in the view)", m.name);
                z.draw(px.data(), v);
                int lockpx = 0;
                for (int yy = 44 + 8 + m.y; yy < 44 + 8 + m.y + 12; yy++)
                    for (int xx = 292 + 8 + m.x + m.w - 10; xx < 292 + 8 + m.x + m.w; xx++) lockpx += px[yy * Panel::W + xx] == 0xFFFFD020u;
                CHECK(lockpx >= 30, "a padlock drawn on %s (%d)", m.name, lockpx);
                if (!out.empty()) ppm(out + "/09d_locked.ppm", px);
                int changed = 0;
                for (int roll = 0; roll < 4 && !changed; roll++)
                {
                    CHECK(z.shortcut('R') && z.input.zones.back().action == TESS_ZA_DICE && z.input.zones.back().n == 0,
                          "Ctrl+R: DICE the page");
                    run(z, 0.3);
                    tess_core_view(&v);
                    for (uint32_t i = 0; i < v.zone_count && i < before.size(); i++)
                        changed += (int)i != mix && v.zones[i].kind == TESS_ZONE_KNOB && before[i] != v.zones[i].value;
                }
                CHECK(changed > 0, "DICE: the page's other values rolled (%d)", changed);
                CHECK(std::string(v.zones[mix].value) == held, "..the LOCKed %s kept (%s, %s)", m.name, held.c_str(), v.zones[mix].value);
                CHECK(v.dice_back, "a roll to put back");
                z.draw(px.data(), v);
                z.latch(mx, my);
                CHECK(z.ctx_rows()[6] == "UNLOCK" && z.ctx_row_on(8), "the menu: UNLOCK, PUT BACK on");
                std::vector<std::string> rolled;
                for (uint32_t i = 0; i < v.zone_count; i++) rolled.push_back(v.zones[i].value);
                z.down(mx + 20, row_y(8)), z.up(mx + 20, row_y(8));
                run(z, 0.3);
                tess_core_view(&v);
                int back = 0, diff = 0;
                for (uint32_t i = 0; i < v.zone_count && i < rolled.size(); i++)
                    if (v.zones[i].kind == TESS_ZONE_KNOB && rolled[i] != v.zones[i].value) back++;
                CHECK(back > 0 && !v.dice_back, "PUT BACK: the last roll undone (%d values)", back);
                (void)diff;
                z.draw(px.data(), v);
                z.latch(mx, my), z.down(mx + 20, row_y(6)), z.up(mx + 20, row_y(6));   // UNLOCK
                run(z, 0.3);
                tess_core_view(&v);
                CHECK(!(v.locked >> mix & 1u), "UNLOCK: the bit gone");
                z.draw(px.data(), v);
            }
            tess_core_buttons(1u << 6);   // HOME: a track's menu
            run(z, 0.05);
            tess_core_buttons(0);
            run(z, 0.3);
            tess_core_view(&v);
            z.draw(px.data(), v);
            int tz = -1;
            for (uint32_t i = 0; i < v.zone_count; i++)
                if (v.zones[i].kind == TESS_ZONE_TRACK && v.zones[i].row == 2) tz = (int)i;
            CHECK(tz >= 0, "HOME: T3's zone");
            if (tz >= 0)
            {
                const tess_zone_t &t3 = v.zones[tz];
                int tx = 292 + 8 + t3.x + t3.w / 2, ty = 44 + 8 + t3.y + t3.h / 2;
                z.take_track_request();
                CHECK(z.latch(tx, ty) && z.ctx_rows().size() == 8 && z.ctx_rows()[0] == "T3" && z.ctx_rows()[4] == "MOVE UP",
                      "a right-click on T3: its menu (T3, COPY .. MOVE DOWN)");
                z.draw(px.data(), v);
                if (!out.empty()) ppm(out + "/09c_track_menu.ppm", px);
                int ry = ty + 4 + 4 * 28 + 10;
                if (ty + 8 * 28 + 8 > Panel::H - 8) ry = Panel::H - 8 - 8 * 28 - 8 + 4 + 4 * 28 + 10;
                z.down(tx + 20, ry), z.up(tx + 20, ry);
                Panel::TrackRequest rq = z.take_track_request();
                CHECK(rq.what == Panel::TR_UP && rq.track == 2, "..MOVE UP: T3 asked up (%d, T%d)", rq.what, rq.track + 1);
            }
        }
    }

    // ---- NOTES' roll (TESS_ZONE_ROLL): the mouse's place goes with each action; a press grabs, a drag turns, a still
    // release clicks, a right-click removes, the wheel turns the note under it; with Shift, the chord
    {
        Panel r;
        r.set_shell(1);
        tess_view_t fake = v;
        fake.zone_count = 1;
        fake.zones[0] = tess_zone_t{};
        fake.zones[0].x = 2, fake.zones[0].y = 132, fake.zones[0].w = 236, fake.zones[0].h = 66;
        fake.zones[0].kind = TESS_ZONE_ROLL, fake.zones[0].role = fake.zones[0].vrole = -1;
        r.draw(px.data(), fake);
        const int ox = 292 + 256 / 32, oy = 44 + 256 / 32, mx = ox + 50, my = oy + 170;
        r.input.zones.clear();
        r.down(mx, my), r.up(mx, my);
        CHECK(r.input.zones.size() == 2 && r.input.zones[0].action == TESS_ZA_GRAB && r.input.zones[1].action == TESS_ZA_CLICK &&
                  r.input.zones[1].z.x == 50 && r.input.zones[1].z.y == 170,
              "the roll: a press grabs, a still release clicks, at the place (%zu actions)", r.input.zones.size());
        r.input.zones.clear();
        r.down(mx, my);
        for (int i = 1; i <= 3; i++) r.move(mx + 1, my - 4 * i);
        r.up(mx + 1, my - 12);
        int turned = 0, clicks = 0;
        for (auto &a : r.input.zones) turned += a.action == TESS_ZA_TURN ? a.n : 0, clicks += a.action == TESS_ZA_CLICK;
        CHECK(turned == 2 && !clicks && r.input.zones.back().z.y == 170,
              "a drag up 12 px: two rows, from where it began, no click (%d, %d)", turned, clicks);
        r.input.zones.clear();
        r.input.buttons |= 1u << 16;   // (SHIFT held: panel.cpp BIT_SHIFT, latched or the Shift key)
        r.down(mx, my), r.move(mx, my + 6), r.up(mx, my + 6);
        r.input.buttons &= ~(1u << 16);
        CHECK(r.input.zones.size() == 3 && r.input.zones[1].action == TESS_ZA_TURN_CHORD && r.input.zones[1].n == -1 &&
                  r.input.zones[2].action == TESS_ZA_DROP,
              "Shift + a drag down: the chord a row down, then let go (DROP)");
        // G3: across, a stroke (PAINT at the place, every 3 px); Alt across, its length (a step every 14 px); Alt up, its
        // velocity (a detent every 6 px); a still click no DROP
        r.input.zones.clear();
        r.down(mx, my);
        for (int i = 1; i <= 10; i++) r.move(mx + 3 * i, my + (i & 1));
        r.up(mx + 30, my);
        int paints = 0, turns = 0;
        for (auto &a : r.input.zones) paints += a.action == TESS_ZA_PAINT, turns += a.action == TESS_ZA_TURN;
        CHECK(paints >= 8 && !turns && r.input.zones.back().action == TESS_ZA_DROP, "a drag across: a stroke (%d PAINTs), DROP", paints);
        {
            int last = -1;
            for (auto &a : r.input.zones) if (a.action == TESS_ZA_PAINT) last = a.z.x;
            CHECK(last == 50 + 30, "..each PAINT at the mouse's place (last x %d)", last);
        }
        r.input.zones.clear();
        r.set_mods(false, false, true);
        r.down(mx, my), r.move(mx + 10, my), r.move(mx + 30, my), r.up(mx + 30, my);
        int len = 0, vel = 0;
        for (auto &a : r.input.zones) len += a.action == TESS_ZA_LENGTH ? a.n : 0;
        CHECK(len == 2, "Alt + a drag 30 px across: two steps longer (%d)", len);
        r.input.zones.clear();
        r.down(mx, my), r.move(mx, my - 6), r.move(mx, my - 18), r.up(mx, my - 18);
        for (auto &a : r.input.zones) vel += a.action == TESS_ZA_VELOCITY ? a.n : 0;
        CHECK(vel == 3, "Alt + a drag 18 px up: velocity three detents (%d)", vel);
        r.set_mods(false, false, false);
        r.input.zones.clear();
        r.down(mx, my), r.up(mx, my);
        CHECK(r.input.zones.size() == 2 && r.input.zones[1].action == TESS_ZA_CLICK, "a still click: GRAB, CLICK, no DROP");
        r.input.zones.clear();
        CHECK(r.latch(mx, my) && r.input.zones.size() == 1 && r.input.zones[0].action == TESS_ZA_HELP &&
                  r.input.zones[0].z.x == 50, "a right-click on the roll: HELP there (remove)");
        r.input.zones.clear();
        CHECK(r.wheel(mx, my, 1) && r.input.zones.size() == 1 && r.input.zones[0].action == TESS_ZA_WHEEL &&
                  r.input.zones[0].z.y == 170, "the wheel on the roll: at the place");
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

    // EXPANDED: MODE in TESSERA's menu
    {
        static tess_tracks_t t2;
        tess_core_track_select(0);
        run(p, 0.2);
        Panel e;
        e.set_inks(inks);
        e.set_tracks((int)tess_core_tracks());
        e.set_version("0.1-beta2");
        e.set_size_pct(150);
        CHECK(e.width() == Panel::W, "FM-1 mode first (%d)", e.width());
        e.down(100, 25), e.up(100, 25);   // the menu; MODE's EXPANDED
        e.down(300, 164), e.up(300, 164);
        CHECK(e.take_mode_request() == 1 && e.expanded() && e.width() == Panel::W_EXPANDED,
              "MODE EXPANDED picked: the panel 1500 wide (%d)", e.width());
        CHECK(!e.menu_shown(), "the menu shuts on a pick");
        e.set_size_pct(100);
        e.down(100, 25), e.up(100, 25);   // SIZE: 150 at most
        e.down(300, 200), e.up(300, 200);
        CHECK(e.take_size_request() == 150, "EXPANDED's SIZE: 150 at the right");
        CHECK(Panel::size_fit(200, true) == 150 && Panel::size_fit(125, true) == 125 && Panel::size_fit(117, false) == 117 &&
                  Panel::size_fit(40, false) == Panel::PCT_MIN && Panel::size_fit(250, false) == 200,
              "SIZE: any whole %% from 75, to 150 in EXPANDED and 200 in FM-1 mode");
        e.set_size_pct(125);
        tess_core_view(&v);
        tess_core_tracks_view(&t2, TESS_TRACKS_W, TESS_TRACKS_H);
        std::vector<uint32_t> wide((size_t)Panel::W_MAX * Panel::H);
        e.draw(wide.data(), v, &t2);
        if (!out.empty()) ppm(out + "/16_expanded.ppm", wide, Panel::W_EXPANDED);
        int same = 0;
        for (int y = 0; y < TESS_TRACKS_H; y += 3)
            for (int x = 0; x < TESS_TRACKS_W; x += 3)
            {
                uint16_t q = t2.screen[y * TESS_TRACKS_W + x];
                uint32_t c = 0xFF000000u | ((q >> 11 & 31) * 255 / 31) << 16 | ((q >> 5 & 63) * 255 / 63) << 8 | (q & 31) * 255 / 31;
                same += wide[(52 + y) * Panel::W_EXPANDED + 568 + x] == c;
            }
        CHECK(same == 80 * 160, "TRACKS at 568,52, 1:1 (%d of %d pixels)", same, 80 * 160);
        CHECK(e.down(668, 152), "a click on TRACKS' row 4");
        e.up(668, 152);
        run(e, 0.2);
        CHECK(tess_core_track() == 3, "row 4 selects T4 (T%u)", tess_core_track() + 1);
        CHECK(e.wheel(978, 152, -1) && !e.input.tzones.empty() && e.input.tzones.back().z.kind == TESS_ZONE_LEVEL &&
                  e.input.tzones.back().action == TESS_ZA_TURN,
              "the wheel on T4's level turns it");
        run(e, 0.1);
        CHECK(!e.key(0x0F, true), "no bank switch in EXPANDED (Tab: nothing)");
        e.down(1396, 524);   // the top key, G5, at the right of the wide keybed
        CHECK(e.input.keys >> 26 & 1u, "the keys across the window: G5 at the right (%08X)", e.input.keys);
        e.up(1396, 524);
        int knobs = 0;   // KNOB 4's ring right of TRACKS
        for (int x = 1412 - 24; x <= 1412 + 24; x++) knobs += wide[100 * Panel::W_EXPANDED + x] == 0xFFFFD020u;
        CHECK(knobs >= 2, "KNOB 4 at x 1412 (%d)", knobs);
        {
            std::vector<uint32_t> big((size_t)Panel::W_EXPANDED * 5 / 4 * Panel::H * 5 / 4);
            Panel::scale(wide.data(), big.data(), Panel::W_EXPANDED, 125);
            int kept = 0, n = 0;
            for (int y = 0; y < Panel::H; y += 8)
                for (int x = 0; x < Panel::W_EXPANDED; x += 8, n++)   // (a multiple of 4: lands whole)
                    kept += big[(y * 5 / 4) * (Panel::W_EXPANDED * 5 / 4) + x * 5 / 4] == wide[y * Panel::W_EXPANDED + x] ||
                            wide[y * Panel::W_EXPANDED + x] != wide[y * Panel::W_EXPANDED + x + 1];
            CHECK(kept >= n * 98 / 100, "125 %%: the pixels land (%d of %d)", kept, n);
        }
        {   // G4: a browser's sound dragged onto a TRACKS row: that track loads it (the project kept and put back after)
            std::vector<uint8_t> kept(tess_core_tsp_max());
            kept.resize(tess_core_save(kept.data(), (uint32_t)kept.size()));
            auto screen_xy = [&](const char *name, uint32_t kind, int8_t row, int *zx, int *zy) {
                tess_core_view(&v);
                tess_core_tracks_view(&t2, TESS_TRACKS_W, TESS_TRACKS_H);
                e.draw(wide.data(), v, &t2);
                for (uint32_t i = 0; i < v.zone_count; i++)
                    if (!std::strcmp(v.zones[i].name, name) && v.zones[i].kind == kind && (row == -128 || (int8_t)(v.zones[i].row & 0xFF) == row))
                    {
                        *zx = 292 + 8 + v.zones[i].x + v.zones[i].w / 2, *zy = 44 + 8 + v.zones[i].y + v.zones[i].h / 2;
                        return true;
                    }
                return false;
            };
            int x, y;
            tess_core_track_select(0);
            for (int i = 0; i < 3 && !screen_xy("T1", TESS_ZONE_TRACK, -128, &x, &y); i++)
            {
                tess_core_buttons(1u << 6);
                run(e, 0.1);
                tess_core_buttons(0);
                run(e, 0.3);
            }
            if (screen_xy("T1", TESS_ZONE_TRACK, -128, &x, &y))
            {
                e.down(x, y), e.up(x, y), e.down(x, y), e.up(x, y);   // (T1's sounds)
                run(e, 0.3);
            }
            bool in = screen_xy("SOUND", TESS_ZONE_CELL, 1, &x, &y);
            CHECK(in, "G4: T1's browser in EXPANDED, a sound under the card");
            int tx = -1, ty = -1;
            for (uint32_t i = 0; i < t2.zone_count; i++)
                if (t2.zones[i].kind == TESS_ZONE_TRACK && t2.zones[i].row == 5)
                    tx = 568 + t2.zones[i].x + t2.zones[i].w / 3, ty = 52 + t2.zones[i].y + t2.zones[i].h / 2;
            CHECK(tx > 0, "TRACKS: T6's row");
            if (in && tx > 0)
            {
                e.input.tzones.clear();
                e.down(x, y);
                e.move(x + 3, y);
                CHECK(!e.input.tzones.size(), "pressed, 3 px: nothing yet");
                for (int i = 1; i <= 8; i++) e.move(x + (tx - x) * i / 8, y + (ty - y) * i / 8);
                run(e, 0.3);
                tess_core_view(&v);
                tess_core_tracks_view(&t2, TESS_TRACKS_W, TESS_TRACKS_H);
                e.draw(wide.data(), v, &t2);
                if (!out.empty()) ppm(out + "/16b_drag_sound.ppm", wide, Panel::W_EXPANDED);
                int frame = 0;
                for (int px2 = tx - 40; px2 < tx + 40; px2++) frame += wide[(ty - 30) * Panel::W_EXPANDED + px2] == 0xFFFFD020u;
                e.up(tx, ty);
                CHECK(e.input.tzones.size() == 1 && e.input.tzones[0].action == TESS_ZA_SOUND && e.input.tzones[0].z.row == 5,
                      "let go on T6's row: SOUND for T6 (%zu)", e.input.tzones.size());
                run(e, 0.3);
                tess_core_tracks_view(&t2, TESS_TRACKS_W, TESS_TRACKS_H);
                std::string a, b6;
                for (uint32_t i = 0; i < t2.zone_count; i++)
                    if (t2.zones[i].kind == TESS_ZONE_TRACK && t2.zones[i].row == 0) a = t2.zones[i].value;
                    else if (t2.zones[i].kind == TESS_ZONE_TRACK && t2.zones[i].row == 5) b6 = t2.zones[i].value;
                CHECK(!a.empty() && a == b6 && tess_core_track() == 0, "T6 loads T1's pick (%s, %s), T1 stays selected", a.c_str(),
                      b6.c_str());
                (void)frame;
                // G5: the browser's count clicked, FIND: a box; each key filters the list at once, Esc clears it
                if (screen_xy("FIND", TESS_ZONE_CELL, -128, &x, &y))
                {
                    e.input.typed.clear();
                    CHECK(e.down(x, y) && e.entry_open() && e.typing(), "G5: the count clicked: FIND's box, typing");
                    e.up(x, y);
                    for (char ch : std::string("pad")) e.text((uint32_t)ch);
                    CHECK(e.input.typed.size() == 3 && !std::strcmp(e.input.typed.back().text, "pad"), "FIND: each key handed on (%zu)",
                          e.input.typed.size());
                    run(e, 0.3);
                    tess_core_view(&v);
                    int found = -1;
                    for (uint32_t i = 0; i < v.zone_count; i++)
                        if (v.zones[i].kind == TESS_ZONE_CELL && !std::strcmp(v.zones[i].name, "FIND")) found = (int)i;
                    CHECK(found >= 0 && !std::strcmp(v.zones[found].value, "PAD"), "the core filters by PAD");
                    tess_core_tracks_view(&t2, TESS_TRACKS_W, TESS_TRACKS_H);
                    e.draw(wide.data(), v, &t2);
                    if (!out.empty()) ppm(out + "/16c_find.ppm", wide, Panel::W_EXPANDED);
                    e.input.typed.clear();
                    CHECK(e.text(TESS_TEXT_CANCEL) && !e.entry_open() && e.input.typed.size() == 1 && !e.input.typed[0].text[0],
                          "Esc: FIND cleared, the box shut");
                    run(e, 0.3);
                }
                else
                    CHECK(false, "G5: the browser's FIND cell");
                if (screen_xy("BACK", TESS_ZONE_KEY, -128, &x, &y))   // (BACK: T1's own sound again)
                {
                    e.down(x, y), e.up(x, y);
                    run(e, 0.5);
                }
            }
            tess_core_buttons(1u << 6);
            run(e, 0.1);
            tess_core_buttons(0);
            run(e, 0.3);
            tess_core_load(kept.data(), (uint32_t)kept.size());
            tess_core_track_select(3);   // (as the checks before left it)
            run(e, 0.2);
        }
        {   // G7: A / B in EXPANDED's header: a click asks for A, a right-click stores B, MORPH dragged asks its place
            e.take_snap_request();
            tess_core_view(&v);
            tess_core_tracks_view(&t2, TESS_TRACKS_W, TESS_TRACKS_H);
            e.draw(wide.data(), v, &t2);
            int bx = -1, m0 = -1, m1 = -1;
            for (int x = 800; x < Panel::W_EXPANDED; x++)
            {
                int p = e.snap_part_at(x, 24);
                if (p == 1 && bx < 0) bx = x;
                if (p == 2 && m0 < 0) m0 = x;
                if (p == 2) m1 = x;
            }
            CHECK(e.snap_part_at(842 + 17, 24) == 0 && bx > 0 && m0 > 0 && e.skin_switch_x() > bx + 34,
                  "G7: A, MORPH and B in the header, left of the skin's switch (B %d, switch %d)", bx, e.skin_switch_x());
            e.down(842 + 17, 24), e.up(842 + 17, 24);
            Panel::SnapRequest r = e.take_snap_request();
            CHECK(r.what == Panel::SNAP_CLICK && r.slot == 0, "a click on A: A asked for (%d %d)", r.what, r.slot);
            e.latch(bx + 17, 24);
            r = e.take_snap_request();
            CHECK(r.what == Panel::SNAP_STORE && r.slot == 1, "a right-click on B: stored in B (%d %d)", r.what, r.slot);
            e.down((m0 + m1) / 2, 24), e.up((m0 + m1) / 2, 24);
            CHECK(e.take_snap_request().what == Panel::SNAP_NONE, "MORPH with A or B empty: nothing asked");
            Panel::SnapState st;
            st.has[0] = st.has[1] = true, st.active = 0;
            e.set_snaps(st);
            const int rail0 = 842 + 34 + 92, rail1 = bx - 10;
            e.down(rail0, 24);
            r = e.take_snap_request();
            CHECK(r.what == Panel::SNAP_MORPH && r.value == 0, "MORPH pressed at its A end: 0 (%d)", r.value);
            for (int i = 1; i <= 6; i++) e.move(rail0 + (rail1 - rail0) * i / 12, 24);
            r = e.take_snap_request();
            CHECK(r.what == Panel::SNAP_MORPH && r.value >= 45 && r.value <= 55, "..dragged half way: about 50 (%d)", r.value);
            e.up(rail0 + (rail1 - rail0) / 2, 24);
            st.active = -1, st.morph = r.value;
            e.set_snaps(st);
            e.draw(wide.data(), v, &t2);
            int handle = 0;
            for (int x = rail0; x < rail1; x++) handle += wide[24 * Panel::W_EXPANDED + x] == 0xFFFFD020u;
            CHECK(handle >= 6, "MORPH's handle drawn (%d)", handle);
            if (!out.empty()) ppm(out + "/16d_ab.ppm", wide, Panel::W_EXPANDED);
            e.note("Clip saved: Tessera 2026-11-26 2204 still tide.mid");   // (a toast, G9: EXPANDED lets it run on)
            e.draw(wide.data(), v, &t2);
            if (!out.empty()) ppm(out + "/16e_toast.ppm", wide, Panel::W_EXPANDED);
            CHECK(wide[(44 + 256 + 3) * Panel::W_EXPANDED + 292 + 300] != wide[(44 + 256 + 3) * Panel::W_EXPANDED + 292 + 600],
                  "EXPANDED: a long toast runs on under TRACKS");
            for (uint32_t st = 0; st < tess_core_gen_styles(); st++)   // (G10: a piece with a CHORD track, the keys in its scale)
                if (!std::strcmp(tess_core_gen_style(st), "SHORE")) tess_core_generate(st, 1367, 100, 0);
            tess_core_transport(1);
            run(e, 3.0);
            tess_core_view(&v);
            tess_core_tracks_view(&t2, TESS_TRACKS_W, TESS_TRACKS_H);
            for (int f = 0; f < 130; f++) e.draw(wide.data(), v, &t2);   // (the toast gone)
            if (!out.empty()) ppm(out + "/16f_scale_keys.ppm", wide, Panel::W_EXPANDED);
            CHECK(v.key_chord[0] && v.key_dim && v.key_root, "SHORE: its chord named (%s), keys out of the scale dim, the root's marked",
                  v.key_chord);
            tess_core_transport(0);
        }
        e.set_shell(1);
        CHECK(e.width() == Panel::W && !e.expanded(), "an FM-1 shell: FM-1 mode (%d)", e.width());
        e.set_shell(0);
        CHECK(e.width() == Panel::W_EXPANDED, "TESSERA's shell again: EXPANDED (%d)", e.width());
    }

    {   // G4: MATRIX, a cell dragged onto another: LINK on that one with the first's row; a still press: a click
        Panel m;
        m.set_shell(0);
        std::vector<uint8_t> kept(tess_core_tsp_max());
        kept.resize(tess_core_save(kept.data(), (uint32_t)kept.size()));
        tess_core_track_select(0);
        auto cell_xy = [&](const char *name, int *zx, int *zy, int16_t *row) {
            tess_core_view(&v);
            m.draw(px.data(), v);
            for (uint32_t i = 0; i < v.zone_count; i++)
                if (!std::strcmp(v.zones[i].name, name) && v.zones[i].kind == TESS_ZONE_CELL)
                {
                    *zx = 292 + 8 + v.zones[i].x + v.zones[i].w / 2, *zy = 44 + 8 + v.zones[i].y + v.zones[i].h / 2;
                    if (row) *row = v.zones[i].row;
                    return true;
                }
            return false;
        };
        int fx, fy, tx, ty;
        int16_t frow = 0;
        for (int i = 0; i < 16 && !cell_xy("T1>T1", &fx, &fy, &frow); i++)
        {
            tess_core_buttons(1u << 3);   // (LFO: its pages, MATRIX among them)
            run(m, 0.05);
            tess_core_buttons(0);
            run(m, 0.2);
        }
        bool in = cell_xy("T1>T1", &fx, &fy, &frow) && cell_xy("T1>T2", &tx, &ty, nullptr);
        CHECK(in, "G4: MATRIX through the panel, T1>T1 and T1>T2");
        if (in)
        {
            m.input.zones.clear();
            CHECK(!m.down(fx, fy) && m.input.zones.empty(), "a press on a MATRIX cell: nothing yet");
            for (int i = 1; i <= 6; i++) m.move(fx + (tx - fx) * i / 6, fy + (ty - fy) * i / 6);
            m.draw(px.data(), v);
            if (!out.empty()) ppm(out + "/10g_tessera_matrix_drag.ppm", px);
            m.up(tx, ty);
            CHECK(m.input.zones.size() == 1 && m.input.zones[0].action == TESS_ZA_LINK &&
                      !std::strcmp(m.input.zones[0].z.name, "T1>T2") && m.input.zones[0].n == (int8_t)(frow & 0xFF),
                  "dragged onto T1>T2 and let go: LINK there, from T1's row (%zu)", m.input.zones.size());
            m.input.zones.clear();
            m.down(tx, ty), m.up(tx, ty);   // (another cell: not half a double-click)
            CHECK(m.input.zones.size() == 1 && m.input.zones[0].action == TESS_ZA_CLICK, "a still press: a click on let go");
            m.input.zones.clear();
        }
        tess_core_load(kept.data(), (uint32_t)kept.size());
        tess_core_buttons(1u << 6);
        run(m, 0.1);
        tess_core_buttons(0);
        run(m, 0.3);
    }

    {   // a toast (Cosmati audit G9) in TESSERA's shell: a lit tag under the screen in place of its words, an outline
        // in its last third, then gone
        Panel n;
        n.set_shell(0);
        n.draw(px.data(), v);
        const int tx = 292 + 8, ty = 44 + 256 + 3 + 21;   // (inside the tag, under its words)
        uint32_t calm = px[ty * Panel::W + tx];
        n.note("Loaded Still Tide");
        n.draw(px.data(), v);
        if (!out.empty()) ppm(out + "/toast.ppm", px);
        uint32_t lit = px[ty * Panel::W + tx];
        CHECK(lit != calm, "a toast: a lit tag under the screen (%08x, was %08x)", lit, calm);
        const int ey = 44 + 256 + 3;   // (the tag's top edge: lit while it shows)
        uint32_t edge_calm = calm;
        int frames = 1;
        bool outline = false;
        while (frames < 200 && px[ey * Panel::W + tx] != edge_calm)
        {
            n.draw(px.data(), v);
            frames++;
            outline |= px[ey * Panel::W + tx] != edge_calm && px[ty * Panel::W + tx] == calm;
        }
        CHECK(frames >= 110 && frames <= 130 && outline, "the toast stays 4 s, its end an outline (%d frames)", frames);
        n.note("Saved");   // the mouse on the screen, HOVER HELP OFF: no card, the toast stays
        n.move(292 + 8 + 30, 44 + 8 + 40);
        n.draw(px.data(), v);
        CHECK(px[ty * Panel::W + tx] == lit, "HOVER HELP OFF: no card over the toast");
        n.set_hover_help(true);   // ON: its card under the screen, the row's words and the toast gone round it
        n.draw(px.data(), v);
        if (!out.empty()) ppm(out + "/toast_hover.ppm", px);
        int words = 0;
        for (int y = 44 + 256 + 3; y < 44 + 256 + 29; y++)
            for (int x = 292 + 200; x < 292 + 256; x++) words += px[y * Panel::W + x] != px[(44 + 256 + 3) * Panel::W + 292 + 255];
        CHECK(words == 0, "a card up: PAGE and the toast not cut short beside it (%d pixels)", words);
    }

    {   // the sound browser in TESSERA's shell, through the panel: a sound clicked loads it, BACK clicked takes it back
        Panel b;
        b.set_shell(0);
        tess_core_track_select(0);
        tess_core_view(&v);
        b.draw(px.data(), v);
        auto zone_xy = [&](const char *name, uint32_t kind, int *zx, int *zy) {
            tess_core_view(&v);
            b.draw(px.data(), v);
            for (uint32_t i = 0; i < v.zone_count; i++)
                if (!std::strcmp(v.zones[i].name, name) && v.zones[i].kind == kind)
                {
                    *zx = 292 + 8 + v.zones[i].x + v.zones[i].w / 2, *zy = 44 + 8 + v.zones[i].y + v.zones[i].h / 2;
                    return true;
                }
            return false;
        };
        int x, y;
        for (int i = 0; i < 3 && !zone_xy("T1", TESS_ZONE_TRACK, &x, &y); i++)   // (HOME, from wherever the test was)
        {
            tess_core_buttons(1u << 6);
            run(b, 0.1);
            tess_core_buttons(0);
            run(b, 0.3);
        }
        if (zone_xy("T1", TESS_ZONE_TRACK, &x, &y))
        {
            b.down(x, y), b.up(x, y), b.down(x, y), b.up(x, y);   // (a double-click: T1's sounds)
            run(b, 0.3);
        }
        bool in = zone_xy("SOUND", TESS_ZONE_CELL, &x, &y);
        CHECK(in, "the panel: HOME's T1 double-clicked opens the browser");
        int sx = x, sy = y + 24;   // (a sound under the card)
        b.down(sx, sy), b.up(sx, sy);
        run(b, 1.0);
        bool back = zone_xy("BACK", TESS_ZONE_KEY, &x, &y);
        CHECK(back, "a sound loaded: BACK on the screen");
        if (back)
        {
            b.down(x, y);
            run(b, 0.1);
            b.up(x, y);
            run(b, 0.3);
            CHECK(!zone_xy("SOUND", TESS_ZONE_CELL, &x, &y), "the panel: BACK clicked leaves the browser");
        }
        tess_core_view(&v);
        b.draw(px.data(), v);
    }
    // Cosmati audit G12, the arrows: Left / Right the pages, Shift+Left / Right the sections, Up / Down a list's rows,
    // Shift+Up / Down the tracks
    {
        Panel q;
        q.set_tracks((int)tess_core_tracks());
        auto frames = [&](int n) { for (int i = 0; i < n; i++) { tess_core_view(&v); q.draw(px.data(), v); } };
        auto press = [&](uint32_t bit) { tess_core_buttons(bit); run(q, 0.1); tess_core_buttons(0); run(q, 0.3); frames(1); };
        tess_core_track_select(0);
        press(TESS_BTN_SECTION(TESS_SEC_FX));
        uint64_t h0 = screen_hash(v);
        CHECK(q.key(Panel::KEY_RIGHT, true), "Right: taken");
        q.key(Panel::KEY_RIGHT, false);
        run(q, 0.3);
        frames(1);
        CHECK(screen_hash(v) != h0 && (v.lit_buttons & TESS_BTN_SECTION(TESS_SEC_FX)), "Right: FX's next page");
        q.key(Panel::KEY_LEFT, true);
        q.key(Panel::KEY_LEFT, false);
        run(q, 0.3);
        frames(1);
        CHECK(screen_hash(v) == h0, "Left: back to FX's first page");
        q.key(0x2A, true);   // Shift
        CHECK(q.key(Panel::KEY_RIGHT, true) && (q.input.buttons & TESS_BTN_SECTION(TESS_SEC_SHAPE)),
              "Shift+Right: SHAPE's button held (%08x)", q.input.buttons);
        run(q, 0.1);
        CHECK(q.key(Panel::KEY_RIGHT, false) && !(q.input.buttons & TESS_BTN_SECTION(TESS_SEC_SHAPE)), "..let go with the arrow");
        run(q, 0.3);
        frames(1);
        CHECK(v.lit_buttons & TESS_BTN_SECTION(TESS_SEC_SHAPE), "Shift+Right from FX: SHAPE");
        CHECK(q.key(Panel::KEY_DOWN, true) && q.input.track == 1, "Shift+Down: the next track (%d)", q.input.track);
        q.key(Panel::KEY_DOWN, false);
        run(q, 0.3);
        CHECK(tess_core_track() == 1, "..T2 selected (%u)", tess_core_track());
        CHECK(q.key(Panel::KEY_UP, true) && q.input.track == 0, "Shift+Up: back to T1");
        q.key(Panel::KEY_UP, false);
        q.key(0x2A, false);
        run(q, 0.3);
        frames(1);
        press(TESS_BTN_SECTION(TESS_SEC_SETUP));   // (SETTINGS first, a list: SETUP again, TEMPO, a knob page:
        press(TESS_BTN_SECTION(TESS_SEC_SETUP));   //  Up turns SELECT as ever)
        q.input.enc[0] = 0;
        CHECK(q.key(Panel::KEY_UP, true) && q.input.enc[0] == 1, "Up on a knob page: SELECT up, as before");
        q.key(Panel::KEY_UP, false);
        q.input.enc[0] = 0;
        q.input.changed = false;
        press(1u << 6);   // HOME
    }
    // Cosmati: the knobs named by the page, sideways scroll a page (stopping at the ends), the mouse's back / forward
    {
        Panel q;
        q.set_tracks((int)tess_core_tracks());
        auto frames = [&](int n) { for (int i = 0; i < n; i++) { tess_core_view(&v); q.draw(px.data(), v); } };
        tess_core_buttons(TESS_BTN_SECTION(TESS_SEC_FX));   // (FX's section: FX 1 .. SEND FX 3)
        run(q, 0.1);
        tess_core_buttons(0);
        run(q, 0.3);
        frames(1);
        const tess_zone_t *k1 = q.knob_zone(3);
        CHECK(k1 && k1->value[0], "FX 1: KNOB 1 named by the page (%s %s)", k1 ? k1->name : "-", k1 ? k1->value : "");
        if (!out.empty()) ppm(out + "/30_knob_names.ppm", px);
        if (!out.empty())   // SETUP: its first page SETTINGS, the menu's list over SETUP's tabs
        {
            tess_core_buttons(TESS_BTN_SECTION(TESS_SEC_SETUP));
            run(q, 0.1);
            tess_core_buttons(0);
            run(q, 0.3);
            frames(1);
            ppm(out + "/30b_setup_settings.ppm", px);
            tess_core_buttons(TESS_BTN_SECTION(TESS_SEC_FX));
            run(q, 0.1);
            tess_core_buttons(0);
            run(q, 0.3);
            frames(1);
        }
        uint64_t h0 = screen_hash(v);
        CHECK(!q.hwheel(-1), "sideways left on the first page: nothing (it stops)");
        CHECK(q.hwheel(1), "sideways right: the next page");
        run(q, 0.3);
        frames(1);
        uint64_t h1 = screen_hash(v);
        CHECK(h1 != h0, "sideways right: the page changed");
        CHECK(!q.hwheel(1), "sideways again at once: nothing (one swipe, one page)");
        int steps = 1;
        for (frames(9); steps < 16 && q.hwheel(1); steps++) { run(q, 0.3); frames(9); }
        CHECK(steps > 1 && steps < 16 && !q.hwheel(1), "sideways right to the last page, then nothing (it stops; %d pages)", steps + 1);
        uint64_t h2 = screen_hash(v);
        CHECK(q.hwheel(-1), "sideways left: the page before");
        run(q, 0.3);
        frames(1);
        CHECK(screen_hash(v) != h2, "sideways left: back a page");
        int tab = -1;   // the wheel over the footer's tabs: its pages, down the next one, up the one before
        for (uint32_t i = 0; i < v.zone_count; i++)
            if (v.zones[i].kind == TESS_ZONE_TAB && v.zones[i].row == 0) tab = (int)i;
        CHECK(tab >= 0, "the footer's tab of the page shown is a zone");
        if (tab >= 0)
        {
            const tess_zone_t t = v.zones[tab];
            const int tx = 292 + 8 + t.x + t.w / 2, ty = 44 + 8 + t.y + t.h / 2;
            uint64_t h3 = screen_hash(v);
            frames(9);
            CHECK(q.wheel(tx, ty, 1), "the wheel up over the tabs: the page before");
            run(q, 0.3);
            frames(9);
            CHECK(screen_hash(v) != h3, "..and the page changed");
            CHECK(q.wheel(tx, ty, -1), "the wheel down over the tabs: the next page");
            run(q, 0.3);
            frames(1);
            CHECK(screen_hash(v) == h3, "..back where it was");
        }
        CHECK(q.side_button(false, true) && (q.input.buttons >> 12 & 1u), "the mouse's back button holds BACK (OCT-)");
        CHECK(q.side_button(false, false) && !(q.input.buttons >> 12 & 1u), "and lets it go");
        CHECK(q.side_button(true, true) && (q.input.buttons >> 13 & 1u), "forward: OK (OCT+)");
        q.side_button(true, false);
        run(q, 0.3);
        tess_core_buttons(1u << 6);   // (HOME)
        run(q, 0.1);
        tess_core_buttons(0);
        run(q, 0.3);
    }

    std::printf(fails ? "panel test: %d FAILED\n" : "panel test: ok\n", fails);
    return fails ? 1 : 0;
}
