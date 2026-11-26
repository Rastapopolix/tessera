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
_Static_assert(TESS_ZONE_ROLL == UZ_ROLL, "tess_core.h TESS_ZONE_ROLL is ui_zones.c's UZ_ROLL");
enum { ZQ_STEP = 1, ZQ_DETENT, ZQ_PRESS, ZQ_RELEASE, ZQ_PAGE, ZQ_HELP, ZQ_EDIT, ZQ_PICK, ZQ_PICK_DONE, ZQ_SOUNDS, ZQ_SECTION,
       ZQ_FINE };
#define ZQ_N 256u
static uint32_t zq[ZQ_N], zq_r, zq_w, zq_wait, zq_rows;   /* zq_rows: a list's row moves still to run */
static uint32_t zone_btn, host_btn;                   /* held by a zone's click, and by the front end (physical bits) */
static void zq_put(uint32_t op, uint32_t arg, int32_t sign)
{
    if (zq_w - zq_r < ZQ_N)
        zq[zq_w++ % ZQ_N] = op | (arg & 255u) << 8 | (sign < 0 ? 1u : 0u) << 16;
}
static uint8_t zq_fine;                                 /* (TESS_ZA_TURN_FINE: the detents never sped up) */
static void zq_turn(int32_t role, int32_t n)          /* n detents of a knob (at most 64 at once) */
{
    int32_t k, a = n < 0 ? -n : n;
    if (role < 0 || role >= (int32_t)NE)
        return;
    for (k = 0; k < a && k < 64; k++)
        zq_put(zq_fine ? ZQ_FINE : ZQ_DETENT, (uint32_t)role, n);
}
/* the mouse's help (TESS_ZA_HELP, TESS_BTN_HELP): knob k's card (0..3: KNOB 1..4), as ARP held + that knob opens it,
 * pinned (UZ_HELP_PIN) till help_unpin: a click */
