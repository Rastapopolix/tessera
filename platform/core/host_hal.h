/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Kerem Kilic (Ellic Studio)
 * Portions Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments; Theo Dainis (melodee-tess) */
/* The FM-1's hardware without the FM-1 (moved here from tests/ui_test.c): the stand-ins the firmware's UI and
 * projects call in place of firmware/hal/: the buttons and keys (host_pressed, host_notes, fm1_in), the encoders
 * (host_enc), the LEDs, a 240 x 240 RGB565 screen (host_screen), no flash. After core_sources.h, before
 * core_ui_sources.h. The host tests drive these directly; platform/core/tess_core.c turns the API's events into them.
 * HOST_HAL_SCREEN 0 (the Daisy): no copy of the FM-1's screen is kept (its UI still runs, drawing nowhere). */
#ifndef HOST_HAL_SCREEN
#define HOST_HAL_SCREEN 1
#endif
#define FM1_NCOL 11u
static const int8_t FM1_KEYMAP[6][FM1_NCOL];
static uint8_t fm1_led[FM1_NCOL];
static uint8_t fm1_led_dim[3][FM1_NCOL], fm1_led_dim_mask[3];   /* (hal/fm1_input.h: the dim planes) */
#define FM1_TICKS_PER_US 1u
static uint32_t host_ticks, host_pressed, host_notes;
static int32_t host_enc[7];
static uint32_t fm1_ticks(void) { return host_ticks; }
static uint32_t fm1_input_edges(int x) { uint32_t p = host_pressed; (void)x; host_pressed = 0; return p; }
static uint32_t fm1_input_note_edges(void) { uint32_t n = host_notes; host_notes = 0; return n; }
static int32_t fm1_enc_take(uint32_t e)
{
    e %= 7u;
    int32_t s = host_enc[e]; host_enc[e] = 0;
#ifdef UI_ENC_TAKE_HOOK
    UI_ENC_TAKE_HOOK(e);                           /* a scan interrupt can publish another detent immediately */
#endif
    return s;
}
static void fm1_wdt_feed(void) {}
static void fm1_irq_off(void) {}
static void fm1_irq_on(void) {}
#if HOST_HAL_SCREEN
static uint16_t host_screen[240 * 240];
#endif
static int host_blit_trace;                       /* (tests/ui_render.c UI_AUDIT_BLITS: each transfer listed) */
static uint64_t host_sent;                        /* pixels sent to the LCD, blitted or filled (tests/ui_render.c) */
/* pixels sent twice in one frame, the second time different: the panel shows the first for a moment (a flicker on
 * the device's SPI screen, never in a render; MODS' rows sent blank under their moves and lights, then those) */
static uint64_t host_redrawn;
static uint32_t host_frame_n = 1;
#if HOST_HAL_SCREEN
static uint32_t host_put_f[240 * 240];
static void host_put(uint32_t i, uint16_t v)
{
    if (host_put_f[i] == host_frame_n && host_screen[i] != v) host_redrawn++;
    host_put_f[i] = host_frame_n;
    host_screen[i] = v;
}
#else
static void host_put(uint32_t i, uint16_t v) { (void)i; (void)v; }
#endif
static void lcd_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c)
{
    uint32_t i, j;
    if (host_blit_trace) fprintf(stderr, " [fill %u,%u %ux%u]", x, y, w, h);
    if (x < 240u && y < 240u)
        host_sent += (uint64_t)(w < 240u - x ? w : 240u - x) * (h < 240u - y ? h : 240u - y);
#ifdef UI_FILL_HOOK
    UI_FILL_HOOK(x, y, w, h);                     /* (tests/ui_render.c: what a fill covers is gone) */
#endif
    for (j = 0; j < h && y + j < 240u; j++)
        for (i = 0; i < w && x + i < 240u; i++)
            host_put((y + j) * 240u + x + i, (uint16_t)((c >> 8) | (c << 8)));
}
static void lcd_sync(void) {}
static void lcd_gamma_switch(uint32_t opt) { (void)opt; }   /* (lcd.c: the device's panel) */
#define SCOPE_N 512u                              /* audio.c: the HOME oscilloscope */
static int16_t scope_buf[SCOPE_N];
static uint32_t scope_w;
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *p)
{
    uint32_t i, j;
    host_sent += (uint64_t)w * h;
    if (host_blit_trace) fprintf(stderr, " [blit %u,%u %ux%u]", x, y, w, h);
    for (j = 0; j < h && y + j < 240u; j++)
        for (i = 0; i < w && x + i < 240u; i++) host_put((y + j) * 240u + x + i, p[j * w + i]);
}
static struct { uint32_t stage, crash_magic, crash_audio, crash_stage, crash_page; } melodee_dbg;
#define DBG_CRASH_MAGIC 0x43524831u               /* (audio.c) */
#define FM1_CRASH_MAGIC 0x43525348u               /* hal/fm1_irq.h's record, for ABOUT's LAST CRASH (ui_menu.c) */
static struct { uint32_t magic, count, vec, pc, rets, emu, dbg, sp, psr, icfg, uptime_ms, etm[4], early; } fm1_crash;
#define MELODEE_FLASH 0
#ifndef MELODEE_VERSION
#define MELODEE_VERSION "TEST"
#endif
#define UI_TEST_PALETTE 1                         /* + GRAY: every screen drawn in it must stay gray */
#if MELODEE_ACCESS                                /* the screen reader (ui_speak.c): what it sends, kept for the tests */
#define ANN_NAME "Tessera TEST"
#define ANN_MAX 160u
#include "ann_words.h"                          /* (the words link in the HELLO: speak_test checks it) */
#include "announce.c"
static char host_said[4096];                      /* each SAY a line: "N text" (N NOW, Q queued, K kept), HELLO "H text" */
static uint32_t host_said_n, host_said_frames;
static int ann_wire_send(const uint8_t *p, uint32_t n)
{
    uint32_t i;
    host_said_frames++;
    if (n < 8u || p[0] != 0xF0u || p[n - 1u] != 0xF7u || host_said_n + n + 4u >= sizeof host_said)
        return 0;
    host_said[host_said_n++] = p[4] == ANN_HELLO ? 'H' : p[5] & ANN_KEEP ? 'K' : p[5] & ANN_NOW ? 'N' : 'Q';
    host_said[host_said_n++] = ' ';
    for (i = 6u; i + 1u < n; i++)
        host_said[host_said_n++] = p[i] ? (char)p[i] : '|';
    host_said[host_said_n++] = '\n';
    host_said[host_said_n] = 0;
    return 0;
}
#endif
