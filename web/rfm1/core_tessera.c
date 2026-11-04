/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (Tessera); the glue's pattern: Quixotic7/FM1VST core-glue/melodee (GPL-3.0-only) */
/* The Tessera core for FM1VST (core-api/fm1core.h, ABI 6), built as an .rfm1 for RemoteFM1 (web/rfm1/build.sh). It
 * is the web simulator's firmware (web/sim/sim.c: sound, sequencer, Tessera, UI on tests/ui_test.c's HAL stubs) behind
 * the core ABI instead of the browser's exports. No coroutine: main.c's loop is cut at its waits, as sim.c cuts it,
 * so every entry point returns:
 *   tick(ms)   the device's millisecond (fm1_ms, the 1 MHz tick count), the panel's input in
 *   frame()    one pass of the loop (ui_input, ui_leds, ui_draw), every 15 ms
 *   idle()     ui_input, the other milliseconds (main.c: `while (fm1_ms - m < 15) ui_input();`)
 *   audio()    mix_block, CTL frames at a time, as many as the host asks (128 per block: four)
 * Left out with the hardware, as the web simulator leaves them: the flash (the project slots are RAM; saves are lost
 * when the app closes), USB audio, the USB host, the update, the editor's SysEx, the CPU budget. The flash image the
 * host keeps is FM1VST's (common/nor.c), erased and never written. */
#include <stdio.h>
#include <stdlib.h>                       /* first: its exit / atexit prototypes must not see the macros */
#include <string.h>
#include "fm1core.h"                      /* (before hostsim.c, which defines __attribute__ away) */
#include "panel.h"                        /* (core-glue/common) */
#include "nor.h"
#include "version.h"
#include "exit.h"                         /* exit / atexit -> fm1core_exit / fm1core_atexit (common/exit.h) */
#include "env.h"                          /* getenv -> NULL: no environment in a core (common/env.h) */

#define UI_TEST_NO_MAIN 1
#define TRK_START_TESS 1                  /* the device's power-on start (engines.c), not the tests' */
#include "ui_test.c"

/* the real matrix (hal/fm1_input.h; ui_test.c leaves it empty), as web/sim/sim.c has it */
static const int8_t FM1_KEYMAP[6][FM1_NCOL] = {
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    { 5, 11,  4, 10,  3,  9,  2,  8, -1, -1, -1},
    {34, 35, 36, 37, 38, 40, 39, 13,  7,  6, 12},
    {23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33},
    { 0,  1, 15, 14, 17, 16, 19, 18, 20, 21, 22},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
};

#include "sim_demos.h"                    /* (tools/gen_sim_demos.py: demos/still-tide.tsp, koto-garden.tsp) */

#include "exit_end.h"

/* ---- the glue (hidden: reached only through the descriptor) ---- */
fm1panel_t fm1core_ts_panel;
#define FM1CORE_PANEL (&fm1core_ts_panel)

static uint32_t ts_booted;

/* power-on, as web/sim/sim.c web_power_on: main.c melodee_init (ui_test.c ui_power_on) with the device's BPM and
 * scale, the demos in slots A and B (A loaded, stopped), the default palette */
static void ts_power_on(void)
{
    uint32_t i;
    ui_power_on();
    song.g[G_BPM] = TRK_DEF_BPM;
    for (i = 0; i < NTRK; i++)
        if (TRK_DEF[i][0] != ENGI_DRUM)
            trk[i].p[P_SCALE] = TRK_DEF_SCALE;
    memcpy(proj_bank_slot[0], SIM_DEMO_A, sizeof SIM_DEMO_A);
    proj_bank_len[0] = sizeof SIM_DEMO_A;
    memcpy(proj_bank_slot[1], SIM_DEMO_B, sizeof SIM_DEMO_B);
    proj_bank_len[1] = sizeof SIM_DEMO_B;
    proj_fetch(0);
    proj_fetch(1);
    project_load(0);
    settings.palette = UI_DEFAULT_INDEX;
    palette_set(settings.palette);
    song.master_q12 = 2048;               /* until the panel's MASTER arrives */
    usb.config = 1;                       /* a host is there: MIDI out flows (usb.c usb_midi_out_ready) */
    screen_clear();
    ui.force = 1;
    ts_booted = 1;
}

/* the panel in: buttons and keys are matrix ids already (the panel table is the firmware's own, panel.c); a tap
 * shorter than a tick is a press now and a release at the next tick; the encoders' detents by matrix encoder;
 * MASTER as main.c reads the pot (squared) */
static void ts_in(void)
{
    fm1panel_t *p = FM1CORE_PANEL;
    uint32_t b = p->buttons | fm1p_take_button_taps(p);
    uint32_t k = (p->keys | fm1p_take_key_taps(p)) & ((1u << 27) - 1u);
    uint32_t i, m;
    host_pressed |= b & ~fm1_in.buttons;
    fm1_in.buttons = b;
    host_notes |= k & ~fm1_in.notes;
    fm1_in.notes = k;
    for (i = 0; i < FM1P_NENC && i < 7u; i++)
        host_enc[i] += fm1p_take_enc(p, i);
    m = p->master < 0 ? 0u : p->master > 1023 ? 1023u : (uint32_t)p->master;
    song.master_q12 = (int32_t)((m * m) >> 8);
}

/* the LEDs as the PWM drives them (FM1VST's Melodee glue me_leds_out): fm1_led full, else the brightest dim plane
 * holding the LED at its duty; the LCD (host_screen holds the panel's big-endian pixels, host_sent counts them) */
