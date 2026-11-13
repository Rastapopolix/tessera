/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Andamento in the browser: Tessera's core at 48 kHz (the Daisy Seed's rate) with Andamento's first panel UI
 * (platform/andamento/and_ui.c) in place of the FM-1's, built to WebAssembly by web/andamento/build.sh. The firmware
 * runs on tests/ui_test.c's stubs, as web/sim/sim.c does; the FM-1's own UI is never run: the page draws the
 * 128 x 64 screen and turns Andamento's controls into and_ui.c's calls. Audio time drives the clock, as in web/sim. */
#define UI_TEST_NO_MAIN 1
#define TRK_START_TESS 1
#include "ui_test.c"
#include "../../firmware/src/mo_font.h"
#include "../../platform/andamento/and_fb.c"           /* the screen: 128 x 64, 16 levels */
#include "../../platform/andamento/and_ui.c"

#include "sim_demos.h"                       /* (web/andamento/build.sh: demos/still-tide.tsp, koto-garden.tsp) */

#undef __attribute__
#define EXPORT __attribute__((used, visibility("default")))

static uint32_t web_booted;

/* power-on as web/sim: the device's song, the two demos in slots A and B, A loaded (stopped) */
static void web_power_on(void)
{
    uint32_t i;
    ui_power_on();
    TRK_DEF_SONG();
    for (i = 0; i < NTRK; i++)
        if (TRK_DEF[i][0] != ENGI_DRUM)
            TRK_DEF_SCALE_OF(&trk[i]);
    memcpy(proj_bank_slot[0], SIM_DEMO_A, sizeof SIM_DEMO_A);
    proj_bank_len[0] = sizeof SIM_DEMO_A;
    memcpy(proj_bank_slot[1], SIM_DEMO_B, sizeof SIM_DEMO_B);
    proj_bank_len[1] = sizeof SIM_DEMO_B;
    proj_fetch(0);
    proj_fetch(1);
    project_load(0);
    song.master_q12 = 2048;
    and_ext_init();                              /* tracks 5..8 (and_ext.c) */
    web_booted = 1;
}
EXPORT void web_boot(void)
{
    if (!web_booted)
        web_power_on();
}

/* output ring, stereo float, filled a block of CTL at a time as the milliseconds pass */
#define RING 4096u
static float ring[RING * 2];
static uint32_t ring_w, ring_r, web_samples_due;

static void web_ms(void)
{
    fm1_ms++;
    host_ticks += 1000u;
    web_samples_due += FS;
    while (web_samples_due >= 1000u * CTL) {
        static int32_t o[2u * CTL];
        uint32_t k;
        web_samples_due -= 1000u * CTL;
        mix_block(o, CTL);
        and_master_block(o, CTL);                /* (the master bus: tilt EQ then compressor, and_master.c) */
        and_ext_block();                         /* (tracks 5..8: their steps on the clock mix_block moved on) */
        for (k = 0; k < CTL; k++) {
            uint32_t i = (ring_w++ % RING) * 2u;
            ring[i] = (float)o[2u * k] * (1.0f / 32768.0f);
            ring[i + 1u] = (float)o[2u * k + 1u] * (1.0f / 32768.0f);
        }
    }
}

static float out_l[1024], out_r[1024];
EXPORT float *web_out_l(void) { return out_l; }
EXPORT float *web_out_r(void) { return out_r; }
EXPORT void web_render(uint32_t n)
{
    uint32_t k;
    if (n > 1024u)
        n = 1024u;
    while (ring_w - ring_r < n)
        web_ms();
    for (k = 0; k < n; k++) {
        uint32_t i = (ring_r++ % RING) * 2u;
        out_l[k] = ring[i];
        out_r[k] = ring[i + 1u];
    }
}

/* the panel: encoders 0..7 the grid, 8 ENERGY; buttons AB_*; keys 0..15 (bottom row 0..7); VOL 0..1023 */
EXPORT void web_enc(uint32_t i, int32_t n) { and_enc(i, n); }
EXPORT void web_push(uint32_t i) { and_enc_push(i); }
EXPORT void web_button(uint32_t b, uint32_t down) { and_button(b, down); }
EXPORT void web_key(uint32_t k, uint32_t down) { and_key(k, down); }
EXPORT void web_volume(uint32_t v)
{
    uint32_t k10 = v > 1023u ? 1023u : v;
    song.master_q12 = (int32_t)((k10 * k10) >> 8);
}
/* the screen: and_draw into and_fb (128 x 64, a byte a pixel, 0..15) */
EXPORT uint8_t *web_screen(void)
{
    and_draw();
    return &and_fb[0][0];
}
EXPORT uint32_t web_led_buttons(void) { return and_led_buttons(); }
/* the keys' lights: 16 x RGB (bottom row first), 0..255 a channel */
EXPORT const uint8_t *web_led_keys(void) { return and_led_keys(); }
/* tracks 5..8's MIDI since the last call: up to 64 messages of 3 bytes at web_midi_buf(), web_midi_take() of them */
static uint8_t web_midi[64 * 3];
EXPORT uint8_t *web_midi_buf(void) { return web_midi; }
EXPORT uint32_t web_midi_take(void) { return and_ext_midi_take(web_midi, 64); }
/* the jack strip: pair i's (0..3: tracks 5..8) CV in mV, and its gate */
EXPORT int32_t web_cv(uint32_t i) { return and_ext_cv(i); }
EXPORT uint32_t web_gate(uint32_t i) { return (uint32_t)and_ext_gate(i); }
EXPORT uint32_t web_sample_rate(void) { return FS; }
EXPORT uint32_t web_now_ms(void) { return fm1_ms; }
EXPORT uint32_t web_playing(void) { return song.playing; }
/* the demo: slot A (STILL TIDE) or B (KOTO GARDEN) loaded from its start and played */
EXPORT void web_demo(uint32_t slot)
{
    transport_req = 2;
    project_load(slot & 1u);
    transport_req = 1;
}