static void core_help(uint32_t k)
{
    if (ui.menu || ui.confirm || ui.layer || pop.on)
        return;
    help.on = 0;                                      /* (another knob's, or again: from its top) */
    uz_help_pin = 1;
    (void)help_knob(k & 3u, 0);
}
static int help_unpin(void)                           /* 1: a help card closed: a pinned one, or one ARP held (or a
                                                       * window's latched ARP) opened, which a click closes too */
{
    if (!uz_help_pin && !help.on)
        return 0;
    uz_help_pin = 0;
    if (help.on) {
        help.on = 0;
        ui.force = 1;
    }
    return 1;
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
    case ZQ_FINE:                                     /* a detent never sped up (TESS_ZA_TURN_FINE) */
        ui.enc_t[arg % NE] = 0;
        tess_core_enc(arg, sg);
        break;
    case ZQ_SECTION:                                  /* a section's button (ui.c cs_open): not over a menu, a dialog, a
                                                       * picker (as the FM-1's buttons, which those take) */
        if (!ui.menu && !ui.confirm && !pop.on && !ui.layer) cs_open(arg);
        ui.force = 1;
        break;
    case ZQ_PAGE:                                     /* a footer tab: the family's next page that way (SELECT's) */
        page_scroll(sg);
        ui.force = 1;
        break;
    case ZQ_EDIT:                                     /* TRACKS' row double-clicked: its EDIT page (one there already
                                                       * stays, as EDIT pressed again would go on) */
        if (!ui.menu && !ui.confirm && !pop.on && !ui.layer && (ui.home || !in_fam(ui.page, FAM_EDIT)))
            open_family(FAM_EDIT);
        ui.force = 1;
        break;
    case ZQ_PICK:                                     /* MODS' SOURCE / DESTINATION clicked: its picker, kept */
        pick_touch(arg);
        if (pop.on == POP_PICK)
            uz_pick_pin = 1;
        ui.force = 1;
        break;
    case ZQ_PICK_DONE:                                /* a row of that picker clicked: taken, the picker goes */
        if (pop.on == POP_PICK && uz_pick_pin)
            pop_close();
        break;
    case ZQ_SOUNDS:                                   /* HOME's track double-clicked: its sounds (SAVE > PRESETS) */
        if (!ui.menu && !ui.confirm && !pop.on && !ui.layer && page_titled("PRESETS") < NPAGES) {
            ui.page = (uint8_t)page_titled("PRESETS");
            ui.home = 0;
            page_entered();
        }
        ui.force = 1;
        break;
    case ZQ_HELP:                                     /* KNOB arg+1's help card, kept till a click (a list's row:
                                                       * the cursor there first, KNOB 2 its value's) */
        core_help(arg);
        break;
    case ZQ_PRESS:
        m = 1u << panel.btn[arg % NB];
        host_pressed |= m & ~fm1_in.buttons;
        ly_ptr |= m;                                  /* (a click's press: a mouse's) */
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
/* a boxed value's column (ui_mono.c mo_cols MV_BOX: a choice, not a dial) clicked: the detents to its next value, from
 * the last back to the first; 0 not a box (a dial: a click does nothing). A box with no parameter of the page's own
 * (LOCK, ROLL): one detent on */
static int32_t zone_box(int32_t role)
{
    int16_t *vp = 0;
    const param_desc_t *d;
    uint32_t c = (uint32_t)(role - EN_K1);
    if (role < EN_K1 || role > EN_K4 || ui.home || ui.menu || ui.confirm || pop.on || ui.layer || !mo_vals[c].used ||
        mo_vals[c].kind != MV_BOX)
        return 0;
    d = page_desc(cur_page(), c, &vp);
    if (d && vp && d->max > d->min && *vp >= d->max)
        return -(int32_t)(d->max - d->min) > -64 ? -(int32_t)(d->max - d->min) : -63;
    return 1;
}
/* a column that names a key ("[OCT+]", "[OK]": mo_cols MV_KEY) clicked: that key (B_OCTUP / B_OCTDN), -1 none */
static int32_t zone_box_key(int32_t role)
{
    uint32_t c = (uint32_t)(role - EN_K1);
    const char *s;
    if (role < EN_K1 || role > EN_K4 || ui.home || ui.menu || ui.confirm || pop.on || !mo_vals[c].used ||
        mo_vals[c].kind != MV_KEY)
        return -1;
    s = mo_vals[c].s;
    if (s[0] == 'B' || str_len(s) >= 4u && s[3] == '-') return B_OCTDN;   /* (BACK, OCT-) */
    return B_OCTUP;                                                         /* (OK, OCT+) */
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
/* KNOB c+1's value one of two (FREE / NOTE, BI / UNI, ON / OFF): the turn to the other, +1 or -1; 0 when it has more */
static int32_t zone_two(int32_t role)
{
    int16_t *vp = 0;
    const param_desc_t *d;
    if (role < EN_K1 || role > EN_K4 || ui.home || ui.menu || ui.confirm || pop.on || ui.layer)
        return 0;
    d = page_desc(cur_page(), (uint32_t)(role - EN_K1), &vp);
    if (!d || !vp || d->max - d->min != 1)
        return 0;
    return *vp > d->min ? -1 : 1;
}
/* the tempo dragged or double-clicked (a screen's header, TRACKS'): a BPM a detent, 120; not while a DAW or EXT clock
 * sets it */
static void core_tempo(uint32_t action, int32_t n)
{
    int32_t k;
    if (action == TESS_ZA_WHEEL)
        action = TESS_ZA_TURN;
    if (action != TESS_ZA_TURN && action != TESS_ZA_DOUBLE)
        return;
    if (song.g[G_CLOCK]) {
        ui_message("CLOCK IS EXT");
    } else if (tess_host_clock) {
        ui_message("DAW SETS TEMPO");
    } else {
        k = action == TESS_ZA_DOUBLE ? GP[G_BPM].def : song.g[G_BPM] + n;
        song.g[G_BPM] = (int16_t)clamp(k, GP[G_BPM].min, GP[G_BPM].max);
        ui.bpm_t = 40;
    }
    ui.force = 1;
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
/* CHORD USER's progression edited a card at a time (TESS_ZONE_CHORD): the selected track's, when its generator is
 * CHORD on PROG USER (a turn: its root; a click: its quality, where the build keeps one; "+": a chord more; a
 * right-click: the chord taken) */
#if TESS_CH_QUAL
/* a card's click: AUTO (the track's CHRD), then these, round */
static const uint8_t CH_QUAL_CYCLE[] = {0, CH_MAJ, CH_MIN, CH_DOM7, CH_MAJ7, CH_MIN7, CH_SUS2, CH_SUS4, CH_7SUS4,
                                        CH_ADD9, CH_MAJ6, CH_POW};
#endif
static void core_chord_user(int32_t slot, uint32_t action, int32_t n)
{
    gen_chord_t *g;
    uint32_t len, i;
    if (song.sel >= TESSERA_TRACKS || tessera.mode[song.sel] != TESS_ENG_CHORD || slot < 0)
        return;
    g = &tessera.chord[song.sel];
    if (g->prog != GEN_CH_USER)
        return;
    len = g->ulen ? g->ulen : 1u;
    TESS_IRQ_OFF();
    if (!g->ulen)                                       /* (none yet: the one, the tonic) */
        g->user[0] = 0, g->ulen = 1;
    if ((action == TESS_ZA_TURN || action == TESS_ZA_WHEEL) && (uint32_t)slot < len && n)
        g->user[slot] = (uint8_t)(((int32_t)g->user[slot] + n % 7 + 7) % 7);
    else if (action == TESS_ZA_CLICK && (uint32_t)slot == len && len < GEN_CH_USER_N) {
        g->user[len] = (uint8_t)((g->user[len - 1u] + 3u) % 7u), g->ulen = (uint8_t)(len + 1u);
#if TESS_CH_QUAL
        g->uq[len] = 0;                                 /* (AUTO: the track's CHRD) */
    } else if (action == TESS_ZA_CLICK && (uint32_t)slot < len) {   /* (a card: its quality, the next one, round */
        for (i = 0; i < sizeof CH_QUAL_CYCLE && CH_QUAL_CYCLE[i] != g->uq[slot]; i++) ;   /* to AUTO) */
        g->uq[slot] = CH_QUAL_CYCLE[(i + 1u) % sizeof CH_QUAL_CYCLE];
#endif
    } else if (action == TESS_ZA_HELP && (uint32_t)slot < len && len > 1u) {
        for (i = (uint32_t)slot; i + 1u < len; i++) {
            g->user[i] = g->user[i + 1u];
#if TESS_CH_QUAL
            g->uq[i] = g->uq[i + 1u];
#endif
        }
        g->ulen = (uint8_t)(len - 1u);
    }
    TESS_IRQ_ON();
    ui.force = 1;
}

#if TESSERA_TRACKS > 4
/* ENTRIES' arrangement grid edited a cell at a time (TESS_ZONE_EGRID): a click the next kind (AUTO IN OUT FADE), a
 * right-click AUTO; a letter's head the whole column, as its first track's cell goes */
static void core_egrid(int32_t row, uint32_t action)
{
    uint32_t s = (uint32_t)(row & 0xFF) / 16u, tr = (uint32_t)row & 15u, c, g, k;
    if (action == TESS_ZA_DOUBLE) action = TESS_ZA_CLICK;   /* (quick clicks: each one the next) */
    if (row < 0 || (action != TESS_ZA_CLICK && action != TESS_ZA_HELP)) return;
    if (row & 0x100) s = (uint32_t)row & 0xFFu, tr = 0;
    if (s >= ET_G_SECS || tr >= TESSERA_TRACKS) return;
    g = tessera.et_grid[s];
    c = action == TESS_ZA_HELP ? ET_G_AUTO : ((g >> (2u * tr) & 3u) + 1u) & 3u;
    for (k = 0; k < TESSERA_TRACKS; k++)
        if (k == tr || (row & 0x100))
            g = (g & ~(3u << (2u * k))) | c << (2u * k);
    TESS_IRQ_OFF();
    tessera.et_grid[s] = (uint16_t)g;
    TESS_IRQ_ON();
    ui.force = 1;
}
#endif
#if TESS_HAS_POINTER
/* ---- LOCK, DICE, MUTATE (Cosmati audit G6). A lock: a parameter (its ID, its track) DICE and MUTATE leave alone, kept
 * by the front end with its state (tess_core_locks). DICE: the page's values (its KNOB zones) rolled round their
 * defaults, 40 % of their range either way; MUTATE: a track's sound (envelope, filter and pitch depths, LFO, ENGINE
 * 1..8, FX 1's mix, rate and character, the FM6 operators on an FM6 track) nudged n % of each range either way. Either
 * one can be put back once (n -1), until the next */
#define LK_MAX 64u
static struct { uint16_t pid; uint8_t trk; } lk[LK_MAX];
static uint32_t lk_n;
static struct { uint16_t pid; uint8_t trk; int16_t old; } dk[96];
static uint32_t dk_n;
static uint32_t dk_seed = 0x2545F491u;
static int lk_at(uint32_t pid, uint32_t tr)
{
    uint32_t i;
    for (i = 0; i < lk_n; i++)
        if (lk[i].pid == pid && lk[i].trk == tr % NTRK)
            return (int)i;
    return -1;
}
static uint32_t dk_rand(uint32_t n)                     /* 0 .. n - 1 */
{
    dk_seed ^= fm1_ms * 2654435761u;
    dk_seed ^= dk_seed << 13, dk_seed ^= dk_seed >> 17, dk_seed ^= dk_seed << 5;
    return n ? dk_seed % n : 0u;
}
/* pid on track tr to v, kept for UNDO; 1 if it changed */
static uint32_t dk_set(uint32_t pid, uint32_t tr, int32_t v)
{
    int32_t was = pid_get(pid, tr);
    if (v == was || dk_n >= NELEM(dk) || !tess_core_param_set(pid, tr, v))
        return 0;
    dk[dk_n].pid = (uint16_t)pid, dk[dk_n].trk = (uint8_t)tr, dk[dk_n].old = (int16_t)was;
    dk_n++;
    return 1;
}
static void dk_say(const char *what, uint32_t n)
{
    char b[16];
    fmt_int(b, (int32_t)n);
    str_cpy(b + str_len(b), n == 1u ? " VALUE" : " VALUES", 8);
    ui_say(what, b);
    ui.force = 1;
}
static void dice_page(void)
{
    uint32_t i, pid, tr, n = 0;
    tess_param_t in;
    dk_n = 0;
    for (i = 0; i < uz_n; i++) {
        tess_zone_t z;
        int32_t span, lo, hi;
        if (uz[i].kind != TESS_ZONE_KNOB)
            continue;
        memset(&z, 0, sizeof z);
        z.kind = uz[i].kind, z.group = uz[i].group, z.role = uz[i].role, z.vrole = uz[i].vrole, z.row = uz[i].row;
        if (!tess_core_zone_param(&z, &pid, &tr) || !tess_core_param_info(pid, tr, &in) || !in.settable ||
            !in.per_track || in.max <= in.min || pid == P_MUTE || pid == P_LEVEL || pid == P_SLEN || lk_at(pid, tr) >= 0)
            continue;
        span = (in.max - in.min) * 2 / 5;
        lo = in.def - span < in.min ? in.min : in.def - span;
        hi = in.def + span > in.max ? in.max : in.def + span;
        n += dk_set(pid, tr, lo + (int32_t)dk_rand((uint32_t)(hi - lo + 1)));
    }
    dk_say("DICE: ", n);
}
static void mutate(uint32_t tr, int32_t pct)
{
    static const uint8_t SOUND[] = {P_ATK, P_DEC, P_SUS, P_REL, P_ED_FLT, P_ED_PIT, P_ED_SHP, P_LRATE, P_LD_PIT,
                                    P_LD_FLT, P_LD_SHP, P_LD_AMP, P_DETUNE, P_SPRD, P_FXMIX, P_FXRATE, P_FXCHAR,
                                    P_E0, P_E1, P_E2, P_E3, P_E4, P_E5, P_E6, P_E7,
                                    P_FM1_ATK, P_FM1_DEC, P_FM1_SUS, P_FM1_REL, P_FM1_LEVEL,
                                    P_FM2_ATK, P_FM2_DEC, P_FM2_SUS, P_FM2_REL, P_FM2_LEVEL,
                                    P_FM3_ATK, P_FM3_DEC, P_FM3_SUS, P_FM3_REL, P_FM3_LEVEL,
                                    P_FM4_ATK, P_FM4_DEC, P_FM4_SUS, P_FM4_REL, P_FM4_LEVEL};
    uint32_t i, n = 0, fm = trk[tr].eng_req == ENGI_FM6;
    tess_param_t in;
    dk_n = 0;
    for (i = 0; i < sizeof SOUND; i++) {
        uint32_t pid = SOUND[i];
        int32_t d, v;
        if ((pid >= P_FM1_ATK && pid <= P_FM4_LEVEL && !fm) || lk_at(pid, tr) >= 0 ||
            !tess_core_param_info(pid, tr, &in) || !in.settable || in.max <= in.min)
            continue;
        d = (in.max - in.min) * pct / 100;
        if (d < 1)
            d = 1;
        v = pid_get(pid, tr) + (int32_t)dk_rand((uint32_t)(2 * d + 1)) - d;
        n += dk_set(pid, tr, v < in.min ? in.min : v > in.max ? in.max : v);
    }
    dk_say("MUTATE: ", n);
}
static void dice_undo(void)
{
    uint32_t n = dk_n;
    while (dk_n) {
        dk_n--;
        tess_core_param_set(dk[dk_n].pid, dk[dk_n].trk, dk[dk_n].old);
    }
    dk_say("PUT BACK: ", n);
}
uint32_t tess_core_locks(uint8_t *out, uint32_t cap)
{
    uint32_t i;
    if (!out)
        return lk_n * 3u;
    for (i = 0; i < lk_n && i * 3u + 3u <= cap; i++)
        out[i * 3u] = (uint8_t)lk[i].pid, out[i * 3u + 1u] = (uint8_t)(lk[i].pid >> 8), out[i * 3u + 2u] = lk[i].trk;
    return i * 3u;
}
void tess_core_locks_load(const uint8_t *in, uint32_t len)
{
    uint32_t i;
    lk_n = 0;
    for (i = 0; in && i + 3u <= len && lk_n < LK_MAX; i += 3u)
        if (in[i + 2u] < NTRK) {
            lk[lk_n].pid = (uint16_t)(in[i] | (uint32_t)in[i + 1u] << 8), lk[lk_n].trk = in[i + 2u];
            lk_n++;
        }
}
#else
uint32_t tess_core_locks(uint8_t *out, uint32_t cap) { (void)out, (void)cap; return 0; }
void tess_core_locks_load(const uint8_t *in, uint32_t len) { (void)in, (void)len; }
#endif
void tess_core_zone_do(const tess_zone_t *z, uint32_t action, int32_t n)
{
#if TESS_HAS_POINTER
    int32_t d, k;
    if (action == TESS_ZA_DICE) {                     /* (DICE, MUTATE, put back: any zone, or none) */
        if (n < 0)
            dice_undo();
        else if (n > 0)
            mutate(z && z->kind == TESS_ZONE_TRACK && z->row >= 0 && z->row < (int32_t)NTRK ? (uint32_t)z->row : song.sel,
                   n > 50 ? 50 : n);
        else
            dice_page();
        return;
    }
    if (action == TESS_ZA_LOCK) {                     /* (a value's menu: LOCK, UNLOCK) */
        uint32_t pid, tr;
        int at;
        if (!z || !tess_core_zone_param(z, &pid, &tr))
            return;
        at = lk_at(pid, tr);
        if (at >= 0)
            lk[at] = lk[--lk_n];
        else if (lk_n < LK_MAX)
            lk[lk_n].pid = (uint16_t)pid, lk[lk_n].trk = (uint8_t)(tr % NTRK), lk_n++;
        ui_say(at >= 0 ? "UNLOCKED" : "LOCKED", "");
        ui.force = 1;
        return;
    }
    if (action == TESS_ZA_RANDOM) {                   /* (a value's menu: RANDOM) */
        static uint32_t seed = 0x9E3779B9u;
        uint32_t pid, tr;
        tess_param_t info;
        if (!tess_core_zone_param(z, &pid, &tr) || !tess_core_param_info(pid, tr, &info) || !info.settable ||
            info.max <= info.min)
            return;
        seed ^= fm1_ms * 2654435761u;
        seed ^= seed << 13, seed ^= seed >> 17, seed ^= seed << 5;
        tess_core_param_set(pid, tr, info.min + (int32_t)(seed % (uint32_t)(info.max - info.min + 1)));
        ui.force = 1;
        return;
    }
    if (action == TESS_ZA_TURN_FINE) {                /* (as TURN, a detent at a time) */
        zq_fine = 1;
        tess_core_zone_do(z, TESS_ZA_TURN, n);
        zq_fine = 0;
        return;
    }
    if (z && (action == TESS_ZA_CLICK || action == TESS_ZA_DOUBLE) && help_unpin())
        return;                                       /* a pinned help card: any click closes it, and does no more */
    if (z && action == TESS_ZA_DOUBLE && z->kind == TESS_ZONE_CELL && z->group == UZ_G_PIC && z->role == EN_K2 &&
        !ui.home && cur_page()->graph == GR_BROWSE) {
        zq_key(B_OCTUP);                              /* the browser: a sound double-clicked (its first click went there;
                                                       * its zone moved with it): kept, OK */
        return;
    }
    if (action == TESS_ZA_DROP) {                     /* (a drag let go: wherever the screen is now) */
        roll_drop();
        return;
    }
    if (!z || !zone_live(z))
        return;
    if (z->kind == TESS_ZONE_ROLL) {                  /* NOTES' steps: the note at the mouse's place (ui_roll.c); a
                                                       * DRUM track's grid (row bit 14) */
        uint32_t op = action == TESS_ZA_CLICK ? ROLL_ADD : action == TESS_ZA_HELP ? ROLL_DEL : action == TESS_ZA_GRAB ?
                      ROLL_GRAB : action == TESS_ZA_TURN || action == TESS_ZA_WHEEL ? ROLL_MOVE : action ==
                      TESS_ZA_TURN_CHORD ? ROLL_MOVE_ALL : action == TESS_ZA_PAINT ? ROLL_PAINT : action ==
                      TESS_ZA_LENGTH ? ROLL_LEN : action == TESS_ZA_VELOCITY ? ROLL_VEL : 0u;
        int lanes = (z->row & 0x4000) != 0;
        if (action == TESS_ZA_WHEEL) {
            if (lanes) lane_do(ROLL_GRAB, z->x, z->y, 0);
            else roll_do(ROLL_GRAB, z->x, z->y, 0);
        }
        if (lanes) lane_do(op, z->x, z->y, n);
        else roll_do(op, z->x, z->y, n);
        if (action == TESS_ZA_WHEEL) roll_drop();
        return;
    }
    if (action == TESS_ZA_HELP) {                     /* a value's column or a list's row: what it does */
        if (z->kind == TESS_ZONE_KNOB && z->role >= EN_K1 && z->role <= EN_K4) {
            help_unpin();
            zq_put(ZQ_HELP, (uint32_t)(z->role - EN_K1), 1);
        } else if (z->kind == TESS_ZONE_ROW && z->vrole >= EN_K1 && z->vrole <= EN_K4) {
            help_unpin();
            tess_core_zone_do(z, TESS_ZA_CLICK, 0);   /* (the cursor on the row) */
            zq_put(ZQ_HELP, (uint32_t)(z->vrole - EN_K1), 1);
        } else if (z->kind == TESS_ZONE_POINT && (z->row & 0x200) && zone_live(z)) {
            zq_turn(z->vrole, -1);                    /* (BOUNCE's picture: a right-click a ball less) */
        } else if (z->kind == TESS_ZONE_CHORD) {
            core_chord_user(z->row, action, 0);       /* (CHORD USER's card: a right-click takes the chord) */
#if TESSERA_TRACKS > 4
        } else if (z->kind == TESS_ZONE_EGRID) {
            core_egrid(z->row, action);               /* (ENTRIES' grid: a right-click, AUTO) */
#endif
        } else if (z->kind == TESS_ZONE_MUTE && z->role == 1 && z->row >= 0 && z->row < (int32_t)NTRK) {
            help_unpin();                             /* MIXER's MUTE word: a right-click solos the track (again: */
            core_mute_solo(TESS_ZONE_SOLO, (uint32_t)z->row);   /* off), as a click mutes it */
            ui.force = 1;
        }
        return;
    }
    if (uz_help_pin && help.on && z->kind == TESS_ZONE_SCROLL && (action == TESS_ZA_WHEEL || action == TESS_ZA_TURN)) {
        (void)help_knob(help.knob, action == TESS_ZA_WHEEL ? -n : n);   /* (the card's text: as ARP + its knob) */
        return;
    }
    if (action == TESS_ZA_WHEEL && z->kind == TESS_ZONE_ROW) {   /* a list under the wheel: it moves, a row a notch
                                                                  * (up: up the list, its knob anticlockwise) */
        zq_turn(z->role, -n);
        return;
    }
    if (action == TESS_ZA_WHEEL && (z->kind == TESS_ZONE_SCROLL || z->group == UZ_G_SIDES ||
                                    (z->group == UZ_G_PIC && !ui.home && cur_page()->graph == GR_BROWSE))) {
        action = TESS_ZA_TURN;                        /* (a text, the browser's and SCALES' two lists: wheel up, up */
        n = -n;                                       /* them, as every list) */
    } else if (action == TESS_ZA_WHEEL) {             /* (anything else: as a turn; a marker: up-down) */
        action = z->kind == TESS_ZONE_POINT ? TESS_ZA_TURN_Y : TESS_ZA_TURN;
    }
    switch (z->kind) {
    case TESS_ZONE_KNOB:
        if (action == TESS_ZA_TURN) {
            zq_turn(z->role, n);
        } else if ((d = zone_two(z->role)) != 0) {
            zq_turn(z->role, d);                      /* (two values, FREE / NOTE, BI / UNI: each click the other) */
        } else if (action == TESS_ZA_CLICK && (k = zone_box_key(z->role)) >= 0) {
            zq_key((uint32_t)k);                      /* ("[OCT+]": the key it names, pressed) */
        } else if (action == TESS_ZA_CLICK && (d = zone_box(z->role)) != 0) {
            zq_turn(z->role, d);                      /* (a boxed value: the next one, round to the first) */
        } else if (action == TESS_ZA_DOUBLE) {
            zone_reset(z->role);
        }
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
        else if (action == TESS_ZA_CLICK && pop.on == POP_PICK && uz_pick_pin)
            zq_put(ZQ_PICK_DONE, 0, 1);               /* (a picker the mouse opened: the row taken, it goes) */
        break;
    case TESS_ZONE_KEY:
        if (action != TESS_ZA_TURN && z->row >= 0 && z->row < (int32_t)NB)
            zq_key((uint32_t)z->row);
        break;
    case TESS_ZONE_TRACK:
        if (action != TESS_ZA_TURN)
            tess_core_track_select((uint32_t)z->row);
        if (action == TESS_ZA_DOUBLE && ui.home)
            zq_put(ZQ_SOUNDS, 0, 1);                  /* (HOME: a double-click, and its sounds) */
        break;
    case TESS_ZONE_POINT:                             /* a picture's marker: left-right one knob, up-down another */
        if ((z->row & 0x400) && action == TESS_ZA_TURN)   /* (up-down only: CONSTELLATION's ship, its brackets) */
            break;
        if (z->row & 0x200) {                         /* (a picture of one knob, BOUNCE's BALLS: a click one more,
                                                       * a right-click one less, the wheel or a drag up and down) */
            if (action == TESS_ZA_CLICK || action == TESS_ZA_DOUBLE) zq_turn(z->vrole, 1);
            else if (action == TESS_ZA_HELP) zq_turn(z->vrole, -1);
            else if (action == TESS_ZA_TURN_Y) zq_turn(z->vrole, n);
            break;
        }
        if (z->row & 0x100) {                         /* (PIECE's SHAPE USER: point row & 7's level, up and down) */
            if (action == TESS_ZA_TURN_Y)
                tessera_piece_knob((uint32_t)z->row & 3u, n, 1 + (((uint32_t)z->row >> 2) & 1u));
            ui.force = 1;
            break;
        }
        if (action == TESS_ZA_DOUBLE) {
            zone_reset(z->role);
            zone_reset(z->vrole);
        } else if (action == TESS_ZA_TURN && z->role >= 0) {
            zq_turn(z->role, (z->row & 1) ? -n : n);
        } else if (action == TESS_ZA_TURN_Y || action == TESS_ZA_TURN) {
            if (z->vrole >= 0) zq_turn(z->vrole, (z->row & 2) ? -n : n);   /* (row bit 1: up turns it down, a
                                                                             * picture drawn with its values downward) */
            else if (z->role >= 0) zq_turn(z->role, (z->row & 1) ? -n : n);
        }
        break;
    case TESS_ZONE_OP:                                /* FM6's chart: an operator picked, or its pages opened */
        if (z->row < 0 || z->row > 5 || (action != TESS_ZA_CLICK && action != TESS_ZA_DOUBLE)) break;
        fm6_opsel = (uint8_t)z->row;
        if (action == TESS_ZA_DOUBLE && sec_on()) {
            sec_build();
            for (k = 0; k < (int32_t)sec.n; k++)
                if (sec.op[k] == (uint8_t)(z->row + 1)) { sec_go((uint32_t)k); break; }
        }
        ui.force = 1;
        break;
    case TESS_ZONE_PICK:                              /* MODS' SOURCE / DESTINATION: the mod's row first (KNOB 2), */
        d = z->row - uz_list.sel;                     /* then its knob: a click its picker, kept; a turn its value */
        for (k = 0; k < (d < 0 ? -d : d) && k < 64; k++)
            zq_put(ZQ_STEP, (uint32_t)z->vrole, d);
        if (d) {
            uz_list.sel = z->row;
            uz_list_sel_pending = 1;
        }
        if (action == TESS_ZA_TURN)
            zq_turn(z->role, n);
        else if (z->role >= EN_K1 && z->role <= EN_K4)
            zq_put(ZQ_PICK, (uint32_t)(z->role - EN_K1), 1);
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
    case TESS_ZONE_CHORD:                             /* CHORD USER's cards: a root turned, a chord added or taken */
        core_chord_user(z->row, action, n);
        break;
#if TESSERA_TRACKS > 4
    case TESS_ZONE_EGRID:                             /* ENTRIES' grid: a cell's next kind, or a column's */
        core_egrid(z->row, action);
        break;
#endif
    case TESS_ZONE_SMAP:                              /* EDIT's map: the tile's page (its row and tile, then OCT+) */
        if (smap.on && (action == TESS_ZA_CLICK || action == TESS_ZA_DOUBLE) && z->row >= 0) {
            sec_build();
            if ((uint32_t)z->row / 8u < sec.n) {
                smap.row = (uint8_t)(z->row / 8);
                smap.col = (uint8_t)(z->row % 8);
                smap_input(0, 0, 2u);
            }
        }
        break;
    case TESS_ZONE_TAB:                               /* the footer's tabs: that page (row: the pages from here) */
        if (action == TESS_ZA_CLICK)
            for (k = 0; k < (z->row < 0 ? -z->row : z->row) && k < 16; k++) zq_put(ZQ_PAGE, 0, z->row);
        break;
    case TESS_ZONE_TEMPO:                             /* the header's BPM: as SELECT on HOME, a step a detent */
        if (!ui.menu && !ui.confirm && !pop.on)
            core_tempo(action, n);
        break;
    case TESS_ZONE_SCROLL:                            /* a text to read: its knob, the wheel the other way round */
        if (action == TESS_ZA_TURN)
            zq_turn(z->role, n);
        break;
    case TESS_ZONE_LEVEL:                             /* MIXER's strips: a track's level (role 0..2: FX SEND 1..3, the
                                                       * inspector's), pan */
    case TESS_ZONE_PAN:
        if (z->row >= 0 && z->row < (int32_t)NTRK && (action == TESS_ZA_TURN || action == TESS_ZA_DOUBLE)) {
            uint32_t p = z->kind == TESS_ZONE_PAN ? (uint32_t)P_PAN : z->role >= 0 && z->role < (int)FX_SLOTS ?
                         (uint32_t)P_FXSEND + (uint32_t)z->role : (uint32_t)P_LEVEL;
            const param_desc_t *pd = pid_desc(p, (uint32_t)z->row);
            int32_t v = action == TESS_ZA_DOUBLE ? pd->def : (int32_t)trk[z->row].p[p] + n;
            pid_set(p, (uint32_t)z->row, v < pd->min ? pd->min : v > pd->max ? pd->max : v);
            ui.force = 1;
        }
        break;
    case TESS_ZONE_CELL:                              /* a place in a grid: its knobs turned to it, a detent each */
        if (action == TESS_ZA_LINK) {                 /* (MATRIX: a cell dragged onto this one, ui_modsc.c) */
            if (z->group == UZ_G_PIC && !ui.home && cur_page()->graph == GR_MATRIX)
                mx_link((int32_t)mx_from + n, (int32_t)mx_to + (int8_t)(uint8_t)((uint16_t)z->row >> 8));
            break;
        }
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
            if (z->group == UZ_G_PIC && !ui.home && cur_page()->graph == GR_MATRIX)
                zq_key(B_OCTUP);                      /* (MATRIX: and OPEN, the cell's mod in MODS) */

        }
        break;
    }
#else
    (void)z, (void)action, (void)n;
#endif
}

#if TESS_HAS_POINTER && !MELODEE_ACCESS
static tess_zone_t hover_z;
static char hover_txt[384];
/* z's help text into hover_txt, as its knob's card (help_find) would have it, the card's own state left as it was; a
 * list's row: its own help, the list's cursor put there for the asking and back. Nothing while a card shows (its text
 * may live in the same buffer, pid.c) */
static void hover_find(const tess_zone_t *z)
{
    int32_t k = z->kind == TESS_ZONE_KNOB ? z->role : z->kind == TESS_ZONE_ROW ? z->vrole : -1;
    hover_txt[0] = 0;
    if (help.on || k < EN_K1 || k > EN_K4 || ui.menu || ui.confirm)
        return;
    {
        uint8_t was_help[sizeof help], row = lst.row;
        uint32_t tr = help_tr;
        memcpy(was_help, &help, sizeof help);
        if (z->kind == TESS_ZONE_ROW && list_on() && z->row >= 0)
            lst.row = (uint8_t)z->row;
        help_find((uint32_t)(k - EN_K1));
        str_cpy(hover_txt, help_text(), sizeof hover_txt);
        lst.row = row;
        help_tr = tr;
        memcpy(&help, was_help, sizeof help);
    }
}
#endif
void tess_core_hover(const tess_zone_t *z)
{
#if TESS_HAS_POINTER && !MELODEE_ACCESS
    memset(&hover_z, 0, sizeof hover_z);
    hover_txt[0] = 0;
    if (z && zone_live(z)) {
        hover_z = *z;
        hover_find(z);
    }
#else
    (void)z;
#endif
}

void tess_core_away(void)
{
#if TESS_HAS_POINTER
    if (ui.confirm || help_unpin())               /* (a YES / NO stays; a pinned card: closed, and no more) */
        return;
    if (pop.on)
        zq_key(B_OCTDN);                          /* a picker, a sheet: as BACK (OCT-) */
#endif
}

void tess_core_key_words(int on)
{
#if TESS_HAS_POINTER
    if ((uint8_t)(on != 0) != uz_key_words) {
        uz_key_words = (uint8_t)(on != 0);
        ui.force = 1;                                 /* (the hints again, in the other words) */
    }
    ui_csec = uz_key_words;                           /* (the shell's own section buttons: ui.c cs_open) */
#else
    (void)on;
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
#if TESS_HAS_POINTER
static uint32_t host_extra;                           /* SHIFT, OCTAVE DOWN / UP held (labels' TESS_BTN_* bits) */
#endif
void tess_core_buttons(uint32_t labels)
{
    uint32_t m = 0, b;
    for (b = 0; b < NB; b++)
        if ((labels >> b) & 1u)
            m |= 1u << panel.btn[b];
    host_pressed |= m & ~fm1_in.buttons;
#if TESS_HAS_POINTER
    {   /* Smalti's own buttons: SHIFT holds OCT- with no press (no edge: never BACK, never an octave; its release no
         * tap), the knobs' second layer; OCTAVE DOWN / UP step the keys' octave, on any page, as OCT- / OCT+ do on
         * Stage (both: back to 0) */
        uint32_t ex = labels & (TESS_BTN_SHIFT | TESS_BTN_OCT_DOWN | TESS_BTN_OCT_UP | TESS_BTN_HELP |
                                TESS_BTN_SECTION(TESS_SEC_N) - TESS_BTN_SECTION(0)),
                 go = ex & ~host_extra;
        _Static_assert(TESS_SEC_N == UI_SEC_N && TESS_SEC_SETUP == UI_SEC_SETUP && TESS_BTN_SECTION(TESS_SEC_N - 1) < (1ull << 32) &&
                       TESS_BTN_SECTION(0) > TESS_BTN_POINTER, "the sections: ui.c's, bits above POINTER");
        for (b = 0; b < TESS_SEC_N; b++)              /* a section's button: its pages (ui.c cs_open; a pinned card */
            if (go & TESS_BTN_SECTION(b)) {           /* closes first) */
                help_unpin();
                zq_put(ZQ_SECTION, b, 1);
            }
        if (go & TESS_BTN_HELP) {                     /* ?: KNOB 1's help card (turn another knob for its), or it closes */
            if (!help_unpin())
                zq_put(ZQ_HELP, 0, 1);
        } else if (m & ~fm1_in.buttons) {
            help_unpin();                             /* (a button pressed: a pinned card closes, the press goes on) */
        }
        if (ex & TESS_BTN_SHIFT)
            m |= 1u << panel.btn[B_OCTDN];
        if ((ex ^ host_extra) & TESS_BTN_SHIFT)
            ui.force = 1;
        if (go & (TESS_BTN_OCT_DOWN | TESS_BTN_OCT_UP)) {
            if ((ex & (TESS_BTN_OCT_DOWN | TESS_BTN_OCT_UP)) == (TESS_BTN_OCT_DOWN | TESS_BTN_OCT_UP))
                song.octave = 0;
            else if (go & TESS_BTN_OCT_DOWN)
                song.octave -= song.octave > -3 ? 1 : 0;
            else
                song.octave += song.octave < 3 ? 1 : 0;
            ui.force = 1;
        }
        host_extra = ex;
    }
    if (m & ~fm1_in.buttons)                          /* (who pressed: ui_layer.c ly_ptr) */
        ly_ptr = labels & TESS_BTN_POINTER ? ly_ptr | (m & ~fm1_in.buttons) : ly_ptr & ~(m & ~fm1_in.buttons);
    host_btn = m;
    fm1_in.buttons = m | zone_btn;                /* (a key a zone's click holds a moment: tess_core_zone_do) */
#else
    fm1_in.buttons = m;
#endif
}
void tess_core_keys(uint32_t mask)
{
    mask &= (1u << TESS_KEYS) - 1u;
#if TESS_HAS_POINTER
    if (mask & ~fm1_in.notes)
        help_unpin();                                 /* (a key played: a pinned help card closes) */
#endif
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
#if TESS_HAS_POINTER
/* the keys' jobs on the page shown (tessera-notes docs/analysis/plugin-key-roles.md), for a front end's labels over the
 * keys: sh 1-3 characters, lg the hover's meaning; both empty where the keys just play. The same tests in the same order
 * as ui_input.c gives the keys to NAME, a held layer (MIXER, PERF, LANDSCAPE: GLO's), the pattern keys, AWAKE, CHORD
 * USER, ATLAS's PART 4, the DRUM grid and a USER scale */
static void key_put(char *sh, char *lg, const char *a, const char *b, const char *c)   /* lg: a, b and c joined */
{
    str_cpy(sh, a, 4);
    lg[0] = 0;
    str_cpy(lg, b, 16);
    if (c)
        str_cpy(lg + str_len(lg), c, 16 - str_len(lg));
}
static void key_legend(uint32_t k, char *sh, char *lg)
{
    uint32_t p = key_place(k), blk = (uint32_t)key_black(k), l = layer_held() ? ui.ly : mixer_live() ? LAYER_GLO : 0u;
    char a[8], b[16];
    sh[0] = lg[0] = 0;
    if (name_on()) {
        static const char *const NB[] = {"<", "SPC", ">", "DEL", "MOD"}, *const NL[] = {"CURSOR LEFT", "SPACE",
                                         "CURSOR RIGHT", "DELETE", "ABC / 123"};
        uint32_t f = blk ? nm_black(k) : (uint32_t)NB_NONE;
        _Static_assert(NB_LEFT == 0 && NB_SPACE == 1 && NB_RIGHT == 2 && NB_DEL == 3 && NB_MODE == 4 && NB_NONE == 5,
                       "ui_name.c's black keys in NB's order");
        if (!blk && p < 16u)
            key_put(sh, lg, nm_group(p), "TYPE ", nm_group(p));
        else if (f < NB_NONE)
            key_put(sh, lg, NB[f], NL[f], 0);
        return;
    }
    if (l == LAYER_GLO) {
        static const char *const W[4] = {"ALL", "HLD", "FIL", "TAP"}, *const WL[4] = {"UNMUTE ALL", "HOLD (2X: LATCH)",
                                         "FILL (HELD)", "TAP TEMPO"};
        uint32_t t = glo_key_trk(k);
        a[0] = blk ? 'M' : 'S', a[1] = (char)('1' + t), a[2] = 0;
        b[0] = 'T', b[1] = a[1], b[2] = 0;
        if (t < NTRK)
            key_put(sh, lg, a, b, blk ? " MUTE" : " SOLO (HELD)");
        else if (!blk && p >= 4u && p < 8u)
            key_put(sh, lg, W[p - 4u], WL[p - 4u], 0);
        return;
    }
    if (l == LAYER_SCL) {
        key_put(sh, lg, N_NOTE[(k + 5u) % 12u], "ROOT ", N_NOTE[(k + 5u) % 12u]);
        return;
    }
    if (l)
        return;
    if (pattern_keys_on()) {
        for (uint32_t i = 0; i < NPAT; i++)
            if (PAT_KEY[i] == k) {
                a[0] = 'P', a[1] = (char)('1' + i), a[2] = 0;
                key_put(sh, lg, a, "PATTERN ", a + 1);
            }
        return;
    }
    if (awake_entry_on()) {
        if (!blk && p < 16u) {
            fmt_int(a, (int32_t)p + 1);
            key_put(sh, lg, a, "STEP ", a);
        }
        return;
    }
    if (ch_user_entry_on()) {
        key_put(sh, lg, N_NOTE[(k + 5u) % 12u], "NEXT CHORD ON ", N_NOTE[(k + 5u) % 12u]);
        return;
    }
    if (gr_layer_on()) {
        for (uint32_t i = 0; blk && p < NLANE && i < GEN_GRIDS_P4_N; i++)
            if (GEN_GRIDS_P4_NOTE[i] == DRUM_LANE_NOTE[p])
                key_put(sh, lg, drum_lane_abbr(TSEL, p), "PART 4: ", drum_lane_name(TSEL, p));
        return;
    }
    if (song.grid == 1u) {                                /* the DRUM grid (grid_keys) */
        if (!blk) {
            uint32_t i = ui.bank * 16u + p;
            if (i < (uint32_t)TSEL->p[P_SLEN]) {
                fmt_int(a, (int32_t)i + 1);
                key_put(sh, lg, a, "STEP ", a);
            }
        } else if (p < NLANE) {
            key_put(sh, lg, drum_lane_abbr(TSEL, p), "LANE ", drum_lane_name(TSEL, p));
        } else if (p == GK_ACC) {
            key_put(sh, lg, "ACC", "ACCENT (HELD)", 0);
        } else {
            key_put(sh, lg, p == GK_PGUP ? "PG>" : "<PG", p == GK_PGUP ? "NEXT PAGE" : "PAGE BEFORE", 0);
        }
        return;
    }
    if (uscale_edit()) {                                  /* a USER scale: each key's note in or out (the root stays) */
        uint32_t d = (53u + k + 120u - (uint32_t)TSEL->p[P_ROOT]) % 12u;
        uint32_t in = (uscale_mask[(uscale_sel - 1u) % USCALE_SLOTS] >> d) & 1u;
        str_cpy(b, N_NOTE[(53u + k) % 12u], sizeof b);
        key_put(sh, lg, !d ? "R" : in ? "IN" : "-", b, !d ? " ROOT" : in ? " IN: TAKE OUT" : " OUT: ADD");
    }
}

/* the keys and the scale (Cosmati audit G10): where the keys play the selected track's notes, the ones that give no new
 * note in its scale dim and the root's marked (tess_view_t.key_dim / key_root); the CHORD track's chord named */
static void key_scale(tess_view_t *v)
{
    const track_t *t = TSEL;
    const engine_t *e = ENGINES[eng_idx(t->eng_req)];
    uint32_t k, n, mask, last = KB_SILENT, pcs = tessera_chord_pcs(), r = tessera_chord_root() % 12u;
    v->key_dim = v->key_root = 0;
    v->key_chord[0] = 0;
    if (pcs)
        chord_name(v->key_chord, r, ((pcs >> r) | (pcs << (12u - r))) & 0xFFFu);
    for (k = 0; k < TESS_KEYS; k++)
        if (v->key_short[k][0])
            return;                                     /* (the keys have jobs on this page) */
    if (name_on() || layer_held() || mixer_live() || pattern_keys_on() || awake_entry_on() || ch_user_entry_on() ||
        gr_layer_on() || song.grid == 1u || uscale_edit() || micro_active(t) || (e->keys && e->keys(t, 0) >= 0))
        return;
    mask = scale_mask(t);
    for (k = 0; k < TESS_KEYS; k++) {
        n = kb_map(t, k);
        if (n == KB_SILENT || n == last || !((mask >> ((n + 120u - (uint32_t)t->p[P_ROOT]) % 12u)) & 1u))
            v->key_dim |= 1u << k;
        else if ((n + 120u - (uint32_t)t->p[P_ROOT]) % 12u == 0u)
            v->key_root |= 1u << k;
        if (n != KB_SILENT)
            last = n;
    }
}
#endif
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
#if TESS_HAS_POINTER
    if (ui_csec && !ui.home && cs_of(ui.page) < UI_SEC_N)   /* (the shown page's section) */
        v->lit_buttons |= TESS_BTN_SECTION(cs_of(ui.page));
#endif
    v->lit_keys = led_keys(fm1_led);
    v->dim_buttons = led_buttons(led_dim_plane());
    v->dim_keys = led_keys(led_dim_plane());
    v->breath_buttons = led_buttons(fm1_led_dim[2]);
    v->breath_keys = led_keys(fm1_led_dim[2]);
#if TESS_HAS_POINTER
    if (uz_key_words && !oct_nav()) {             /* Stage, NOTES: the OCT LEDs show the octave: on the front end's
                                                   * own OCTAVE DOWN / UP, its BACK and OK dark there */
        uint32_t oct = 3u << 12;
        v->lit_buttons = (v->lit_buttons & ~oct) | ((v->lit_buttons >> 12) & 3u) << 17;
        v->dim_buttons &= ~oct;
        v->breath_buttons &= ~oct;
    }
#endif
#if TESS_HAS_POINTER
    for (i = 0; i < TESS_KEYS; i++)
        key_legend(i, v->key_short[i], v->key_long[i]);
    key_scale(v);
#else
    for (i = 0; i < TESS_KEYS; i++)
        v->key_short[i][0] = v->key_long[i][0] = 0;
    v->key_dim = v->key_root = 0;
    v->key_chord[0] = 0;
#endif
    v->screen_draws = (uint32_t)host_sent;
    v->playing = song.playing;
    v->master = tess_core_master_get();
    v->ink = settings.palette;
    v->track = song.sel;
    v->zone_count = 0;
    v->typing = 0;
    v->closable = 0;
    v->sheet = 0;
#if TESS_HAS_POINTER && !MELODEE_ACCESS
    v->hover = hover_z;
    str_cpy(v->hover_help, hover_txt, sizeof v->hover_help);
#else
    memset(&v->hover, 0, sizeof v->hover);
    v->hover_help[0] = 0;
#endif
    v->zone_gen = 0;
#if TESS_HAS_POINTER
    {                                             /* the zones, in the order a hit is looked for: a list over the
                                                   * page, a dialog's or a screen's keys, the page's columns,
                                                   * MIXER's strips, a screen's two lists, HOME's tracks */
        static const uint8_t ORDER[] = {UZ_G_LIST, UZ_G_KEYS, UZ_G_PIC, UZ_G_COLS, UZ_G_STRIPS, UZ_G_SIDES, UZ_G_TRACKS,
                                        UZ_G_TABS, UZ_G_HEAD};
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
        for (i = 0; i < 4u; i++)                      /* KNOB 1..4: the value, and where the mods have it now */
            v->knob_at[i] = v->knob_mod[i] = -1;
        for (i = 0; i < n; i++) {
            const tess_zone_t *z = &v->zones[i];
            uint32_t pid, tr;
            tess_param_t in;
            int32_t at, sh;
            if (z->kind != TESS_ZONE_KNOB || z->role < EN_K1 || z->role > EN_K4 || !tess_core_zone_param(z, &pid, &tr) ||
                !tess_core_param_info(pid, tr, &in) || in.max <= in.min)
                continue;
            at = (pid_get(pid, tr) - in.min) * 1000 / (in.max - in.min);
            v->knob_at[z->role - EN_K1] = (int16_t)clamp(at, 0, 1000);
            if (cord_moves(pid, tr, &sh))
                v->knob_mod[z->role - EN_K1] = (int16_t)clamp(at + (int32_t)(((int64_t)sh * 1000) >> 15), 0, 1000);
        }
        v->locked = 0;
        for (i = 0; lk_n && i < n; i++) {             /* (a value LOCKed: its bit) */
            uint32_t pid, tr;
            if (tess_core_zone_param(&v->zones[i], &pid, &tr) && lk_at(pid, tr) >= 0)
                v->locked |= (uint64_t)1u << i;
        }
        v->dice_back = dk_n ? 1u : 0u;
        v->typing = name_on() ? 1u : 0u;
        v->closable = !ui.confirm && (help.on || pop.on) ? 1u : 0u;
        v->sheet = page_sheet() != 0u;
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
                   TESS_ZONE_PAN == T2_Z_PAN && TESS_ZONE_TRACK == UZ_TRACK, "tess_core.h's TRACKS zones are ui_tracks.c's");
    tracks_draw((int32_t)(w < 1u ? 1u : w), (int32_t)(h < 1u ? 1u : h), (fm1_ms / 250u) & 1u);
    if (w >= TESS_TRACKS_W) t2_seen = 1, t2_seen_ms = fm1_ms;   /* (the large one: MIXER's strips there, ui.c mix_inspect) */
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
    if (z && z->kind == TESS_ZONE_TEMPO) {            /* (the header's tempo: dragged, the wheel, double-click) */
        core_tempo(action, n);
        return;
    }
    if (tr >= NTRK)
        return;
#if TESS_HAS_POINTER
    if (z->kind == TESS_ZONE_TRACK && action == TESS_ZA_SOUND) {
        browse_drop(n, tr);                           /* (a browser's sound dropped on the row: ui_browser.c) */
        return;
    }
#endif
    if (z->kind == TESS_ZONE_TRACK && click) {
        tess_core_track_select(tr);
#if TESS_HAS_POINTER
        if (action == TESS_ZA_DOUBLE)
            zq_put(ZQ_EDIT, 0, 1);                    /* (a double-click: and its EDIT page) */
#endif
    } else if ((z->kind == TESS_ZONE_MUTE || z->kind == TESS_ZONE_SOLO) && click) {
        core_mute_solo(z->kind, tr);
    } else if (z->kind == TESS_ZONE_LEVEL || z->kind == TESS_ZONE_PAN) {   /* (the level, a send, PAN: turned; a
                                                                            * double-click its default) */
        uint32_t p = z->kind == TESS_ZONE_PAN ? (uint32_t)P_PAN : z->role >= 0 && z->role < (int)FX_SLOTS ?
                     (uint32_t)P_FXSEND + (uint32_t)z->role : (uint32_t)P_LEVEL;
        int32_t v = trk[tr].p[p] + n;
        if (action == TESS_ZA_DOUBLE) v = TP[p].def;
        else if (click || !n) return;
        pid_set(p, tr, v < TP[p].min ? TP[p].min : v > TP[p].max ? TP[p].max : v);
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
#if MELODEE_SMF
_Static_assert(sizeof(tess_smf_ev) == sizeof(smf_ev), "tess_smf_ev is smf.c's smf_ev");
uint32_t tess_core_smf(const tess_smf_ev *ev, uint32_t n, uint32_t bpm, const char *name, uint32_t bars, uint8_t *out,
                       uint32_t cap)
{
    return smf_write((const smf_ev *)ev, n, bpm, name, bars, out, cap);
}
#else
uint32_t tess_core_smf(const tess_smf_ev *ev, uint32_t n, uint32_t bpm, const char *name, uint32_t bars, uint8_t *out,
                       uint32_t cap)
{
    (void)ev, (void)n, (void)bpm, (void)name, (void)bars, (void)out, (void)cap;
    return 0;
}
#endif

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

/* ---- a value typed (Cosmati audit G1: Ctrl / Cmd-click a value, type it) */
#if TESS_HAS_POINTER
static void tt_norm(const char *s, char *d, uint32_t cap)   /* upper case, no spaces: "-6 dB" == "-6DB" */
{
    uint32_t n = 0;
    for (; *s && n + 1u < cap; s++)
        if (*s != ' ')
            d[n++] = *s >= 'a' && *s <= 'z' ? (char)(*s - 32) : *s;
    d[n] = 0;
}
static int tt_num(const char *s, int32_t *x1000)        /* a number at s's start (sign, digits, a point): x 1000 */
{
    int32_t v = 0, f = 0, fd = 1, neg = 0, any = 0;
    if (*s == '+' || *s == '-') neg = *s++ == '-';
    for (; *s >= '0' && *s <= '9'; s++) any = 1, v = v > 2000000 ? v : v * 10 + (*s - '0');
    if (*s == '.')
        for (s++; *s >= '0' && *s <= '9'; s++)
            if (fd < 1000) any = 1, f = f * 10 + (*s - '0'), fd *= 10;
    if (!any) return 0;
    *x1000 = (v * 1000 + f * 1000 / fd) * (neg ? -1 : 1);
    return 1;
}
#endif
uint32_t tess_core_zone_param(const tess_zone_t *z, uint32_t *pid, uint32_t *track)
{
#if TESS_HAS_POINTER
    if (!z || !zone_live(z))
        return 0;
    if ((z->kind == TESS_ZONE_LEVEL || z->kind == TESS_ZONE_PAN) && z->row >= 0 && z->row < (int32_t)NTRK) {
        *pid = PID_TRACK(z->kind == TESS_ZONE_PAN ? P_PAN : z->role >= 0 && z->role < (int)FX_SLOTS ?
                         (uint32_t)P_FXSEND + (uint32_t)z->role : (uint32_t)P_LEVEL);
        *track = (uint32_t)z->row;
        return 1;
    }
    if (z->kind == TESS_ZONE_KNOB && z->role >= EN_K1 && z->role <= EN_K4) {
        uint8_t keep[sizeof help];                    /* (the help card's state as it was: only asked, not shown) */
        uint32_t tr0 = help_tr, ok;
        memcpy(keep, &help, sizeof help);
        help_find((uint32_t)(z->role - EN_K1));
        ok = help.pid != PID_NONE && pid_find(help.pid) != 0;
        *pid = help.pid, *track = help_tr % NTRK;
        memcpy(&help, keep, sizeof help);
        help_tr = tr0;
        return ok;
    }
#else
    (void)z, (void)pid, (void)track;
#endif
    return 0;
}
uint32_t tess_core_zone_type(const tess_zone_t *z, const char *text)
{
#if TESS_HAS_POINTER
    uint32_t pid, tr;
    tess_param_t info;
    char want[24], got[24], t[24];
    int32_t v, best = 0, num, x, bd = -1;
    int has_num;
    if (z && text && z->kind == TESS_ZONE_CELL && !strcmp(z->name, "FIND")) {   /* the browser's FIND: the list keeps
                                                                                 * the sounds named so (browse.c) */
        uint32_t i;
        if (ui.home || cur_page()->graph != GR_BROWSE)
            return 1;
        for (i = 0; text[i] && i + 1u < sizeof br_find; i++)
            br_find[i] = text[i] >= 'a' && text[i] <= 'z' ? (char)(text[i] - 32) : text[i];
        br_find[i] = 0;
        brw.on = 0;                                   /* (a sound on its way: its place was in the list before) */
        ui.force = 1;
        return 0;
    }
    if (!text || !tess_core_zone_param(z, &pid, &tr) || !tess_core_param_info(pid, tr, &info) || !info.settable)
        return 1;
    tt_norm(text, want, sizeof want);
    if (!want[0])
        return 1;
    has_num = tt_num(want, &num);
    for (v = info.min; v <= info.max && v - info.min < 65536; v++) {   /* a name or a value as shown, else the nearest number */
        tess_core_param_text(pid, tr, v, t, sizeof t);
        tt_norm(t, got, sizeof got);
        if (!strcmp(got, want)) {
            bd = 0, best = v;
            break;
        }
        if (has_num && tt_num(got, &x)) {
            int32_t d = x > num ? x - num : num - x;
            if (bd < 0 || d < bd) bd = d, best = v;
        }
    }
    if (bd < 0 || !tess_core_param_set(pid, tr, best))
        return 1;
    ui.force = 1;
    return 0;
#else
    (void)z, (void)text;
    return 1;
#endif
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
#include "tess_track.c"                                /* a track copied, pasted, moved */
