/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments; Theo Dainis (melodee-tess) */
/* Tessera's core API (tess_core.h) over the shared build: included last by platform/core/core_unit.c. The driver
 * (audio time drives the clock: each millisecond of the device renders the audio due and runs main.c's loop, a frame
 * every 15 ms, ui_input between) came from web/sim/sim.c, the plugin's parts from platform/plugin/tess_core.c. */
#include "tess_core.h"

#ifndef TESS_CORE_UI
#define TESS_CORE_UI 1                            /* the FM-1's UI runs (its input, LEDs and frames) */
#endif
#ifndef TESS_CORE_BOOT_HOOK
#define TESS_CORE_BOOT_HOOK() ((void)0)
#endif

_Static_assert(TESS_CORE_BLOCK == CTL, "tess_core.h: TESS_CORE_BLOCK is the control block");
_Static_assert(TESS_NB == NB && TESS_NE == NE, "tess_core.h: the FM-1's buttons and encoders");

/* the real matrix (hal/fm1_input.h; host_hal.h leaves it empty): the LED places of the keys and buttons */
static const int8_t FM1_KEYMAP[6][FM1_NCOL] = {
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    { 5, 11,  4, 10,  3,  9,  2,  8, -1, -1, -1},
    {34, 35, 36, 37, 38, 40, 39, 13,  7,  6, 12},
    {23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33},
    { 0,  1, 15, 14, 17, 16, 19, 18, 20, 21, 22},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
};

uint32_t tess_core_rate(void) { return FS; }
uint32_t tess_core_tracks(void) { return NTRK; }

static uint32_t core_booted, core_last_frame, core_samples_due;
static uint32_t core_track_req = 0xFFu;           /* tess_core_track_select: the track asked for (0xFF none) */

/* power-on (main.c melodee_init via host_boot.h's ui_power_on, with the device's BPM and scale), in the default ink */
void tess_core_boot(void)
{
    uint32_t i;
    if (core_booted)
        return;
    dsp_tables_init();                            /* the RAM tables (fm1_main on the FM-1) */
    ui_power_on();
    tessera_ensure_init();                        /* (the generators' settings now, not at the first step: a host reads them first) */
    TRK_DEF_SONG();                               /* (ui_power_on: the tests' start; main.c melodee_init: the device's) */
    for (i = 0; i < NTRK; i++)
        if (TRK_DEF[i][0] != ENGI_DRUM)
            TRK_DEF_SCALE_OF(&trk[i]);
    TESS_CORE_BOOT_HOOK();                        /* (the simulator: its demos in the slots, A loaded) */
    settings.palette = UI_DEFAULT_INDEX;          /* (ui_power_on: the tests' GRAY) */
    palette_set(settings.palette);
    song.master_q12 = 2048;                       /* until the front end sends MASTER */
    screen_clear();
    ui.force = 1;
    core_last_frame = fm1_ms - 15u;
    core_booted = 1;
#ifdef TESS_PLUGIN_IO
    settings_midi_out = MIDI_OUT_ALL;             /* (a DAW wants the generators' notes too; MENU can change it) */
#endif
}

/* one pass of main.c's loop: what a host has (no USB, editor, UBOOT, flash) */
static void core_frame(void)
{
    if (core_track_req < NTRK) {                  /* (between passes, as KNOB 1 on TRACKS would) */
        track_select(core_track_req);
        core_track_req = 0xFFu;
    }
    ui_input();
    ui_leds();
    ui_draw();
#ifdef ANN_VER
    ui_speak(fm1_ms);                             /* the screen reader, as main.c (silent until a reader's HELLO: the
                                                   * web simulator's page is one, web/sim/sim.c web_reader) */
#endif
}

/* the FM-1's millisecond: its clock, and main.c's loop (a frame every 15 ms, ui_input between) */
static void core_tick_ms(void)
{
    fm1_ms++;
    host_ticks += 1000u;
#if TESS_CORE_UI
    if (!core_booted)
        return;
    if (fm1_ms - core_last_frame >= 15u) {
        core_last_frame = fm1_ms;
        core_frame();
    } else {
        ui_input();
    }
#endif
}

