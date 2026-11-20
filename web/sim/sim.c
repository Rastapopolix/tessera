/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments; Theo Dainis (melodee-tess) */
/* Web simulator: the firmware (sound, sequencer, Tessera, UI) through Tessera's core API (platform/core: the shared
 * build on the host HAL), built to WebAssembly by web/sim/build.sh. It exports Felucca's browser interface
 * (web/emu/felucca_web.c of hugelton/Felucca: web_render, web_buttons, web_keys, web_enc, web_master, web_midi, the
 * LEDs, the screen), so Felucca's page and worklet (web/sim/index.html, worklet.js) drive it unchanged: the same FM-1
 * drawing as the other FM-1 browser builds. Audio time drives the clock (platform/core/tess_core.c).
 * Not simulated: flash (saves are lost on reload; web_flash_writes_count stays 0), USB, the CPU budget.
 * The plugin (platform/plugin) builds this file too, for the demos; its own interface is tess_core.h. */
static void web_demos_load(void);
#define TESS_CORE_BOOT_HOOK() web_demos_load()   /* (tess_core_boot: the demos in the slots, A loaded) */
#include "../../platform/core/core_unit.c"

#include "sim_demos.h"                       /* (web/sim/build.sh: demos/still-tide.tsp, koto-garden.tsp) */

/* the demos in the slots, as the backup page would put them there: A STILL TIDE (the first in the demo video),
 * B KOTO GARDEN; then A loaded, so the simulator starts with STILL TIDE (SAVE > PROJECT loads B) */
static void web_demos_load(void)
{
    memcpy(proj_bank_slot[0], SIM_DEMO_A, sizeof SIM_DEMO_A);
    proj_bank_len[0] = sizeof SIM_DEMO_A;
    memcpy(proj_bank_slot[1], SIM_DEMO_B, sizeof SIM_DEMO_B);
    proj_bank_len[1] = sizeof SIM_DEMO_B;
    proj_fetch(0);
    proj_fetch(1);
    project_load(0);
}

#ifdef __EMSCRIPTEN__
#define EXPORT __attribute__((used, visibility("default")))
#else
#define EXPORT                               /* (the plugin: its interface is tess_core.h, these stay inside) */
#endif

/* the page's shell picker (web/sim/index.html): the ink by name (the Mono look's COLOR, as MENU > COLOR sets it;
 * an unknown name, or the detailed look, leaves it). name: n bytes at web_ink_name(), not terminated */
static char web_ink_buf[16];
EXPORT char *web_ink_name(void) { return web_ink_buf; }
EXPORT uint32_t web_ink(uint32_t n)
{
#if MELODEE_LOOK
    uint32_t i;
    for (i = 0; i < NPALETTES && n < sizeof web_ink_buf; i++)
        if (strlen(UI_PALETTES[i].name) == n && !memcmp(UI_PALETTES[i].name, web_ink_buf, n)) {
            settings.palette = i;
            palette_set(i);
            ui.force = 1;
            return 1;
        }
#else
    (void)n;
#endif
    return 0;
}

EXPORT float *web_out_l(void) { return out_l; }
EXPORT float *web_out_r(void) { return out_r; }
/* n frames (<= 1024) into out_l / out_r: the device runs until they exist */
EXPORT void web_render(uint32_t n) { core_render(n > 1024u ? 1024u : n); }

/* no flash here: the page's saved sectors are ignored, nothing is ever written */
static uint8_t nor[4096];
EXPORT uint8_t *web_nor(void) { return nor; }
EXPORT uint32_t web_nor_size(void) { return 0; }
EXPORT void web_nor_erase(void) { memset(nor, 0xFF, sizeof nor); }
EXPORT uint32_t web_flash_writes_count(void) { return 0; }
EXPORT void web_boot(void) { tess_core_boot(); }

/* input, by the printed labels: buttons bit b = B_FX .. B_OCTUP (panel.c), keys bit k = key k (F3 .. G5),
 * encoders by role (EN_SELECT .. EN_K4), MASTER 0..1023 (main.c: the pot, squared) */
