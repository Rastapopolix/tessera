/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Andamento's screen (platform/andamento: and_fb.c, ssd1322.c) on the PC, through a model of the SSD1322.
 *   oled_render OUTDIR    (run by tests/run_tests.sh; tests/oled_render.py makes the PNGs)
 * The driver's SPI calls go into a small model of the controller: commands with their argument counts, the column and
 * row window, the write pointer, the display memory (480 x 128 pixels, 4 bits, 2 a byte). The pictures are read back
 * out of that memory the way the panel shows it (columns 28..91, a byte a pixel), not from the framebuffer, so a
 * packing or addressing mistake shows in them. Checks:
 *   init      every command known, with its number of arguments; horizontal addressing, 64 rows, display off
 *   frames    exactly the screen's window written (8,192 bytes, nothing outside it), both halves of each byte the
 *             pixel's level, the picture read back equal to the framebuffer, again on a second frame
 *   the card  the bring-up card (and_testcard): the border lit, every one of the 16 levels used
 * Output: OUTDIR/ppm/<screen>.ppm, true size, in the panel's yellow (Tessera's YELLOW ink, 255 208 32) at level / 15.
 * Exit 1 on a failed check. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../firmware/src/mo_font.h"
#include "../platform/andamento/and_fb.c"
#include "../platform/andamento/ssd1322.c"

static uint32_t nfail;
#define CHECK(c, ...) do { if (!(c)) { nfail++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

/* ------------------------------------------------------------ the model --- */
/* the commands the driver may send and their argument counts (SSD1322 datasheet's command table, as u8g2 uses it) */
static int8_t argc_of(uint8_t c)
{
    switch (c) {
    case 0xA6: case 0xA9: case 0xAE: case 0xAF: case 0xB9: case 0x5C: return 0;
    case 0xFD: case 0xB3: case 0xCA: case 0xA2: case 0xA1: case 0xAB: case 0xC7: case 0xC1:
    case 0xB1: case 0xBB: case 0xBE: return 1;
    case 0xA0: case 0xB4: case 0x15: case 0x75: return 2;
    default: return -1;
    }
}
static struct {
    uint8_t ram[128][240], wrote[128][240];  /* display memory: row, byte (column address x 2 + half) */
    uint8_t reg[256][2];                     /* the last arguments of each command */
    uint8_t cmd, nargs, args[2], writing, half, on;
    int8_t need;
    uint32_t col_s, col_e, row_s, row_e, col, row;
    uint32_t ncmd, ndata, nerr;
} m;

static void model_reset(void)
{
    memset(&m, 0, sizeof m);
    m.col_e = 119;
    m.row_e = 127;
}
static void model_err(const char *what, uint32_t v)
{
    if (m.nerr++ < 8u)
        printf("FAIL: SSD1322 model: %s (0x%02X)\n", what, (unsigned)v);
    nfail++;
}
static void model_apply(void)
{
    m.reg[m.cmd][0] = m.args[0];
    m.reg[m.cmd][1] = m.args[1];
    if (m.cmd == 0x15) {
        if (m.args[0] > m.args[1] || m.args[1] > 119u)
            model_err("column window", m.args[1]);
        m.col_s = m.args[0], m.col_e = m.args[1];
    } else if (m.cmd == 0x75) {
        if (m.args[0] > m.args[1] || m.args[1] > 127u)
            model_err("row window", m.args[1]);
        m.row_s = m.args[0], m.row_e = m.args[1];
    } else if (m.cmd == 0xAE || m.cmd == 0xAF) {
        m.on = m.cmd == 0xAF;
    }
}

void and_spi_cmd(uint8_t c)
{
    if (m.need > 0)
        model_err("a command before the last one had its arguments", c);
    m.ncmd++;
    m.cmd = c;
    m.nargs = 0;
    m.writing = 0;
    m.need = argc_of(c);
    if (m.need < 0) {
        model_err("unknown command", c);
        m.need = 0;
        return;
    }
    if (c == 0x5C) {                                  /* write: from the window's first byte, row by row */
        m.writing = 1, m.half = 0, m.col = m.col_s, m.row = m.row_s;
        return;
    }
    if (m.need == 0)
        model_apply();
}
void and_spi_data(const uint8_t *p, uint32_t n)
{
    for (; n; n--, p++) {
        if (m.writing) {
            m.ram[m.row][m.col * 2u + m.half] = *p;
            m.wrote[m.row][m.col * 2u + m.half] = 1;
            m.ndata++;
            if (++m.half == 2u) {
                m.half = 0;
                if (++m.col > m.col_e) {
                    m.col = m.col_s;
                    if (++m.row > m.row_e)
                        m.row = m.row_s;
                }
            }
        } else if (m.need > 0) {
            m.args[m.nargs++] = *p;
            if (--m.need == 0)
                model_apply();
        } else {
            model_err("data with no command to take it", *p);
        }
    }
}

/* ------------------------------------------------------------- checks --- */
static uint8_t frame[SSD_FRAME];

/* the screen as the panel shows it: from the model's memory, columns SSD_COL0.., a byte a pixel */
static uint32_t panel_level(uint32_t x, uint32_t y)
{
    return m.ram[y][SSD_COL0 * 2u + x] & 15u;
}

