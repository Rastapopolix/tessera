/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* The pages at 8 tracks (CORE_TRACKS 8, the Mono look: the plugin, Andamento): a page that shows four tracks shows the
 * selected track's bank (core.h UI_BASE: 1..4 or 5..8), and its knobs work on that bank.
 *   HOME, MIXER, MIDI IN, MIDI OUT, PATTERNS, SONG: T5 selected, the page differs from T1's under the header,
 *   and is the same whichever track of the bank is picked but for the pick; HOME and MIXER outline the selected
 *   column at the left (not off the screen); nothing is drawn below the lists' last row before the foot.
 *   MIDI OUT KNOB 2 (T6 selected) sets T6's channel, OCT- held its TRANSPOSE; PATTERNS KNOB 2 queues T6's pattern;
 *   ENTRIES OCT- held KNOB 1 sets T5's ROLE; MIXER KNOB 3 sets T7's level. T1..T4 untouched.
 *   MATRIX: the rows follow FROM's bank and the columns TO's. (ENTRIES at 8 tracks is the grid of all eight, T1-T4 left
 *   and T5-T8 right, whichever bank: ui_mono.c mo_pic_egrid, tests/egrid_test.c; only its OCT- knobs follow the bank.)
 * Built: clang -Ibuild/gen -Ifirmware/src -Itests -DMELODEE_USB_AUDIO=1 -DMELODEE_LOOK=1 -DCORE_TRACKS=8
 *        tests/bank_pages_test.c -lm (core-api.yml). At 4 tracks it checks nothing and passes. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

#if CORE_TRACKS > 4 && MELODEE_LOOK
static uint16_t shot[240 * 240];

static void show(const char *title)                     /* the page (HOME: ""), drawn whole */
{
    uint32_t k;
    if (title[0]) go_title(title);
    else ui.home = 1;
    ui.force = 1;
    for (k = 0; k < 6u; k++) frame();
}
static void pick(uint32_t tr, const char *title)
{
    track_select(tr);
    show(title);
}
static int body_differs(void)                           /* under the header (row 20), above the foot (row 200) */
{
    return memcmp(shot + 240 * 20, host_screen + 240 * 20, 240 * 180 * sizeof shot[0]) != 0;
}
static uint16_t bg(void)                                /* the background: the screen's commonest pixel */
{
    static uint32_t count[65536];
    uint32_t x;
    uint16_t b = 0;
    memset(count, 0, sizeof count);
    for (x = 0; x < 240u * 240u; x++)
        if (++count[host_screen[x]] > count[b]) b = host_screen[x];
    return b;
}
static int drawn(uint32_t x, uint32_t y) { return host_screen[y * 240u + x] != bg(); }
static uint32_t lit_rows(uint32_t y0, uint32_t y1)      /* rows y0..y1 with anything drawn */
{
    uint32_t y, x, n = 0, b = bg();
    for (y = y0; y <= y1; y++)
        for (x = 0; x < 240u; x++)
            if (host_screen[y * 240u + x] != b) { n++; break; }
    return n;
}

static int test_pages(void)
{
    static const char *const PG[] = {"", "MIXER", "MIDI IN", "MIDI OUT", "PATTERNS", "SONG"};
    uint32_t i;
    int bad = 0;
    char w[96];
    ui_power_on();
    for (i = 4; i < 8u; i++) trk[i].p[P_MUTE] = 1;     /* (5..8 look different from 1..4) */
    chain_config.count = 1;                             /* (SONG: a section, its blocks a pattern a track) */
    chain_config.row[0].slot = 0;
    chain_config.row[0].repeat = 1;
    memset(chain_patterns[0], 0, NTRK);
    for (i = 0; i < sizeof PG / sizeof PG[0]; i++) {
        const char *nm = PG[i][0] ? PG[i] : "HOME";
        pick(0, PG[i]);
        memcpy(shot, host_screen, sizeof shot);
        pick(4, PG[i]);
        snprintf(w, sizeof w, "%s: T5 shows the bank 5-8, not 1-4", nm);
        bad += check(w, body_differs());
        memcpy(shot, host_screen, sizeof shot);
        pick(5, PG[i]);
        pick(4, PG[i]);
        snprintf(w, sizeof w, "%s: the same bank drawn again the same", nm);
        bad += check(w, !body_differs());
    }
    pick(4, "");                                        /* the pick outlined in the first column (x 0..59) */
    bad += check("HOME: T5 outlined at the left", drawn(0, 60) && drawn(1, 60));
    pick(4, "MIXER");
    bad += check("MIXER: T5's strip outlined at the left", drawn((uint32_t)CARD_X(0) - 2u, PV_STRIP_TOP + 40u) ||
                                                         drawn((uint32_t)CARD_X(0) - 1u, PV_STRIP_TOP + 40u));
    pick(4, "MIDI IN");                                 /* four rows (54 .. 98 cells), nothing over the foot's line */
    bad += check("MIDI IN: four rows, none past the fourth", lit_rows(2u * 98u, 2u * 99u - 1u) == 0u);
    for (i = 4; i < 8u; i++) trk[i].p[P_MUTE] = 0;
    chain_config.count = 0;
    return bad;
}

