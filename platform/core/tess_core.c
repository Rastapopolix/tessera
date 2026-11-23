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

/* every track its seed now (a new project's tracks 1..4 have none until first used; tessera_reset gives 5..8 theirs):
 * EVOLVE ROLL in its range 1..128, not 0, for a DAW that reads the start's values as the parameters' defaults */
static void core_seeds(void)
{
    uint32_t k;
    for (k = 0; k < TESSERA_TRACKS; k++)
        tess_seed_fresh(k);
#if MELODEE_FM6
    {                                             /* a track not on FM6 keeps FM6's INIT VOICE in its record, not the
                                                   * core's zeros: their name would come back as spaces after a load,
                                                   * and a state saved, loaded and saved again must be the same bytes
                                                   * (as generate.c gen_patches_reset does for a piece) */
        uint8_t v[FP_SIZE + 1u];
        for (k = 0; k < PROJ_NTRK; k++)
            if (trk[k].eng_req != ENGI_FM6) {
                fm6_unpack(FM6_INIT, v);
                fm6_put_patch(k, v, 1);
            }
    }
#endif
}

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
    song.g[G_ROUTE] = 1;                          /* ROUT SEL: a DAW sends on channel 1, and the notes play the selected
                                                   * track, as the panel's keys do (GLO > SYSTEM can set the ranges) */
    core_seeds();                                 /* (the plugin starts on a new project: tess_core_project_new's) */
#endif
}

/* ---- the screen's zones (TESS_HAS_POINTER: ui_zones.c): what a front end's mouse asks of them, done through the
 * knobs and keys. A queue, a step a millisecond (the UI reads the knobs every one): a list's rows one detent each, as
 * a new turn (no acceleration: three rows are three, however quick), a key pressed 40 ms then let go; a turn's
 * detents as the hand's (a quick wheel speeds up as a quick knob does) */
#if TESS_HAS_POINTER
_Static_assert(TESS_ZONES == UZ_MAX, "tess_core.h TESS_ZONES is ui_zones.c's UZ_MAX");
enum { ZQ_STEP = 1, ZQ_DETENT, ZQ_PRESS, ZQ_RELEASE, ZQ_PAGE };
#define ZQ_N 256u
static uint32_t zq[ZQ_N], zq_r, zq_w, zq_wait, zq_rows;   /* zq_rows: a list's row moves still to run */
static uint32_t zone_btn, host_btn;                   /* held by a zone's click, and by the front end (physical bits) */
static void zq_put(uint32_t op, uint32_t arg, int32_t sign)
{
    if (zq_w - zq_r < ZQ_N)
        zq[zq_w++ % ZQ_N] = op | (arg & 255u) << 8 | (sign < 0 ? 1u : 0u) << 16;
}
static void zq_turn(int32_t role, int32_t n)          /* n detents of a knob (at most 64 at once) */
{
    int32_t k, a = n < 0 ? -n : n;
    if (role < 0 || role >= (int32_t)NE)
        return;
    for (k = 0; k < a && k < 64; k++)
        zq_put(ZQ_DETENT, (uint32_t)role, n);
}
static void zq_run(void)
{
    uint32_t q, op, arg, m;
    int32_t sg;
    if (zq_wait) {
        zq_wait--;
        return;
    }
    if (zq_r == zq_w)
        return;
    q = zq[zq_r++ % ZQ_N];
    op = q & 255u, arg = (q >> 8) & 255u, sg = (q >> 16) & 1u ? -1 : 1;
    switch (op) {
    case ZQ_STEP:                                     /* a row: one detent, a turn of its own */
        ui.enc_t[arg % NE] = 0;
        tess_core_enc(arg, sg);
        if (zq_rows && !--zq_rows)
            uz_list_sel_pending = -1;                 /* (the list's cursor is its own again when next drawn) */
        break;
    case ZQ_DETENT:
        tess_core_enc(arg, sg);
        break;
    case ZQ_PAGE:                                     /* a footer tab: the family's next page that way (SELECT's) */
        page_scroll(sg);
        ui.force = 1;
        break;
    case ZQ_PRESS:
        m = 1u << panel.btn[arg % NB];
        host_pressed |= m & ~fm1_in.buttons;
        zone_btn |= m;
        fm1_in.buttons |= m;
        zq_wait = 40u;
        break;
    case ZQ_RELEASE:
        m = 1u << panel.btn[arg % NB];
        zone_btn &= ~m;
        fm1_in.buttons = host_btn | zone_btn;
        zq_wait = 20u;
        break;
    }
}
static void zq_key(uint32_t b)                        /* a key tapped */
{
    zq_put(ZQ_PRESS, b, 1);
    zq_put(ZQ_RELEASE, b, 1);
}
/* KNOB c+1's value back to its default (a double-click on its column): a page's own parameter only */
static void zone_reset(int32_t role)
{
    int16_t *vp = 0;
    const param_desc_t *d;
    uint32_t c = (uint32_t)(role - EN_K1);
    if (role < EN_K1 || role > EN_K4 || ui.home || ui.menu || ui.confirm || pop.on || ui.layer)
        return;
    d = page_desc(cur_page(), c, &vp);
    if (d && vp && d->max > d->min && *vp != d->def)
        edit_param(c, (int32_t)d->def - *vp);
}
static int zone_live(const tess_zone_t *z)          /* the zone is still on the screen */
{
    uint32_t i;
    for (i = 0; i < uz_n; i++)
        if (uz[i].kind == z->kind && uz[i].role == z->role && uz[i].vrole == z->vrole && uz[i].row == z->row)
            return 1;
    return 0;
}
#endif
static void core_ui_input(void)
{
#if TESS_HAS_POINTER
    zq_run();
#endif
    ui_input();
}

