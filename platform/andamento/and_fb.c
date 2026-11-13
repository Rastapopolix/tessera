/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Andamento's screen as memory: 128 x 64 pixels, a byte each, 0 (off) .. 15 (full), the 16 levels of the yellow
 * NHD-2.7-12864WDY3-M (its SSD1322 keeps 4 bits a pixel). Everything Andamento shows is drawn here first; ssd1322.c
 * turns it into the panel's bytes, the web prototype shows it as it is. Yellow is the panel's own colour, so this
 * is Tessera's default Mono look (YELLOW ink on black): ink at 15, dimmed marks at lower levels.
 *
 * Plain C with no hardware, included in a unity build after firmware/src/mo_font.h (the Mono look's 5 x 7 face):
 * platform/daisy/tessera_core.c, tests/oled_render.c, and the web prototype with Andamento's UI (web/andamento/sim.c
 * and platform/andamento/and_ui.c, which draws only through this file). */

#define AND_W 128
#define AND_H 64

static uint8_t and_fb[AND_H][AND_W];             /* one level 0..15 a pixel */

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
static void and_clear(void)
{
    memset(and_fb, 0, sizeof and_fb);
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
