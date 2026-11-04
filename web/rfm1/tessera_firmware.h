/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (Tessera); the stubs: tests/ui_test.c (Kerem Kilic, Ellic Studio; Leo Kuroshita,
 * Hügelton Instruments) */
/* The firmware the .rfm1 runs: the ONE include list (included once, by core_tessera.c). It follows
 * firmware/src/melodee.c as tests/ui_test.c builds it for the host (the sound side through tests/hostsim.c, then the
 * UI), with ui_test.c's HAL stubs, but with the flash: MELODEE_FLASH 1, storage.c on FM1VST's NOR image
 * (core-glue/common/nor.c, the 1 MiB the app keeps), so settings, projects and user presets survive the app.
 * Calibration (MENU > PANEL, panel_setup) waits in loops for the buttons: here the panel table is the app's, so the
 * menu says so instead. */
#include <stddef.h>
#include <stdint.h>
static uint32_t host_slots[3u * 0x14000u / 4u];          /* USR1..3 (zero: empty), as the flash at 0xA0000 */
#define SMP_USER_XIP(k) ((const uint8_t *)host_slots + (k) * SMP_USER_SIZE)
#define main hostsim_main
#include "hostsim.c"
#undef main

/* ------------------------------------------------- HAL (tests/ui_test.c's) --- */
#define FM1_NCOL 11u
static const int8_t FM1_KEYMAP[6][FM1_NCOL] = {   /* the real matrix (hal/fm1_input.h), as web/sim/sim.c has it */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    { 5, 11,  4, 10,  3,  9,  2,  8, -1, -1, -1},
    {34, 35, 36, 37, 38, 40, 39, 13,  7,  6, 12},
    {23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33},
    { 0,  1, 15, 14, 17, 16, 19, 18, 20, 21, 22},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
};
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
    int32_t s;
    e %= 7u;
    s = host_enc[e];
    host_enc[e] = 0;
    return s;
}
static void fm1_wdt_feed(void) {}
static void fm1_irq_off(void) {}
static void fm1_irq_on(void) {}
static uint16_t host_screen[240 * 240];          /* the panel's RAM, big-endian as sent */
static uint64_t host_sent;                        /* pixels sent: the core's lcd_writes */
static void lcd_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c)
{
    uint32_t i, j;
    if (x < 240u && y < 240u)
        host_sent += (uint64_t)(w < 240u - x ? w : 240u - x) * (h < 240u - y ? h : 240u - y);
    for (j = 0; j < h && y + j < 240u; j++)
        for (i = 0; i < w && x + i < 240u; i++)
            host_screen[(y + j) * 240u + x + i] = (uint16_t)((c >> 8) | (c << 8));
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
    for (j = 0; j < h && y + j < 240u; j++)
        for (i = 0; i < w && x + i < 240u; i++)
            host_screen[(y + j) * 240u + x + i] = p[j * w + i];
}
static struct { uint32_t stage, crash_magic, crash_audio, crash_stage, crash_page; } melodee_dbg;
#define DBG_CRASH_MAGIC 0x43524831u               /* (audio.c) */
#define FM1_CRASH_MAGIC 0x43525348u               /* hal/fm1_irq.h's record, for ABOUT's LAST CRASH (ui_menu.c) */
static struct { uint32_t magic, count, vec, pc, rets, emu, dbg, sp, psr, icfg, uptime_ms, etm[4], early; } fm1_crash;

/* --------------------------------------------------------------------- UI --- */
#define MELODEE_FLASH 1                           /* storage.c on the NOR image (below) */
#ifndef MELODEE_VERSION
#define MELODEE_VERSION "rfm1"
#endif
#include "../firmware/src/gfx.c"
#include "../firmware/src/panel.c"
#include "../firmware/src/ui.c"
#include "../firmware/src/icons.c"
#include "../firmware/src/ui_graph.c"
#include "../firmware/src/ui_draw.c"
static void ts_no_calibration(void) { ui_message("ON THE FM-1"); }
#define panel_setup() ts_no_calibration()        /* (MENU > PANEL: see above) */
#include "../firmware/src/ui_menu.c"
#undef panel_setup
#include "../firmware/src/ui_input.c"
#include "../firmware/src/ui_layer.c"

/* --------------------------------------------- storage, presets, projects --- */
/* storage.c's hooks (firmware/src/storage_hw.c on the device): the NOR image's own rules (an erase is 4 KiB of
 * 0xFF and 45 ms of silent audio, a program only clears bits), the expected part, no IRQs to mask */
static uint8_t flash_ok;                          /* JEDEC id matched at boot (persist_boot) */
static int st_read(uint32_t off, void *dst, uint32_t n) { return fm1nor_read(off, dst, n); }
static int st_erase(uint32_t off) { return fm1nor_erase(off); }
static int st_prog(uint32_t off, const void *src, uint32_t n) { return fm1nor_prog_span(off, src, n); }
static uint32_t irq_save(void) { return 0; }
static void irq_restore(uint32_t f) { (void)f; }
static uint32_t fl_jedec_ram(void) { return 0x856014u; }   /* (persist_boot: the expected part) */
static void fl_plain_window_init(void) {}
#define FL_FAR(fn) (fn)
#include "../firmware/src/storage.c"
#include "../firmware/src/upreset.c"
#include "../firmware/src/project.c"
#include "../firmware/src/fm6_store.c"
