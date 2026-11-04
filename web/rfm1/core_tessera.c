/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (Tessera); the glue's pattern: Quixotic7/FM1VST core-glue/melodee (GPL-3.0-only) */
/* The Tessera core for FM1VST (core-api/fm1core.h, ABI 6), built as an .rfm1 for RemoteFM1 (web/rfm1/build.sh): the
 * firmware of tessera_firmware.h (the host build with the flash) behind the core ABI. No coroutine: main.c's loop is
 * cut at its waits, as web/sim/sim.c cuts it, so every entry point returns:
 *   init       main.c fm1_main's power-on: the flash (settings, projects, user presets), the parts, the BOOT project
 *   tick(ms)   the device's millisecond (fm1_ms, the 1 MHz tick count), the panel's input in
 *   frame()    one pass of the loop (ui_input, the settings save, ui_leds, ui_draw), every frame the host draws
 *   idle()     ui_input, the other milliseconds (main.c: `while (fm1_ms - m < 15) ui_input();`)
 *   audio()    mix_block, CTL frames at a time, as many as the host asks (128 per block: four)
 * The flash is FM1VST's NOR image (common/nor.c), which the app keeps between runs. Its first run (the image erased)
 * puts the demos in slots A and B and loads A, as the web simulator starts; later runs boot as the device does.
 * Left out with the hardware: USB audio, the USB host, the update, the editor's SysEx, the CPU budget. */
#include <stdio.h>
#include <stdlib.h>                       /* first: its exit / atexit prototypes must not see the macros */
#include <string.h>
#include "fm1core.h"                      /* (before hostsim.c, which defines __attribute__ away) */
#include "panel.h"                        /* (core-glue/common) */
#include "nor.h"
#include "version.h"
#include "exit.h"                         /* exit / atexit -> fm1core_exit / fm1core_atexit (common/exit.h) */
#include "env.h"                          /* getenv -> NULL: no environment in a core (common/env.h) */

#define TRK_START_TESS 1                  /* the device's power-on start (engines.c), not the tests' */
#include "tessera_firmware.h"
#include "sim_demos.h"                    /* (tools/gen_sim_demos.py: demos/still-tide.tsp, koto-garden.tsp) */

#include "exit_end.h"

/* ---- the glue (hidden: reached only through the descriptor) ---- */
fm1panel_t fm1core_ts_panel;
#define FM1CORE_PANEL (&fm1core_ts_panel)

static uint32_t ts_booted;

/* main.c melodee_init (static there; main.c is the device's boot and cannot be built on the host). Verbatim. */
static void ts_melodee_init(void)
{
    uint32_t i;
    chain_defaults(&chain_config);
    pattern_init();
    for (i = 0; i < G_COUNT; i++)
        song.g[i] = GP[i].def;
    song.g[G_BPM] = TRK_DEF_BPM;
    undo_depth++;                             /* (no undo copy of the power-on loads) */
    fm6_init();                               /* every track's FM6 patch: the init voice */
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        track_defaults(t);
        set_engine_of(t, TRK_DEF[i][0]);
        apply_preset_to(t, TRK_DEF[i][1]);    /* with its sends */
        if (TRK_DEF[i][0] != ENGI_DRUM)
            t->p[P_SCALE] = TRK_DEF_SCALE;
        t->engine = t->eng_req;
        track_defaults_steps(t);              /* (a sound load never touches them) */
        pat_sig[i] = steps_sig(t);            /* (empty: not the user's) */
        pat_last[i] = 0;
    }
    undo_depth--;
    song.sel = 0;
    song.master_q12 = 2048;
    ui.home = 1;
    ui.force = 1;
}

/* the app's first run: nothing written to the flash yet */
static int ts_flash_erased(void)
{
    const uint8_t *f = fm1nor_flash();
    uint32_t i;
    for (i = 0; i < FM1NOR_SIZE; i++)
        if (f[i] != 0xFFu)
            return 0;
    return 1;
}

/* power-on, main.c fm1_main's order up to its loop (no waits, no splash): the flash, the settings, the panel table,
 * the parts, the GLO values as last used, the BOOT project or the template. A first run: the demos in A and B (as
 * the backup page would put them there), A loaded */
