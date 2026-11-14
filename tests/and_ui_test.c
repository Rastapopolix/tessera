/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Andamento's two screens (platform/andamento/and_ui.c): the page screen (Tessera's header, a picture, the tabs) and
 * the dial screen (the 8 encoders' settings as dials), through the virtual prototype's own build (web/andamento/sim.c,
 * at 48 kHz) driven as the page drives it.
 *   and_ui_test [dir]  (run by tests/run_tests.sh; exit 1 on a failed check; with a dir, both screens of every page as
 *                      dir/ppm/ui_*_page.ppm and dir/ppm/ui_*_dial.ppm, for tests/oled_render.py)
 * Checks, on every page and sub-page of track 1 (internal) and track 5 (external), and on HOME (and SOUND, MOD and FX
 * again with each engine on track 1):
 *   layout    the dial screen's lint (the FM-1's Mono lint, tests/ui_render.c, at one pixel a cell): no value cut;
 *             each value inside its own 32 px cell; a clear pixel between a box and its text; neighbours apart
 *   frame     the page screen: the header's line, the 8 tracks' marks, the tabs (the shown one inverted)
 *   hot       an encoder turned: its value inverted on the dial screen, the setting large in the picture
 *   panels    the two frames are separate (drawing one leaves the other)
 *   map       SOUND, MOD and FX from Tessera's PAGES[] (platform/andamento/and_pages.c): every one of an engine's own
 *             pages with a value to set is on one of them (none lost), each cell once a sub-page; LEVEL on AMP,
 *             ENGINE and PRESET on VOICE and they load; an ORACLE / FM6 page's cell writes the patch; FX MODE's SPLIT
 *             shows the three sends */
#include "../web/andamento/sim.c"

#include <stdio.h>

static uint32_t nfail;
#define CHECK(c, ...) do { if (!(c)) { nfail++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

static const char *dir;
static void run_ms(uint32_t ms) { while (ms--) web_ms(); }
static void btn(uint32_t b) { web_button(b, 1); web_button(b, 0); }