/* ---- audio out: an output ring, stereo float, filled a block of CTL at a time as the device's milliseconds pass,
 * drained by tess_core_render. The HOME scope is fed as audio.c audio_block feeds it. */
#define RING 4096u
static float ring[RING * 2];
static uint32_t ring_w, ring_r;

#ifdef TESS_PLUGIN_IO
static void tess_plugin_block(uint32_t frame);
#endif
static void core_ms(void)
{
    uint32_t k;
    core_samples_due += FS;
    fm1_ms++;
    host_ticks += 1000u;
    while (core_samples_due >= 1000u * CTL) {
        static int32_t o[2u * CTL];
        core_samples_due -= 1000u * CTL;
        mix_block(o, CTL);
        for (k = 0; k < CTL; k++) {
            uint32_t i = (ring_w++ % RING) * 2u;
            if (k & 1u)
                scope_buf[scope_w++ & (SCOPE_N - 1u)] = (int16_t)o[2u * k];
            ring[i] = (float)o[2u * k] * (1.0f / 32768.0f);
            ring[i + 1u] = (float)o[2u * k + 1u] * (1.0f / 32768.0f);
        }
#ifdef TESS_PLUGIN_IO
        tess_plugin_block(ring_w - CTL);          /* (the plugin's track outputs) */
#endif
    }
#if TESS_CORE_UI
    if (!core_booted)
        return;
    if (fm1_ms - core_last_frame >= 15u) {        /* main.c: a frame, then ui_input until 15 ms are over */
        core_last_frame = fm1_ms;
        core_frame();
    } else {
        ui_input();
    }
#endif
}

static float out_l[1024], out_r[1024];       /* (the last render: web/sim/sim.c hands them to the page) */
static void core_render(uint32_t n)
{
    uint32_t k;
    while (ring_w - ring_r < n)
        core_ms();
    for (k = 0; k < n; k++) {
        uint32_t i = (ring_r++ % RING) * 2u;
        out_l[k] = ring[i];
        out_r[k] = ring[i + 1u];
    }
}

#ifdef TESS_PLUGIN_IO
/* the stems beside the ring, frame for frame (tess_plugin_block: after each mix_block) */
static float stem_ring[RING][NTRK];
static void tess_plugin_block(uint32_t frame)
{
    uint32_t k, t;
    if (!tess_plugin_stems)
        return;
    for (k = 0; k < CTL; k++)
        for (t = 0; t < NTRK; t++)
            stem_ring[(frame + k) % RING][t] = (float)track_capture[k * NTRK + t] * (1.0f / 32768.0f);
}

/* MIDI OUT: a ring of packets, each with the frame being made when it was sent (usb.c) */
#define MO_RING 512u
static uint32_t mo_pkt[MO_RING], mo_frame[MO_RING], mo_wr, mo_rd;
static void tess_plugin_midi_out(uint32_t pkt)
{
    if (mo_wr - mo_rd >= MO_RING)
        return;                                   /* (full: as the USB queue, dropped) */
    mo_pkt[mo_wr % MO_RING] = pkt;
    mo_frame[mo_wr % MO_RING] = ring_w;
    mo_wr++;
}
uint32_t tess_core_midi_out(uint32_t *pkt, uint32_t *frame)
{
    if (mo_rd == mo_wr || (int32_t)(mo_frame[mo_rd % MO_RING] - ring_r) >= 0)
        return 0;
    *pkt = mo_pkt[mo_rd % MO_RING];
    *frame = mo_frame[mo_rd % MO_RING];
    mo_rd++;
    return 1;
}
#else
uint32_t tess_core_midi_out(uint32_t *pkt, uint32_t *frame)
{
    (void)pkt;
    (void)frame;
    return 0;
}
#endif