#if TESS_HAS_POINTER || (TESS_SCR_COUNT > 1 && MELODEE_LOOK)
/* track tr's M (GLO's black key: MENU MUTES' now, bar or phrase) or S (its solo latched, or let go), as TRACKS' and
 * MIXER's zones click them */
static void core_mute_solo(uint32_t kind, uint32_t tr)
{
    if (kind == TESS_ZONE_MUTE) {
        mute_press(tr, 0);
    } else {
        solo_latch ^= (uint8_t)(1u << tr);
        track_solo = (uint8_t)(track_solo ^ (1u << tr));   /* (at once: layer_keys keeps it from the next pass) */
    }
    ui.force = 1;
}
#endif
void tess_core_zone_do(const tess_zone_t *z, uint32_t action, int32_t n)
{
#if TESS_HAS_POINTER
    int32_t d, k;
    if (!z || !zone_live(z))
        return;
    if (action == TESS_ZA_WHEEL && z->kind == TESS_ZONE_ROW) {   /* a list under the wheel: it moves, a row a notch
                                                                  * (up: up the list, its knob anticlockwise) */
        zq_turn(z->role, -n);
        return;
    }
    if (action == TESS_ZA_WHEEL && z->kind == TESS_ZONE_SCROLL) {   /* (a text: wheel up, up it) */
        action = TESS_ZA_TURN;
        n = -n;
    } else if (action == TESS_ZA_WHEEL) {             /* (anything else: as a turn) */
        action = TESS_ZA_TURN;
    }
    switch (z->kind) {
    case TESS_ZONE_KNOB:
        if (action == TESS_ZA_TURN)
            zq_turn(z->role, n);
        else if (action == TESS_ZA_DOUBLE)
            zone_reset(z->role);
        break;
    case TESS_ZONE_ROW:
        if (action == TESS_ZA_TURN && z->vrole < 0) {   /* a list that is its value (a picker, a whole list): it
                                                         * scrolls */
            zq_turn(z->role, n);
            break;
        }
        d = z->row - uz_list.sel;                     /* there first: a detent a row */
        for (k = 0; k < (d < 0 ? -d : d) && k < 64; k++) {
            zq_put(ZQ_STEP, (uint32_t)z->role, d);
            zq_rows++;
        }
        if (d) {
            uz_list.sel = z->row;
            uz_list_sel_pending = 1;
        }
        if (action == TESS_ZA_TURN)
            zq_turn(z->vrole, n);                     /* then its value */
        else if (action == TESS_ZA_DOUBLE && uz_list.ok)
            zq_key(B_OCTUP);                          /* then OCT+: done, or its whole list */
        break;
    case TESS_ZONE_KEY:
        if (action != TESS_ZA_TURN && z->row >= 0 && z->row < (int32_t)NB)
            zq_key((uint32_t)z->row);
        break;
    case TESS_ZONE_TRACK:
        if (action != TESS_ZA_TURN)
            tess_core_track_select((uint32_t)z->row);
        break;
    case TESS_ZONE_MUTE:                              /* MIXER's strips: as TRACKS' M, S */
    case TESS_ZONE_SOLO:
        if (action == TESS_ZA_TURN || z->row < 0 || z->row >= (int32_t)NTRK) break;
        if (z->kind == TESS_ZONE_MUTE && z->role == 1) {   /* (MIXER's word, the strip wide: click MUTE, soloed SOLO
                                                       * off; double-click SOLO, the click's MUTE taken back) */
            static int32_t last_tr = -1;              /* (the track the last click muted) */
            uint32_t tr = (uint32_t)z->row;
            if (action == TESS_ZA_DOUBLE) {
                if (last_tr == (int32_t)tr) core_mute_solo(TESS_ZONE_MUTE, tr), core_mute_solo(TESS_ZONE_SOLO, tr);
                last_tr = -1;
            } else if (track_solo >> tr & 1u) {
                core_mute_solo(TESS_ZONE_SOLO, tr), last_tr = -1;
            } else {
                core_mute_solo(TESS_ZONE_MUTE, tr), last_tr = (int32_t)tr;
            }
            break;
        }
        core_mute_solo(z->kind, (uint32_t)z->row);
        break;
    case TESS_ZONE_TAB:                               /* the footer's tabs: that page (row: the pages from here) */
        if (action == TESS_ZA_CLICK)
            for (k = 0; k < (z->row < 0 ? -z->row : z->row) && k < 16; k++) zq_put(ZQ_PAGE, 0, z->row);
        break;
    case TESS_ZONE_SCROLL:                            /* a text to read: its knob, the wheel the other way round */
        if (action == TESS_ZA_TURN)
            zq_turn(z->role, n);
        break;
    case TESS_ZONE_LEVEL:                             /* MIXER's strips: a track's level, pan */
    case TESS_ZONE_PAN:
        if (z->row >= 0 && z->row < (int32_t)NTRK && (action == TESS_ZA_TURN || action == TESS_ZA_DOUBLE)) {
            uint32_t p = z->kind == TESS_ZONE_LEVEL ? (uint32_t)P_LEVEL : (uint32_t)P_PAN;
            const param_desc_t *pd = pid_desc(p, (uint32_t)z->row);
            int32_t v = action == TESS_ZA_DOUBLE ? pd->def : (int32_t)trk[z->row].p[p] + n;
            pid_set(p, (uint32_t)z->row, v < pd->min ? pd->min : v > pd->max ? pd->max : v);
            ui.force = 1;
        }
        break;
    case TESS_ZONE_CELL:                              /* a place in a grid: its knobs turned to it, a detent each */
        if (action == TESS_ZA_TURN) {
            zq_turn(z->role, n);
            break;
        }
        {
            int32_t turn[2] = {(int8_t)(uint8_t)((uint16_t)z->row & 0xFFu), (int8_t)(uint8_t)((uint16_t)z->row >> 8)};
            int32_t role[2] = {z->role, z->vrole}, j;
            for (j = 0; j < 2; j++)
                for (k = 0, d = turn[j]; role[j] >= 0 && k < (d < 0 ? -d : d) && k < 64; k++)
                    zq_put(ZQ_STEP, (uint32_t)role[j], d);
            uz_begin(z->group);                       /* (its grid's cells go until it is drawn again: a second click
                                                       * counts from there, not from the turns already on their way) */
        }
        break;
    }
#else
    (void)z, (void)action, (void)n;
#endif
}