static int test_knobs(void)
{
    int bad = 0, ch[8], tr[8], lv[8];
    uint32_t i;
    ui_power_on();
    stop_transport();
    for (i = 0; i < 8u; i++) {
        ch[i] = tessera.midi_ch[i];
        tr[i] = tessera.midi_tr[i];
        lv[i] = trk[i].p[P_LEVEL];
    }
    pick(5, "MIDI OUT");
    turn(EN_K2, 3);
    bad += check("MIDI OUT, T6: KNOB 2 sets T6's channel", tessera.midi_ch[5] != ch[5] && tessera.midi_ch[1] == ch[1]);
    fm1_in.buttons |= 1u << panel.btn[B_OCTDN];
    frame();
    turn(EN_K2, 2);
    fm1_in.buttons &= ~(1u << panel.btn[B_OCTDN]);
    frame();
    bad += check("MIDI OUT, T6, OCT- held: KNOB 2 sets T6's TRANSPOSE", tessera.midi_tr[5] != tr[5] && tessera.midi_tr[1] == tr[1]);
    pick(5, "PATTERNS");
    turn(EN_K2, 1);
    bad += check("PATTERNS, T6: KNOB 2 switches T6's pattern", trk[5].pattern == 1u && trk[1].pattern == 0u);
    pick(4, "ENTRIES");
    fm1_in.buttons |= 1u << panel.btn[B_OCTDN];
    frame();
    turn(EN_K1, 1);
    fm1_in.buttons &= ~(1u << panel.btn[B_OCTDN]);
    frame();
    bad += check("ENTRIES, T5, OCT- held: KNOB 1 sets T5's ROLE", tessera.et_role[4] != 0u && tessera.et_role[0] == 0u);
    pick(4, "MIXER");
    turn(EN_K3, -3);
    bad += check("MIXER, T5: KNOB 3 sets T7's level", trk[6].p[P_LEVEL] != lv[6] && trk[2].p[P_LEVEL] == lv[2]);
    return bad;
}

static int test_matrix_banks(void)
{
    int bad = 0;
    ui_power_on();
    pick(0, "MATRIX");
    memcpy(shot, host_screen, sizeof shot);
    mx_from = 5, mx_to = 0, mx_k = 0;                  /* FROM T6: the rows 5..8, the columns still 1..4 */
    show("MATRIX");
    bad += check("MATRIX: FROM T6 shows the rows 5..8", body_differs());
    memcpy(shot, host_screen, sizeof shot);
    mx_to = 6;
    show("MATRIX");
    bad += check("MATRIX: TO T7 shows the columns 5..8", body_differs());
    mx_from = mx_to = 0;
    return bad;
}

int main(void)
{
    int bad;
    setvbuf(stdout, NULL, _IONBF, 0);
    bad = test_pages() + test_knobs() + test_matrix_banks();
    track_select(0);
    printf(bad ? "bank pages: %d FAILED\n" : "bank pages: all passed\n", bad);
    return bad ? 1 : 0;
}
#else
int main(void)
{
    printf("bank pages: 4 tracks or the detailed look, nothing to check\n");
    return 0;
}
#endif