void tess_core_render(float *l, float *r, float *const *stems, uint32_t n)
{
    uint32_t r0 = ring_r, k, t;
    if (n > 1024u)
        n = 1024u;
#ifdef TESS_PLUGIN_IO
    if (stems)
        tess_plugin_stems = 1;
#endif
    core_render(n);
    memcpy(l, out_l, n * sizeof *l);
    memcpy(r, out_r, n * sizeof *r);
    if (stems)
        for (k = 0; k < n; k++)
            for (t = 0; t < NTRK; t++)
#ifdef TESS_PLUGIN_IO
                stems[t][k] = stem_ring[(r0 + k) % RING][t];
#else
                stems[t][k] = 0.0f;
#endif
    (void)r0;
}

/* the fixed-block path: the clock follows the samples, a millisecond at a time, as core_ms does */
void tess_core_block(int32_t *out)
{
    uint32_t k;
    mix_block(out, CTL);
    for (k = 1; k < CTL; k += 2u)
        scope_buf[scope_w++ & (SCOPE_N - 1u)] = (int16_t)out[2u * k];
    ring_w += CTL;
    ring_r = ring_w;                              /* (nothing waits in the ring: tess_core_frames counts these) */
    core_samples_due += CTL * 1000u;
    while (core_samples_due >= FS) {
        core_samples_due -= FS;
        core_tick_ms();
    }
}

uint32_t tess_core_frames(void) { return ring_r; }

/* ---- events in: by the printed labels (panel.c maps them to the matrix), as the device's scan would see them */
void tess_core_buttons(uint32_t labels)
{
    uint32_t m = 0, b;
    for (b = 0; b < NB; b++)
        if ((labels >> b) & 1u)
            m |= 1u << panel.btn[b];
    host_pressed |= m & ~fm1_in.buttons;
    fm1_in.buttons = m;
}
void tess_core_keys(uint32_t mask)
{
    mask &= (1u << TESS_KEYS) - 1u;
    host_notes |= mask & ~fm1_in.notes;
    fm1_in.notes = mask;
}
void tess_core_enc(uint32_t role, int32_t n)
{
    if (role < NE)
        host_enc[panel.enc[role] % 7u] += n * panel.dir[role];
}
void tess_core_master(uint32_t v)
{
    uint32_t k10 = v > 1023u ? 1023u : v;
    song.master_q12 = (int32_t)((k10 * k10) >> 8);   /* (main.c: the pot, squared) */
}
uint32_t tess_core_master_get(void)
{
    uint32_t q = (uint32_t)song.master_q12 << 8, r = 0;   /* master_q12 = k10^2 >> 8 (tess_core_master) */
    while ((r + 1u) * (r + 1u) <= q)
        r++;
    return r > 1023u ? 1023u : r;
}
/* a channel message, as a USB-MIDI packet arrives */
uint32_t tess_core_midi(uint32_t status, uint32_t d1, uint32_t d2)
{
    if (status < 0x80u || status >= 0xF0u)
        return 1;
    if (mi_w - mi_r >= MQ)
        return 0;
    midi_in_q[mi_w % MQ] = (status >> 4) | status << 8 | (d1 & 127u) << 16 | (d2 & 127u) << 24;
    mi_w++;
    return 1;
}
void tess_core_transport(int play)
{
    if (play && !song.playing)
        transport_req = 1;
    else if (!play && song.playing)
        transport_req = 2;
}
uint32_t tess_core_playing(void) { return song.playing; }

/* ---- the screen and the LEDs: bit b = the button labelled b, keys bit k. Lit: fm1_led; dim: the idle-key and
 * idle-button planes; breath: the third plane (ui_input.c ui_leds) */
