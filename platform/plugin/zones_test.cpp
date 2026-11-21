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
 *     end on row 1: a row is reached detent by detent, never accelerated.
 * usage: tessera-zones-test */
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

int main()
{
    tess_core_boot();
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

    std::printf(fails ? "zones test: %d FAILED\n" : "zones test: ok\n", fails);
    return fails ? 1 : 0;
}