static void send_and_check(const char *name)
{
    uint32_t x, y, out = 0, mism = 0, halves = 0, n0 = m.ndata;
    memset(m.wrote, 0, sizeof m.wrote);
    ssd1322_pack(and_fb, frame);
    ssd1322_frame(frame);
    CHECK(m.ndata - n0 == SSD_FRAME, "%s: %u bytes written, not %u", name, (unsigned)(m.ndata - n0), SSD_FRAME);
    CHECK(m.half == 0 && m.col == m.col_s && m.row == m.row_s, "%s: the write pointer not back at the window's start",
          name);
    for (y = 0; y < 128u; y++)
        for (x = 0; x < 240u; x++) {
            int in = y < AND_H && x >= SSD_COL0 * 2u && x < SSD_COL0 * 2u + AND_W;
            if (m.wrote[y][x] != in)
                out++;
        }
    CHECK(out == 0, "%s: %u bytes of the display memory written outside the screen, or left unwritten in it", name,
          (unsigned)out);
    for (y = 0; y < AND_H; y++)
        for (x = 0; x < AND_W; x++) {
            uint8_t b = m.ram[y][SSD_COL0 * 2u + x];
            if ((b >> 4) != (b & 15u))
                halves++;
            if (panel_level(x, y) != and_fb[y][x])
                mism++;
        }
    CHECK(halves == 0, "%s: %u bytes whose two halves differ", name, (unsigned)halves);
    CHECK(mism == 0, "%s: %u pixels on the panel differ from the framebuffer", name, (unsigned)mism);
}

static void write_ppm(const char *dir, const char *name)
{
    char path[512];
    FILE *f;
    uint32_t x, y;
    snprintf(path, sizeof path, "%s/ppm/%s.ppm", dir, name);
    if (!(f = fopen(path, "wb"))) {
        CHECK(0, "cannot write %s", path);
        return;
    }
    fprintf(f, "P6\n%d %d\n255\n", AND_W, AND_H);
    for (y = 0; y < AND_H; y++)
        for (x = 0; x < AND_W; x++) {
            uint32_t l = panel_level(x, y);
            uint8_t rgb[3] = {(uint8_t)(255u * l / 15u), (uint8_t)(208u * l / 15u), (uint8_t)(32u * l / 15u)};
            fwrite(rgb, 1, 3, f);
        }
    fclose(f);
}

/* the face: every character the screen draws (MO_FONT ' ' .. '`'; small letters are drawn as capitals) at full ink,
 * then a digit at each level */
static void draw_face(void)
{
    char line[22];
    uint32_t c = ' ', i, y = 4;
    and_clear();
    while (c <= '`') {
        for (i = 0; i < 21u && c <= '`'; i++, c++)
            line[i] = (char)c;
        line[i] = 0;
        and_text(1, (int32_t)y, line, 15, 21);
        y += 11;
    }
    for (i = 0; i < 16u; i++) {
        char s[2] = {"0123456789ABCDEF"[i], 0};
        and_text(1 + (int32_t)i * 8, 54, s, i ? i : 1, 1);
    }
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : "build/oled";
    uint32_t x, y, used = 0, border = 1;

    model_reset();
    ssd1322_init();
    CHECK(m.ncmd == sizeof SSD_INIT / sizeof SSD_INIT[0], "init: %u commands", (unsigned)m.ncmd);
    CHECK(m.need == 0, "init: the last command short of arguments");
    CHECK(m.reg[0xFD][0] == 0x12, "init: the command interface not unlocked");
    CHECK(m.reg[0xCA][0] == AND_H - 1u, "init: multiplex %u, not the panel's %u rows", m.reg[0xCA][0] + 1u, AND_H);
    CHECK((m.reg[0xA0][0] & 1u) == 0, "init: vertical addressing (the frame is sent row by row)");
    CHECK(!m.on, "init: the display on before a frame is in");

    and_clear();                                      /* the first frame: black, then the display on */
    send_and_check("blank");
    ssd1322_on(1);
    CHECK(m.on, "the display not on");

    and_testcard();
    for (y = 0; y < AND_H; y++)
        for (x = 0; x < AND_W; x++) {
            used |= 1u << and_fb[y][x];
            if ((x == 0 || y == 0 || x == AND_W - 1u || y == AND_H - 1u) && and_fb[y][x] != 15u)
                border = 0;
        }
    CHECK(used == 0xFFFFu, "testcard: levels used 0x%04X, not all 16", (unsigned)used);
    CHECK(border, "testcard: the border not lit all round");
    send_and_check("testcard");
    send_and_check("testcard, again");                /* a second frame lands on the first (the window re-sent) */
    write_ppm(dir, "testcard");

    draw_face();
    send_and_check("face");
    write_ppm(dir, "face");

    printf("SSD1322: %u init commands, a frame %u bytes + 7 of window and write commands, %.1f ms at 10 MHz SPI\n",
           (unsigned)(sizeof SSD_INIT / sizeof SSD_INIT[0]), SSD_FRAME, (SSD_FRAME + 7) * 8 / 10e6 * 1e3);
    printf("%s: %u failed\n", nfail ? "FAIL" : "ok", (unsigned)nfail);
    return nfail ? 1 : 0;
}