void tess_core_screen(tess_screen_t *s)
{
    s->w = s->h = TESS_SCREEN;
    s->format = TESS_PIX_RGB565;
    s->draws = (uint32_t)host_sent;
#if HOST_HAL_SCREEN
    s->pixels = host_screen;                      /* (the panel's byte order: tess_core_view swaps it) */
#else
    s->pixels = 0;
#endif
}
static uint32_t led_on(const uint8_t *l, uint32_t id)
{
    uint32_t c, r;
    for (c = 0; c < FM1_NCOL; c++)
        for (r = 1; r < 5u; r++)
            if (FM1_KEYMAP[r][c] == (int8_t)id)
                return (l[c] >> r) & 1u;
    return 0;
}
static uint32_t led_buttons(const uint8_t *l)
{
    uint32_t b, m = 0;
    for (b = 0; b < NB; b++)
        m |= led_on(l, panel.btn[b]) << b;
    return m;
}
static uint32_t led_keys(const uint8_t *l)
{
    uint32_t k, m = 0;
    for (k = 0; k < TESS_KEYS; k++)
        m |= led_on(l, 14u + k) << k;
    return m;
}
static uint8_t led_dim[FM1_NCOL];
static const uint8_t *led_dim_plane(void)
{
    uint32_t c;
    for (c = 0; c < FM1_NCOL; c++)
        led_dim[c] = (uint8_t)((fm1_led_dim[0][c] | fm1_led_dim[1][c]) & ~fm1_led[c]);
    return led_dim;
}
void tess_core_view(tess_view_t *v)
{
    uint32_t i;
#if HOST_HAL_SCREEN
    for (i = 0; i < TESS_SCREEN * TESS_SCREEN; i++)        /* the panel's byte order -> RGB565 */
        v->screen[i] = (uint16_t)(host_screen[i] >> 8 | host_screen[i] << 8);
#else
    for (i = 0; i < TESS_SCREEN * TESS_SCREEN; i++)
        v->screen[i] = 0;
#endif
    v->lit_buttons = led_buttons(fm1_led);
    v->lit_keys = led_keys(fm1_led);
    v->dim_buttons = led_buttons(led_dim_plane());
    v->dim_keys = led_keys(led_dim_plane());
    v->breath_buttons = led_buttons(fm1_led_dim[2]);
    v->breath_keys = led_keys(fm1_led_dim[2]);
    v->screen_draws = (uint32_t)host_sent;
    v->playing = song.playing;
    v->master = tess_core_master_get();
    v->track = song.sel;
}

uint32_t tess_core_track(void) { return song.sel; }
void tess_core_track_select(uint32_t t)
{
    if (t < NTRK)
        core_track_req = t;
}

/* ---- the project */
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

uint32_t tess_core_slot_get(uint32_t slot, uint8_t *out, uint32_t cap)
{
    uint32_t n;
    if (slot > 3u || !project_used(slot))
        return 0;
    n = proj_bank_len[slot];
    if (n > cap)
        return 0;
    memcpy(out, proj_bank_slot[slot], n);
    return n;
}
uint32_t tess_core_slot_put(uint32_t slot, const uint8_t *in, uint32_t len)
{
    if (slot > 3u || len < 16u || len > TSP_MAX || !tsp_valid(in, len))
        return 1;
    memcpy(proj_bank_slot[slot], in, len);
    proj_bank_len[slot] = len;
    proj_fetch(slot);                             /* (its name in the slot list) */
    ui.force = 1;
    return 0;
}
void tess_core_slot_load(uint32_t slot)
{
    if (slot > 3u)
        return;
    project_load(slot);
    ui.force = 1;
}

