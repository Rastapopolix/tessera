/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Andamento's screens as memory: two panels of 128 x 64 pixels, a byte each, 0 (off) .. 15 (full), the 16 levels of the yellow
 * NHD-2.7-12864WDY3-M (its SSD1322 keeps 4 bits a pixel). Everything Andamento shows is drawn here first; ssd1322.c
 * turns it into the panel's bytes, the web prototype shows it as it is. Yellow is the panel's own colour, so this
 * is Tessera's default Mono look (YELLOW ink on black): ink at 15, dimmed marks at lower levels. Two panels side by
 * side (2026-11-14): the page screen (Tessera's header, the picture, the tabs) and the dial screen over the 8 encoders;
 * a left/right-hand setting picks which side each is on. Every drawing call goes to and_fb, the panel and_target set.
 *
 * Plain C with no hardware, included in a unity build after firmware/src/mo_font.h (the Mono look's 5 x 7 face):
 * platform/daisy/tessera_core.c, tests/oled_render.c, and the web prototype with Andamento's UI (web/andamento/sim.c
 * and platform/andamento/and_ui.c, which draws only through this file). */

#include <math.h>

#define AND_W 128
#define AND_H 64
#define AND_NSCR 2                               /* two panels (andamento README, Screen): the page and the dials */
enum { AND_SCR_PAGE, AND_SCR_DIAL };

static uint8_t and_fbs[AND_NSCR][AND_H][AND_W];  /* one level 0..15 a pixel, a frame a panel */
static uint8_t (*and_fb)[AND_W] = and_fbs[AND_SCR_PAGE];   /* the frame drawn on now (and_target) */

/* draw on panel s from now on (AND_SCR_*) */
static void and_target(uint32_t s) { and_fb = and_fbs[s < AND_NSCR ? s : 0u]; }

static void and_px(int32_t x, int32_t y, uint32_t l)
{
    if ((uint32_t)x < AND_W && (uint32_t)y < AND_H)
        and_fb[y][x] = (uint8_t)(l & 15u);
}
static void and_fill(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t l)
{
    int32_t i, j;
    for (j = y; j < y + h; j++)
        for (i = x; i < x + w; i++)
            and_px(i, j, l);
}
/* a 1-pixel outline */
static void and_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t l)
{
    and_fill(x, y, w, 1, l);
    and_fill(x, y + h - 1, w, 1, l);
    and_fill(x, y, 1, h, l);
    and_fill(x + w - 1, y, 1, h, l);
}
/* text in the 5 x 7 face, 6 px a character, small letters as capitals; at most n characters; the width drawn */
static int32_t and_text(int32_t x, int32_t y, const char *s, uint32_t l, uint32_t n)
{
    int32_t x0 = x;
    for (; *s && n; s++, n--, x += 6) {
        uint32_t c = (uint8_t)*s, r, b;
        if (c >= 'a' && c <= 'z')
            c -= 32;                             /* (one case on this screen) */
        if (c < ' ' || c > 'z')
            c = '?';
        for (r = 0; r < 7u; r++)
            for (b = 0; b < 5u; b++)
                if ((MO_FONT[c - ' '][r] >> (4u - b)) & 1u)
                    and_px(x + (int32_t)b, y + (int32_t)r, l);
    }
    return x - x0;
}
/* a line from (x0, y0) to (x1, y1), both ends drawn */
static void and_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t l)
{
    int32_t dx = x1 > x0 ? x1 - x0 : x0 - x1, dy = y1 > y0 ? y0 - y1 : y1 - y0;
    int32_t sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, e = dx + dy;
    for (;;) {
        int32_t e2 = 2 * e;
        and_px(x0, y0, l);
        if (x0 == x1 && y0 == y1)
            return;
        if (e2 >= dy) e += dy, x0 += sx;
        if (e2 <= dx) e += dx, y0 += sy;
    }
}
/* a 1-pixel ring of radius r round (cx, cy) */
static void and_ring(int32_t cx, int32_t cy, int32_t r, uint32_t l)
{
    int32_t x = r, y = 0, e = 1 - r;
    while (x >= y) {
        and_px(cx + x, cy + y, l), and_px(cx - x, cy + y, l), and_px(cx + x, cy - y, l), and_px(cx - x, cy - y, l);
        and_px(cx + y, cy + x, l), and_px(cx - y, cy + x, l), and_px(cx + y, cy - x, l), and_px(cx - y, cy - x, l);
        y++;
        if (e < 0)
            e += 2 * y + 1;
        else
            x--, e += 2 * (y - x) + 1;
    }
}
/* Tessera's dial (the FM-1's mo_dial): the ring dim, a pointer to it in l, v 0..1000 over 270 degrees from the lower
 * left round the top to the lower right */
static void and_dial(int32_t cx, int32_t cy, int32_t r, int32_t v, uint32_t ring, uint32_t l)
{
    float a = (225.0f - 270.0f * (float)(v < 0 ? 0 : v > 1000 ? 1000 : v) / 1000.0f) * 0.0174532925f;
    and_ring(cx, cy, r, ring);
    and_line(cx, cy, cx + (int32_t)lrintf((float)r * cosf(a)), cy - (int32_t)lrintf((float)r * sinf(a)), l);
}
/* text at twice the size (10 x 14 a character, 12 px apart); the width drawn */
static int32_t and_text2(int32_t x, int32_t y, const char *s, uint32_t l, uint32_t n)
{
    int32_t x0 = x;
    for (; *s && n; s++, n--, x += 12) {
        uint32_t c = (uint8_t)*s, r, b;
        if (c >= 'a' && c <= 'z')
            c -= 32;
        if (c < ' ' || c > 'z')
            c = '?';
        for (r = 0; r < 7u; r++)
            for (b = 0; b < 5u; b++)
                if ((MO_FONT[c - ' '][r] >> (4u - b)) & 1u)
                    and_fill(x + 2 * (int32_t)b, y + 2 * (int32_t)r, 2, 2, l);
    }
    return x - x0;
}
static void and_clear(void)
{
    memset(and_fb, 0, (size_t)AND_W * AND_H);
}

/* The bring-up card: what the Daisy shows before there is a UI, and the first picture to check on the real panel.
 *   the border      every edge pixel lit: a missing or doubled edge means the column window (SSD_COL0) is off
 *   the corners     TL / TR at the top, BL / BR at the foot: mirrored or upside-down text means the remap is wrong
 *   the 16 bars     levels 0..F left to right, each labelled: a step that does not brighten is the gray table */
static void and_testcard(void)
{
    uint32_t i;
    and_clear();
    and_rect(0, 0, AND_W, AND_H, 15);
    and_text(3, 3, "TL", 15, 2);
    and_text(AND_W - 3 - 11, 3, "TR", 15, 2);
    and_text(AND_W / 2 - 26, 3, "ANDAMENTO", 15, 9);
    for (i = 0; i < 16u; i++) {
        int32_t x = 8 + (int32_t)i * 7;
        char s[2] = {"0123456789ABCDEF"[i], 0};
        and_fill(x, 13, 7, 20, i);
        and_text(x + 1, 35, s, 15, 1);
    }
    and_text(AND_W / 2 - 35, 45, "SSD1322 16 LV", 8, 13);
    and_text(3, AND_H - 10, "BL", 15, 2);
    and_text(AND_W - 3 - 11, AND_H - 10, "BR", 15, 2);
    and_text(AND_W / 2 - 23, AND_H - 10, "128 X 64", 15, 8);
}