static void ts_out(void)
{
    const uint8_t *const planes[4] = {fm1_led, fm1_led_dim[0], fm1_led_dim[1], fm1_led_dim[2]};
    const uint8_t levels[4] = {FM1P_LED_LIT, fm1p_duty_level(fm1_led_dim_mask[0]), fm1p_duty_level(fm1_led_dim_mask[1]),
                               fm1p_duty_level(fm1_led_dim_mask[2])};
    uint32_t i;
    fm1p_leds_brightest(FM1CORE_PANEL, planes, levels, 4);
    fm1p_lcd_out(FM1CORE_PANEL, host_screen, (uint32_t)host_sent);
    for (i = 0; i < FM1P_NB; i++)
        FM1CORE_PANEL->btn_id[i] = panel.btn[i];
    for (i = 0; i < FM1P_NE - 1u; i++) {
        FM1CORE_PANEL->enc_id[i] = panel.enc[i];
        FM1CORE_PANEL->enc_dir[i] = panel.dir[i];
    }
}

int fm1core_ts_halted(void) { return fm1core_halted(); }
void fm1core_ts_shutdown(void) { fm1core_run_exit(); }
void fm1core_ts_options(const char *flash_path, int no_flash, int save_on_exit, int headless)
{
    (void)save_on_exit;
    fm1core_exit_cleanup = fm1nor_close;
    fm1nor_options(flash_path, no_flash, headless, "build/tessera/flash.bin");
}
int fm1core_ts_boot_options(int fail, const char *reason, int stage)
{
    (void)fail;
    (void)reason;
    (void)stage;
    return 1;
}
void fm1core_ts_init(int demo)
{
    (void)demo;
    fm1core_exit_cleanup = fm1nor_close;
    fm1nor_open();
    if (!FM1CORE_PANEL->master)
        FM1CORE_PANEL->master = 724;      /* -> master_q12 2047, as melodee_init's 2048 */
    ts_power_on();
    ui_leds();
    ui_draw();
    ts_out();
}
void fm1core_ts_tick(uint32_t ms)
{
    if (fm1core_halted())
        return;
    if ((int32_t)(ms - fm1_ms) > 0) {
        host_ticks += (ms - fm1_ms) * 1000u;
        fm1_ms = ms;
    }
    ts_in();
}
void fm1core_ts_frame(void)
{
    if (fm1core_halted() || !ts_booted)
        return;
    ts_in();
    ui_input();
    ui_leds();
    ui_draw();
    ts_out();
}
void fm1core_ts_idle(void)
{
    if (fm1core_halted() || !ts_booted)
        return;
    ts_in();
    ui_input();
    ts_out();
}

/* audio: mix_block's Q15 stereo, CTL frames at a time (the device's block), clipped to int16; the HOME scope is fed
 * as audio.c audio_block feeds it. frames: a multiple of FM1P_BLOCK (128 = 4 CTL) */
void fm1core_ts_audio(int16_t *stereo, uint32_t frames)
{
    static int32_t o[2u * CTL];
    uint32_t f, k;
    if (fm1core_halted()) {
        memset(stereo, 0, (size_t)frames * 2u * sizeof *stereo);
        return;
    }
    for (f = 0; f + CTL <= frames; f += CTL) {
        mix_block(o, CTL);
        for (k = 0; k < 2u * CTL; k++) {
            int32_t v = o[k];
            if ((k & 3u) == 2u)
                scope_buf[scope_w++ & (SCOPE_N - 1u)] = (int16_t)v;
            stereo[2u * f + k] = (int16_t)(v > 32767 ? 32767 : v < -32768 ? -32768 : v);
        }
    }
}

/* MIDI in: a USB-MIDI packet into usb.c's queue, as the device's endpoint delivers it (0: full, the host retries).
 * SysEx (the editor, the update) is left out: taken and dropped */
int fm1core_ts_midi_in(uint32_t pkt)
{
    uint32_t cin = pkt & 15u;
    if (fm1core_halted() || cin < 8u || cin == 15u)
        return 1;
    if (mi_w - mi_r >= MQ)
        return 0;
    midi_in_q[mi_w % MQ] = pkt;
    mi_w++;
    return 1;
}
int fm1core_ts_midi_out_take(uint32_t *pkt)
{
    if (fm1core_halted() || mo_r == mo_w)
        return 0;
    *pkt = midi_out_q[mo_r % MQ];
    mo_r++;
    return 1;
}
void fm1core_ts_dump(void)
{
    printf("tessera: fm1_ms %u playing %u bpm %d page %u home %u master_q12 %d  midi in %u out %u\n",
           (unsigned)fm1_ms, (unsigned)song.playing, (int)song.g[G_BPM], (unsigned)ui.page, (unsigned)ui.home,
           (int)song.master_q12, (unsigned)(mi_w - mi_r), (unsigned)(mo_w - mo_r));
}
void fm1core_ts_stats(uint32_t *shed, uint32_t *cpu_pct)
{
    *shed = 0;
    *cpu_pct = 0;                         /* (the CPU budget is not simulated) */
}
void fm1core_ts_ui_info(char *buf, uint32_t n)
{
    snprintf(buf, n, "%s page %u part %u menu %u playing %u", ui.home ? "home" : "page", (unsigned)ui.page,
             (unsigned)song.sel + 1u, (unsigned)ui.menu, (unsigned)song.playing);
}
const int8_t *fm1core_ts_keymap(void) { return &FM1_KEYMAP[0][0]; }

/* ---- the flash (fm1core.h ABI 2): common/nor.c's image, never written (the slots are RAM) ---- */
uint8_t *fm1core_ts_flash(void) { return fm1nor_flash(); }
uint32_t fm1core_ts_flash_dirty(void) { return fm1nor_dirty(); }
int fm1core_ts_flash_stage(const uint8_t *bytes, uint32_t n) { return fm1nor_stage(bytes, n); }
int fm1core_ts_flash_sync(void) { return 1; }
