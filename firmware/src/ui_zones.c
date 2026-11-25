/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The screen's zones (TESS_HAS_POINTER: the plugin): where the Mono pages put what a mouse can use, as they draw it.
 * A zone is a rectangle on the 240 x 240 screen and what it is: a knob's value (KNOB 1..4's column, a list's side), a
 * list's row (the knob that moves the list to it, the knob that turns its value), a key (OK, BACK, YES, NO) or a track
 * (HOME's columns). The core's API hands them to a front end (tess_view_t.zones) and does what a front end asks of one
 * (tess_core_zone_do: turn, click, double-click), through the same knobs and keys as the panel, so a page behaves the
 * same under the mouse as under the hand.
 *
 * The zones are kept in groups (the page's columns, a list's rows, the keys, HOME's tracks): a group is drawn again
 * whole, so drawing it replaces it (uz_begin). A forced frame (ui.force: another screen, a popup, a dialog) clears them
 * all first (uz_frame), and what is drawn under a popup or a dialog (cv_under) leaves none. Without TESS_HAS_POINTER
 * every UZ(...) is nothing: the FM-1's image is unchanged. Included by core_ui_sources.h and melodee.c before
 * ui_draw.c */
#if TESS_HAS_POINTER
enum { UZ_KNOB = 1, UZ_ROW, UZ_KEY, UZ_TRACK, UZ_MUTE, UZ_SOLO, UZ_LEVEL, UZ_CELL, UZ_PAN, UZ_SCROLL, UZ_TAB, UZ_TEMPO, UZ_PICK, UZ_POINT };   /* (tess_core.h's TESS_ZONE_*) */
enum { UZ_ROLL = 15 };                                 /* NOTES' steps (tess_core.h's TESS_ZONE_ROLL; ui_roll.c) */
enum { UZ_SMAP = 17 };                                 /* EDIT's sections map's tiles (TESS_ZONE_SMAP; ui_sections.c) */
enum { UZ_G_COLS, UZ_G_LIST, UZ_G_KEYS, UZ_G_TRACKS, UZ_G_SIDES, UZ_G_STRIPS, UZ_G_TABS, UZ_G_HEAD, UZ_G_PIC, UZ_GROUPS };
#define UZ_MAX 48
typedef struct {
    int16_t x, y, w, h;                                 /* in screen pixels (2 a Mono cell) */
    uint8_t kind, group;
    int8_t role, vrole;                                 /* the knob it turns (3..6 KNOB 1..4, 0 SELECT; -1 none); a row's
                                                         * value knob (-1 none) */
    int16_t row;                                        /* a row: its place in the whole list; a key: its button label
                                                         * (B_OCTUP ..); a track: 0..7; a cell: its turns (uz_cell) */
    char name[16], value[16];                           /* what it shows, for a front end's hover */
} uz_t;
static uz_t uz[UZ_MAX];
static uint32_t uz_n, uz_gen;
/* 1: the front end has its own BACK, OK and SHIFT buttons and its own octave buttons (tess_core_key_words: Smalti's
 * shell): the screen names BACK / OK / SHIFT for OCT- / OCT+ (ui_mono.c mo_kw), and OCT- / OCT+ no longer step the
 * octave on Stage or NOTES (ui_input.c; the octave buttons do) */
static uint8_t uz_key_words;
/* the list shown: the knob that moves its cursor (one detent a row), its value knob, where the cursor is, and whether
 * OCT+ does a row (a double-click: there, then OCT+). sel follows the moves asked for until the list is drawn again */
static struct { int8_t role, vrole, ok; int16_t sel, first; } uz_list = {-1, -1, 0, 0, 0};
static int16_t uz_list_sel_pending = -1;                /* (tess_core.c: a move still on its way; the drawn cursor waits) */

static void uz_begin(uint32_t g)                        /* group g drawn again: its zones go */
{
    uint32_t i, k = 0;
    for (i = 0; i < uz_n; i++)
        if (uz[i].group != g)
            uz[k++] = uz[i];
    uz_n = k;
    uz_gen++;
}
static void uz_begin_row(uint32_t g, int32_t row)      /* one of group g's parts (MIXER's strip: its track) drawn again */
{
    uint32_t i, k = 0;
    for (i = 0; i < uz_n; i++)
        if (uz[i].group != g || uz[i].row != row)
            uz[k++] = uz[i];
    uz_n = k;
    uz_gen++;
}
static void uz_frame(void)                              /* (ui_draw_frame: a forced frame) */
{
    if (ui.force && uz_n) {
        uz_n = 0;
        uz_gen++;
    }
}
/* a zone of x, y, w, h Mono cells (2 px each) */
static void uz_add(uint32_t g, uint32_t kind, int32_t x, int32_t y, int32_t w, int32_t h, int32_t role, int32_t vrole,
                   int32_t row, const char *name, const char *value)
{
    uz_t *z;
    if (cv_under || uz_n >= UZ_MAX || w <= 0 || h <= 0)
        return;
    z = &uz[uz_n++];
    z->x = (int16_t)(x * 2), z->y = (int16_t)(y * 2), z->w = (int16_t)(w * 2), z->h = (int16_t)(h * 2);
    z->kind = (uint8_t)kind, z->group = (uint8_t)g;
    z->role = (int8_t)role, z->vrole = (int8_t)vrole;
    z->row = (int16_t)row;
    str_cpy(z->name, name ? name : "", sizeof z->name);
    str_cpy(z->value, value ? value : "", sizeof z->value);
    uz_gen++;
}
/* a place in a grid (PATTERNS, SONG): a click turns knob ra da detents, then knob rb db (rb -1: none), from where they
 * are as the page draws it; row holds both (low byte da, high db: -128..127 each) */
static void uz_cell(uint32_t g, int32_t x, int32_t y, int32_t w, int32_t h, int32_t ra, int32_t da, int32_t rb,
                    int32_t db, const char *name, const char *value)
{
    da = clamp(da, -127, 127), db = clamp(db, -127, 127);
    uz_add(g, UZ_CELL, x, y, w, h, ra, rb, (int32_t)(int16_t)(uint16_t)((uint32_t)(uint8_t)(int8_t)da |
                                                                      (uint32_t)(uint8_t)(int8_t)db << 8),
           name, value);
}
/* the list about to be drawn: its knobs, the cursor's place in the whole list, the first row shown, OCT+ does a row */
static void uz_list_set(int32_t role, int32_t vrole, int32_t sel, int32_t first, int ok)
{
    if (cv_under)
        return;
    uz_list.role = (int8_t)role, uz_list.vrole = (int8_t)vrole, uz_list.ok = (int8_t)ok;
    uz_list.first = (int16_t)first;
    if (uz_list_sel_pending < 0)
        uz_list.sel = (int16_t)sel;
}
/* its rows: n from (x, y), each h cells high and dy apart, w wide; names and values from the caller's arrays (or none) */
static void uz_list_rows(int32_t x, int32_t y, int32_t w, int32_t h, int32_t dy, int32_t n, const char *const *names,
                         const char *const *values)
{
    int32_t i;
    if (cv_under)
        return;
    uz_begin(UZ_G_LIST);
    for (i = 0; i < n; i++)
        uz_add(UZ_G_LIST, UZ_ROW, x, y + i * dy, w, h, uz_list.role, uz_list.vrole, uz_list.first + i,
               names ? names[i] : 0, values ? values[i] : 0);
}
/* NOTES' roll as mo_pic_notes drew it: the page's first step, LEN, the pitch under its lowest bar (c0) and the span
 * the bars' heights cover (mo_note_h), the page's lowest and highest note (lo > hi: none); ui_roll.c reads a click
 * back from them */
static struct { int16_t first, len, c0, span, lo, hi; } uz_roll;
#define UZ(...) (__VA_ARGS__)
/* 1: a help card the mouse opened (tess_core_zone_do TESS_ZA_HELP, Smalti's ?): it stays without ARP held, its knobs
 * as ARP's (another knob's card, a turn scrolls), till a click closes it (ui_help.c help_knob, ui_input.c) */
static uint8_t uz_help_pin;
#define UZ_HELP_PIN uz_help_pin
/* 1: a picker the mouse opened (MODS' SOURCE / DESTINATION clicked, tess_core.c ZQ_PICK): it stays (no timeout,
 * ui_popup.c pick_poll) till a row is clicked, a click away or BACK; pop_close lets it go */
static uint8_t uz_pick_pin;
#define UZ_PICK_PIN uz_pick_pin
#else
#define UZ(...) ((void)0)
#define UZ_HELP_PIN 0
#define UZ_PICK_PIN 0
#endif