/* ---- parameters by central ID (pid.c) */
uint32_t tess_core_param_count(void) { return pid_count(); }
uint32_t tess_core_param_info(uint32_t pid, uint32_t track, tess_param_t *info)
{
    const pid_entry_t *e = pid_find(pid);
    const param_desc_t *d;
    if (!e || (e->flags & PID_F_ACTION))
        return 0;
    d = pid_desc(pid, track);
    if (!d)
        return 0;
    info->name = e->name;
    info->min = d->min;
    info->max = d->max;
    info->def = d->def;
    info->per_track = (e->flags & PID_F_TRACK) ? 1u : 0u;
    info->settable = !(e->flags & PID_F_DEVICE) && (pid >= PID_TESS(0) || d->max > d->min);   /* (as pid_set) */
    info->span_min = d->min;
    info->span_max = d->max;
    if (pid >= P_E0 && pid <= P_E7) {             /* an engine's knob: its range over every engine */
        uint32_t k;
        for (k = 0; k < NENGINES; k++)
            if (ENGINES[k] && ENGINES[k]->edit) {
                const param_desc_t *x = &ENGINES[k]->edit[pid - P_E0];
                if (x->max > x->min) {
                    info->span_min = x->min < info->span_min ? x->min : info->span_min;
                    info->span_max = x->max > info->span_max ? x->max : info->span_max;
                }
            }
    }
    return 1;
}
uint32_t tess_core_param_at(uint32_t n, uint32_t track, tess_param_t *info)
{
    const pid_entry_t *e = pid_at(n);
    if (!e)
        return PID_NONE;
    if (!tess_core_param_info(e->pid, track, info)) {
        info->name = e->name;
        info->min = info->max = info->def = 0;
        info->per_track = (e->flags & PID_F_TRACK) ? 1u : 0u;
        info->settable = 0;
        info->span_min = info->span_max = 0;
    }
    return e->pid;
}
int32_t tess_core_param_get(uint32_t pid, uint32_t track) { return pid_get(pid, track); }
/* as a project load would keep it, so a host's value saves and loads back the same: SCL (and a linked QNT, MPC degree,
 * ROOT) on every track as the UI's knob does (ui.c scale_share), a QUAD LINK without its own track (tessera.c) */
uint32_t tess_core_param_set(uint32_t pid, uint32_t track, int32_t v)
{
    track %= NTRK;
    if (pid == PID_TESS(TX_QD_LINK))
        v &= ~(int32_t)(1u << track);
    if (!pid_set(pid, track, v))
        return 0;
    if (pid < PID_GLOBAL(0) && scale_shared(pid))
        scale_share(&trk[track]);
    return 1;
}
uint32_t tess_core_param_text(uint32_t pid, uint32_t track, int32_t v, char *out, uint32_t cap)
{
    const param_desc_t *d = pid_find(pid) ? pid_desc(pid, track) : 0;
    char val[24];
    const char *unit;
    uint32_t n = 0, i;
    if (!d || !cap)
        return 0;
    if (d->fmt == F_ENUM && d->names && (v < d->min || v > d->max))
        v = clamp(v, d->min, d->max);             /* (a name's index: never past the list) */
    param_format(d, v, val, &unit);
    for (i = 0; val[i] && n + 1u < cap; i++)
        out[n++] = val[i];
    for (i = 0; unit[i] && n + 1u < cap; i++)
        out[n++] = unit[i];
    out[n] = 0;
    return n;
}

/* ---- the Mono look's ink (MENU > COLOR; the plugin's shell picker, as web/sim/sim.c web_ink) */
uint32_t tess_core_ink(const char *name)
{
#if MELODEE_LOOK
    uint32_t i;
    for (i = 0; name && i < NPALETTES; i++)
        if (!strcmp(UI_PALETTES[i].name, name)) {
            settings.palette = i;
            palette_set(i);
            ui.force = 1;
            return 1;
        }
#else
    (void)name;
#endif
    return 0;
}

/* ---- the DAW (TESS_HOST_CLOCK: mix.c int_clock_units asks tess_host_units while the DAW plays) */
#ifdef TESS_HOST_CLOCK
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
#else
void tess_core_host(int playing, int has_pos, double bpm, double beats)
{
    (void)has_pos;
    (void)bpm;
    (void)beats;
    tess_core_transport(playing);          /* (no DAW clock in this build: start and stop only) */
}
#endif

uint32_t tess_core_macro(uint32_t k) { return tessera.macro[k & 3u]; }
void tess_core_macro_set(uint32_t k, uint32_t v) { tessera_param_set(0, TX_MACRO1 + (k & 3u), (int32_t)(v > 100u ? 100u : v)); }
double tess_core_position(void) { return (bar_n * 4.0 + beat_n) + (double)beat_pos / MIDI_BEAT_UNITS; }
uint32_t tess_core_bpm(void) { return (uint32_t)song.g[G_BPM]; }
