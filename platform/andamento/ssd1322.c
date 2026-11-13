/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Andamento's screen driver: the yellow NHD-2.7-12864WDY3-M, 128 x 64 at 16 levels, on its SSD1322 controller in
 * 4-wire SPI. Plain C with no hardware: the platform gives it two calls, and_spi_cmd and and_spi_data, and does the
 * reset pulse; everything else is here, so the host tests check it byte for byte (tests/oled_render.c).
 *
 * How the panel is wired, and so how the bytes go. The SSD1322 holds 480 x 128 pixels of 4 bits, and its column
 * address counts 4 pixels (2 bytes) at a time. Newhaven's 128 x 64 module drives 256 of its outputs (segments), two
 * for each of its 128 pixels, starting at column address 28. So a row of the screen is 64 column addresses (28..91)
 * and 128 bytes, a byte a pixel: the level goes in both halves of the byte (both of the pixel's segments), which is
 * also right whichever of the two the glass actually connects. A whole frame is 64 x 128 = 8,192 bytes, 6.6 ms at the
 * controller's 10 MHz SPI limit.
 *
 * In 4-wire SPI the DC pin tells the controller what a byte is: a command with DC low, and the command's arguments
 * and the pixel data with DC high (and_spi_data).
 *
 * Source of the register values (2026-11-13): u8g2's driver for this module, u8x8_d_ssd1322_nhd_128x64
 * (github.com/olikraus/u8g2, csrc/u8x8_d_ssd1322.c, BSD-2), whose comments cite the NHD-2.7-12864WDW3-M datasheet
 * (the white version of the same module); its column offset 28 and remap 0x16 / 0x11 are taken from there. Not yet
 * checked on the panel: the first picture on the board (and_testcard) says whether they are right. */

/* the platform's two calls (platform/daisy/main.cpp; recorded by tests/oled_render.c) */
void and_spi_cmd(uint8_t c);                          /* one command byte, DC low */
void and_spi_data(const uint8_t *p, uint32_t n);      /* n bytes with DC high: a command's arguments, or pixels */

#define SSD_COL0 28u                                  /* the column address of the screen's left edge */
#define SSD_NCOL (AND_W / 2u)                         /* column addresses a row takes: 64 */
#define SSD_FRAME (AND_W * AND_H)                     /* bytes a frame: 8,192 */

typedef struct {
    uint8_t cmd, n, a[2];                             /* the command, how many arguments, the arguments */
} ssd_op_t;

/* after the reset pulse, in this order; the display stays off (ssd1322_on) until a first frame is in */
static const ssd_op_t SSD_INIT[] = {
    {0xFD, 1, {0x12}},          /* unlock the command interface */
    {0xAE, 0, {0}},             /* display off */
    {0xB3, 1, {0x91}},          /* clock divider / oscillator: about 80 frames a second */
    {0xCA, 1, {0x3F}},          /* multiplex: 64 rows */
    {0xA2, 1, {0x00}},          /* display offset 0 */
    {0xA1, 1, {0x00}},          /* start line 0 */
    {0xAB, 1, {0x01}},          /* the internal VDD regulator on */
    {0xA0, 2, {0x16, 0x11}},    /* remap: horizontal addressing, column and nibble remap, COM scan reversed; dual COM */
    {0xC7, 1, {0x0F}},          /* master current: full */
    {0xC1, 1, {0x9F}},          /* contrast */
    {0xB1, 1, {0xF2}},          /* phase lengths (reset, precharge) */
    {0xBB, 1, {0x1F}},          /* precharge voltage */
    {0xB4, 2, {0xA0, 0xFD}},    /* display enhancement A: VSL external (on the module), low grays enhanced */
    {0xBE, 1, {0x04}},          /* VCOMH */
    {0xB9, 0, {0}},             /* the default gray table: linear (a measured table, command 0xB8, can come later) */
    {0xA6, 0, {0}},             /* normal display (not inverted) */
    {0xA9, 0, {0}},             /* no partial display */
};

static void ssd_op(const ssd_op_t *o)
{
    and_spi_cmd(o->cmd);
    if (o->n)
        and_spi_data(o->a, o->n);
}

/* the controller set up for this panel, display still off: after the reset pulse (RESET low >= 2 us, then wait) */
static void ssd1322_init(void)
{
    uint32_t i;
    for (i = 0; i < sizeof SSD_INIT / sizeof SSD_INIT[0]; i++)
        ssd_op(&SSD_INIT[i]);
}

/* the display on (1) or off (0, the controller keeps its memory) */
static void ssd1322_on(uint32_t on)
{
    and_spi_cmd(on ? 0xAF : 0xAE);
}

/* the framebuffer as the panel's bytes: out holds SSD_FRAME bytes, row by row, a byte a pixel, its level in both halves */
static void ssd1322_pack(const uint8_t fb[AND_H][AND_W], uint8_t *out)
{
    uint32_t x, y;
    for (y = 0; y < AND_H; y++)
        for (x = 0; x < AND_W; x++) {
            uint32_t l = fb[y][x] & 15u;
            *out++ = (uint8_t)(l << 4 | l);
        }
}

/* a packed frame to the panel: the window (all of the screen), write, the 8,192 bytes. On the Daisy, buf lives in
 * memory the SPI's DMA can read (not cached: libDaisy's DMA_BUFFER_MEM_SECTION) and stays untouched until it is sent */
static void ssd1322_frame(const uint8_t *buf)
{
    static const ssd_op_t WIN[2] = {
        {0x15, 2, {SSD_COL0, SSD_COL0 + SSD_NCOL - 1u}},   /* columns 28..91 */
        {0x75, 2, {0, AND_H - 1u}},                        /* rows 0..63 */
    };
    ssd_op(&WIN[0]);
    ssd_op(&WIN[1]);
    and_spi_cmd(0x5C);                                     /* write to the display memory: the data follows */
    and_spi_data(buf, SSD_FRAME);
}