void tess_core_text(uint32_t ch)
{
#if TESS_HAS_POINTER
    _Static_assert(NT_BACK == TESS_TEXT_BACK && NT_OK == TESS_TEXT_OK && NT_CANCEL == TESS_TEXT_CANCEL &&
                   NT_LEFT == TESS_TEXT_LEFT && NT_RIGHT == TESS_TEXT_RIGHT && NT_DELETE == TESS_TEXT_DELETE,
                   "ui_name.c's keys as tess_core.h's");
    int r = name_text(ch);
    if (r == 2 || r == 3)
        zq_key(r == 2 ? B_OCTUP : B_OCTDN);          /* (OK, cancel: as the keys, through the same queue) */
    if (r)
        ui.force = 1;
#else
    (void)ch;
#endif
}

/* one pass of main.c's loop: what a host has (no USB, editor, UBOOT, flash) */
static void core_frame(void)
{
    if (core_track_req < NTRK) {                  /* (between passes, as KNOB 1 on TRACKS would) */
        if (core_track_req != song.sel) pop_close();   /* (an open sheet is the last track's: it closes) */
        track_select(core_track_req);
        core_track_req = 0xFFu;
    }
    core_ui_input();
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
        core_ui_input();
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
        core_ui_input();
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
    song.g[G_CLOCK] = 0;                          /* the DAW is the clock: CLK stays INT (no MIDI clock reaches a */
    settings_clock_out = 0;                       /* plugin, tess_core_midi), and no CLOCK OUT into the DAW */
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
#if TESS_HAS_POINTER
    host_btn = m;
    fm1_in.buttons = m | zone_btn;                /* (a key a zone's click holds a moment: tess_core_zone_do) */
#else
    fm1_in.buttons = m;
#endif
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
uint32_t tess_core_note_expr(uint32_t kind, int32_t ch, int32_t key, uint32_t v)
{
#if TESS_MPE
    uint32_t c, ok = 1;
    v = v > 127u ? 127u : v;
    for (c = 0; c < 16u; c++) {
        if (ch >= 0 && (uint32_t)ch != c)
            continue;
        if (key < 0)                       /* (every key: the channel's pressure or CC74, as an MPE member channel's) */
            ok &= tess_core_midi(kind ? 0xB0u | c : 0xD0u | c, kind ? 74u : v, kind ? v : 0u);
        else if (kind == TESS_EXPR_PRESSURE)
            ok &= tess_core_midi(0xA0u | c, (uint32_t)key & 127u, v);   /* (Poly AT) */
        else if (mi_w - mi_r >= MQ)
            ok = 0;
        else {                             /* (one key's slide: no MIDI 1.0 message, so code index 0: seq.c) */
            midi_in_q[mi_w % MQ] = (0xA0u | c) << 8 | ((uint32_t)key & 127u) << 16 | v << 24;
            mi_w++;
        }
    }
    return ok;
#else
    (void)kind; (void)ch; (void)key; (void)v;
    return 0;
#endif
}
uint32_t tess_core_note_tune(int32_t ch, int32_t key, int32_t cents)
{
#if TESS_MPE
    return mpe_tune_put(ch, key, cents * 256 / 100);
#else
    (void)ch; (void)key; (void)cents;
    return 0;
#endif
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
    v->ink = settings.palette;
    v->track = song.sel;
    v->zone_count = 0;
    v->typing = 0;
    v->zone_gen = 0;
#if TESS_HAS_POINTER
    {                                             /* the zones, in the order a hit is looked for: a list over the
                                                   * page, a dialog's or a screen's keys, the page's columns,
                                                   * MIXER's strips, a screen's two lists, HOME's tracks */
        static const uint8_t ORDER[] = {UZ_G_LIST, UZ_G_KEYS, UZ_G_COLS, UZ_G_STRIPS, UZ_G_SIDES, UZ_G_TRACKS, UZ_G_TABS};
        _Static_assert(sizeof ORDER == UZ_GROUPS, "every zone group in ORDER, once");
        uint32_t g, n = 0;
        for (g = 0; g < UZ_GROUPS; g++)
            for (i = 0; i < uz_n && n < TESS_ZONES; i++)
                if (uz[i].group == ORDER[g]) {
                    tess_zone_t *z = &v->zones[n++];
                    z->x = (uint16_t)uz[i].x, z->y = (uint16_t)uz[i].y, z->w = (uint16_t)uz[i].w, z->h = (uint16_t)uz[i].h;
                    z->kind = uz[i].kind, z->group = uz[i].group;
                    z->role = uz[i].role, z->vrole = uz[i].vrole, z->row = uz[i].row;
                    str_cpy(z->name, uz[i].name, sizeof z->name);
                    str_cpy(z->value, uz[i].value, sizeof z->value);
                }
        v->zone_count = n;
        v->typing = name_on() ? 1u : 0u;
        v->zone_gen = uz_gen;
    }
#endif
}

uint32_t tess_core_track(void) { return song.sel; }
void tess_core_track_select(uint32_t t)
{
    if (t < NTRK)
        core_track_req = t;
}

/* ---- screen 2, TRACKS (ui_tracks.c) */
void tess_core_tracks_view(tess_tracks_t *v, uint32_t w, uint32_t h)
{
#if TESS_SCR_COUNT > 1 && MELODEE_LOOK
    uint32_t i;
    _Static_assert(TESS_TRACKS_W == T2_W_MAX && TESS_TRACKS_H == T2_H_MAX && TESS_TRACKS_ZONES == T2_ZONES,
                   "tess_core.h's TRACKS sizes are ui_tracks.c's");
    _Static_assert(TESS_ZONE_MUTE == T2_Z_MUTE && TESS_ZONE_SOLO == T2_Z_SOLO && TESS_ZONE_LEVEL == T2_Z_LEVEL &&
                   TESS_ZONE_TRACK == UZ_TRACK, "tess_core.h's TRACKS zones are ui_tracks.c's");
    tracks_draw((int32_t)(w < 1u ? 1u : w), (int32_t)(h < 1u ? 1u : h), (fm1_ms / 250u) & 1u);
    v->w = (uint32_t)t2_w, v->h = (uint32_t)t2_h;
    for (i = 0; i < v->w * v->h; i++)                  /* (drawn as RGB565 already: no LCD byte order here) */
        v->screen[i] = t2_px[i];
    v->zone_count = 0;
    for (i = 0; i < t2_zn; i++) {                       /* M, S and the levels first, then the rows */
        tess_zone_t *z = &v->zones[v->zone_count++];
        const uz_t *u = &t2_z[i];
        z->x = (uint16_t)u->x, z->y = (uint16_t)u->y, z->w = (uint16_t)u->w, z->h = (uint16_t)u->h;
        z->kind = u->kind, z->group = u->group, z->role = u->role, z->vrole = u->vrole, z->row = u->row;
        str_cpy(z->name, u->name, sizeof z->name);
        str_cpy(z->value, u->value, sizeof z->value);
    }
    {
        uint32_t a, b;                                  /* (drawn row by row: the rows to the end) */
        for (a = 0, b = 0; a < v->zone_count; a++)
            if (v->zones[a].kind != TESS_ZONE_TRACK) {
                tess_zone_t t = v->zones[a];
                memmove(&v->zones[b + 1], &v->zones[b], (a - b) * sizeof t);
                v->zones[b++] = t;
            }
    }
#else
    (void)w, (void)h;
    v->w = v->h = 0;
    v->zone_count = 0;
#endif
}
void tess_core_tracks_do(const tess_zone_t *z, uint32_t action, int32_t n)
{
#if TESS_SCR_COUNT > 1 && MELODEE_LOOK
    uint32_t tr = z ? (uint32_t)z->row : NTRK, click = action != TESS_ZA_TURN;
    if (tr >= NTRK)
        return;
    if (z->kind == TESS_ZONE_TRACK && click) {
        tess_core_track_select(tr);
    } else if ((z->kind == TESS_ZONE_MUTE || z->kind == TESS_ZONE_SOLO) && click) {
        core_mute_solo(z->kind, tr);
    } else if (z->kind == TESS_ZONE_LEVEL && !click && n) {
        int32_t v = (trk[tr].p[P_LEVEL] & 127) + n;
        pid_set(P_LEVEL, tr, v < 0 ? 0 : v > 127 ? 127 : v);
    }
    ui.force = 1;
#else
    (void)z, (void)action, (void)n;
#endif
}

/* ---- the project */
uint32_t tess_core_tsp_max(void) { return TSP_MAX; }

/* as project_save_as, without the slot write or its messages */
uint32_t tess_core_save(uint8_t *out, uint32_t cap)
{
    project_t *p = &proj_scratch;
    uint32_t len;
    project_capture(p);
#ifdef TESS_PLUGIN_IO
    p->g[G_ROUTE] = 0;                            /* (the record's ROUT is the FM-1's power-on one: every target leaves it
                                                   * unread (project.c GLO_KEPT), the plugin keeps its own beside the
                                                   * record (tess_core_setup_save), and the bytes stay the FM-1's) */
#endif
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
/* a new project, as an FM-1 with nothing saved starts with (project.c project_new_blank: the power-on sounds, every
 * pattern empty, no generator running, the device's MIDI setup kept); the slots are left as they are */
void tess_core_project_new(void)
{
    project_new_blank();
    core_seeds();
    ui.force = 1;
}

/* the device's own MIDI and tuning setup (project.c GLO_KEPT: a project load leaves it as it is), for a front end to
 * keep beside the project: version 1, ROUT, DRUM's channel, NOTES OUT, TUNE (-50..50, +128) */
#define SETUP_LEN 5u
uint32_t tess_core_setup_save(uint8_t *out, uint32_t cap)
{
    if (cap < SETUP_LEN)
        return 0;
    out[0] = 1u;
    out[1] = (uint8_t)song.g[G_ROUTE];
    out[2] = settings_drumch;
    out[3] = settings_midi_out;
    out[4] = (uint8_t)(song.g[G_TUNE] + 128);
    return SETUP_LEN;
}
uint32_t tess_core_setup_load(const uint8_t *in, uint32_t len)
{
    int32_t tune;
    if (len < SETUP_LEN || in[0] != 1u)           /* (a later version: its first five bytes are these) */
        return 1;
    if (in[1] < sizeof N_ROUTE / sizeof N_ROUTE[0])
        song.g[G_ROUTE] = in[1];
    if (in[2] <= 16u)
        settings_drumch = in[2];
    if (in[3] < MIDI_OUT_N)
        settings_midi_out = in[3];
    tune = (int32_t)in[4] - 128;
    if (tune >= -50 && tune <= 50)
        song.g[G_TUNE] = (int16_t)tune;
    ui.force = 1;
    return 0;
}

/* every note MIDI OUT still has on goes off (each track's, midi_out.c), as a STOP sends them */
void tess_core_notes_off(void)
{
    uint32_t k;
    for (k = 0; k < NTRK; k++)
        midi_out_flush(k);
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
#ifdef TESS_PLUGIN_IO
    if (pid == PID_GLOBAL(G_CLOCK))
        info->settable = 0;                       /* (CLK stays INT in the plugin, tess_plugin_block: not a parameter) */
#endif
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
/* a key from outside (the plugin's LISTEN: a chord played to it): ROOT on every melodic track (a drum track keeps
 * its own, as KEY LINK leaves it), and minor 1 / 0 turns the song's scale to its minor / major twin (MAJ <-> MIN,
 * PEN <-> MPEN; any other scale stays, -1 keeps it). 1 if anything changed */
uint32_t tess_core_key(uint32_t root, int32_t minor)
{
    static const uint8_t TWIN[][2] = {{1u, 2u}, {5u, 6u}};   /* (scales.h: MAJ MIN, PEN MPEN: major, minor) */
    uint32_t k, i, mel = NTRK, changed = 0;
    root %= 12u;
    for (k = 0; k < NTRK; k++) {
        if (drum_track(&trk[k]))
            continue;
        if (mel == NTRK)
            mel = k;
        if ((uint32_t)trk[k].p[P_ROOT] != root && pid_set(P_ROOT, k, (int32_t)root))
            changed = 1;
    }
    if (minor < 0 || mel == NTRK)
        return changed;
    for (i = 0; i < sizeof TWIN / sizeof TWIN[0]; i++)
        if ((uint32_t)trk[mel].p[P_SCALE] == TWIN[i][!minor] && pid_set(P_SCALE, mel, TWIN[i][minor != 0])) {
            scale_share(&trk[mel]);
            changed = 1;
            break;
        }
    return changed;
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

uint32_t tess_core_ink_at(uint32_t i, const char **name, uint32_t rgb[4])
{
#if MELODEE_LOOK
    uint32_t k;
    if (i < NPALETTES) {
        if (name)
            *name = UI_PALETTES[i].name;
        for (k = 0; rgb && k < 4u; k++) {   /* RGB565 -> 0xRRGGBB */
            uint32_t c = UI_PALETTES[i].trk[k];
            rgb[k] = ((c >> 11 & 31u) * 255u / 31u) << 16 | ((c >> 5 & 63u) * 255u / 63u) << 8 | (c & 31u) * 255u / 31u;
        }
    }
    return NPALETTES;
#else
    (void)i; (void)name; (void)rgb;
    return 0;
#endif
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

#if TESS_GENERATE
uint32_t tess_core_gen_styles(void) { return generate_styles(); }
const char *tess_core_gen_style(uint32_t s) { return generate_style_name(s); }
/* the DAW's clock: what generate or the swap stopped starts again where the DAW is (its next block: host_locate), not
 * from the top */
static void gen_host_follow(void)
{
#ifdef TESS_HOST_CLOCK
    if (tess_host_clock && transport_req == 1u) {
        transport_req = 0;
        tess_host_clock = 0;
    }
#endif
}
uint32_t tess_core_generate(uint32_t style, uint32_t seed, uint32_t variation, uint32_t flags)
{
    uint32_t r = generate(style, seed, variation, flags & TESS_GEN_KEEP_TEMPO ? GEN_KEEP_TEMPO : 0u);
    gen_host_follow();
    return r;
}
uint32_t tess_core_gen_swap(void)
{
    uint32_t r = generate_swap();
    gen_host_follow();
    return r;
}
uint32_t tess_core_gen_last(uint32_t *style, uint32_t *seed, uint32_t *variation) { return generate_last(style, seed, variation); }
#else
uint32_t tess_core_gen_styles(void) { return 0; }
const char *tess_core_gen_style(uint32_t s) { (void)s; return ""; }
uint32_t tess_core_generate(uint32_t style, uint32_t seed, uint32_t variation, uint32_t flags)
{
    (void)style; (void)seed; (void)variation; (void)flags;
    return 1;
}
uint32_t tess_core_gen_swap(void) { return 1; }
uint32_t tess_core_gen_last(uint32_t *style, uint32_t *seed, uint32_t *variation)
{
    (void)style; (void)seed; (void)variation;
    return 0;
}
#endif
