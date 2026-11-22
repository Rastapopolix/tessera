/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The screen's zones (tess_view_t.zones, tess_core_zone_do: a mouse on the screen), without a window: the core as the
 * plugin builds it (TESS_HAS_POINTER), its buttons tapped as the panel does.
 *   - HOME: a zone a track; a click selects it;
 *   - a page (FX): a zone a knob, KNOB 1..4 left to right; a turn moves that value, a double-click puts its default
 *     back; a zone from a screen no longer shown does nothing;
 *   - a list page (SCL): a zone a row, KNOB 1 the row and KNOB 2 its value; the wheel on a row turns that row's value
 *     and no other; a double-click on a row with a list behind it opens it (Scale: SCALES), whose OK goes back;
 *   - a whole list (Quantize's): its rows; a double-click takes a row; two clicks at once (row 6, then row 1 and OK)
 *     end on row 1: a row is reached detent by detent, never accelerated;
 *   - TRACKS, the second screen (tess_core_tracks_view, EXPANDED mode): 480 x 240, a row, M, S and a level zone a
 *     track, M, S and the levels before the rows; a row's click selects its track, M mutes and unmutes (stopped: at
 *     once), S latches a solo that stays, the wheel on the level turns it; 128 x 64 (Andamento's screen) draws too.
 * usage: tessera-zones-test [tracks.ppm] [mosaic.ppm] (the TRACKS page as a picture, and in MOSAIC) */
#include <cstdio>
#include <cstring>
#include <string>

#include "tess_core.h"

static int fails = 0;
#define CHECK(c, ...)                          \
    do                                         \
    {                                          \
        if (!(c))                              \
        {                                      \
            std::printf("FAIL: " __VA_ARGS__); \
            std::printf("\n");                 \
            fails++;                           \
        }                                      \
    } while (0)

static float L[1024], R[1024];
static tess_view_t v;
enum { B_FX = 0, B_SEL = 1, B_HOME = 6, B_OCTDN = 12 };

static void run(int blocks)   // 10 ms blocks
{
    for (int b = 0; b < blocks; b++)
        tess_core_render(L, R, nullptr, tess_core_rate() / 100);
    tess_core_view(&v);
}
static void tap(int b)
{
    tess_core_buttons(1u << b);
    run(5);
    tess_core_buttons(0);
    run(20);
}
static int count(uint32_t kind)
{
    int n = 0;
    for (uint32_t i = 0; i < v.zone_count; i++) n += v.zones[i].kind == kind;
    return n;
}
static int find(const char *name, uint32_t kind = 0)
{
    for (uint32_t i = 0; i < v.zone_count; i++)
        if (!std::strcmp(v.zones[i].name, name) && (!kind || v.zones[i].kind == kind)) return (int)i;
    return -1;
}
static void dump(const char *what)
{
    std::printf("%s: %u zones\n", what, v.zone_count);
    for (uint32_t i = 0; i < v.zone_count; i++)
    {
        const tess_zone_t &z = v.zones[i];
        std::printf("  %u %3u,%3u %3ux%-3u knob %d value %d row %d  %s %s\n", z.kind, z.x, z.y, z.w, z.h, z.role, z.vrole,
                    z.row, z.name, z.value);
    }
}
static void act(int i, uint32_t a, int n = 0, int blocks = 30)
{
    if (i < 0) return;
    tess_zone_t z = v.zones[i];
    tess_core_zone_do(&z, a, n);
    run(blocks);
}

static tess_tracks_t tv;
static int tfind(uint32_t kind, int track)
{
    for (uint32_t i = 0; i < tv.zone_count; i++)
        if (tv.zones[i].kind == kind && tv.zones[i].row == track) return (int)i;
    return -1;
}
static void tact(uint32_t kind, int track, uint32_t a, int n = 0)
{
    int i = tfind(kind, track);
    if (i < 0) return;
    tess_zone_t z = tv.zones[i];
    tess_core_tracks_do(&z, a, n);
    run(10);
    tess_core_tracks_view(&tv, TESS_TRACKS_W, TESS_TRACKS_H);
}
static std::string tval(uint32_t kind, int track)
{
    int i = tfind(kind, track);
    return i < 0 ? "?" : tv.zones[i].value;
}
static void tracks(const char *ppm)
{
    int nt = (int)tess_core_tracks();
    tess_core_tracks_view(&tv, TESS_TRACKS_W, TESS_TRACKS_H);
    CHECK(tv.w == TESS_TRACKS_W && tv.h == TESS_TRACKS_H, "TRACKS drawn at 480 x 240 (%u x %u)", tv.w, tv.h);
    int rows = 0, ms = 0, lit = 0;
    bool order = true, seen_row = false;
    for (uint32_t i = 0; i < tv.zone_count; i++)
    {
        uint32_t k = tv.zones[i].kind;
        rows += k == TESS_ZONE_TRACK;
        ms += k == TESS_ZONE_MUTE || k == TESS_ZONE_SOLO || k == TESS_ZONE_LEVEL;
        if (k == TESS_ZONE_TRACK) seen_row = true;
        else if (seen_row) order = false;
    }
    CHECK(rows == nt && ms == 3 * nt, "TRACKS: a row, M, S and level a track (%d rows, %d others, %d tracks)", rows, ms,
          nt);
    CHECK(order, "TRACKS: M, S and the levels before the rows");
    for (uint32_t i = 0; i < tv.w * tv.h; i++) lit += tv.screen[i] != tv.screen[0];
    CHECK(lit > 3000, "TRACKS has something drawn (%d pixels)", lit);
    if (ppm)
        if (FILE *f = std::fopen(ppm, "wb"))
        {
            std::fprintf(f, "P6\n%u %u\n255\n", tv.w, tv.h);
            for (uint32_t i = 0; i < tv.w * tv.h; i++)
            {
                uint16_t p = tv.screen[i];
                unsigned char rgb[3] = {(unsigned char)((p >> 11 & 31) * 255 / 31), (unsigned char)((p >> 5 & 63) * 255 / 63),
                                        (unsigned char)((p & 31) * 255 / 31)};
                std::fwrite(rgb, 1, 3, f);
            }
            std::fclose(f);
        }

    tact(TESS_ZONE_TRACK, 2, TESS_ZA_CLICK);
    CHECK(tess_core_track() == 2, "a click on row 3 selects T3 (T%u)", tess_core_track() + 1);
    tact(TESS_ZONE_MUTE, 1, TESS_ZA_CLICK);
    CHECK(tval(TESS_ZONE_MUTE, 1) == "ON", "M mutes T2 (stopped: at once; %s)", tval(TESS_ZONE_MUTE, 1).c_str());
    tact(TESS_ZONE_MUTE, 1, TESS_ZA_CLICK);
    CHECK(tval(TESS_ZONE_MUTE, 1) == "OFF", "M again unmutes it (%s)", tval(TESS_ZONE_MUTE, 1).c_str());
    tact(TESS_ZONE_SOLO, 0, TESS_ZA_CLICK);
    run(30);
    tess_core_tracks_view(&tv, TESS_TRACKS_W, TESS_TRACKS_H);
    CHECK(tval(TESS_ZONE_SOLO, 0) == "ON" && tval(TESS_ZONE_SOLO, 1) == "OFF", "S latches T1's solo, and it stays (%s)",
          tval(TESS_ZONE_SOLO, 0).c_str());
    tact(TESS_ZONE_SOLO, 0, TESS_ZA_CLICK);
    CHECK(tval(TESS_ZONE_SOLO, 0) == "OFF", "S again lets it go (%s)", tval(TESS_ZONE_SOLO, 0).c_str());
    std::string before = tval(TESS_ZONE_LEVEL, 3);
    tact(TESS_ZONE_LEVEL, 3, TESS_ZA_TURN, -12);
    CHECK(tval(TESS_ZONE_LEVEL, 3) != before, "the wheel on T4's level turns it (%s -> %s)", before.c_str(),
          tval(TESS_ZONE_LEVEL, 3).c_str());
    tact(TESS_ZONE_LEVEL, 3, TESS_ZA_TURN, 12);
    CHECK(tval(TESS_ZONE_LEVEL, 3) == before, "and back (%s)", tval(TESS_ZONE_LEVEL, 3).c_str());

    tess_core_tracks_view(&tv, 128, 64);   // Andamento's screen: 21 characters a row, no zones
    lit = 0;
    for (uint32_t i = 0; i < tv.w * tv.h; i++) lit += tv.screen[i] != tv.screen[0];
    CHECK(tv.w == 128 && tv.h == 64 && lit > 300, "TRACKS at 128 x 64 (%u x %u, %d pixels)", tv.w, tv.h, lit);
}

static void hold(int b, int blocks)
{
    tess_core_buttons(1u << b);
    run(blocks);
    tess_core_buttons(0);
    run(20);
}
static std::string ink_name()   // the ink now, by its name (tess_core_ink_at)
{
    const char *n = nullptr;
    uint32_t rgb[4];
    tess_core_ink_at(v.ink, &n, rgb);
    return n ? n : "?";
}
static void settings()
{
    tap(B_HOME);
    hold(B_HOME, 120);   // (HOME held: SETTINGS)
    int rows = count(TESS_ZONE_ROW);
    CHECK(rows >= 8, "SETTINGS: a zone a row (%d)", rows);
    // the wheel moves the list, a row a notch (down: down the list), and changes no value
    int c = find("COLOR", TESS_ZONE_ROW);
    std::string ink = c >= 0 ? v.zones[c].value : "?";
    act(c, TESS_ZA_WHEEL, -3);
    int g = find("GROUND", TESS_ZONE_ROW);
    act(g, TESS_ZA_WHEEL, -6, 60);   // (past the screen's last row: the list scrolls)
    c = find("COLOR", TESS_ZONE_ROW);
    CHECK(c < 0, "the wheel scrolls SETTINGS past its first screen (COLOR off it)");
    bool later = false;
    for (uint32_t i = 0; i < v.zone_count; i++) later |= v.zones[i].row >= 9;
    CHECK(later, "the wheel brings the rows below into view");
    act(find("LIGHTS", TESS_ZONE_ROW) >= 0 ? find("LIGHTS", TESS_ZONE_ROW) : 0, TESS_ZA_WHEEL, 20, 80);   // back to the top
    c = find("COLOR", TESS_ZONE_ROW);
    CHECK(c >= 0 && ink == v.zones[c].value, "the wheel back to the top; COLOR unchanged (%s, %s)", ink.c_str(),
          c >= 0 ? v.zones[c].value : "?");

    // COLOR's sheet (KNOB 2 on COLOR): a zone an ink; a click picks it, the wheel steps through them
    tess_core_enc(4, 1);   // (KNOB 2: the sheet open, the next ink)
    run(30);
    int inks = count(TESS_ZONE_ROW);
    CHECK(inks >= 9 && find("COLOR", TESS_ZONE_ROW) < 0, "the INK sheet: a zone an ink (%d)", inks);
    int w = find("WHITE", TESS_ZONE_ROW), m = find("MOSAIC", TESS_ZONE_ROW);
    CHECK(w >= 0 && m >= 0, "the INK sheet: WHITE and MOSAIC among them");
    act(m, TESS_ZA_CLICK, 0, 60);
    std::string now = ink_name();
    CHECK(now == "MOSAIC", "a click on MOSAIC picks it (%s)", now.c_str());
    act(find("WHITE", TESS_ZONE_ROW), TESS_ZA_CLICK, 0, 60);
    now = ink_name();
    CHECK(now == "WHITE", "a click on WHITE picks it (%s)", now.c_str());
    act(find("WHITE", TESS_ZONE_ROW), TESS_ZA_WHEEL, -1, 30);
    std::string next = ink_name();
    CHECK(next != "WHITE", "the wheel down on the sheet: the next ink (%s)", next.c_str());
    hold(B_HOME, 120);   // (SETTINGS shut)
}

// MOSAIC with 8 tracks: tracks 5..8 a step lighter than 1..4 (T1 and T5 told apart on TRACKS), one ink alike
static uint16_t row_ink(int track)   // the colour of a track's number on TRACKS (its row's first lit pixel)
{
    int i = tfind(TESS_ZONE_TRACK, track);
    if (i < 0) return 0;
    const tess_zone_t &z = tv.zones[i];
    for (uint32_t x = z.x; x < (uint32_t)(z.x + z.w) && x < tv.w; x++)
        for (uint32_t y = z.y; y < (uint32_t)(z.y + z.h) && y < tv.h; y++)
            if (uint16_t c = tv.screen[y * tv.w + x]) return c;
    return 0;
}
static void mosaic(const char *ppm)
{
    if (tess_core_tracks() < 8) return;
    CHECK(tess_core_ink("MOSAIC"), "the ink MOSAIC");
    run(30);
    tess_core_tracks_view(&tv, TESS_TRACKS_W, TESS_TRACKS_H);
    uint16_t t1 = row_ink(0), t5 = row_ink(4), t2 = row_ink(1), t6 = row_ink(5);
    std::printf("MOSAIC on TRACKS: T1 %04x T5 %04x, T2 %04x T6 %04x\n", t1, t5, t2, t6);
    CHECK(t1 && t5 && t1 != t5 && t2 != t6, "MOSAIC: tracks 5..8 not tracks 1..4's colours");
    if (ppm)
        if (FILE *f = std::fopen(ppm, "wb"))
        {
            std::fprintf(f, "P6\n%u %u\n255\n", tv.w, tv.h);
            for (uint32_t k = 0; k < tv.w * tv.h; k++)
            {
                uint16_t c = tv.screen[k];
                unsigned char rgb[3] = {(unsigned char)((c >> 11) * 255 / 31), (unsigned char)(((c >> 5) & 63) * 255 / 63),
                                        (unsigned char)((c & 31) * 255 / 31)};
                std::fwrite(rgb, 1, 3, f);
            }
            std::fclose(f);
        }
    CHECK(tess_core_ink("YELLOW"), "the ink YELLOW");
    run(30);
    tess_core_tracks_view(&tv, TESS_TRACKS_W, TESS_TRACKS_H);
    CHECK(row_ink(0) == row_ink(4), "one ink: tracks 5..8 the same ink as 1..4 (%04x, %04x)", row_ink(0), row_ink(4));
}

int main(int argc, char **argv)
{
    tess_core_boot();
    tess_core_slot_load(0);   // (STILL TIDE, slot A: the values below are its; a new project's FX MIX sits at 0 %)
    run(50);

    // ---- HOME: the tracks
    CHECK(count(TESS_ZONE_TRACK) == 4, "HOME: a zone a track (%d)", count(TESS_ZONE_TRACK));
    act(find("T3"), TESS_ZA_CLICK);
    CHECK(v.track == 2, "a click on T3 selects it (%u)", v.track);

    // ---- a page: FX
    tap(B_FX);
    dump("FX");
    CHECK(count(TESS_ZONE_KNOB) == 4, "FX: a zone a knob (%d)", count(TESS_ZONE_KNOB));
    bool order = v.zone_count >= 4;
    for (int i = 0; i < 4 && order; i++)
        order = v.zones[i].role == 3 + i && v.zones[i].x == 60 * i && v.zones[i].w == 60 && v.zones[i].name[0];
    CHECK(order, "FX: KNOB 1..4 left to right, a column each, named");
    tess_zone_t mix = v.zones[1];
    std::string was = mix.value;
    act(1, TESS_ZA_TURN, -2);
    std::string turned = v.zones[1].value;
    CHECK(turned != was, "a turn of %s's zone moves it (%s, then %s)", mix.name, was.c_str(), turned.c_str());
    act(1, TESS_ZA_DOUBLE);
    std::string reset = v.zones[1].value;
    act(1, TESS_ZA_DOUBLE);
    CHECK(reset != turned && std::string(v.zones[1].value) == reset,
          "a double-click puts %s's default back (%s, then %s, again %s)", mix.name, turned.c_str(), reset.c_str(),
          v.zones[1].value);
    tess_zone_t gone = v.zones[1];
    tap(B_HOME);
    tess_core_zone_do(&gone, TESS_ZA_TURN, 5);
    run(30);
    tap(B_FX);
    CHECK(std::string(v.zones[1].value) == reset, "a zone no longer on the screen does nothing (%s, %s)", reset.c_str(),
          v.zones[1].value);

    // ---- a list page: SCL (SCALES is SEL's first screen; OCT- its list)
    tap(B_SEL);
    tap(B_OCTDN);
    dump("SCL");
    int scale = find("Scale", TESS_ZONE_ROW), root = find("Root", TESS_ZONE_ROW);
    CHECK(count(TESS_ZONE_ROW) >= 5 && scale >= 0 && root >= 0, "SCL: a zone a row, Root and Scale among them");
    CHECK(scale >= 0 && v.zones[scale].role == 3 && v.zones[scale].vrole == 4, "SCL's rows: KNOB 1 the row, KNOB 2 its value");
    if (scale >= 0 && root >= 0)
    {
        std::string s0 = v.zones[scale].value, r0 = v.zones[root].value;
        act(scale, TESS_ZA_TURN, 1);
        CHECK(std::string(v.zones[scale].value) != s0 && std::string(v.zones[root].value) == r0,
              "the wheel on Scale turns the scale (%s, then %s) and nothing else (Root %s, %s)", s0.c_str(),
              v.zones[scale].value, r0.c_str(), v.zones[root].value);
        act(scale, TESS_ZA_DOUBLE, 0, 40);
        CHECK(find("OK", TESS_ZONE_KEY) >= 0 && find("SCALE", TESS_ZONE_KNOB) >= 0,
              "a double-click on Scale opens SCALES (its OK, the scales' knob)");
        act(find("OK", TESS_ZONE_KEY), TESS_ZA_CLICK, 0, 40);
        CHECK(find("Scale", TESS_ZONE_ROW) >= 0, "SCALES' OK goes back to SCL's list");
    }

    // ---- a whole list: Quantize's
    int q = find("Quantize", TESS_ZONE_ROW);
    act(q, TESS_ZA_DOUBLE, 0, 40);
    dump("Quantize's list");
    bool whole = count(TESS_ZONE_ROW) >= 7 && v.zones[0].vrole < 0;
    CHECK(whole, "a double-click on Quantize opens its whole list (%d rows)", count(TESS_ZONE_ROW));
    if (whole)
    {
        std::string want = v.zones[2].name;
        act(2, TESS_ZA_DOUBLE, 0, 60);
        q = find("Quantize", TESS_ZONE_ROW);
        CHECK(q >= 0 && want == v.zones[q].value, "a double-click takes the row (%s, Quantize %s)", want.c_str(),
              q >= 0 ? v.zones[q].value : "?");
        act(q, TESS_ZA_DOUBLE, 0, 40);   // the list again: row 6, then row 1 at once and OK
        tess_zone_t r6 = v.zones[6], r1 = v.zones[1];
        want = r1.name;
        tess_core_zone_do(&r6, TESS_ZA_CLICK, 0);
        tess_core_zone_do(&r1, TESS_ZA_DOUBLE, 0);
        run(80);
        q = find("Quantize", TESS_ZONE_ROW);
        CHECK(q >= 0 && want == v.zones[q].value, "two clicks at once end on the second row (%s, Quantize %s)",
              want.c_str(), q >= 0 ? v.zones[q].value : "?");
    }

    settings();
    tracks(argc > 1 ? argv[1] : nullptr);
    mosaic(argc > 2 ? argv[2] : nullptr);

    std::printf(fails ? "zones test: %d FAILED\n" : "zones test: ok\n", fails);
    return fails ? 1 : 0;
}