static void ts_power_on(void)
{
    int first = ts_flash_erased();
    settings_init();
    lcd_fill(0, 0, 240, 240, T_BG);
    persist_boot();
    settings_init();
    panel_init();
    ts_melodee_init();
    usb.config = 1;                       /* a host is there: MIDI out flows (usb.c usb_midi_out_ready) */
    glo_restore();
    if (first && flash_ok && !st_save(OBJ_SLOT(0), SIM_DEMO_A, sizeof SIM_DEMO_A) &&
        !st_save(OBJ_SLOT(1), SIM_DEMO_B, sizeof SIM_DEMO_B)) {
        proj_fetch(0);
        proj_fetch(1);
        project_load(0);
    } else {
        project_boot();
    }
    lcd_fill(0, 0, 240, 240, T_BG);
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
/* shutdown: a settings change the firmware has queued (settings_poll saves only while stopped) is written, as it
 * would be once the device stops */
void fm1core_ts_shutdown(void)
{
    if (ts_booted && flash_ok && persist_pending)
        settings_poll();
    fm1core_run_exit();
}
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
    fm1nor_open();                        /* (common/nor.c: erased, then the stage or the file) */
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
    fm6_service();                        /* a pending FM6 settings save */
    ui_input();
    glo_poll();                           /* a kept GLO value changed: the settings, later */
    settings_poll();                      /* queued settings save: only while stopped */
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
 * as audio.c audio_block feeds it. A flash erase is silence, as on the device (its IRQs are off while it erases:
 * common/nor.c counts the blocks). frames: a multiple of FM1P_BLOCK (128 = 4 CTL) */
void fm1core_ts_audio(int16_t *stereo, uint32_t frames)
{
    static int32_t o[2u * CTL];
    uint32_t b, f, k;
    if (fm1core_halted()) {
        memset(stereo, 0, (size_t)frames * 2u * sizeof *stereo);
        return;
    }
    for (b = 0; b + FM1P_BLOCK <= frames; b += FM1P_BLOCK) {
        if (fm1nor_stall_take()) {
            memset(stereo + 2u * b, 0, FM1P_BLOCK * 2u * sizeof *stereo);
            continue;
        }
        for (f = b; f < b + FM1P_BLOCK; f += CTL) {
            mix_block(o, CTL);
            for (k = 0; k < 2u * CTL; k++) {
                int32_t v = o[k];
                if ((k & 3u) == 2u)
                    scope_buf[scope_w++ & (SCOPE_N - 1u)] = (int16_t)v;
                stereo[2u * f + k] = (int16_t)(v > 32767 ? 32767 : v < -32768 ? -32768 : v);
            }
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
    printf("  flash: %s, %u writes, %u erases with the audio stalled%s\n", flash_ok ? "ok" : "RAM only",
           (unsigned)fm1nor_dirty(), (unsigned)fm1nor_stalls(), persist_pending ? ", a settings save pending" : "");
}
void fm1core_ts_stats(uint32_t *shed, uint32_t *cpu_pct)
{
    *shed = 0;
    *cpu_pct = 0;                         /* (the CPU budget is not simulated) */
}
void fm1core_ts_ui_info(char *buf, uint32_t n)
{
    snprintf(buf, n, "%s page %u part %u menu %u playing %u slot %d msg '%s'", ui.home ? "home" : "page",
             (unsigned)ui.page, (unsigned)song.sel + 1u, (unsigned)ui.menu, (unsigned)song.playing,
             (int)song.g[G_SLOT], ui.msg_t ? ui.msg : "");
}
const int8_t *fm1core_ts_keymap(void) { return &FM1_KEYMAP[0][0]; }

/* ---- the flash (fm1core.h ABI 2): common/nor.c ---- */
uint8_t *fm1core_ts_flash(void) { return fm1nor_flash(); }
uint32_t fm1core_ts_flash_dirty(void) { return fm1nor_dirty(); }
int fm1core_ts_flash_stage(const uint8_t *bytes, uint32_t n) { return fm1nor_stage(bytes, n); }
/* make the image current: what settings_poll would save is written now if it differs from the flash. Projects and
 * user presets are written the moment they are saved (SAVE); the song being edited is RAM on the device as well.
 * 1: current; 0: the firmware defers the write (it never erases while the transport runs) */
int fm1core_ts_flash_sync(void)
{
    if (fm1core_halted() || !ts_booted || !flash_ok)
        return 1;
    persist_pending = 1;
    settings_poll();
    return persist_pending ? 0 : 1;
}