EXPORT void web_buttons(uint32_t labels) { tess_core_buttons(labels); }
EXPORT void web_keys(uint32_t mask) { tess_core_keys(mask); }
EXPORT void web_enc(uint32_t role, int32_t n) { tess_core_enc(role, n); }
EXPORT void web_master(uint32_t v) { tess_core_master(v); }
/* a channel message from Web MIDI, as a USB-MIDI packet arrives: 0 = the queue was full */
EXPORT uint32_t web_midi(uint32_t status, uint32_t d1, uint32_t d2) { return tess_core_midi(status, d1, d2); }

/* the screen reader (announce.c, MELODEE_ACCESS): the page is its reader. web_reader(1) is the reader's HELLO
 * (again at least every 5 s, or the firmware stops talking), web_reader(0) its BYE; what the firmware says collects
 * in host_said (host_hal.h: a line each, "N text" cuts off what is being said, "Q text" queued, "K text" kept whole,
 * "H name|words link" the HELLO back), read with web_said and emptied with web_said_clear */
#if MELODEE_ACCESS
EXPORT void web_reader(uint32_t on)
{
    static const uint8_t hello[5] = { ANN_H0, ANN_H1, ANN_H2, ANN_HELLO, ANN_VER }, bye[4] = { ANN_H0, ANN_H1, ANN_H2, ANN_BYE };
    ann_frame(on ? hello : bye, on ? 5u : 4u, fm1_ms);
}
EXPORT char *web_said(void) { return host_said; }
EXPORT uint32_t web_said_n(void) { return host_said_n; }
EXPORT void web_said_clear(void) { host_said_n = 0; host_said[0] = 0; }
#endif

/* the LEDs: bit b = the button labelled b, keys bit k (tess_core.c) */
EXPORT uint32_t web_lit_buttons(void) { return led_buttons(fm1_led); }
EXPORT uint32_t web_lit_keys(void) { return led_keys(fm1_led); }
EXPORT uint32_t web_dim_buttons(void) { return led_buttons(led_dim_plane()); }
EXPORT uint32_t web_dim_keys(void) { return led_keys(led_dim_plane()); }
EXPORT uint32_t web_dim_level(void) { return 0; }
EXPORT uint32_t web_breath_buttons(void) { return led_buttons(fm1_led_dim[2]); }
EXPORT uint32_t web_breath_keys(void) { return led_keys(fm1_led_dim[2]); }

EXPORT uint16_t *web_screen(void) { return host_screen; }        /* 240 x 240 RGB565, the panel's byte order */
EXPORT uint32_t web_screen_draws(void) { return (uint32_t)host_sent; }
EXPORT uint32_t web_now_ms(void) { return fm1_ms; }
EXPORT uint32_t web_playing(void) { return song.playing; }
EXPORT uint32_t web_sample_rate(void) { return FS; }
/* (web/sim/test_sim.mjs) track tr's MODE (tessera.mode) */
EXPORT uint32_t web_test_mode(uint32_t tr) { return tr < TESSERA_TRACKS ? tessera.mode[tr] : 0u; }

/* the demo button: STILL TIDE (slot A) loaded again, from its start, and PLAY */
EXPORT void web_demo(void)
{
    if (song.playing) {
        transport_req = 2;
        return;                              /* (a second press stops it) */
    }
    project_load(0);
    ui.force = 1;
    transport_req = 1;
}

/* project files (the page's Projects row): a slot's TSP1 record out as a .tsp file, or a .tsp file into a slot, the
 * same record the FM-1 keeps (the web editor's Projects > Project files moves it on and off the device). The slot list
 * shows the name at once; SAVE > PROJECT loads it, as on the device. web_slot_buf: the record's bytes, both ways. */
static uint8_t web_slot_raw[TSP_MAX];
EXPORT uint8_t *web_slot_buf(void) { return web_slot_raw; }
EXPORT uint32_t web_slot_buf_size(void) { return TSP_MAX; }
/* slot 0..3 (A..D) -> web_slot_buf: the record's length, 0 = empty */
EXPORT uint32_t web_slot_get(uint32_t slot) { return tess_core_slot_get(slot, web_slot_raw, sizeof web_slot_raw); }
/* web_slot_buf's first len bytes -> slot 0..3: 0 done, 1 not a whole TSP1 record this firmware reads */
EXPORT uint32_t web_slot_put(uint32_t slot, uint32_t len) { return tess_core_slot_put(slot, web_slot_raw, len); }
