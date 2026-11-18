/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Tessera's core for the plugin (tess_core.h): web/sim/sim.c, the whole firmware on the host tests' HAL stubs, as
 * the web simulator runs it, built natively. Audio time drives the device's clock, so an offline bounce plays the
 * same as real time. */
#include "sim.c"
#include "tess_core.h"

uint32_t tess_core_rate(void) { return FS; }

void tess_core_boot(void) { web_boot(); }

void tess_core_render(float *l, float *r, uint32_t n)
{
    if (n > 1024u)
        n = 1024u;
    web_render(n);
    memcpy(l, out_l, n * sizeof *l);
    memcpy(r, out_r, n * sizeof *r);
}

uint32_t tess_core_midi(uint32_t status, uint32_t d1, uint32_t d2) { return web_midi(status, d1, d2); }

void tess_core_transport(int play)
{
    if (play && !song.playing)
        transport_req = 1;
    else if (!play && song.playing)
        transport_req = 2;
}

uint32_t tess_core_playing(void) { return song.playing; }

uint32_t tess_core_tsp_max(void) { return TSP_MAX; }

/* as project_save_as, without the slot write or its messages */
uint32_t tess_core_save(uint8_t *out, uint32_t cap)
{
    project_t *p = &proj_scratch;
    uint32_t len;
    project_capture(p);
    proj_wire_gen++;
    len = tsp_pack(proj_wire_u.raw, p, 1);
    if (!len || len > cap)
        return 0;
    memcpy(out, proj_wire_u.raw, len);
    return len;
}

/* as project_load after proj_read_slot, from a buffer: the project and its runtime (patterns, generators, cords) */
uint32_t tess_core_load(const uint8_t *in, uint32_t len)
{
    if (len < 16u || len > TSP_MAX)
        return 1;
    proj_wire_gen++;
    memcpy(proj_wire_u.raw, in, len);
    if (!tsp_valid(proj_wire_u.raw, len))
        return 1;
    if (!project_restore_runtime(&proj_scratch))
        tsp_restore(proj_wire_u.raw);
    ui.force = 1;
    return 0;
}

/* ---- the panel */
void tess_core_buttons(uint32_t mask) { web_buttons(mask); }
void tess_core_keys(uint32_t mask) { web_keys(mask); }
void tess_core_enc(uint32_t role, int32_t n) { web_enc(role, n); }
void tess_core_master(uint32_t v) { web_master(v); }

void tess_core_view(tess_view_t *v)
{
    uint32_t i;
    const uint16_t *fb = web_screen();
    for (i = 0; i < TESS_SCREEN * TESS_SCREEN; i++)        /* the panel's byte order -> RGB565 */
        v->screen[i] = (uint16_t)(fb[i] >> 8 | fb[i] << 8);
    v->lit_buttons = web_lit_buttons();
    v->lit_keys = web_lit_keys();
    v->dim_buttons = web_dim_buttons();
    v->dim_keys = web_dim_keys();
    v->breath_buttons = web_breath_buttons();
    v->breath_keys = web_breath_keys();
    v->screen_draws = web_screen_draws();
    v->playing = song.playing;
    v->master = tess_core_master_get();
}

/* ---- the DAW (TESS_HOST_CLOCK: mix.c int_clock_units asks tess_host_units while the DAW plays) */
static double host_rate_u;                 /* units a core sample at the DAW's tempo */
static double host_given, host_expect;     /* units handed out since the lock; where the DAW should be by now */
static double host_frac;
static uint32_t tess_host_units(uint32_t n)
{
    double want = n * host_rate_u, err = host_expect - host_given;
    uint32_t u;
    host_expect += want;
    want += err * 0.02;                    /* (the gap to the DAW's own position: closed over ~50 blocks) */
    if (want < 0)
        want = 0;
    want += host_frac;
    u = (uint32_t)want;
    host_frac = want - u;
    host_given += u;
    return u;
}

static void host_locate(double units)
{
    seq_start();                           /* (as a MIDI Continue after a Song Position: midi_clock_seek) */
    if (units >= 1.0)
        seq_seek((uint32_t)units);
    host_given = host_expect = units;
    host_frac = 0;
}

void tess_core_host(int playing, int has_pos, double bpm, double beats)
{
    if (!playing) {                        /* (the plugin calls this on the DAW's STOP only, not every stopped block) */
        if (song.playing)
            transport_req = 2;
        tess_host_clock = 0;
        return;
    }
    if (!has_pos || song.g[G_CLOCK] || bpm < 20.0 || bpm > 400.0) {   /* (CLK on MIDI: its master's clock rules) */
        tess_host_clock = 0;
        if (!song.playing)
            transport_req = 1;
        return;
    }
    double units = (beats > 0 ? beats : 0) * MIDI_BEAT_UNITS;
    int bpm_i = (int)(bpm + 0.5);
    song.g[G_BPM] = (int16_t)(bpm_i < 40 ? 40 : bpm_i > 240 ? 240 : bpm_i);   /* (the display, and timings set in ms) */
    host_rate_u = bpm / 60.0 * MIDI_BEAT_UNITS / FS;
    if (!tess_host_clock || !song.playing) {
        tess_host_clock = 1;
        host_locate(units);
    } else if (units - host_expect > MIDI_BEAT_UNITS || host_expect - units > MIDI_BEAT_UNITS) {
        host_locate(units);                /* (a loop or a locate while playing) */
    } else {
        host_expect = units;
    }
}

uint32_t tess_core_macro(uint32_t k) { return tessera.macro[k & 3u]; }
void tess_core_macro_set(uint32_t k, uint32_t v) { tessera_param_set(0, TX_MACRO1 + (k & 3u), (int32_t)(v > 100u ? 100u : v)); }
uint32_t tess_core_master_get(void)
{
    uint32_t q = (uint32_t)song.master_q12 << 8, r = 0;   /* master_q12 = k10^2 >> 8 (web_master) */
    while ((r + 1u) * (r + 1u) <= q)
        r++;
    return r > 1023u ? 1023u : r;
}

double tess_core_position(void) { return (bar_n * 4.0 + beat_n) + (double)beat_pos / MIDI_BEAT_UNITS; }
uint32_t tess_core_bpm(void) { return (uint32_t)song.g[G_BPM]; }
