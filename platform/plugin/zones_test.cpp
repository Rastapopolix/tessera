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
enum { B_FX = 0, B_SEL = 1, B_LFO = 3, B_EDIT = 4, B_GLO = 5, B_HOME = 6, B_SAVE = 7, B_ARP = 8, B_SEQ = 9, B_OCTDN = 12, B_OCTUP = 13 };

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
static void mixer()   // MIXER (GLO's first page): a strip's LEVEL, PAN, MUTE and SOLO
{
    tap(B_HOME);
    for (int i = 0; i < 8 && find_strip(TESS_ZONE_PAN, "T1", "PAN") < 0; i++) tap(B_GLO);
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
static uint32_t screen_hash()   // the screen as it is now (FNV-1a of its pixels)
{
    tess_screen_t sc;
    tess_core_screen(&sc);
    uint32_t h = 2166136261u;
    for (uint32_t i = 0; i < sc.w * sc.h; i++) h = (h ^ sc.pixels[i]) * 16777619u;
    return h;
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
static void typing()   // NAME (SAVE > PROJECT, EDIT renames slot A): a computer keyboard types into it
{
    tap(B_HOME);
    tap(B_SAVE);
    for (int i = 0; i < 6 && find("A", TESS_ZONE_ROW) < 0; i++) tap(B_SAVE);
    act(find("A", TESS_ZONE_ROW), TESS_ZA_CLICK, 0, 40);
    CHECK(!v.typing, "PROJECT: not typing");
    tap(B_EDIT);
    tess_core_view(&v);
    CHECK(v.typing, "EDIT on PROJECT's slot A: NAME open, typing");
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
    CHECK(arrow >= 0, "GLO's footer: a zone on the right arrow");
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
        CHECK(left >= 0 && v.zones[left].row < 0, "then a left arrow, a page back");
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
                int next = -1;
                for (uint32_t z = 0; z < v.zone_count; z++)
                    if (v.zones[z].kind == TESS_ZONE_TAB && v.zones[z].row == 1) next = (int)z;
                std::string want = next >= 0 ? v.zones[next].value : "";
                act(next, TESS_ZA_CLICK, 0, 40);
                int here = -1;
                for (uint32_t z = 0; z < v.zone_count; z++)
                    if (v.zones[z].kind == TESS_ZONE_TAB && v.zones[z].row == 0) here = (int)z;
                CHECK(next >= 0 && here >= 0 && want == v.zones[here].value, "from STORE, the next tab clicked: %s",
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
    help_close();
    ux_round();
    slot_tabs();
    mouse2();
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
    slots();
    grids();
    mixer();
    reading();
    tess_core_slot_put(0, SIM_DEMO_A, sizeof SIM_DEMO_A);   // (a song saved in slot A, to rename)
    typing();
    tracks(argc > 1 ? argv[1] : nullptr);
    mosaic(argc > 2 ? argv[2] : nullptr);

    std::printf(fails ? "zones test: %d FAILED\n" : "zones test: ok\n", fails);
    return fails ? 1 : 0;
}
