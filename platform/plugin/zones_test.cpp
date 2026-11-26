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
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>

#include "tess_core.h"
#include "sim_demos.h"   // (build/gen: SIM_DEMO_A, demos/still-tide.tsp)

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
enum { B_FX = 0, B_SEL = 1, B_ENV = 2, B_LFO = 3, B_EDIT = 4, B_GLO = 5, B_HOME = 6, B_SAVE = 7, B_ARP = 8, B_SEQ = 9, B_OCTDN = 12, B_OCTUP = 13 };

static void run(int blocks)   // 10 ms blocks
{
    for (int b = 0; b < blocks; b++)
        tess_core_render(L, R, nullptr, tess_core_rate() / 100);
    tess_core_view(&v);
}
static void tap(int b)
{
    tess_core_buttons(1u << b | TESS_BTN_POINTER);   // (the window's: a mouse's click)
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
static void slots()   // SAVE > PROJECT: a zone a slot; a click picks it, the wheel moves the list
{
    tap(B_HOME);
    tap(B_SAVE);
    for (int i = 0; i < 6 && find("A", TESS_ZONE_ROW) < 0; i++) tap(B_SAVE);   // (SAVE's pages: to PROJECT)
    int a = find("A", TESS_ZONE_ROW), b = find("B", TESS_ZONE_ROW);
    CHECK(a >= 0 && b >= 0, "PROJECT: a zone a slot (A %d, B %d; %u zones)", a, b, v.zone_count);
    if (a < 0 || b < 0) { dump("PROJECT"); return; }
    CHECK(!std::strstr(v.zones[a].value, "STILL") && !std::strstr(v.zones[b].value, "KOTO"),
          "PROJECT: the plugin's slots start empty (A %s, B %s)", v.zones[a].value, v.zones[b].value);
    act(b, TESS_ZA_CLICK, 0, 40);
    int c = find("C", TESS_ZONE_ROW);
    act(c, TESS_ZA_CLICK, 0, 40);
    b = find("B", TESS_ZONE_ROW);
    c = find("C", TESS_ZONE_ROW);
    CHECK(c >= 0 && b >= 0 && v.zones[c].h > v.zones[b].h, "a click on C picks it (its card the taller row)");
    act(c, TESS_ZA_WHEEL, 2, 40);   // (up two: A)
    a = find("A", TESS_ZONE_ROW);
    b = find("B", TESS_ZONE_ROW);
    CHECK(a >= 0 && b >= 0 && v.zones[a].h > v.zones[b].h, "the wheel up two from C: A picked");
    tap(B_HOME);
}
static int find_cell(const char *name, const char *value)   // a CELL by its hover: name, value's start
{
    for (uint32_t i = 0; i < v.zone_count; i++)
        if (v.zones[i].kind == TESS_ZONE_CELL && !std::strcmp(v.zones[i].name, name) &&
            !std::strncmp(v.zones[i].value, value, std::strlen(value)))
            return (int)i;
    return -1;
}
static void grids()   // PATTERNS and SONG: a click on a cell turns its knobs to it
{
    tap(B_HOME);
    for (int i = 0; i < 12 && count(TESS_ZONE_CELL) < 32; i++) tap(B_SEQ);   // (SEQ's pages: to PATTERNS)
    int cells = count(TESS_ZONE_CELL), tracks = count(TESS_ZONE_TRACK);
    CHECK(cells >= 32 && tracks >= 4, "PATTERNS: a cell a track and pattern (%d), a zone a track's number (%d)", cells,
          tracks);
    if (cells < 32) { dump("PATTERNS"); return; }
    int c = find_cell("T2", "P5");
    act(c, TESS_ZA_CLICK, 0, 60);
    c = find_cell("T2", "P5");
    CHECK(c >= 0 && std::strstr(v.zones[c].value, "PLAYING") || (c >= 0 && std::strstr(v.zones[c].value, "NEXT")),
          "a click on T2's P5 queues it (%s)", c >= 0 ? v.zones[c].value : "?");
    act(find_cell("T2", "P1"), TESS_ZA_CLICK, 0, 60);
    c = find_cell("T2", "P1");
    CHECK(c >= 0 && (std::strstr(v.zones[c].value, "PLAYING") || std::strstr(v.zones[c].value, "NEXT")),
          "then a click on P1: back (%s)", c >= 0 ? v.zones[c].value : "?");
    // a quick second click before the page is drawn again: the grid's cells are gone until it is (no stale turns)
    c = find_cell("T3", "P4");
    if (c >= 0)
    {
        tess_zone_t z = v.zones[c];
        tess_core_zone_do(&z, TESS_ZA_CLICK, 0);
        tess_core_zone_do(&z, TESS_ZA_CLICK, 0);   // (the same zone again at once: let go)
        run(60);
        tess_core_view(&v);
        c = find_cell("T3", "P4");
        CHECK(c >= 0 && (std::strstr(v.zones[c].value, "PLAYING") || std::strstr(v.zones[c].value, "NEXT")),
              "two clicks at once on T3's P4: P4, not past it (%s)", c >= 0 ? v.zones[c].value : "?");
        act(find_cell("T3", "P1"), TESS_ZA_CLICK, 0, 60);
    }

    // SONG (the next of SEQ's pages): a section's letter picks it; a block picks its section and its track
    for (int i = 0; i < 12 && count(TESS_ZONE_CELL) >= 32; i++) tap(B_SEQ);
    for (int i = 0; i < 12 && find("ADD", TESS_ZONE_CELL) < 0 && find("SECTION", TESS_ZONE_CELL) < 0; i++) tap(B_SEQ);
    int add = find("ADD", TESS_ZONE_CELL), sec = find("SECTION", TESS_ZONE_CELL);
    CHECK(add >= 0 || sec >= 0, "SONG: a zone a section (the one to add too)");
    if (add < 0 && sec < 0) { dump("SONG"); tap(B_HOME); return; }
    for (int i = 0; i < 2; i++)   // (two sections: + picked, KNOB 3 adds one there)
    {
        act(find("ADD", TESS_ZONE_CELL), TESS_ZA_CLICK, 0, 40);
        tess_core_enc(5, 1);
        run(40);
        tess_core_view(&v);
    }
    int blocks = 0;
    for (uint32_t i = 0; i < v.zone_count; i++) blocks += v.zones[i].kind == TESS_ZONE_CELL && v.zones[i].name[0] == 'T';
    CHECK(blocks >= 8, "SONG with two sections: a block a track and section (%d)", blocks);
    std::printf("SONG: %d sections' blocks\n", blocks);
    if (blocks)
    {
        uint32_t want = v.track == 2 ? 1 : 2;
        char tn[3] = {'T', (char)('1' + want), 0};
        int b = -1;
        for (uint32_t i = 0; i < v.zone_count && b < 0; i++)
            if (v.zones[i].kind == TESS_ZONE_CELL && !std::strncmp(v.zones[i].name, tn, 2)) b = (int)i;
        act(b, TESS_ZA_CLICK, 0, 60);
        CHECK(v.track == want, "SONG: a click on T%u's block selects T%u (%u)", want + 1, want + 1, v.track + 1);
    }
    tap(B_HOME);
}
static uint32_t pid_named(const char *name)   // a track parameter's central ID, by its name
{
    tess_param_t info;
    for (uint32_t n = 0; n < tess_core_param_count(); n++)
    {
        uint32_t pid = tess_core_param_at(n, 0, &info);
        if (info.name && !std::strcmp(info.name, name)) return pid;
    }
    return 0xFFFFu;
}
static int find_strip(uint32_t kind, const char *track, const char *value)
{
    for (uint32_t i = 0; i < v.zone_count; i++)
        if (v.zones[i].kind == kind && !std::strcmp(v.zones[i].name, track) && !std::strcmp(v.zones[i].value, value))
            return (int)i;
    return -1;
}
static std::string tab_now()   // the footer's tab of the page on the screen
{
    for (uint32_t i = 0; i < v.zone_count; i++)
        if (v.zones[i].kind == TESS_ZONE_TAB && v.zones[i].row == 0) return v.zones[i].value;
    return "";
}
static void glo_clicks()   // GLO clicked twice quickly by the mouse: two pages, no LOCK; held by it: the layer stays open
{
    tap(B_HOME);
    tap(B_GLO);
    std::string a = tab_now();
    tap(B_GLO);
    std::string b = tab_now();
    CHECK(!a.empty() && !b.empty() && a != b, "GLO clicked twice quickly: the next page (%s, then %s)", a.c_str(), b.c_str());
    tap(B_HOME);
    tap(B_GLO);
    CHECK(tab_now() == b, "GLO again from HOME: the GLO page last used (%s, now %s)", b.c_str(), tab_now().c_str());
    tap(B_HOME);
    tess_core_buttons(1u << B_GLO);   // (an FM-1's panel: tapped twice quickly, the LOCK)
    run(5);
    tess_core_buttons(0);
    run(10);
    tess_core_buttons(1u << B_GLO);
    run(5);
    tess_core_buttons(0);
    run(20);
    CHECK(v.key_long[0][0], "an FM-1's GLO tapped twice quickly: LOCK, the layer's keys (%s)", v.key_long[0]);
    tap(B_GLO);
    CHECK(!v.key_long[0][0], "..GLO again: closed (%s)", v.key_long[0]);
    tess_core_buttons(1u << B_GLO);   // (an FM-1's GLO held, let go: closed)
    run(100);
    tess_core_buttons(0);
    run(20);
    CHECK(!v.key_long[0][0], "an FM-1's GLO held and let go: the layer closes (%s)", v.key_long[0]);
    tap(B_HOME);
    tess_core_buttons(1u << B_GLO | TESS_BTN_POINTER);   // held a second by the mouse, let go: open still
    run(100);
    tess_core_buttons(0);
    run(20);
    CHECK(v.key_long[0][0], "GLO held by the mouse and let go: the layer stays open (%s)", v.key_long[0]);
    tap(B_GLO);
    CHECK(!v.key_long[0][0], "..clicked again: closed (%s)", v.key_long[0]);
    tap(B_HOME);
}
static void mixer()   // MIXER (GLO's first page): a strip's LEVEL, PAN, MUTE and SOLO
{
    tap(B_HOME);
    for (int i = 0; i < 16 && find_strip(TESS_ZONE_PAN, "T1", "PAN") < 0; i++) tap(B_GLO);
    uint32_t lvl = pid_named("LEVEL"), pan = pid_named("PAN"), mute = pid_named("MUTE");
    int l2 = find_strip(TESS_ZONE_LEVEL, "T2", "LEVEL"), p2 = find_strip(TESS_ZONE_PAN, "T2", "PAN"),
        m2 = find_strip(TESS_ZONE_MUTE, "T2", "MUTE/SOLO");
    CHECK(l2 >= 0 && p2 >= 0 && m2 >= 0, "MIXER: T2's strip: LEVEL, PAN, the MUTE / SOLO word");
    CHECK(lvl != 0xFFFFu && pan != 0xFFFFu && mute != 0xFFFFu, "LEVEL, PAN, MUTE by name");
    if (l2 < 0 || p2 < 0 || m2 < 0) { dump("MIXER"); return; }
    {
        auto &z = v.zones[m2];
        CHECK(z.w >= 48, "the word's zone as wide as the word (%u)", (unsigned)z.w);
    }
    int32_t l0 = tess_core_param_get(lvl, 1), p0 = tess_core_param_get(pan, 1);
    act(l2, TESS_ZA_TURN, -6, 40);
    CHECK(tess_core_param_get(lvl, 1) < l0, "a drag down on T2's LEVEL lowers it (%d, was %d)", tess_core_param_get(lvl, 1), l0);
    act(find_strip(TESS_ZONE_LEVEL, "T2", "LEVEL"), TESS_ZA_DOUBLE, 0, 40);
    CHECK(tess_core_param_get(lvl, 1) == 104, "a double-click: T2's LEVEL back to its default (%d)", tess_core_param_get(lvl, 1));
    act(find_strip(TESS_ZONE_PAN, "T2", "PAN"), TESS_ZA_TURN, 10, 40);
    CHECK(tess_core_param_get(pan, 1) == p0 + 10, "a drag on T2's PAN turns it (%d, was %d)", tess_core_param_get(pan, 1), p0);
    act(find_strip(TESS_ZONE_PAN, "T2", "PAN"), TESS_ZA_DOUBLE, 0, 40);
    CHECK(tess_core_param_get(pan, 1) == 0, "a double-click: T2's PAN centred");
    int32_t m0 = tess_core_param_get(mute, 1);
    act(find_strip(TESS_ZONE_MUTE, "T2", "MUTE/SOLO"), TESS_ZA_CLICK, 0, 40);
    CHECK(tess_core_param_get(mute, 1) != m0, "a click on T2's MUTE mutes it (%d)", tess_core_param_get(mute, 1));
    act(find_strip(TESS_ZONE_MUTE, "T2", "MUTE/SOLO"), TESS_ZA_CLICK, 0, 40);
    CHECK(tess_core_param_get(mute, 1) == m0, "again: unmuted");
    int w2 = find_strip(TESS_ZONE_MUTE, "T2", "MUTE/SOLO");
    act(w2, TESS_ZA_CLICK, 0, 2);
    act(w2, TESS_ZA_DOUBLE, 0, 40);
    CHECK(find_strip(TESS_ZONE_MUTE, "T2", "SOLO") >= 0 && tess_core_param_get(mute, 1) == m0,
          "a double-click: T2 soloed, its click's MUTE taken back (%d)", tess_core_param_get(mute, 1));
    act(find_strip(TESS_ZONE_MUTE, "T2", "SOLO"), TESS_ZA_CLICK, 0, 40);
    CHECK(find_strip(TESS_ZONE_MUTE, "T2", "MUTE/SOLO") >= 0 && tess_core_param_get(mute, 1) == m0,
          "a click on SOLO: T2 not soloed, not muted");
    act(find_strip(TESS_ZONE_MUTE, "T2", "MUTE/SOLO"), TESS_ZA_HELP, 0, 40);   // (a right-click)
    CHECK(find_strip(TESS_ZONE_MUTE, "T2", "SOLO") >= 0 && tess_core_param_get(mute, 1) == m0,
          "a right-click on the word: T2 soloed, not muted");
    act(find_strip(TESS_ZONE_MUTE, "T2", "SOLO"), TESS_ZA_HELP, 0, 40);
    CHECK(find_strip(TESS_ZONE_MUTE, "T2", "MUTE/SOLO") >= 0, "a right-click again: T2 not soloed");
    tap(B_HOME);
}
static void typed_values()   // G1: a value typed for its zone; a fine turn never sped up
{
    tap(B_HOME);
    tap(B_FX);
    int k = -1;
    for (uint32_t i = 0; i < v.zone_count; i++)
        if (v.zones[i].kind == TESS_ZONE_KNOB && v.zones[i].role == 4) { k = (int)i; break; }   // KNOB 2: FX MIX
    CHECK(k >= 0, "FX: KNOB 2's zone");
    if (k < 0) return;
    tess_zone_t z = v.zones[k];
    uint32_t pid = 0, tr = 0;
    CHECK(tess_core_zone_param(&z, &pid, &tr) && tr == v.track, "its parameter (%x, T%u)", pid, tr + 1);
    CHECK(!tess_core_zone_type(&z, "50 %"), "\"50 %\" typed: set");
    run(10);
    std::string a = v.zones[k].value;
    CHECK(a == "50%", "..FX MIX shows 50%% (%s)", a.c_str());
    CHECK(!tess_core_zone_type(&z, "33"), "\"33\" typed: the nearest shown");
    run(10);
    CHECK(std::string(v.zones[k].value) == "33%" || std::string(v.zones[k].value) == "32%" ||
              std::string(v.zones[k].value) == "34%", "..%s", v.zones[k].value);
    CHECK(tess_core_zone_type(&z, "LOUD"), "words that are no value: refused");
    int32_t b = tess_core_param_get(pid, tr);
    for (int i = 0; i < 6; i++) { tess_core_zone_do(&v.zones[k], TESS_ZA_TURN_FINE, 1); run(1); }
    run(10);
    CHECK(tess_core_param_get(pid, tr) == b + 6, "six fine detents, quickly: six steps (%d, was %d)",
          tess_core_param_get(pid, tr), b);
    tess_zone_t row = {};
    row.kind = TESS_ZONE_ROW;
    CHECK(!tess_core_zone_param(&row, &pid, &tr), "a zone that is no value: no parameter");
    tap(B_HOME);
}
static int tfind_role(uint32_t kind, int track, int role)
{
    for (uint32_t i = 0; i < tv.zone_count; i++)
        if (tv.zones[i].kind == kind && tv.zones[i].row == track && tv.zones[i].role == role) return (int)i;
    return -1;
}
static void mixer8()   // MIXER on the TRACKS screen (Cosmati audit I1): a strip a track, its fader, PAN, sends, M, S
{
    if (tess_core_tracks() < 8) return;
    tap(B_HOME);
    for (int i = 0; i < 16 && find_strip(TESS_ZONE_PAN, "T1", "PAN") < 0; i++) tap(B_GLO);
    tess_core_tracks_view(&tv, TESS_TRACKS_W, TESS_TRACKS_H);
    int heads = 0, pans = 0;
    for (uint32_t i = 0; i < tv.zone_count; i++) heads += tv.zones[i].kind == TESS_ZONE_TRACK, pans += tv.zones[i].kind == TESS_ZONE_PAN;
    CHECK(heads == 8 && pans == 8 && tfind_role(TESS_ZONE_LEVEL, 7, 0) >= 0, "MIXER: 8 strips, each a PAN and a send (%d, %d)",
          heads, pans);
    {
        int a = tfind(TESS_ZONE_TRACK, 0), b = tfind(TESS_ZONE_TRACK, 7);
        CHECK(a >= 0 && b >= 0 && tv.zones[b].x > tv.zones[a].x + 400 && tv.zones[b].y == tv.zones[a].y,
              "..side by side, T1 left, T8 right");
    }
    if (const char *o = std::getenv("MIXER_PPM"))   // (a look at it)
        if (FILE *f = std::fopen(o, "wb"))
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
    uint32_t lvl = pid_named("LEVEL"), pan = pid_named("PAN"), mute = pid_named("MUTE");
    int32_t l0 = tess_core_param_get(lvl, 5), p0 = tess_core_param_get(pan, 5);
    tact(TESS_ZONE_LEVEL, 5, TESS_ZA_TURN, -10);
    CHECK(tess_core_param_get(lvl, 5) == l0 - 10, "T6's fader dragged down 10 (%d, was %d)", tess_core_param_get(lvl, 5), l0);
    tact(TESS_ZONE_LEVEL, 5, TESS_ZA_DOUBLE);
    CHECK(tess_core_param_get(lvl, 5) == 104, "..a double-click: its default (%d)", tess_core_param_get(lvl, 5));
    tact(TESS_ZONE_PAN, 5, TESS_ZA_TURN, 8);
    CHECK(tess_core_param_get(pan, 5) == p0 + 8, "T6's PAN turned (%d)", tess_core_param_get(pan, 5));
    tact(TESS_ZONE_PAN, 5, TESS_ZA_DOUBLE);
    CHECK(tess_core_param_get(pan, 5) == 0, "..a double-click: centred");
    {
        int i = tfind_role(TESS_ZONE_LEVEL, 6, 0);
        std::string s0 = i >= 0 ? tv.zones[i].value : "?";
        if (i >= 0) { tess_zone_t z = tv.zones[i]; tess_core_tracks_do(&z, TESS_ZA_TURN, 20); run(10); }
        tess_core_tracks_view(&tv, TESS_TRACKS_W, TESS_TRACKS_H);
        i = tfind_role(TESS_ZONE_LEVEL, 6, 0);
        CHECK(i >= 0 && s0 != tv.zones[i].value, "T7's send turned (%s -> %s)", s0.c_str(), i >= 0 ? tv.zones[i].value : "?");
    }
    int32_t m0 = tess_core_param_get(mute, 7);
    tact(TESS_ZONE_MUTE, 7, TESS_ZA_CLICK);
    CHECK(tess_core_param_get(mute, 7) != m0 && tval(TESS_ZONE_MUTE, 7) == "ON", "T8's M mutes it");
    tact(TESS_ZONE_MUTE, 7, TESS_ZA_CLICK);
    CHECK(tess_core_param_get(mute, 7) == m0, "..again: not");
    tact(TESS_ZONE_TRACK, 5, TESS_ZA_CLICK);
    run(10);
    CHECK(tess_core_track() == 5, "a click on T6's head selects it (T%u)", tess_core_track() + 1);

    // the FM-1 screen, the TRACKS screen shown: the selected track's inspector (its LEVEL, PAN, sends, M / S)
    tess_core_tracks_view(&tv, TESS_TRACKS_W, TESS_TRACKS_H);
    run(10);
    CHECK(find_strip(TESS_ZONE_LEVEL, "T6", "LEVEL") >= 0 && find_strip(TESS_ZONE_LEVEL, "T5", "LEVEL") < 0 &&
              find_strip(TESS_ZONE_PAN, "T6", "PAN") >= 0,
          "EXPANDED: MIXER on the FM-1 screen is T6's inspector, no bank of strips");
    if (const char *o = std::getenv("INSPECT_PPM"))   // (a look at it)
        if (FILE *f = std::fopen(o, "wb"))
        {
            tess_screen_t sc;
            tess_core_screen(&sc);
            std::fprintf(f, "P6\n%u %u\n255\n", sc.w, sc.h);
            for (uint32_t p = 0; p < sc.w * sc.h; p++)
            {
                uint16_t c = sc.pixels[p];
                unsigned char rgb[3] = {(unsigned char)((c >> 11) << 3), (unsigned char)(((c >> 5) & 63) << 2),
                                        (unsigned char)((c & 31) << 3)};
                std::fwrite(rgb, 1, 3, f);
            }
            std::fclose(f);
        }
    {
        int sn = find_strip(TESS_ZONE_LEVEL, "T6", "FX SEND");
        if (sn < 0) sn = find_strip(TESS_ZONE_LEVEL, "T6", "SEND 1");
        uint32_t send = pid_named("FX SEND");
        int32_t s0 = send != 0xFFFFu ? tess_core_param_get(send, 5) : -1;
        if (sn >= 0) act(sn, TESS_ZA_TURN, 5, 10);
        CHECK(sn >= 0 && s0 >= 0 && tess_core_param_get(send, 5) == (s0 + 5 > 127 ? 127 : s0 + 5),
              "..its send's bar turned (%d, was %d)", s0 >= 0 ? tess_core_param_get(send, 5) : -1, s0);
        tess_core_tracks_view(&tv, TESS_TRACKS_W, TESS_TRACKS_H);
        l0 = tess_core_param_get(lvl, 5);
        tess_core_enc(3, 3);
        run(10);
        CHECK(tess_core_param_get(lvl, 5) > l0, "..KNOB 1 turns T6's level (%d, was %d)", tess_core_param_get(lvl, 5), l0);
        tess_core_tracks_view(&tv, TESS_TRACKS_W, TESS_TRACKS_H);
        p0 = tess_core_param_get(pan, 5);
        tess_core_enc(4, -4);
        run(10);
        CHECK(tess_core_param_get(pan, 5) < p0, "..KNOB 2 its PAN (%d, was %d)", tess_core_param_get(pan, 5), p0);
        int32_t l1 = tess_core_param_get(lvl, 1);
        run(60);   // (the TRACKS screen no longer asked for: the strips again)
        CHECK(find_strip(TESS_ZONE_LEVEL, "T5", "LEVEL") >= 0, "TRACKS gone: the bank's strips again");
        (void)l1;
    }
    tess_core_track_select(0);
    tap(B_HOME);
    run(10);
    tess_core_tracks_view(&tv, TESS_TRACKS_W, TESS_TRACKS_H);
    pans = 0;
    for (uint32_t i = 0; i < tv.zone_count; i++) pans += tv.zones[i].kind == TESS_ZONE_PAN;
    CHECK(!pans, "HOME: the rows again (%d PAN zones)", pans);
}
static void keys8()   // MIXER's keys with 8 tracks (Cosmati audit I2): the lower octave's mute / solo T1-T4, the upper's T5-T8
{
    tap(B_HOME);
    for (int i = 0; i < 16 && find_strip(TESS_ZONE_PAN, "T1", "PAN") < 0; i++) tap(B_GLO);
    uint32_t mute = pid_named("MUTE");
    CHECK(!std::strcmp(v.key_short[1], "M1") && !std::strcmp(v.key_short[0], "S1") && !std::strcmp(v.key_short[13], "M5") &&
              !std::strcmp(v.key_short[20], "M8") && !std::strcmp(v.key_short[14], "S5") && !std::strcmp(v.key_short[19], "S8"),
          "keys: F#3 M1, F3 S1, F#4 M5, C#5 M8, G4 S5, C5 S8 (%s %s %s %s %s %s)", v.key_short[1], v.key_short[0],
          v.key_short[13], v.key_short[20], v.key_short[14], v.key_short[19]);
    CHECK(!std::strcmp(v.key_long[20], "T8 MUTE") && !v.key_short[10][0] && !std::strcmp(v.key_short[12], "TAP"),
          "C#5 T8 MUTE (%s); D#4 nothing, F4 still TAP (%s)", v.key_long[20], v.key_short[12]);
    int32_t m0 = tess_core_param_get(mute, 4), m1 = tess_core_param_get(mute, 0);
    tess_core_keys(1u << 13);   // F#4
    run(5);
    tess_core_keys(0);
    run(40);
    CHECK(tess_core_param_get(mute, 4) != m0 && tess_core_param_get(mute, 0) == m1, "F#4 mutes T5, not T1 (%d, %d)",
          tess_core_param_get(mute, 4), tess_core_param_get(mute, 0));
    tess_core_keys(1u << 13);
    run(5);
    tess_core_keys(0);
    run(40);
    CHECK(tess_core_param_get(mute, 4) == m0, "again: unmuted");
    tess_core_track_select(4);
    run(20);
    tess_core_keys(1u << 19);   // C5 held: T8 soloed
    run(20);
    CHECK(find_strip(TESS_ZONE_MUTE, "T8", "SOLO") >= 0, "C5 held: T8 soloed");
    tess_core_keys(0);
    run(20);
    CHECK(find_strip(TESS_ZONE_MUTE, "T8", "MUTE/SOLO") >= 0, "let go: not");
    tess_core_track_select(0);
    tap(B_HOME);
}
static void sheet_row(const char *label)   // the page's sheet (OCT+, tapped or held) and its row, double-clicked
{
    tap(B_OCTUP);
    tess_core_view(&v);
    if (find(label, TESS_ZONE_ROW) < 0) {
        tap(B_OCTDN);
        hold(B_OCTUP, 120);
        tess_core_view(&v);
    }
    act(find(label, TESS_ZONE_ROW), TESS_ZA_DOUBLE, 0, 40);
    tess_core_view(&v);
}
static uint32_t screen_hash()   // the screen as it is now (FNV-1a of its pixels)
{
    tess_screen_t sc;
    tess_core_screen(&sc);
    uint32_t h = 2166136261u;
    for (uint32_t i = 0; i < sc.w * sc.h; i++) h = (h ^ sc.pixels[i]) * 16777619u;
    return h;
}
static void smap()   // EDIT's sections map (Page map on the sound's sheet, T3's FM6): a tile a page, a click opens it
{
    tess_core_track_select(2);
    run(20);
    tap(B_HOME);
    tap(B_EDIT);
    sheet_row("Page map");
    int n = count(TESS_ZONE_SMAP);
    CHECK(n >= 4, "the sections map: a zone a tile (%d)", n);
    int pick = -1;
    for (uint32_t i = 0; i < v.zone_count && pick < 0; i++)
        if (v.zones[i].kind == TESS_ZONE_SMAP && v.zones[i].row >= 16) pick = (int)i;   // (a tile of the third section)
    if (pick >= 0)
    {
        std::string name = v.zones[pick].name;
        uint32_t h = screen_hash();
        act(pick, TESS_ZA_CLICK, 0, 40);
        CHECK(count(TESS_ZONE_SMAP) == 0 && screen_hash() != h, "a tile clicked opens its page (%s), the map closed",
              name.c_str());
    }
    else
        dump("smap");
    tap(B_HOME);
    CHECK(v.sheet == 1, "HOME: the view says the page has a sheet (OK held)");
    tess_core_track_select(0);
    run(20);
}
static void chord_user()   // CHORD USER on GEN: a card a chord; the wheel turns its root, "+" adds one, a right-click takes it
{
    uint32_t mode = pid_named("GENERATOR MODE"), prog = pid_named("CHORD PROG");
    int32_t was_mode = tess_core_param_get(mode, 1), was_prog = tess_core_param_get(prog, 1);
    tess_core_track_select(1);   // (T2: a synth, so a chord's quality is heard; T4's kit plays roots)
    tess_core_param_set(mode, 1, 12);   // (CHORD)
    tess_core_param_set(prog, 1, 7);    // (USER)
    run(20);
    tap(B_HOME);
    int i = -1;
    for (int k = 0; k < 6 && (i = find("CHORD 1", TESS_ZONE_CHORD)) < 0; k++) tap(B_SEQ);
    CHECK(i >= 0, "GEN on a CHORD USER track: a zone a card (MODE %d PROG %d)", tess_core_param_get(mode, 1),
          tess_core_param_get(prog, 1));
    if (i >= 0)
    {
        int n0 = count(TESS_ZONE_CHORD);
        std::string c1 = v.zones[i].value;
        act(i, TESS_ZA_WHEEL, 1);
        i = find("CHORD 1", TESS_ZONE_CHORD);
        CHECK(i >= 0 && c1 != v.zones[i].value, "the wheel turns the first chord's root (%s -> %s)", c1.c_str(),
              i >= 0 ? v.zones[i].value : "?");
        if (const char *shot = std::getenv("CHORD_PPM"))   // (a picture of the cards, to look at)
        {
            tess_screen_t sc;
            tess_core_screen(&sc);
            if (FILE *f = std::fopen(shot, "wb"))
            {
                std::fprintf(f, "P6\n%u %u\n255\n", sc.w, sc.h);
                for (uint32_t k = 0; k < sc.w * sc.h; k++)
                {
                    uint16_t p = sc.pixels[k];
                    unsigned char rgb[3] = {(unsigned char)((p >> 11 & 31) * 255 / 31), (unsigned char)((p >> 5 & 63) * 255 / 63),
                                            (unsigned char)((p & 31) * 255 / 31)};
                    std::fwrite(rgb, 1, 3, f);
                }
                std::fclose(f);
            }
        }
        act(find("+", TESS_ZONE_CHORD), TESS_ZA_CLICK);
        CHECK(count(TESS_ZONE_CHORD) == n0 + 1 || (n0 == 8 && count(TESS_ZONE_CHORD) == 8), "+ adds a chord (%d -> %d)",
              n0, count(TESS_ZONE_CHORD));
        int n1 = count(TESS_ZONE_CHORD);
        act(find("CHORD 1", TESS_ZONE_CHORD), TESS_ZA_HELP);
        CHECK(count(TESS_ZONE_CHORD) == n1 - 1 || n1 - 1 == 8, "a right-click takes a chord (%d -> %d)", n1,
              count(TESS_ZONE_CHORD));
        std::string q0 = v.zones[find("CHORD 1", TESS_ZONE_CHORD)].value;   // a click: its quality, the next one
        std::string q1 = q0;
        for (int k = 0; k < 3 && q1 == q0; k++)   // (AUTO may already be the first one's: maj on I)
        {
            act(find("CHORD 1", TESS_ZONE_CHORD), TESS_ZA_CLICK);
            q1 = v.zones[find("CHORD 1", TESS_ZONE_CHORD)].value;
        }
        CHECK(q1 != q0, "a click gives the first chord a quality (%s -> %s)", q0.c_str(), q1.c_str());
        static uint8_t rec[1 << 17];
        uint32_t len = tess_core_save(rec, sizeof rec);
        for (int k = 0; k < 3; k++) act(find("CHORD 1", TESS_ZONE_CHORD), TESS_ZA_CLICK);
        std::string q2 = v.zones[find("CHORD 1", TESS_ZONE_CHORD)].value;
        CHECK(len && q2 != q1, "three clicks more: another quality (%s)", q2.c_str());
        tess_core_load(rec, len);
        run(20);
        int j = find("CHORD 1", TESS_ZONE_CHORD);
        CHECK(j >= 0 && q1 == v.zones[j].value, "the project keeps the quality (%s -> %s)", q1.c_str(),
              j >= 0 ? v.zones[j].value : "?");
        int a = 0;   // round to AUTO: the track's own chord again
        while (a++ < 12 && v.zones[find("CHORD 1", TESS_ZONE_CHORD)].value != q0)
            act(find("CHORD 1", TESS_ZONE_CHORD), TESS_ZA_CLICK);
        CHECK(v.zones[find("CHORD 1", TESS_ZONE_CHORD)].value == q0, "the clicks come round to AUTO (%d)", a);
    }
    else
        dump("chord user");
    tess_core_param_set(mode, 1, was_mode);
    tess_core_param_set(prog, 1, was_prog);
    tap(B_HOME);
    tess_core_track_select(0);
    run(20);
}
static std::string page_now()   // the page shown: its footer tab's (row 0)
{
    for (uint32_t i = 0; i < v.zone_count; i++)
        if (v.zones[i].kind == TESS_ZONE_TAB && v.zones[i].row == 0) return v.zones[i].value;
    return "";
}
static std::string tabs_now()   // the footer's tabs' pages, in order
{
    std::string t;
    for (uint32_t i = 0; i < v.zone_count; i++)
        if (v.zones[i].kind == TESS_ZONE_TAB && v.zones[i].w > 6) t += std::string(v.zones[i].value) + "|";
    return t;
}
static void press_section(int s)
{
    tess_core_buttons(TESS_BTN_SECTION(s) | TESS_BTN_POINTER);
    run(5);
    tess_core_buttons(0);
    run(20);
}
static void sections()   // Cosmati's section buttons (key words on): a section's pages, its tabs, its light
{
    tess_core_key_words(1);
    tap(B_HOME);
    press_section(TESS_SEC_PIECE);
    std::string p1 = page_now(), t = tabs_now();
    CHECK(p1 == "PIECE", "PIECE: its first page (%s)", p1.c_str());
    CHECK(t.find("CHAIN|") != std::string::npos && t.find("ENTRIES|") != std::string::npos &&
              t.find("MIXER|") == std::string::npos, "..its tabs PIECE's, not GLO's (%s)", t.c_str());
    CHECK(v.lit_buttons & TESS_BTN_SECTION(TESS_SEC_PIECE), "..PIECE lit");
    press_section(TESS_SEC_PIECE);
    CHECK(page_now() == "CHAIN", "PIECE again: the next (%s)", page_now().c_str());
    press_section(TESS_SEC_SETUP);
    t = tabs_now();
    CHECK(page_now() == "SETTINGS" && t.find("SETTINGS|TEMPO|SYSTEM|") != std::string::npos,
          "SETUP: SETTINGS first, the menu's list with SETUP's tabs (%s / %s)", page_now().c_str(), t.c_str());
    bool rows = false;
    for (uint32_t i = 0; i < v.zone_count; i++)
        if (v.zones[i].kind == TESS_ZONE_ROW && !std::strcmp(v.zones[i].name, "GROUND")) rows = true;
    CHECK(rows, "..the settings' rows (GROUND among them)");
    press_section(TESS_SEC_SETUP);
    CHECK(page_now() == "TEMPO", "SETUP again: TEMPO, the list closed (%s)", page_now().c_str());
    t = tabs_now();
    CHECK(t.find("SYSTEM|") != std::string::npos && t.find("PATTERNS|") == std::string::npos, "..with SYSTEM, MIDI (%s)",
          t.c_str());
    press_section(TESS_SEC_PIECE);
    CHECK(page_now() == "CHAIN", "PIECE from elsewhere: the page last used there (%s)", page_now().c_str());
    press_section(TESS_SEC_MIX);
    CHECK(page_now() == "MIXER" && !(v.lit_buttons & TESS_BTN_SECTION(TESS_SEC_PIECE)) &&
              (v.lit_buttons & TESS_BTN_SECTION(TESS_SEC_MIX)), "MIX: MIXER, MIX lit (%s)", page_now().c_str());
    press_section(TESS_SEC_FX);
    std::string f1 = page_now();
    int n = 1;
    for (; n < 12; n++)
    {
        press_section(TESS_SEC_FX);
        if (page_now() == f1) break;
    }
    CHECK(f1 == "FX 1" && n > 2 && n < 12, "FX: round its pages and back to FX 1 (%d)", n);
    tap(B_HOME);
    CHECK(!(v.lit_buttons & (TESS_BTN_SECTION(TESS_SEC_N) - TESS_BTN_SECTION(0))), "HOME: no section lit");
    tess_core_key_words(0);
    run(10);
}
static void egrid()   // ENTRIES' arrangement grid (8 tracks): a cell a track and FORM letter; a click the next kind,
{                     // a right-click AUTO, a letter's head the whole column
    tap(B_HOME);
    for (int k = 0; k < 16 && find("T1 SECTION A", TESS_ZONE_EGRID) < 0; k++) tap(B_GLO);
    int i = find("T1 SECTION A", TESS_ZONE_EGRID);
    CHECK(i >= 0 && count(TESS_ZONE_EGRID) == 8 * 3 + 6, "ENTRIES: a zone a cell, 8 tracks x A B C, and the heads (%d)",
          count(TESS_ZONE_EGRID));
    if (i < 0) { dump("entries"); tap(B_HOME); return; }
    CHECK(!std::strcmp(v.zones[i].value, "AUTO"), "a cell starts AUTO (%s)", v.zones[i].value);
    act(i, TESS_ZA_CLICK);
    CHECK(!std::strcmp(v.zones[find("T1 SECTION A", TESS_ZONE_EGRID)].value, "IN"), "a click: IN");
    act(find("T1 SECTION A", TESS_ZONE_EGRID), TESS_ZA_DOUBLE);   // (a quick second click: the next again)
    CHECK(!std::strcmp(v.zones[find("T1 SECTION A", TESS_ZONE_EGRID)].value, "OUT"), "a quick second click: OUT");
    act(find("T8 SECTION C", TESS_ZONE_EGRID), TESS_ZA_CLICK);
    act(find("T8 SECTION C", TESS_ZONE_EGRID), TESS_ZA_CLICK);
    act(find("T8 SECTION C", TESS_ZONE_EGRID), TESS_ZA_CLICK);
    CHECK(!std::strcmp(v.zones[find("T8 SECTION C", TESS_ZONE_EGRID)].value, "FADE"), "three clicks: FADE (T8, C)");
    if (const char *shot = std::getenv("EGRID_PPM"))   // (a picture of the grid, to look at)
    {
        tess_screen_t sc;
        tess_core_screen(&sc);
        if (FILE *f = std::fopen(shot, "wb"))
        {
            std::fprintf(f, "P6\n%u %u\n255\n", sc.w, sc.h);
            for (uint32_t k = 0; k < sc.w * sc.h; k++)
            {
                uint16_t p = sc.pixels[k];
                unsigned char rgb[3] = {(unsigned char)((p >> 11 & 31) * 255 / 31), (unsigned char)((p >> 5 & 63) * 255 / 63),
                                        (unsigned char)((p & 31) * 255 / 31)};
                std::fwrite(rgb, 1, 3, f);
            }
            std::fclose(f);
        }
    }
    static uint8_t rec[1 << 17];
    uint32_t len = tess_core_save(rec, sizeof rec);
    act(find("T1 SECTION A", TESS_ZONE_EGRID), TESS_ZA_HELP);
    CHECK(!std::strcmp(v.zones[find("T1 SECTION A", TESS_ZONE_EGRID)].value, "AUTO"), "a right-click: AUTO");
    act(find("SECTION B", TESS_ZONE_EGRID), TESS_ZA_CLICK);
    int all = 0;
    for (int k = 1; k <= 8; k++)
    {
        char nm[16];
        std::snprintf(nm, sizeof nm, "T%d SECTION B", k);
        int j = find(nm, TESS_ZONE_EGRID);
        all += j >= 0 && !std::strcmp(v.zones[j].value, "IN");
    }
    CHECK(all == 8, "B's head: every track IN in B (%d)", all);
    act(find("SECTION B", TESS_ZONE_EGRID), TESS_ZA_HELP);
    tess_core_load(rec, len);
    run(20);
    CHECK(!std::strcmp(v.zones[find("T1 SECTION A", TESS_ZONE_EGRID)].value, "OUT") &&
              !std::strcmp(v.zones[find("T8 SECTION C", TESS_ZONE_EGRID)].value, "FADE") &&
              !std::strcmp(v.zones[find("T2 SECTION B", TESS_ZONE_EGRID)].value, "AUTO"),
          "the project keeps the grid");
    for (int k = 0; k < 3; k++)   // (back to every cell AUTO)
    {
        char nm[16];
        std::snprintf(nm, sizeof nm, "SECTION %c", 'A' + k);
        act(find(nm, TESS_ZONE_EGRID), TESS_ZA_HELP);
    }
    tap(B_HOME);
    run(20);
}
static void hosted_words()   // inside a DAW the MIDI page's input is the DAW's, CLK's INT the DAW's (TESS_HOSTED)
{
    tap(B_HOME);
    int i = -1;
    for (int k = 0; k < 16 && (i = find("MIDI", TESS_ZONE_KNOB)) < 0; k++) tap(B_GLO);
    CHECK(i >= 0 && !std::strcmp(v.zones[i].value, "DAW"), "GLO's MIDI page: the input is DAW (%s)", i >= 0 ? v.zones[i].value : "-");
    tap(B_HOME);
}
static void reading()   // ABOUT and a long HELP card: a SCROLL zone; the wheel scrolls (down, then back up)
{
    tap(B_HOME);
    hold(B_HOME, 120);   // SETTINGS
    for (int i = 0; i < 6 && find("ABOUT", TESS_ZONE_ROW) < 0; i++) act(0, TESS_ZA_WHEEL, -6, 40);
    act(find("ABOUT", TESS_ZONE_ROW), TESS_ZA_DOUBLE, 0, 60);   // (there, OK: ABOUT)
    int z = find("ABOUT", TESS_ZONE_SCROLL);
    CHECK(z >= 0, "ABOUT: its text a SCROLL zone");
    if (z >= 0)
    {
        uint32_t top = screen_hash();
        act(z, TESS_ZA_WHEEL, -4, 40);   // (down)
        uint32_t down = screen_hash();
        act(find("ABOUT", TESS_ZONE_SCROLL), TESS_ZA_WHEEL, 4, 40);   // (back up)
        CHECK(down != top && screen_hash() == top, "ABOUT: the wheel down scrolls it, up brings it back");
    }
    else dump("ABOUT");
    tap(B_OCTDN);
    hold(B_HOME, 120);
    tap(B_HOME);
    // a HELP card (ARP held, a knob turned) longer than the card: the wheel over it scrolls it
    int found = 0;
    for (int page = 0; page < 10 && !found; page++)
    {
        tap(B_SEQ);
        for (int k = 0; k < 4 && !found; k++)
        {
            tess_core_buttons(1u << B_ARP);
            run(5);
            tess_core_enc(3 + k, 1);
            run(30);
            tess_core_view(&v);
            for (uint32_t i = 0; i < v.zone_count && !found; i++)
                if (v.zones[i].kind == TESS_ZONE_SCROLL)
                {
                    found = 1;
                    uint32_t top = screen_hash();
                    tess_zone_t zz = v.zones[i];
                    tess_core_zone_do(&zz, TESS_ZA_WHEEL, -2);
                    run(30);
                    CHECK(screen_hash() != top, "HELP (%s): the wheel down scrolls the card", zz.name);
                }
            tess_core_buttons(0);
            run(20);
        }
    }
    CHECK(found, "a HELP card longer than the card: a SCROLL zone (SEQ's pages)");
    tap(B_HOME);
}
static void typing()   // NAME (SAVE > PROJECT, Rename on slot A's sheet): a computer keyboard types into it
{
    tap(B_HOME);
    tap(B_SAVE);
    for (int i = 0; i < 6 && find("A", TESS_ZONE_ROW) < 0; i++) tap(B_SAVE);
    act(find("A", TESS_ZONE_ROW), TESS_ZA_CLICK, 0, 40);
    CHECK(!v.typing, "PROJECT: not typing");
    sheet_row("Rename");
    CHECK(v.typing, "Rename on PROJECT's slot A: NAME open, typing");
    for (int i = 0; i < 14; i++) tess_core_text(TESS_TEXT_BACK);   // (the name it has: gone)
    for (const char *c = "dub sketch!7"; *c; c++) tess_core_text((uint8_t)*c);   // ('!' isn't in the set: left out)
    tess_core_text(TESS_TEXT_LEFT);
    tess_core_text(TESS_TEXT_BACK);   // (the H before the cursor's 7: gone)
    tess_core_text(TESS_TEXT_OK);
    run(80);
    tess_core_view(&v);
    CHECK(!v.typing, "OK (Enter): NAME shut");
    int c = find("A", TESS_ZONE_ROW);
    CHECK(c >= 0 && !std::strcmp(v.zones[c].value, "DUB SKETC7"), "slot A renamed by typing: DUB SKETC7 (%s)",
          c >= 0 ? v.zones[c].value : "?");
    tap(B_HOME);
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
    // inside a DAW (TESS_HOSTED) the list leaves out the device's own rows: walked a row a notch to its end
    {
        std::string seen = "|";
        for (int k = 0; k < 40; k++) {
            for (uint32_t i = 0; i < v.zone_count; i++)
                if (v.zones[i].kind == TESS_ZONE_ROW && seen.find("|" + std::string(v.zones[i].name) + "|") == std::string::npos)
                    seen += std::string(v.zones[i].name) + "|";
            int r = find("COLOR", TESS_ZONE_ROW) >= 0 ? find("COLOR", TESS_ZONE_ROW) : 0;
            for (uint32_t i = 0; i < v.zone_count; i++) if (v.zones[i].kind == TESS_ZONE_ROW) r = (int)i;   // (the last row)
            act(r, TESS_ZA_WHEEL, -1, 20);
        }
        CHECK(seen.find("|ABOUT|") != std::string::npos && seen.find("|MIDI LEARN|") != std::string::npos,
              "SETTINGS walked to its end (%s)", seen.c_str());
        for (const char *gone : {"KNOBS", "LARGE VALUE", "SCREEN OFF", "PANEL GAMMA", "CLOCK OUT", "USB AUDIO", "USB OUTPUT",
                                 "USB VOLUME", "SIM CONTROL", "CALIBRATION"})
            CHECK(seen.find("|" + std::string(gone) + "|") == std::string::npos, "SETTINGS in a DAW: no %s row", gone);
        std::printf("SETTINGS in a DAW: %s\n", seen.c_str());
        act(find("ABOUT", TESS_ZONE_ROW) >= 0 ? find("ABOUT", TESS_ZONE_ROW) : 0, TESS_ZA_WHEEL, 40, 120);   // back to the top
    }

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

static void sheet_track()   // a sheet open on one track: another track picked (TRACKS' row) closes it
{
    tap(B_HOME);
    act(find("T1"), TESS_ZA_CLICK);
    hold(B_OCTUP, 120);   // (OCT+ held: the page's sheet)
    int rows = count(TESS_ZONE_ROW);
    if (!rows) { dump("no sheet"); CHECK(rows > 0, "OCT+ held on HOME opens its sheet"); return; }
    tess_core_track_select(1);   // (as TRACKS' row in EXPANDED, beside the sheet)
    run(5);
    tess_core_view(&v);
    CHECK(v.track == 1 && count(TESS_ZONE_ROW) == 0, "another track picked: the last track's sheet closes (%d rows)",
          count(TESS_ZONE_ROW));
    tap(B_HOME);
}
static void tabs()   // the footer's tabs: a click goes to that page
{
    tap(B_GLO);   // (MIXER, GLO's first page: its tabs in the footer)
    int n = count(TESS_ZONE_TAB), far = -1;
    for (int i = 0; i < (int)v.zone_count; i++)
        if (v.zones[i].kind == TESS_ZONE_TAB && v.zones[i].row >= 2) { far = i; break; }
    CHECK(n >= 3 && far >= 0, "GLO's footer: a zone a tab (%d)", n);
    if (far < 0) { dump("tabs"); tap(B_HOME); return; }
    std::string want = v.zones[far].value;
    act(far, TESS_ZA_CLICK, 0, 40);
    int here = -1;
    for (int i = 0; i < (int)v.zone_count; i++)
        if (v.zones[i].kind == TESS_ZONE_TAB && v.zones[i].row == 0) here = i;
    CHECK(here >= 0 && want == v.zones[here].value, "a click on %s's tab: that page (now %s)", want.c_str(),
          here >= 0 ? v.zones[here].value : "?");
    tap(B_HOME);
    tap(B_GLO);   // the footer's arrow: the page past it (MIXER's family has more tabs than fit)
    int arrow = -1;
    for (int i = 0; i < (int)v.zone_count; i++)
        if (v.zones[i].kind == TESS_ZONE_TAB && v.zones[i].x >= 116) arrow = i;
    CHECK(arrow >= 0 && v.zones[arrow].row == 1, "GLO's footer: a zone on the right arrow, the next page (row %d)",
          arrow >= 0 ? v.zones[arrow].row : 0);
    if (arrow >= 0)
    {
        want = v.zones[arrow].value;
        act(arrow, TESS_ZA_CLICK, 0, 40);
        here = -1;
        for (int i = 0; i < (int)v.zone_count; i++)
            if (v.zones[i].kind == TESS_ZONE_TAB && v.zones[i].row == 0) here = i;
        CHECK(here >= 0 && want == v.zones[here].value, "a click on the right arrow: %s (now %s)", want.c_str(),
              here >= 0 ? v.zones[here].value : "?");
        int left = -1;
        for (int i = 0; i < (int)v.zone_count; i++)
            if (v.zones[i].kind == TESS_ZONE_TAB && v.zones[i].x == 0) left = i;
        CHECK(left < 0 || v.zones[left].row == -1, "a left arrow: the page next to this one, back (row %d)",
              left >= 0 ? v.zones[left].row : 0);
        for (int k = 0; k < 16 && left < 0; k++)   // right arrow again until the tabs scroll: a left arrow
        {
            arrow = left = -1;
            for (int i = 0; i < (int)v.zone_count; i++)
                if (v.zones[i].kind == TESS_ZONE_TAB && v.zones[i].x >= 116) arrow = i;
            if (arrow < 0) break;
            act(arrow, TESS_ZA_CLICK, 0, 40);
            for (int i = 0; i < (int)v.zone_count; i++)
                if (v.zones[i].kind == TESS_ZONE_TAB && v.zones[i].x == 0) left = i;
        }
        CHECK(left >= 0, "the right arrow, again: the tabs scroll, a left arrow");
        if (left >= 0)
        {
            std::string back;
            for (int i = 0; i < (int)v.zone_count; i++)
                if (v.zones[i].kind == TESS_ZONE_TAB && v.zones[i].row == -1) back = v.zones[i].value;
            CHECK(v.zones[left].row == -1 && back == v.zones[left].value, "the left arrow: the page before (%s, not %s)",
                  back.c_str(), v.zones[left].value);
            want = v.zones[left].value;
            act(left, TESS_ZA_CLICK, 0, 40);
            here = -1;
            for (int i = 0; i < (int)v.zone_count; i++)
                if (v.zones[i].kind == TESS_ZONE_TAB && v.zones[i].row == 0) here = i;
            CHECK(here >= 0 && want == v.zones[here].value, "a click on the left arrow: %s (now %s)", want.c_str(),
                  here >= 0 ? v.zones[here].value : "?");
        }
    }
    tap(B_HOME);
}
static void help_close()   // a help card the mouse opened (right-click): a click on it closes it, even a short one
{
    tap(B_FX);
    act(0, TESS_ZA_HELP);
    int card = -1;
    for (int i = 0; i < (int)v.zone_count; i++)
        if (v.zones[i].kind == TESS_ZONE_SCROLL) card = i;
    CHECK(card >= 0, "FX: KNOB 1's help card, a zone (to click)");
    uint32_t open = screen_hash();
    act(card, TESS_ZA_CLICK);
    card = -1;
    for (int i = 0; i < (int)v.zone_count; i++)
        if (v.zones[i].kind == TESS_ZONE_SCROLL) card = i;
    CHECK(card < 0 && screen_hash() != open, "a click on the card closes it");
    tap(B_HOME);
}
static std::string page_tab()   // the footer's tab of the page shown (its title)
{
    tess_core_view(&v);
    for (int i = 0; i < (int)v.zone_count; i++)
        if (v.zones[i].kind == TESS_ZONE_TAB && v.zones[i].row == 0) return v.zones[i].value;
    return "";
}
static void ux_round()   // beta7: a click away, the tempo, TRACKS' double-click
{
    // a click away closes a sheet, then a pinned help card; nothing open: not closable
    tap(B_HOME);
    hold(B_OCTUP, 120);
    CHECK(count(TESS_ZONE_ROW) > 0 && v.closable, "OCT+ held: HOME's sheet, closable (%u)", v.closable);
    tess_core_away();
    run(20);
    tess_core_view(&v);
    CHECK(count(TESS_ZONE_ROW) == 0 && !v.closable, "a click away closes the sheet (%d rows)", count(TESS_ZONE_ROW));
    tap(B_FX);
    act(0, TESS_ZA_HELP);
    CHECK(count(TESS_ZONE_SCROLL) == 1 && v.closable, "a help card the mouse opened: closable");
    tess_core_away();
    run(20);
    tess_core_view(&v);
    CHECK(count(TESS_ZONE_SCROLL) == 0 && !v.closable, "a click away closes the help card");
    // the tempo in the header: the wheel a step a notch, a double-click its default
    tap(B_HOME);
    int t = find("TEMPO", TESS_ZONE_TEMPO);
    CHECK(t >= 0, "HOME's header: a TEMPO zone");
    if (t >= 0)
    {
        uint32_t was = tess_core_bpm();
        act(t, TESS_ZA_WHEEL, 3);
        CHECK(tess_core_bpm() == was + 3, "the wheel up 3 on the tempo: %u, then %u", was, tess_core_bpm());
        t = find("TEMPO", TESS_ZONE_TEMPO);
        act(t, TESS_ZA_DOUBLE);
        CHECK(tess_core_bpm() == 120, "a double-click: 120 (%u)", tess_core_bpm());
    }
    tap(B_GLO);   // (MIXER: a screen of its own, its header with the tempo; a page of four values has no room for it)
    for (int i = 0; i < 16 && find_strip(TESS_ZONE_PAN, "T1", "PAN") < 0; i++) tap(B_GLO);   // (GLO reopens its last page)
    t = find("TEMPO", TESS_ZONE_TEMPO);
    CHECK(t >= 0, "MIXER's header: a TEMPO zone too");
    // TRACKS (EXPANDED): a row double-clicked opens that track's EDIT page
    tess_core_track_select(2);
    run(5);
    tap(B_HOME);
    tap(B_EDIT);
    std::string edit = page_tab();   // (T3's EDIT page, as its EDIT button opens it)
    tess_core_track_select(0);
    run(5);
    tap(B_HOME);
    tess_zone_t row = {};
    row.kind = TESS_ZONE_TRACK;
    row.row = 2;
    tess_core_tracks_do(&row, TESS_ZA_DOUBLE, 0);
    run(30);
    CHECK(tess_core_track() == 2 && !edit.empty() && page_tab() == edit, "TRACKS: T3 double-clicked: T3, its EDIT page "
          "(%s, want %s)", page_tab().c_str(), edit.c_str());
    tess_core_tracks_do(&row, TESS_ZA_DOUBLE, 0);   // (there already: it stays, not the next EDIT page)
    run(30);
    CHECK(page_tab() == edit, "again: the same EDIT page (%s)", page_tab().c_str());
    tess_core_track_select(0);
    tap(B_HOME);
}
static void slot_tabs()   // a slot page (FM6's STORE): its family's tabs in the footer, as the pages beside it; the
{                         // tabs walked by clicks across it
    bool found = false;
    for (uint32_t tr = 0; tr < tess_core_tracks() && !found; tr++)
    {
        tess_core_track_select(tr);
        run(5);
        tap(B_HOME);
        tap(B_EDIT);
        int dir = 1;
        for (int i = 0; i < 20 && !found; i++)
        {
            tess_core_view(&v);
            bool slots = false;
            for (uint32_t z = 0; z < v.zone_count; z++)
                slots |= v.zones[z].kind == TESS_ZONE_ROW && v.zones[z].name[0] == 'F' && v.zones[z].name[1] == '0';
            if (slots)
            {
                found = true;
                CHECK(count(TESS_ZONE_TAB) >= 3, "FM6's STORE: the EDIT pages' tabs in its footer (%d)", count(TESS_ZONE_TAB));
                if (const char *o = std::getenv("ZONES_SHOT"))   // (a look at it: ZONES_SHOT=file.ppm)
                    if (FILE *f = std::fopen(o, "wb"))
                    {
                        tess_screen_t sc;
                        tess_core_screen(&sc);
                        std::fprintf(f, "P6\n%u %u\n255\n", sc.w, sc.h);
                        for (uint32_t p = 0; p < sc.w * sc.h; p++)
                        {
                            uint16_t c = sc.pixels[p];
                            unsigned char rgb[3] = {(unsigned char)((c >> 11) << 3), (unsigned char)(((c >> 5) & 63) << 2),
                                                    (unsigned char)((c & 31) << 3)};
                            std::fwrite(rgb, 1, 3, f);
                        }
                        std::fclose(f);
                    }
                int next = -1;   // (the tab beside it: STORE is the EDIT pages' last, so the one before)
                for (int side = 1; side >= -1 && next < 0; side -= 2)
                    for (uint32_t z = 0; z < v.zone_count; z++)
                        if (v.zones[z].kind == TESS_ZONE_TAB && v.zones[z].row == side) next = (int)z;
                std::string want = next >= 0 ? v.zones[next].value : "";
                act(next, TESS_ZA_CLICK, 0, 40);
                int here = -1;
                for (uint32_t z = 0; z < v.zone_count; z++)
                    if (v.zones[z].kind == TESS_ZONE_TAB && v.zones[z].row == 0) here = (int)z;
                CHECK(next >= 0 && here >= 0 && want == v.zones[here].value, "from STORE, the tab beside it clicked: %s",
                      want.c_str());
                break;
            }
            int next = -1;   // (the next page that way: its tab; at the family's end, back the other way)
            for (int pass = 0; pass < 2 && next < 0; pass++)
            {
                for (uint32_t z = 0; z < v.zone_count; z++)
                    if (v.zones[z].kind == TESS_ZONE_TAB && v.zones[z].row == dir) next = (int)z;
                if (next < 0) dir = -dir;
            }
            if (next < 0) break;
            act(next, TESS_ZA_CLICK, 0, 40);
        }
    }
    CHECK(found, "an FM6 track's STORE page");
    tess_core_track_select(0);
    run(5);
    tap(B_HOME);
}
static int reach(int button, const char *name, uint32_t kind, int taps = 12)   // tap a button till a zone shows
{
    for (int i = 0; i < taps; i++)
    {
        tess_core_view(&v);
        if (find(name, kind) >= 0) return find(name, kind);
        tap(button);
    }
    tess_core_view(&v);
    return find(name, kind);
}
static void mouse2()   // the mouse, round 2: MODS, MATRIX, HOME's double-click, the browser, HOVER HELP
{
    // MODS: a row a mod; its SOURCE a picker, kept till a row of it is clicked
    tap(B_HOME);
    int src = reach(B_LFO, "SOURCE", TESS_ZONE_PICK);
    CHECK(src >= 0, "MODS: a SOURCE zone a mod");
    if (src >= 0)
    {
        int rows = count(TESS_ZONE_ROW);
        CHECK(rows >= 4, "MODS: a row a mod (%d)", rows);
        tess_zone_t s2 = v.zones[src];
        for (int i = 0; i < (int)v.zone_count; i++)
            if (v.zones[i].kind == TESS_ZONE_PICK && !std::strcmp(v.zones[i].name, "SOURCE") && v.zones[i].row == 1)
                s2 = v.zones[i];
        tess_core_zone_do(&s2, TESS_ZA_CLICK, 0);
        run(400);   // (longer than a picker's own 1.5 s: kept)
        tess_core_view(&v);
        int pick_rows = 0;
        for (int i = 0; i < (int)v.zone_count; i++)
            pick_rows += v.zones[i].kind == TESS_ZONE_ROW && v.zones[i].role == 3;   // (EN_K1: the picker's list)
        CHECK(pick_rows > 0 && v.closable, "MOD 2's SOURCE clicked: its list, still open after 1.5 s (%d rows)", pick_rows);
        int r = -1;   // (the picker's last row shown: another source than now)
        for (int i = 0; i < (int)v.zone_count; i++)
            if (v.zones[i].kind == TESS_ZONE_ROW && v.zones[i].role == 3) r = i;
        if (r >= 0)
        {
            std::string want = v.zones[r].name;
            act(r, TESS_ZA_CLICK, 0, 40);
            CHECK(!v.closable, "a row of it clicked: the list goes");
            int s = -1;
            for (int i = 0; i < (int)v.zone_count; i++)
                if (v.zones[i].kind == TESS_ZONE_PICK && !std::strcmp(v.zones[i].name, "SOURCE") && v.zones[i].row == 1)
                    s = i;
            CHECK(s >= 0 && std::string(v.zones[s].value).size() > 0 && std::string(v.zones[s].value) != "--",
                  "MOD 2's source is set (%s)", s >= 0 ? v.zones[s].value : "?");
        }
        else
            CHECK(false, "the picker's rows");
    }
    // MATRIX: a cell clicked opens its mod in MODS
    tap(B_HOME);
    int cell = reach(B_LFO, "T1>T1", TESS_ZONE_CELL, 16);
    CHECK(cell >= 0, "MATRIX: a cell a pair of tracks");
    {   // G4: a cell dragged onto another: a new mod from the first's row (FROM) to the second's column (TO), opened in MODS
        int from = find("T1>T1", TESS_ZONE_CELL), to = find("T1>T3", TESS_ZONE_CELL);
        if (to < 0) to = find("T1>T2", TESS_ZONE_CELL);
        CHECK(from >= 0 && to >= 0, "MATRIX: T1>T1 and a cell to its right");
        if (from >= 0 && to >= 0)
        {
            std::string name = v.zones[to].name, was = v.zones[to].value;
            int8_t n = (int8_t)(uint8_t)((uint16_t)v.zones[from].row & 0xFFu);
            act(to, TESS_ZA_LINK, n, 40);
            bool mods = find(name.c_str(), TESS_ZONE_CELL) < 0;
            CHECK(mods, "dropped on %s: MODS opened (the MATRIX cells gone)", name.c_str());
            tap(B_OCTDN);   // (OCT- in MODS: back to MATRIX)
            run(20);
            int again = find(name.c_str(), TESS_ZONE_CELL);
            CHECK(again >= 0 && !std::strcmp(v.zones[again].value, "OPEN") && was == "--",
                  "back in MATRIX: %s has a mod now (%s, was %s)", name.c_str(), again >= 0 ? v.zones[again].value : "?", was.c_str());
        }
    }
    // HOME: a track double-clicked opens its sounds (TESSERA's shell: BACK / OK of its own, as the plugin has them)
    tess_core_key_words(1);
    tap(B_HOME);
    int t2 = find("T2", TESS_ZONE_TRACK);
    act(t2, TESS_ZA_CLICK, 0, 5);
    t2 = find("T2", TESS_ZONE_TRACK);
    act(t2, TESS_ZA_DOUBLE, 0, 40);
    int snd = find("SOUND", TESS_ZONE_CELL), ok = find("OK", TESS_ZONE_KEY);
    CHECK(tess_core_track() == 1 && snd >= 0 && ok >= 0, "HOME: T2 double-clicked: its sounds (the browser)");
    if (snd >= 0 && ok >= 0)
    {
        int below = -1;
        for (int i = 0; i < (int)v.zone_count; i++)
            if (v.zones[i].kind == TESS_ZONE_CELL && v.zones[i].role == 4 && (int8_t)(v.zones[i].row & 0xFF) == 1) below = i;
        std::string name = below >= 0 ? v.zones[below].value : "";
        act(below, TESS_ZA_CLICK, 0, 80);
        int card = -1;
        for (int i = 0; i < (int)v.zone_count; i++)
            if (v.zones[i].kind == TESS_ZONE_CELL && v.zones[i].role == 4 && (v.zones[i].row & 0xFF) == 0) card = i;
        CHECK(card >= 0, "a sound clicked: the browser on it (%s)", name.c_str());
        int back = find("BACK", TESS_ZONE_KEY);
        CHECK(back >= 0, "a sound loaded: BACK there to take it back");
        act(back, TESS_ZA_CLICK, 0, 60);   // (BACK: T2's sound from before, and the browser goes, as OK would)
        CHECK(find("OK", TESS_ZONE_KEY) < 0 && find("SOUND", TESS_ZONE_CELL) < 0, "the browser's BACK clicked: it goes");
    }
    tess_core_key_words(0);
    // HOVER HELP: a value's column has its help text
    tap(B_FX);
    tess_zone_t k1 = v.zones[0];
    tess_core_hover(&k1);
    run(2);
    tess_core_view(&v);
    CHECK(v.hover.kind == TESS_ZONE_KNOB && std::strlen(v.hover_help) > 20, "HOVER HELP: FX's KNOB 1 says what it does (%.40s)",
          v.hover_help);
    tess_core_hover(nullptr);
    run(2);
    tess_core_view(&v);
    CHECK(!v.hover.kind && !v.hover_help[0], "no zone: no text");
    tess_core_track_select(0);
    run(5);
    tap(B_HOME);
}
static void rdo(int step, int cy, uint32_t a, int n = 0)   // NOTES' roll: the mouse at step (0..15), row cy (cells)
{
    int i = find("NOTES", TESS_ZONE_ROLL);
    if (i < 0) return;
    tess_zone_t z = v.zones[i];
    z.x = (uint16_t)(2 * (3 + step * 7 + step / 4 * 2 + 2)), z.y = (uint16_t)(2 * cy);
    tess_core_zone_do(&z, a, n);
    run(30);
}
static std::string kval(const char *name)
{
    int i = find(name, TESS_ZONE_KNOB);
    return i < 0 ? "?" : v.zones[i].value;
}
static int midi_of(const std::string &s)   // "C#4" -> 61 (C4 = 60), -1 not a note
{
    static const int PC[7] = {9, 11, 0, 2, 4, 5, 7};
    if (s.empty() || s[0] < 'A' || s[0] > 'G') return -1;
    size_t k = 1;
    int n = PC[s[0] - 'A'];
    if (k < s.size() && s[k] == '#') n++, k++;
    if (k < s.size() && s[k] == 'b') n--, k++;
    return k < s.size() ? n + 12 * (std::atoi(s.c_str() + k) + 1) : -1;
}
static bool lit(int cx, int cy)   // a Mono cell drawn in (its top-left pixel)
{
    return v.screen[(2 * cy) * TESS_SCREEN + 2 * cx] != v.screen[0];
}
static void roll()   // NOTES as a piano roll (batch 4): a click a note, a chord, a drag its pitch, a right-click gone
{
    tess_core_project_new();
    run(50);
    int i = reach(B_SEQ, "NOTES", TESS_ZONE_ROLL);
    CHECK(i >= 0, "NOTES (a new project's T1): a ROLL zone over the steps");
    if (i < 0) { dump("no NOTES"); return; }
    const tess_zone_t &z = v.zones[i];
    CHECK(z.x <= 8 && z.x + z.w >= 236 && z.y <= 132 && z.y + z.h >= 194, "the roll covers the 16 steps (%u,%u %ux%u)", z.x,
          z.y, z.w, z.h);
    CHECK(kval("LENGTH") == "--", "step 1 starts empty (LENGTH %s)", kval("LENGTH").c_str());

    rdo(0, 78, TESS_ZA_CLICK);   // an empty page: a note on step 1, about the last note entered
    int p1 = midi_of(kval("PITCH"));
    CHECK(kval("STEP").rfind("1/", 0) == 0 && kval("LENGTH") != "--" && p1 > 0, "a click on step 1: a note there (%s %s)",
          kval("STEP").c_str(), kval("PITCH").c_str());
    rdo(2, 66, TESS_ZA_CLICK);   // higher up, step 3: a higher note
    int p3 = midi_of(kval("PITCH"));
    CHECK(kval("STEP").rfind("3/", 0) == 0 && p3 > p1, "a click higher on step 3: a higher note there (%d, then %d)", p1, p3);
    rdo(2, 82, TESS_ZA_CLICK);   // step 3 again, lower: a chord, its second note picked
    int p3b = midi_of(kval("PITCH"));
    int x3 = 3 + 2 * 7;
    CHECK(p3b > 0 && p3b != p3 && kval("STEP").rfind("3/", 0) == 0, "a second click on step 3: a second note (%d, %d)", p3,
          p3b);
    {
        int top = 0, notch = 0;   // (a chord's notes at their own pitch: the lower one a notch cut in step 3's bar)
        for (int cy = 60; cy < 92 && !top; cy++) if (lit(x3, cy)) top = cy;
        for (int cy = top + 1; cy < 92 && top && !notch; cy++) if (lit(x3, cy) && !lit(x3 + 2, cy)) notch = cy;
        CHECK(top > 0 && notch > top, "the chord's second note at its pitch, a notch in step 3's bar (top %d, notch %d)", top,
              notch);
        rdo(2, notch, TESS_ZA_HELP);   // a right-click on it: that note gone, the bar whole again
        CHECK(notch > 0 && lit(x3 + 2, notch), "a right-click on the chord's note removes that note");
        CHECK(midi_of(kval("PITCH")) == p3 || kval("STEP").rfind("3/", 0) == 0, "step 3 keeps its first note");
        rdo(2, 82, TESS_ZA_CLICK);   // (the second note again, for the chord drag below)
        rdo(2, 82, TESS_ZA_GRAB);   // a chord's note grabbed where it is drawn, dragged: that note, not the first
        rdo(2, 82, TESS_ZA_TURN, -1);
        CHECK(midi_of(kval("PITCH")) < p3b, "a chord's lower note dragged down: it moves (%d, then %s)", p3b,
              kval("PITCH").c_str());
    }

    rdo(0, 90, TESS_ZA_GRAB);   // drag step 1's note up two rows
    rdo(0, 90, TESS_ZA_TURN, 2);
    int p1b = midi_of(kval("PITCH"));
    CHECK(kval("STEP").rfind("1/", 0) == 0 && p1b > p1 && p1b <= p1 + 4, "step 1 dragged up two rows (%d, then %d)", p1,
          p1b);
    rdo(0, 90, TESS_ZA_GRAB);
    rdo(0, 90, TESS_ZA_TURN, -2);
    CHECK(midi_of(kval("PITCH")) == p1, "and down two: back (%d, %s)", p1, kval("PITCH").c_str());
    rdo(0, 90, TESS_ZA_WHEEL, 1);
    CHECK(midi_of(kval("PITCH")) > p1, "the wheel on it: up a row (%s)", kval("PITCH").c_str());

    rdo(2, 90, TESS_ZA_GRAB);   // step 3's chord whole, up a row
    rdo(2, 90, TESS_ZA_TURN_CHORD, 1);
    CHECK(kval("STEP").rfind("3/", 0) == 0 && midi_of(kval("PITCH")) > p3, "SHIFT + drag: step 3's chord up (%d, then %s)",
          p3, kval("PITCH").c_str());

    for (int k = 0; k < 6; k++) rdo(5, 64 + 4 * k, TESS_ZA_CLICK);   // six pitches on step 6: four notes, no more
    int dels = 0;
    while (dels < 8 && kval("LENGTH") != "--") rdo(5, 90, TESS_ZA_HELP), dels++;
    CHECK(dels == 4 && kval("STEP").rfind("6/", 0) == 0, "six clicks on step 6 made a chord of 4: four right-clicks empty it "
          "(%d)", dels);
    rdo(0, 90, TESS_ZA_HELP);   // step 1's note gone
    CHECK(kval("LENGTH") == "--" || kval("STEP").rfind("1/", 0) != 0, "a right-click on step 1's note: gone (%s %s)",
          kval("STEP").c_str(), kval("LENGTH").c_str());

    tess_core_transport(1);   // playing is fine; recording is not tested here
    run(20);
    rdo(8, 80, TESS_ZA_CLICK);
    CHECK(kval("STEP").rfind("9/", 0) == 0, "while playing: a click still places a note (%s)", kval("STEP").c_str());
    tess_core_transport(0);
    run(20);
}

static bool roll_has(int step)   // NOTES' roll: a bar drawn on step (0..15)
{
    int x = 3 + step * 7 + step / 4 * 2 + 2;
    for (int cy = 64; cy < 91; cy++) if (lit(x, cy)) return true;   // (row 91: the baseline under every step)
    return false;
}
static void ldo(int step, int lane, uint32_t a, int n = 0)   // the drum grid: the mouse on lane (0..7), step
{
    int i = find("GRID", TESS_ZONE_ROLL);
    if (i < 0) return;
    tess_zone_t z = v.zones[i];
    z.x = (uint16_t)(2 * (18 + step * 6 + step / 4 + 2)), z.y = (uint16_t)(2 * (15 + lane * 10 + 3));
    tess_core_zone_do(&z, a, n);
    run(30);
}
static bool glit(int cx, int cy)   // the grid's cell drawn in (against the gap left of the cells, at that row)
{
    return v.screen[(2 * cy) * TESS_SCREEN + 2 * cx] != v.screen[(2 * cy) * TESS_SCREEN + 2 * 16];
}
static bool lane_hit(int step, int lane) { return glit(18 + step * 6 + step / 4 + 1, 15 + lane * 10 + 2); }
static bool lane_acc(int step, int lane) { return glit(18 + step * 6 + step / 4, 15 + lane * 10 + 1); }
static void paint()   // G3: a drag across NOTES' steps paints or erases a run; Alt-drags a note's length, velocity; the
{                     // drum grid the same, a hit for a note, up or down its accent
    tess_core_project_new();
    run(50);
    tess_core_track_select(0);
    int i = reach(B_SEQ, "NOTES", TESS_ZONE_ROLL);
    CHECK(i >= 0, "G3: NOTES' roll");
    if (i < 0) return;
    rdo(0, 80, TESS_ZA_GRAB);   // pressed on an empty step 1, dragged across to step 4: four notes
    for (int k = 1; k <= 3; k++) rdo(k, 80, TESS_ZA_PAINT);
    rdo(3, 80, TESS_ZA_DROP);
    CHECK(roll_has(0) && roll_has(1) && roll_has(2) && roll_has(3) && !roll_has(4), "a drag from empty step 1 to 4: four notes");
    rdo(9, 80, TESS_ZA_GRAB);   // a quick drag right to left skips nothing: steps 10 back to 7
    rdo(6, 80, TESS_ZA_PAINT);
    rdo(6, 80, TESS_ZA_DROP);
    CHECK(roll_has(6) && roll_has(7) && roll_has(8) && roll_has(9) && !roll_has(5) && !roll_has(10),
          "a drag from step 10 back to 7 in one go: all four painted");
    rdo(1, 90, TESS_ZA_GRAB);   // pressed on step 2's note: erasing, steps 2 and 3
    rdo(2, 90, TESS_ZA_PAINT);
    rdo(2, 90, TESS_ZA_DROP);
    CHECK(roll_has(0) && !roll_has(1) && !roll_has(2) && roll_has(3), "a drag from step 2's note to 3: both erased, 1 and 4 kept");
    // the whole stroke one undo step: EDIT held, OCT-
    tess_core_buttons(1u << B_EDIT);
    run(60);
    tess_core_buttons(1u << B_EDIT | 1u << B_OCTDN);
    run(5);
    tess_core_buttons(1u << B_EDIT);
    run(5);
    tess_core_buttons(0);
    run(30);
    CHECK(roll_has(1) && roll_has(2) && roll_has(6), "one undo: the erase stroke back whole, the strokes before kept");
    rdo(0, 90, TESS_ZA_GRAB);   // Alt and a drag across step 1's note: two steps longer
    rdo(0, 90, TESS_ZA_LENGTH, 1);
    rdo(0, 90, TESS_ZA_DROP);
    CHECK(kval("STEP").rfind("1/", 0) == 0 && kval("LENGTH") == "1", "a note can't grow over the next one (LENGTH %s)",
          kval("LENGTH").c_str());
    rdo(3, 90, TESS_ZA_GRAB);   // step 4's note: steps 5, 6 are free
    rdo(3, 90, TESS_ZA_LENGTH, 1);
    rdo(3, 90, TESS_ZA_LENGTH, 1);
    rdo(3, 90, TESS_ZA_DROP);
    CHECK(kval("STEP").rfind("4/", 0) == 0 && kval("LENGTH") == "3", "Alt-drag across step 4's note: 3 steps long (%s)",
          kval("LENGTH").c_str());
    rdo(3, 90, TESS_ZA_GRAB);
    rdo(3, 90, TESS_ZA_LENGTH, -1);
    rdo(3, 90, TESS_ZA_DROP);
    CHECK(kval("LENGTH") == "2", "and back one: 2 (%s)", kval("LENGTH").c_str());
    std::string v0 = kval("VEL");
    rdo(3, 90, TESS_ZA_GRAB);   // Alt and a drag up: its velocity, 4 a detent
    rdo(3, 90, TESS_ZA_VELOCITY, 2);
    rdo(3, 90, TESS_ZA_DROP);
    CHECK(std::atoi(kval("VEL").c_str()) == std::atoi(v0.c_str()) + 8, "Alt-drag up two: VEL %s, then %s", v0.c_str(),
          kval("VEL").c_str());

    // the drum grid (a DRUM track's NOTES)
    int g = -1;
    for (uint32_t tr = 0; tr < tess_core_tracks() && g < 0; tr++)
    {
        tess_core_track_select(tr);
        run(10);
        tap(B_HOME);
        tap(B_SEQ);
        run(10);
        act(find("NOTE", TESS_ZONE_TAB), TESS_ZA_CLICK);
        g = find("GRID", TESS_ZONE_ROLL);
    }
    CHECK(g >= 0, "a DRUM track's NOTES: the grid's zone");
    if (g < 0) { tess_core_track_select(0); return; }
    bool h0 = lane_hit(0, 1);
    ldo(0, 1, TESS_ZA_CLICK);
    CHECK(lane_hit(0, 1) != h0, "a click on a cell toggles its hit");
    if (!lane_hit(0, 1)) ldo(0, 1, TESS_ZA_CLICK);
    for (int k = 1; k < 8; k++) if (lane_hit(k, 1)) ldo(k, 1, TESS_ZA_HELP);
    CHECK(!lane_hit(1, 1) && !lane_hit(5, 1), "right-clicks clear hits");
    ldo(2, 1, TESS_ZA_GRAB);   // off a hit: a drag across draws the lane's hits
    ldo(5, 1, TESS_ZA_PAINT);
    ldo(5, 1, TESS_ZA_DROP);
    CHECK(lane_hit(2, 1) && lane_hit(3, 1) && lane_hit(4, 1) && lane_hit(5, 1) && !lane_hit(6, 1), "a drag from 3 to 6: four hits");
    ldo(3, 1, TESS_ZA_GRAB);   // on a hit: erasing
    ldo(4, 1, TESS_ZA_PAINT);
    ldo(4, 1, TESS_ZA_DROP);
    CHECK(lane_hit(2, 1) && !lane_hit(3, 1) && !lane_hit(4, 1) && lane_hit(5, 1), "a drag from a hit: erased");
    ldo(2, 1, TESS_ZA_GRAB);   // up on a hit: accented; down: not
    ldo(2, 1, TESS_ZA_TURN, 1);
    ldo(2, 1, TESS_ZA_DROP);
    CHECK(lane_acc(2, 1), "a drag up on a hit: accented");
    ldo(2, 1, TESS_ZA_WHEEL, -1);
    CHECK(lane_hit(2, 1) && !lane_acc(2, 1), "the wheel down: not");
    tess_core_track_select(0);
    tap(B_HOME);
}

static void drop_sound()   // G4: a sound of the browser dropped on a TRACKS row: that track loads it, the selected one stays
{
    if (tess_core_tracks() < 8) return;
    tess_core_project_new();
    run(50);
    tess_core_key_words(1);
    tess_core_track_select(0);
    tap(B_HOME);
    act(find("T1", TESS_ZONE_TRACK), TESS_ZA_DOUBLE, 0, 40);
    int below = -1;
    for (int i = 0; i < (int)v.zone_count; i++)
        if (v.zones[i].kind == TESS_ZONE_CELL && v.zones[i].role == 4 && (int8_t)(v.zones[i].row & 0xFF) == 1) below = i;
    CHECK(below >= 0, "G4: T1's browser, a sound below the one shown");
    if (below < 0) { tess_core_key_words(0); return; }
    std::string name = v.zones[below].value;
    act(below, TESS_ZA_CLICK, 0, 80);   // (the press picks it, as the panel's does)
    tess_core_tracks_view(&tv, TESS_TRACKS_W, TESS_TRACKS_H);
    int t1 = tfind(TESS_ZONE_TRACK, 0), t6 = tfind(TESS_ZONE_TRACK, 5);
    CHECK(t1 >= 0 && t6 >= 0, "TRACKS: T1's and T6's rows");
    if (t1 < 0 || t6 < 0) { tess_core_key_words(0); return; }
    std::string picked = tv.zones[t1].value, before = tv.zones[t6].value;
    CHECK(picked != before, "T1 picked %s (%s), T6 %s", picked.c_str(), name.c_str(), before.c_str());
    {
        tess_zone_t z = tv.zones[t6];
        tess_core_tracks_do(&z, TESS_ZA_SOUND, 0);
        run(40);
    }
    tess_core_tracks_view(&tv, TESS_TRACKS_W, TESS_TRACKS_H);
    t6 = tfind(TESS_ZONE_TRACK, 5);
    CHECK(t6 >= 0 && picked == tv.zones[t6].value, "dropped on T6's row: T6 loads %s (%s)", picked.c_str(),
          t6 >= 0 ? tv.zones[t6].value : "?");
    CHECK(tess_core_track() == 0 && find("SOUND", TESS_ZONE_CELL) >= 0, "T1 still selected, its browser still open");
    tess_core_key_words(0);
    tap(B_HOME);
}

static void macro_mods()   // PERF: a MACRO's row clicked opens its mod in MODS (a new one from MCRn when it has none)
{
    tess_core_project_new();
    run(10);
    tess_core_key_words(1);
    tap(B_HOME);
    press_section(TESS_SEC_MIX);
    for (int g = 0; g < 12 && page_now() != "PERF"; g++)
    {
        int nx = -1;
        for (uint32_t i = 0; i < v.zone_count; i++)
            if (v.zones[i].kind == TESS_ZONE_TAB && v.zones[i].row == 1) nx = (int)i;
        if (nx < 0) break;
        act(nx, TESS_ZA_CLICK);
    }
    CHECK(page_now() == "PERF", "MIX: PERF reached (%s)", page_now().c_str());
    int row = -1;
    for (uint32_t i = 0; i < v.zone_count; i++)
        if (v.zones[i].kind == TESS_ZONE_CELL && !std::strcmp(v.zones[i].name, "MCR2")) row = (int)i;
    CHECK(row >= 0 && !std::strcmp(v.zones[row].value, "NO MOD"), "PERF: MCR2's row a zone, NO MOD");
    if (row < 0) { tess_core_key_words(0); return; }
    act(row, TESS_ZA_CLICK);
    CHECK(page_now() == "MODS", "MCR2 clicked: MODS (%s)", page_now().c_str());
    tap(B_HOME);
    press_section(TESS_SEC_MIX);
    for (int g = 0; g < 12 && page_now() != "PERF"; g++)
    {
        int nx = -1;
        for (uint32_t i = 0; i < v.zone_count; i++)
            if (v.zones[i].kind == TESS_ZONE_TAB && v.zones[i].row == 1) nx = (int)i;
        if (nx < 0) break;
        act(nx, TESS_ZA_CLICK);
    }
    row = -1;
    for (uint32_t i = 0; i < v.zone_count; i++)
        if (v.zones[i].kind == TESS_ZONE_CELL && !std::strcmp(v.zones[i].name, "MCR2")) row = (int)i;
    CHECK(row >= 0 && !std::strcmp(v.zones[row].value, "1 MOD"), "..MCR2 drives a mod now (%s)", row >= 0 ? v.zones[row].value : "-");
    if (row >= 0) act(row, TESS_ZA_CLICK);
    CHECK(page_now() == "MODS", "MCR2 clicked again: its mod in MODS, no second one (%s)", page_now().c_str());
    tap(B_HOME);
    press_section(TESS_SEC_MIX);   // (MIX remembers its page: back on MIXER for the tests after)
    for (int g = 0; g < 12 && page_now() != "MIXER"; g++)
    {
        int pv = -1;
        for (uint32_t i = 0; i < v.zone_count; i++)
            if (v.zones[i].kind == TESS_ZONE_TAB && v.zones[i].row == -1) pv = (int)i;
        if (pv < 0) break;
        act(pv, TESS_ZA_CLICK);
    }
    tap(B_HOME);
    tess_core_key_words(0);
}

static void scale_keys()   // G10: the keys that give no new note in the scale dim, the root's marked, the chord named
{
    tess_core_project_new();
    run(10);
    tess_core_track_select(0);
    tap(B_HOME);
    uint32_t root = pid_named("ROOT"), quant = pid_named("QUANTIZE"), trans = pid_named("TRANSPOSE"), scale = pid_named("SCALE");
    CHECK(root != 0xFFFF && quant != 0xFFFF && trans != 0xFFFF && scale != 0xFFFF, "G10: ROOT, QUANTIZE, TRANSPOSE, SCALE");
    tess_core_param_set(root, 0, 0), tess_core_param_set(trans, 0, 0), tess_core_param_set(quant, 0, 1);   // (C, SNAP)
    char sc[24] = "";
    for (int32_t i = 0; i < 40; i++)
    {
        tess_core_param_text(scale, 0, i, sc, sizeof sc);
        if (!std::strcmp(sc, "MAJ") || !std::strcmp(sc, "MAJOR")) { tess_core_param_set(scale, 0, i); break; }
    }
    run(4);
    tess_core_view(&v);
    uint32_t in_key = 0;
    for (int k = 0; k < 12; k++) in_key |= (~v.key_dim >> k & 1u) << ((k + 5) % 12);   // (by pitch class: key 0 is F)
    CHECK(in_key == 0xAB5u, "G10: C major on the keys, SNAP: C D E F G A B lit, the sharps dim (%03x, scale %s)", in_key, sc);
    CHECK(v.key_root == (1u << 7 | 1u << 19), "G10: the C keys marked as the root (%06x)", v.key_root);
    tess_core_param_set(quant, 0, 0);   // OFF: every key its own note, the ones out of the scale dim all the same
    run(4);
    tess_core_view(&v);
    in_key = 0;
    for (int k = 0; k < 12; k++) in_key |= (~v.key_dim >> k & 1u) << ((k + 5) % 12);
    CHECK(in_key == 0xAB5u, "G10: QUANTIZE OFF too (%03x)", in_key);
    tess_core_param_set(root, 0, 9);   // A: the A keys the root
    run(4);
    tess_core_view(&v);
    CHECK(v.key_root == (1u << 4 | 1u << 16), "G10: ROOT A marks the A keys (%06x)", v.key_root);
    tess_core_project_new();
}

static void constel_drag()   // CONSTELLATION: the ship dragged up and down sets Y, a bracket dragged away widens SIZE
{
    tess_core_project_new();
    run(10);
    tess_core_track_select(0);
    CHECK(tess_core_param_set(0x200, 0, 10), "CONSTL: T1's MODE set to CONSTELLATION");   // (TX_N_MODE: CONSTL)
    run(10);
    tess_core_key_words(1);
    tap(B_HOME);
    press_section(TESS_SEC_GEN);
    auto find_pt = [&](const char *name, int flags) {
        for (uint32_t i = 0; i < v.zone_count; i++)
            if (v.zones[i].kind == TESS_ZONE_POINT && !std::strcmp(v.zones[i].name, name) && v.zones[i].row == flags)
                return (int)i;
        return -1;
    };
    int ship = -1;
    for (int g = 0; g < 8 && ship < 0; g++)
    {
        ship = find_pt("Y", 0xC00);
        if (ship < 0) press_section(TESS_SEC_GEN);
    }
    CHECK(ship >= 0 && find_pt("SIZE", 0xC00) >= 0 && find_pt("SIZE", 0xC02) >= 0,
          "CONSTL: the ship (Y) and both brackets (SIZE) are zones (page %s)", page_now().c_str());
    if (ship >= 0)
    {
        const uint32_t py = 0x200 + (uint32_t)v.zones[ship].vrole, ps = 0x200 + (uint32_t)v.zones[find_pt("SIZE", 0xC00)].vrole;
        int32_t y0 = tess_core_param_get(py, 0);
        act(ship, TESS_ZA_TURN, 3);   // (sideways: nothing)
        CHECK(tess_core_param_get(py, 0) == y0, "..a sideways drag leaves Y (%d)", tess_core_param_get(py, 0));
        act(find_pt("Y", 0xC00), TESS_ZA_TURN_Y, 3);
        CHECK(tess_core_param_get(py, 0) == y0 + 3, "..up 3 turns Y 3 up (%d -> %d)", y0, tess_core_param_get(py, 0));
        int32_t s0 = tess_core_param_get(ps, 0);
        act(find_pt("SIZE", 0xC00), TESS_ZA_TURN_Y, 2);
        int32_t s1 = tess_core_param_get(ps, 0);
        act(find_pt("SIZE", 0xC02), TESS_ZA_TURN_Y, 2);   // (up: the bottom bracket in, SIZE back)
        int32_t s2 = tess_core_param_get(ps, 0);
        CHECK(s1 == s0 + 2 && s2 == s0, "..the top bracket up widens SIZE, the bottom one up narrows it (%d %d %d)", s0, s1, s2);
        act(find_pt("SIZE", 0xC00), TESS_ZA_DOUBLE);
        CHECK(tess_core_param_get(ps, 0) == 2, "..a double-click: SIZE's default (%d)", tess_core_param_get(ps, 0));
    }
    tap(B_HOME);
    tess_core_key_words(0);
    tess_core_project_new();
}

static void live_motion()   // G8: KNOB 1..4's places in the view; TRACKS rows: a meter and a note's flash while playing
{
    tess_core_load(SIM_DEMO_A, sizeof SIM_DEMO_A);
    run(30);
    tess_core_track_select(0);
    tap(B_HOME);
    tap(B_FX);
    int known = 0;
    for (int i = 0; i < 4; i++) known += v.knob_at[i] >= 0 && v.knob_at[i] <= 1000;
    CHECK(known >= 2, "G8: FX's knobs have their places in the view (%d)", known);
    tap(B_HOME);
    if (tess_core_tracks() < 8) return;
    tess_core_transport(1);
    int lit = 0, flashed = 0;
    for (int f = 0; f < 120; f++)
    {
        run(2);
        tess_core_tracks_view(&tv, TESS_TRACKS_W, TESS_TRACKS_H);
        for (int k = 0; k < 8; k++)
        {
            int y0 = (14 + k * 11) * 2, on = 0, inv = 0;
            for (int y = y0; y < y0 + 22; y++)
            {
                on += tv.screen[y * TESS_TRACKS_W + 3] != tv.screen[0];      // (the meter: cells 1-2)
                inv += tv.screen[y * TESS_TRACKS_W + 13 * 2 - 1] != tv.screen[0];   // (the role's box edge, inverted)
            }
            lit += on > 0, flashed += inv > 4;
        }
    }
    tess_core_transport(0);
    tess_core_project_new();   // (STILL TIDE's tails out of the way of the tests after)
    run(400);
    CHECK(lit > 20, "TRACKS: the rows' meters move while playing (%d)", lit);
    CHECK(flashed > 0, "TRACKS: a note's flash on its role (%d)", flashed);
}

static void dice_mutate()   // G6: MUTATE nudges a track's sound, a LOCK keeps its value, PUT BACK undoes; locks saved
{
    tess_core_project_new();
    run(50);
    tess_core_locks_load(nullptr, 0);
    const uint32_t tr = 1, atk = 1 /* P_ATK */, rel = 4 /* P_REL */;
    tess_core_param_set(atk, tr, 40);
    tess_core_param_set(rel, tr, 60);
    tap(B_HOME);
    tess_zone_t z{};
    z.kind = TESS_ZONE_TRACK;
    z.row = (int16_t)tr;
    int32_t a0 = tess_core_param_get(atk, tr), r0 = tess_core_param_get(rel, tr);
    {   // (a lock by hand, as the plugin's state would bring it back: P_REL on T2)
        const uint8_t lk[3] = {(uint8_t)rel, 0, (uint8_t)tr};
        tess_core_locks_load(lk, 3);
        uint8_t out[8];
        CHECK(tess_core_locks(nullptr, 0) == 3 && tess_core_locks(out, sizeof out) == 3 && out[0] == rel && out[2] == tr,
              "G6: a lock loaded, and saved back the same");
    }
    int moved = 0;
    for (int i = 0; i < 6; i++)
    {
        tess_core_zone_do(&z, TESS_ZA_DICE, 20);
        run(5);
        moved += tess_core_param_get(atk, tr) != a0;
        CHECK(tess_core_param_get(rel, tr) == r0, "MUTATE: T2's LOCKed REL kept (%d)", tess_core_param_get(rel, tr));
    }
    CHECK(moved > 0, "MUTATE: T2's ATTACK nudged (%d of 6)", moved);
    tess_core_view(&v);
    CHECK(v.dice_back, "a MUTATE to put back");
    int32_t a1 = tess_core_param_get(atk, tr);
    tess_core_zone_do(&z, TESS_ZA_DICE, 20);
    run(5);
    tess_core_zone_do(nullptr, TESS_ZA_DICE, -1);
    run(5);
    tess_core_view(&v);
    CHECK(tess_core_param_get(atk, tr) == a1 && !v.dice_back, "PUT BACK: the last MUTATE undone (%d, %d)", a1,
          tess_core_param_get(atk, tr));
    tess_core_locks_load(nullptr, 0);
    CHECK(!tess_core_locks(nullptr, 0), "locks cleared");
}

static float loud(int blocks)   // the loudest sample over 10 ms blocks
{
    float pk = 0;
    for (int b = 0; b < blocks; b++)
    {
        uint32_t n = tess_core_rate() / 100;
        tess_core_render(L, R, nullptr, n);
        for (uint32_t i = 0; i < n; i++) pk = std::max(pk, std::max(std::fabs(L[i]), std::fabs(R[i])));
    }
    tess_core_view(&v);
    return pk;
}
static void audition_find()   // G5: a sound clicked in the browser while stopped is heard; FIND filters the list by name
{
    tess_core_project_new();
    run(50);
    tess_core_key_words(1);
    tess_core_track_select(0);
    tap(B_HOME);
    act(find("T1", TESS_ZONE_TRACK) >= 0 ? find("T1", TESS_ZONE_TRACK) : find("1", 0), TESS_ZA_DOUBLE, 0, 40);
    if (find("SOUND", TESS_ZONE_CELL) < 0)   // (4-track builds: HOME's sound row; else the browser's own way in)
        for (int i = 0; i < 4 && find("SOUND", TESS_ZONE_CELL) < 0; i++) tap(B_SAVE);
    int below = -1;
    for (int i = 0; i < (int)v.zone_count; i++)
        if (v.zones[i].kind == TESS_ZONE_CELL && v.zones[i].role == 4 && (int8_t)(v.zones[i].row & 0xFF) == 1) below = i;
    CHECK(below >= 0, "G5: T1's browser open");
    if (below < 0) { tess_core_key_words(0); tap(B_HOME); return; }
    CHECK(!tess_core_playing() && loud(30) < 0.001f, "stopped, nothing playing: silent");
    act(below, TESS_ZA_CLICK, 0, 1);
    float heard = loud(60);
    CHECK(heard > 0.01f, "a sound clicked while stopped: heard (peak %.3f)", heard);
    loud(300);
    float after = loud(30);
    CHECK(after < 0.001f, "the audition over: silent again (peak %.4f)", after);

    int f = find("FIND", TESS_ZONE_CELL);
    CHECK(f >= 0, "the browser's count: a FIND cell");
    if (f >= 0)
    {
        tess_zone_t z = v.zones[f];
        CHECK(!tess_core_zone_type(&z, "bass"), "FIND typed: taken");
        run(10);
        int n = 0, ok = 1;
        for (uint32_t i = 0; i < v.zone_count; i++)
            if (v.zones[i].kind == TESS_ZONE_CELL && v.zones[i].role == 4 && v.zones[i].value[0])
            {
                n++;
                std::string s = v.zones[i].value;
                for (auto &c : s) c = (char)std::toupper((unsigned char)c);
                if (s.find("BASS") == std::string::npos) ok = 0, std::printf("  not a BASS: %s\n", v.zones[i].value);
            }
        CHECK(n > 0 && ok, "FIND BASS: the sounds listed all have BASS in their names (%d)", n);
        f = find("FIND", TESS_ZONE_CELL);
        CHECK(f >= 0 && !std::strcmp(v.zones[f].value, "BASS"), "the cell carries the text");
        z = v.zones[f >= 0 ? f : 0];
        tess_core_zone_type(&z, "qqqzzz");
        run(10);
        CHECK(find("SOUND", TESS_ZONE_CELL) < 0 || !v.zones[find("SOUND", TESS_ZONE_CELL)].value[0],
              "FIND with no match: no sounds");
        tess_core_zone_type(&z, "");
        run(10);
        CHECK(find("SOUND", TESS_ZONE_CELL) >= 0, "FIND emptied: the sounds back");
        tess_core_zone_type(&z, "pad");
        run(10);
        tap(B_HOME);
        run(10);
        f = find("FIND", TESS_ZONE_CELL);
        CHECK(f < 0, "HOME: no FIND cell");
    }
    tess_core_key_words(0);
    tap(B_HOME);
}

static void points()   // a picture's markers dragged: ENV's corners turn ATK, DEC + SUS, REL + SUS
{
    tap(B_HOME);
    int dz = -1;
    for (int i = 0; i < 6 && dz < 0; i++) { tap(B_ENV); dz = find("DECAY / SUS", TESS_ZONE_POINT); }
    CHECK(dz >= 0, "ENV: its corners, markers to drag");
    if (dz < 0) { tap(B_HOME); return; }
    auto col = [&](int role) {   // (a page's column by its knob: KNOB 1 is role 3)
        for (uint32_t i = 0; i < v.zone_count; i++)
            if (v.zones[i].kind == TESS_ZONE_KNOB && v.zones[i].role == role) return std::string(v.zones[i].value);
        return std::string("?");
    };
    std::string sus = col(5), dec = col(4);
    tess_zone_t z = v.zones[dz];
    tess_core_zone_do(&z, TESS_ZA_TURN_Y, -4);   // (dragged down)
    run(40);
    tess_core_view(&v);
    CHECK(col(5) != sus && col(4) == dec, "the decay corner dragged down: SUS lower (%s, then %s), DEC as it was",
          sus.c_str(), col(5).c_str());
    dz = find("DECAY / SUS", TESS_ZONE_POINT);
    z = v.zones[dz];
    tess_core_zone_do(&z, TESS_ZA_TURN, 3);   // (dragged right)
    run(40);
    tess_core_view(&v);
    CHECK(col(4) != dec, "dragged right: DEC longer (%s, then %s)", dec.c_str(), col(4).c_str());
    tap(B_HOME);
}
static void tweak_marks()   // TWEAK's pictures: SHIFT's slider and FOLLOW's tracks (found by their columns' labels)
{
    tap(B_HOME);
    int sh = reach(B_ARP, "SHIFT", TESS_ZONE_POINT, 12);
    CHECK(sh >= 0, "TWEAK: SHIFT's slider, a marker");
    CHECK(find("T2", TESS_ZONE_CELL) >= 0, "TWEAK: FOLLOW's tracks, a cell each");
    tap(B_HOME);
}
static void beta10()   // beta10's mouse bugs: boxed values click, FM6's chart and EGs, FX FILTER's peak, PATTERNS' tabs,
{                      // an ARP-held help card clicked away
    tess_core_load(SIM_DEMO_A, sizeof SIM_DEMO_A);
    run(50);
    tap(B_HOME);
    // FX FILTER (STILL TIDE's T3, FX 1): its peak, a marker: across CUT, up and down RES
    act(find("T3", TESS_ZONE_TRACK), TESS_ZA_CLICK);
    tap(B_FX);
    int pk = find("CUT / RES", TESS_ZONE_POINT);
    CHECK(pk >= 0, "FX FILTER: the peak, a marker");
    if (pk >= 0)
    {
        std::string c0 = kval("CUT"), r0 = kval("RES");
        act(pk, TESS_ZA_TURN_Y, 4);
        CHECK(kval("RES") != r0 && kval("CUT") == c0, "the peak dragged up: RES moves, CUT stays (%s %s, then %s %s)",
              c0.c_str(), r0.c_str(), kval("CUT").c_str(), kval("RES").c_str());
        pk = find("CUT / RES", TESS_ZONE_POINT);
        act(pk, TESS_ZA_TURN, -4);
        CHECK(kval("CUT") != c0, "the peak dragged left: CUT moves (%s, then %s)", c0.c_str(), kval("CUT").c_str());
    }
    // a boxed value (FX SEND's MODE, CHAIN ..): a click the next one
    {
        int fx = reach(B_FX, "MODE", TESS_ZONE_KNOB, 8);
        std::string t0 = fx >= 0 ? v.zones[fx].value : "";
        act(fx, TESS_ZA_CLICK);
        fx = find("MODE", TESS_ZONE_KNOB);
        CHECK(fx >= 0 && t0 != v.zones[fx].value, "a click on a boxed value steps it (%s, then %s)", t0.c_str(),
              fx >= 0 ? v.zones[fx].value : "?");
        act(find("FX1", TESS_ZONE_TAB), TESS_ZA_CLICK);   // (FX opens on the page last used: FX 1 again for later)
    }
    tess_core_load(SIM_DEMO_A, sizeof SIM_DEMO_A);
    run(50);
    // FM6's chart (T3 plays FM6, TINE EP): an operator a zone; a double-click opens its pages, whose EG has corners
    tap(B_HOME);
    act(find("T3", TESS_ZONE_TRACK), TESS_ZA_CLICK);
    int op = reach(B_EDIT, "OP 3", TESS_ZONE_OP, 6);
    if (op < 0) dump("EDIT, T3");
    CHECK(op >= 0 && count(TESS_ZONE_OP) == 6, "FM6's chart: an operator a zone (%d)", count(TESS_ZONE_OP));
    if (op >= 0)
    {
        auto knobs = [&]() { std::string k; for (uint32_t i = 0; i < v.zone_count; i++) if (v.zones[i].kind == TESS_ZONE_KNOB) k += v.zones[i].name, k += "|"; return k; };
        std::string was = knobs();
        act(op, TESS_ZA_DOUBLE);
        CHECK(knobs() != was, "a double-click on OP 3 opens its pages (%s, then %s)", was.c_str(), knobs().c_str());
        for (int i = 0; i < 6 && find("R1", TESS_ZONE_POINT) < 0; i++)   // (its tabs, right: to EG RATE)
            act(find(">", TESS_ZONE_TAB) >= 0 ? find(">", TESS_ZONE_TAB) : (int)v.zone_count - 1, TESS_ZA_CLICK);
        int r1 = find("R1", TESS_ZONE_POINT);
        if (r1 < 0) dump("OP 3's pages");
        CHECK(r1 >= 0, "FM6's EG RATE: its corners, markers");
        if (r1 >= 0)
        {
            std::string a = kval("R1");
            act(r1, TESS_ZA_TURN, 3);   // (right: a longer segment, a slower rate)
            CHECK(midi_of(a) < 0 && std::atoi(kval("R1").c_str()) < std::atoi(a.c_str()),
                  "R1's corner dragged right: R1 down (%s, then %s)", a.c_str(), kval("R1").c_str());
        }
    }
    // PATTERNS: the page tabs at its foot
    tap(B_HOME);
    bool pats = false;
    for (int i = 0; i < 8 && !pats; i++) { tap(B_SEQ); pats = count(TESS_ZONE_CELL) >= 16; }
    CHECK(pats && count(TESS_ZONE_TAB) >= 2, "PATTERNS: its page tabs, clickable (%d)", count(TESS_ZONE_TAB));
    // a help card ARP held opened (a window's latched ARP): a click away closes it, ARP still held
    tap(B_HOME);
    tap(B_FX);
    uint32_t page = screen_hash();
    tess_core_buttons(1u << B_ARP);
    run(5);
    tess_core_enc(3, 1);
    run(20);
    CHECK(screen_hash() != page && v.closable, "ARP + KNOB 1: its help card, closable (%u)", v.closable);
    tess_core_away();
    run(20);
    CHECK(screen_hash() == page, "a click away closes it, ARP still held");
    tess_core_buttons(0);
    run(20);
    tap(B_HOME);
}
static void page_knobs()   // G14: the shown page's eight knobs for a DAW's controller (CLAP remote controls)
{
    uint32_t pid[8], tr[8];
    char name[32];
    tess_core_track_select(0);
    tap(B_HOME);
    tap(B_FX);
    uint32_t n = tess_core_page_knobs(pid, tr, name, sizeof name);
    int same = 0, k0 = 0;
    while (k0 < (int)v.zone_count && v.zones[k0].kind != TESS_ZONE_KNOB) k0++;
    for (int k = 0; k < 4 && k0 + k < (int)v.zone_count; k++)
    {
        uint32_t zp, zt;
        if (tess_core_zone_param(&v.zones[k0 + k], &zp, &zt)) same += zp == pid[k] && zt == tr[k];
        else same += pid[k] == 0xFFFFu;
    }
    CHECK(n >= 2 && same == 4 && name[0], "G14: FX's KNOB 1..4 as their zones turn (%u, %d same, \"%s\")", n, same, name);
    int lay = 0;
    for (int k = 4; k < 8; k++) lay += pid[k] != 0xFFFFu;
    CHECK(lay == 0, "G14: FX has no OCT- layer: knobs 5..8 none (%d)", lay);
    uint32_t pid2[8], tr2[8];
    char name2[32];
    tess_core_page_knobs(pid2, tr2, name2, sizeof name2);
    run(5);
    CHECK(!std::memcmp(pid, pid2, sizeof pid) && !std::strcmp(name, name2) && count(TESS_ZONE_KNOB) == 4,
          "G14: asked again, the same, and the page as it was");
    for (int i = 0; i < 16 && find_strip(TESS_ZONE_PAN, "T1", "PAN") < 0; i++) tap(B_GLO);
    uint32_t lvl = pid_named("LEVEL"), pan = pid_named("PAN");
    tess_core_page_knobs(pid, tr, name, sizeof name);
    int ok = 0;
    for (uint32_t k = 0; k < 4; k++)
        ok += pid[k] == lvl && tr[k] == k && pid[4 + k] == pan && tr[4 + k] == k;
    CHECK(ok == 4, "G14: MIXER: knobs 1..4 T1..T4's LEVEL, 5..8 (OCT- held) their PAN (%d, \"%s\")", ok, name);
    run(5);
    CHECK(find_strip(TESS_ZONE_PAN, "T1", "PAN") >= 0,
          "G14: MIXER as it was after asking (OCT- only pretended)");
    tap(B_HOME);
}
int main(int argc, char **argv)
{
    tess_core_boot();
    tess_core_load(SIM_DEMO_A, sizeof SIM_DEMO_A);   // (STILL TIDE: the values below are its; a new project's FX MIX
                                                     // sits at 0 %)
    run(50);

    // ---- HOME: the tracks
    CHECK(count(TESS_ZONE_TRACK) == 4, "HOME: a zone a track (%d)", count(TESS_ZONE_TRACK));
    sheet_track();
    tabs();
    glo_clicks();
    help_close();
    ux_round();
    slot_tabs();
    mouse2();
    points();
    tweak_marks();
    beta10();
    typed_values();
    page_knobs();
    mixer8();
    keys8();
    tess_core_load(SIM_DEMO_A, sizeof SIM_DEMO_A);
    run(50);
    tap(B_HOME);
    act(find("T3"), TESS_ZA_CLICK);
    CHECK(v.track == 2, "a click on T3 selects it (%u)", v.track);

    // ---- a page: FX
    tap(B_FX);
    dump("FX");
    CHECK(count(TESS_ZONE_KNOB) == 4, "FX: a zone a knob (%d)", count(TESS_ZONE_KNOB));
    int k0 = 0;   // (the columns' zones: after a picture's marker, FX FILTER's peak, which a hit finds first)
    while (k0 < (int)v.zone_count && v.zones[k0].kind != TESS_ZONE_KNOB) k0++;
    bool order = (int)v.zone_count >= k0 + 4;
    for (int i = 0; i < 4 && order; i++)
        order = v.zones[k0 + i].role == 3 + i && v.zones[k0 + i].x <= 60 * i && v.zones[k0 + i].x + v.zones[k0 + i].w >= 60 * i + 60 &&
                v.zones[k0 + i].w <= 72 && v.zones[k0 + i].name[0];   // (a column, wider by a value's box)
    CHECK(order, "FX: KNOB 1..4 left to right, a column each, named");
    tess_zone_t mix = v.zones[k0 + 1];
    std::string was = mix.value;
    act(k0 + 1, TESS_ZA_TURN, -2);
    std::string turned = v.zones[k0 + 1].value;
    CHECK(turned != was, "a turn of %s's zone moves it (%s, then %s)", mix.name, was.c_str(), turned.c_str());
    act(k0 + 1, TESS_ZA_DOUBLE);
    std::string reset = v.zones[k0 + 1].value;
    act(k0 + 1, TESS_ZA_DOUBLE);
    CHECK(reset != turned && std::string(v.zones[k0 + 1].value) == reset,
          "a double-click puts %s's default back (%s, then %s, again %s)", mix.name, turned.c_str(), reset.c_str(),
          v.zones[k0 + 1].value);
    tess_zone_t gone = v.zones[k0 + 1];
    tap(B_HOME);
    tess_core_zone_do(&gone, TESS_ZA_TURN, 5);
    run(30);
    tap(B_FX);
    CHECK(std::string(v.zones[k0 + 1].value) == reset, "a zone no longer on the screen does nothing (%s, %s)", reset.c_str(),
          v.zones[k0 + 1].value);

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
    slots();
    grids();
    mixer();
    reading();
    tess_core_slot_put(0, SIM_DEMO_A, sizeof SIM_DEMO_A);   // (a song saved in slot A, to rename)
    typing();
    tracks(argc > 1 ? argv[1] : nullptr);
    mosaic(argc > 2 ? argv[2] : nullptr);
    roll();
    paint();
    drop_sound();
    constel_drag();
    live_motion();
    macro_mods();
    scale_keys();
    dice_mutate();
    audition_find();
    smap();
    chord_user();
    sections();
    egrid();
    hosted_words();

    std::printf(fails ? "zones test: %d FAILED\n" : "zones test: ok\n", fails);
    return fails ? 1 : 0;
}