static void dump1(const char *name, const uint8_t *fb)
{
    char p[256];
    FILE *f;
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
static void dump(const char *name)
{
    char b[96];
    snprintf(b, sizeof b, "ui_%s_page", name);
    dump1(b, &and_fbs[AND_SCR_PAGE][0][0]);
    snprintf(b, sizeof b, "ui_%s_dial", name);
    dump1(b, &and_fbs[AND_SCR_DIAL][0][0]);
}
static uint32_t peak(uint32_t s, int32_t x, int32_t y, int32_t w, int32_t h)
{
    uint32_t m = 0;
    int32_t i, j;
    for (j = y; j < y + h; j++)
        for (i = x; i < x + w; i++)
            if (and_fbs[s][j][i] > m)
                m = and_fbs[s][j][i];
    return m;
}

/* the dial screen's lint, after a draw */
static uint32_t find_sub(uint32_t p, const char *tab)   /* page p's sub-page by its tab (and_nsubs: none) */
{
    uint32_t k, n = and_nsub(p);
    for (k = 0; k < n; k++)
        if (!strcmp(and_subs[k].tab, tab))
            return k;
    return and_nsubs;
}
static int on_page(uint32_t p, uint32_t pid)     /* pid on one of page p's sub-pages */
{
    uint32_t k, i, n = and_nsub(p);
    for (k = 0; k < n; k++)
        for (i = 0; i < AND_NENC; i++)
            if (and_subs[k].c[i] == pid)
                return 1;
    return 0;
}
static void lint(const char *where)
{
    uint32_t i, used = 0;
    for (i = 0; i < AND_NENC; i++) {
        int32_t c0 = (int32_t)(i % 4u) * 32;
        if (!and_cellv[i].used)
            continue;
        used++;
        CHECK(!and_cellv[i].cut, "%s: cell %c, \"%s\" cut", where, 'A' + i, and_cellv[i].s);
        CHECK(and_cellv[i].x0 >= c0 && and_cellv[i].x1 <= c0 + 31, "%s: cell %c, \"%s\" outside its cell (%d..%d)",
              where, 'A' + i, and_cellv[i].s, and_cellv[i].x0, and_cellv[i].x1);
        if (and_cellv[i].kind == AV_BOX && strlen(and_cellv[i].s) < 5u) {   /* (a clear pixel each side of the text,
                                                  * inside the box; five letters: the box's top and foot only) */
            int32_t tw = (int32_t)strlen(and_cellv[i].s) * 6 - 1, tx = c0 + 16 - tw / 2;
            CHECK(tx - and_cellv[i].x0 >= 2 && and_cellv[i].x1 - (tx + tw - 1) >= 2,
                  "%s: cell %c, \"%s\" touches its box", where, 'A' + i, and_cellv[i].s);
        }
        if (i % 4u && and_cellv[i - 1u].used)    /* (neighbours: a clear pixel between) */
            CHECK(and_cellv[i].x0 - and_cellv[i - 1u].x1 >= 2, "%s: cells %c and %c touch", where, 'A' + i - 1, 'A' + i);
    }
    (void)used;
}
/* the page screen's frame */
static void pframe(const char *where, int tabs)
{
    uint32_t tr;
    CHECK(peak(AND_SCR_PAGE, 0, 9, AND_W, 1) == 8, "%s: the header's line", where);
    CHECK(peak(AND_SCR_PAGE, 1, 1, 60, 7) == 15, "%s: a title", where);
    for (tr = 0; tr < AND_TRACKS; tr++)
        CHECK(peak(AND_SCR_PAGE, 88 + (int32_t)tr * 5, 2, 4, 5) > 0, "%s: track %u's mark", where, tr + 1u);
    CHECK(peak(AND_SCR_PAGE, 88 + (int32_t)AND_SEL * 5, 2, 4, 5) == 15, "%s: the selected track's mark filled", where);
    if (tabs)
        CHECK(peak(AND_SCR_PAGE, 0, AND_TAB_Y, AND_W - 25, 1) == 15, "%s: the shown tab inverted", where);
}

int main(int argc, char **argv)
{
    uint32_t p, s, i, tr;
    char where[48];
    dir = argc > 1 ? argv[1] : 0;
    web_boot();
    run_ms(200);

    /* HOME */
    web_screen();
    pframe("HOME", 0);
    lint("HOME");
    for (i = 0; i < AND_NENC; i++)
        CHECK(and_cellv[i].used && and_cellv[i].kind == AV_DIAL, "HOME: encoder %c a dial (track %u's DEPTH)", 'A' + i, i + 1u);
    dump("home");

    /* every page and sub-page, track 1 then track 5 */
    for (tr = 0; tr < 2u; tr++) {
        and_select(tr ? NTRK : 0u);
        for (p = 0; p < 6u; p++) {
            uint32_t n = tr ? 1u : and_nsub(p);
            for (s = 0; s < n; s++) {
                and_ui.page = (uint8_t)p;
                and_ui.sub[p] = (uint8_t)s;
                and_ui.hot_ms = and_ui.say_ms = 0;
                web_screen();
                snprintf(where, sizeof where, "T%u %s %u", tr ? 5u : 1u, (tr ? AND_EXT_PAGE : AND_PAGE)[p], s + 1u);
                pframe(where, 1);
                lint(where);
                snprintf(where, sizeof where, "t%u_%s%u", tr ? 5u : 1u, (tr ? AND_EXT_PAGE : AND_PAGE)[p], s + 1u);
                for (i = 0; where[i]; i++)
                    if (where[i] >= 'A' && where[i] <= 'Z') where[i] = (char)(where[i] + 32);
                dump(where);
            }
        }
    }
    and_select(0);
    and_ui.page = AB_MIX;                        /* (pid 0x000, LEVEL, is a setting, not an empty cell) */
    web_screen();
    CHECK(and_ui.cell[0] == 0x000 && and_cellv[0].used, "MIX: cell A is LEVEL");

    /* the map, with each engine on track 1: every page drawn and linted, every value of the engine's own pages on
     * SOUND or MOD, a cell once per sub-page */
    an_engines();
    for (i = 0; i < an_neng; i++) {
        uint32_t e = an_eng_id[i], pi, c, k, j;
        and_select(0);
        set_engine_of(&trk[0], e);
        for (p = AB_SOUND; p <= AB_FX; p++) {
            uint32_t n = and_nsub(p);
            CHECK(n >= 2u, "%s %s: sub-pages (%u)", N_ENGNAME[e], AND_PAGE[p], n);
            for (s = 0; s < n; s++) {
                and_ui.page = (uint8_t)p;
                and_ui.sub[p] = (uint8_t)s;
                and_ui.hot_ms = and_ui.say_ms = 0;
                web_screen();
                and_nsub(p);
                snprintf(where, sizeof where, "%s %s %s", N_ENGNAME[e], AND_PAGE[p], and_subs[s].tab);
                pframe(where, 1);
                lint(where);
                for (k = 0; k < AND_NENC; k++)
                    for (j = k + 1u; j < AND_NENC; j++)
                        CHECK(and_subs[s].c[k] == PID_NONE || and_subs[s].c[k] != and_subs[s].c[j],
                              "%s: cells %c and %c the same", where, 'A' + k, 'A' + j);
                snprintf(where, sizeof where, "eng_%s_%s_%s", N_ENGNAME[e], AND_PAGE[p], and_subs[s].tab);
                for (k = 0; where[k]; k++)
                    if (where[k] >= 'A' && where[k] <= 'Z') where[k] = (char)(where[k] + 32);
                dump(where);
            }
        }
        for (pi = 0; pi < NPAGES; pi++) {        /* (none of the engine's own values lost) */
            if (!ap_page_ok(pi) || PAGES[pi].fam != FAM_EDIT)
                continue;
            for (c = 0; c < 4u; c++) {
                uint16_t cell = ap_cell(pi, c);
                CHECK(cell == PID_NONE || on_page(AB_SOUND, cell) || on_page(AB_MOD, cell) ||
                      cell == PID_TRACK(P_MUTE),   /* (MUTE: MIX's) */
                      "%s: %s's column %u (%04x) on no page", N_ENGNAME[e], PAGES[pi].title, c + 1u, cell);
            }
        }
        k = find_sub(AB_SOUND, "AMP");
        CHECK(k < and_nsubs && and_subs[k].c[7] == PID_TRACK(P_LEVEL), "%s: AMP's cell H is LEVEL", N_ENGNAME[e]);
        k = find_sub(AB_SOUND, "VOIC");
        CHECK(k < and_nsubs && and_subs[k].c[0] == AN_PID + AN_ENGINE, "%s: VOICE's cell A is ENGINE", N_ENGNAME[e]);
    }

    /* ENGINE and PRESET load; the shown sub-page stays in range when the engine has fewer */
    and_select(0);
    set_engine_of(&trk[0], ENGI_FM6);
    and_ui.page = AB_SOUND, and_ui.sub[AB_SOUND] = (uint8_t)(and_nsub(AB_SOUND) - 1u);
    and_set(AN_PID + AN_ENGINE, 0, (int32_t)an_eng_pos(6u));
    CHECK(trk[0].eng_req == 6u, "VOICE: ENGINE turned to TRIO (6) loads it");
    web_screen();
    CHECK(and_ui.sub[AB_SOUND] < and_nsub(AB_SOUND), "fewer sub-pages: the shown one stays in range");
    and_set(AN_PID + AN_PRESET, 0, 3);
    CHECK(trk[0].preset == 3, "VOICE: PRESET 4 loads it");
    and_ui.page = AB_SOUND, and_ui.sub[AB_SOUND] = (uint8_t)find_sub(AB_SOUND, "VOIC");
    and_ui.hot = 1, and_ui.hot_pid = AN_PID + AN_PRESET, and_ui.hot_ms = fm1_ms | 1u;
    web_screen();
    CHECK(and_cellv[1].used && and_cellv[1].kind == AV_BOX, "VOICE: PRESET's name in a box");
    lint("PRESET");
    dump("preset");
    and_ui.hot_ms = 0;

    /* an engine's own page: its cell writes the patch */
    set_engine_of(&trk[0], ENGI_ORACLE);
    {
        uint32_t pi = ap_find("OSC A"), cell = AP_PID + pi * 4u + 1u;
        int32_t v0 = and_get(cell, 0);
        const param_desc_t *d = and_desc(cell, 0);
        CHECK(d && on_page(AB_SOUND, cell), "ORACLE: OSC A's WAVE on SOUND");
        if (d)
            and_set(cell, 0, d->max == v0 ? d->min : v0 + 1);
        CHECK(and_get(cell, 0) != v0, "ORACLE: OSC A's WAVE turned (in the patch: %d)", v0);
    }
    set_engine_of(&trk[0], ENGI_FM6);
    {
        uint32_t pi = ap_find("ALGO"), cell = AP_PID + pi * 4u;
        int32_t v0 = and_get(cell, 0);
        and_set(cell, 0, v0 + 1);
        CHECK(and_get(cell, 0) == v0 + 1 && fm6_patch[0][FP_ALG] == v0 + 1, "FM6: ALGO turned, in the patch");
        CHECK(on_page(AB_SOUND, AN_PID + AN_FMOP), "FM6: its operator on SOUND");
    }
    set_engine_of(&trk[0], TRK_DEF[0][0]);

    /* FX: one send, or three in SPLIT */
    song.g[G_FXMODE] = 0;
    CHECK(on_page(AB_FX, P_FXSEND) && !on_page(AB_FX, P_FXSEND2), "FX: one send");
    song.g[G_FXMODE] = 1;
    CHECK(on_page(AB_FX, P_FXSEND2) && on_page(AB_FX, P_FXSEND3), "FX MODE SPLIT: three sends");
    song.g[G_FXMODE] = 0;

    /* a setting turned: inverted on the dial screen, large in the picture */
    btn(AB_SOUND);
    and_ui.sub[AB_SOUND] = (uint8_t)find_sub(AB_SOUND, "AMP");   /* (AMP: ATK first) */
    web_enc(0, 3);
    web_screen();
    CHECK(and_ui.hot == 0, "encoder A turned: cell A hot");
    CHECK(and_fbs[AND_SCR_DIAL][23][and_cellv[0].x0 - 1] == 15 && and_fbs[AND_SCR_DIAL][31][and_cellv[0].x1 + 1] == 15,
          "cell A's value inverted (lit round its text)");
    CHECK(peak(AND_SCR_PAGE, 40, AND_PIC_Y + 14, 60, 14) == 15, "the picture: the value large");
    CHECK(peak(AND_SCR_PAGE, 3, AND_PIC_Y + 6, 31, 31) == 15, "the picture: its dial large");
    lint("hot");
    dump("hot");
    run_ms(1300);
    web_screen();
    CHECK(and_fbs[AND_SCR_DIAL][23][and_cellv[0].x0 - 1] == 0, "1.3 s later: cell A plain again");

    /* the two frames are separate */
    and_target(AND_SCR_DIAL);
    and_clear();
    and_target(AND_SCR_PAGE);
    CHECK(peak(AND_SCR_PAGE, 0, 9, AND_W, 1) == 8, "clearing the dial screen leaves the page screen");

    if (nfail) {
        printf("and_ui: %u failed\n", nfail);
        return 1;
    }
    printf("and_ui: all checks passed\n");
    return 0;
}
