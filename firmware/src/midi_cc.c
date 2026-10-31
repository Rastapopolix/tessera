/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* MIDI CC control: external gear (a sequencer, a controller, norns / crow, a DAW) sets a track's parameters on the
 * track's ROUT channel (seq.c midi_track), after midi_control.c's own CCs. Included by seq.c before midi_control.c.
 *   the fixed map (MANUAL.md, MIDI CC; GM's meaning where GM has one): 16..19 the engine's HOME knobs, 20..27 E1..E8,
 *   76 LFO RATE, 77 / 78 LFO to pitch / filter, 79 SUSTAIN, 91 / 93 / 94 FX SEND 1..3, 102 MODE, 103..105 the GEN
 *   page's KNOB 2..4 for the track's MODE (what the page shows)
 *   NRPN: any parameter by its central id (pid.h; docs/PARAMETERS.md): 99 the id / 128, 98 the id % 128, then data
 *   entry 6 (7 bits) and 38 (its 14-bit refinement); a global setting from any routed channel
 * A value spans the parameter's range as 0..127 (14 bits: 0..16383); MOTION records it as a knob, cords and MOD add on
 * top. Not the device's own setup (MIDI input, CLK, ROUT: a controller could cut itself off), nor a button (LOAD ..),
 * nor the song's scale (SCL, QNT, the MPC degree, a linked ROOT: ui.c scale_share keeps them alike on every track).
 * MIDI CC IN OFF (Settings): none of these, nor midi_control.c's parameter CCs (5 7 10 71..75); the playing ones (bend,
 * wheel, expression, sustain, aftertouch, RPN, 120..123) always act. */
#include "pid.h"

static uint8_t settings_cc_off;    /* MIDI CC IN OFF (Settings; saved: favorites.factory[15][24] bit 4) */

/* the GEN page's KNOB 2..4 for each MODE (TX_*, 0xFF none): CC 103..105 and the help (ui_help.c) */
static const uint8_t GEN_KNOB_TX[TESS_ENG_COUNT][3] = {{0xFF, 0xFF, 0xFF}, {TX_TUR_LEN, TX_TUR_PROB, TX_TUR_SPAN},
    {TX_EUC_STEPS, TX_EUC_PULSES, TX_EUC_ROT}, {TX_MKV_LEAP, TX_MKV_DENS, TX_MKV_SPAN}, {TX_MRP_POS, TX_MRP_SNPA, TX_MRP_SNPB},
    {TX_GRD_X, TX_GRD_Y, TX_GRD_DENS}, {TX_BNC_SPEED, TX_BNC_ANGLE, TX_BNC_BALLS},
    {TX_AWK_LENA, TX_AWK_LENB, TX_AWK_PROB}, {0xFF, TX_POL_DIV1, TX_POL_SPAN},
    {TX_QD_DIR, TX_QD_PROB, TX_QD_LINK}, {TX_CS_Y, TX_CS_DENS, TX_CS_AUTO},
    {TX_SR_START, TX_SR_INTV, TX_SR_LOOP}, {TX_CH_PROG, TX_CH_BARS, TX_CH_HIT}, {TX_AN_VARY, TX_AN_ECHO, TX_AN_FADE}};

/* a channel's NRPN: its id's two halves + 1 (0: not chosen), its data entry MSB */
static struct { uint8_t msb, lsb, data; } midi_nrpn[16];

/* parameter pid of track t to v of 0..full (127, or 16383 for 14 bits) across its range */
static void midi_pid(track_t *t, uint32_t pid, uint32_t v, uint32_t full)
{
    const pid_entry_t *e = pid_find(pid);
    const param_desc_t *d;
    uint32_t tr = trk_index(t);
    int32_t x;
    if (!e || (e->flags & (PID_F_ACTION | PID_F_DEVICE)) || !(d = pid_desc(pid, tr)) || d->max <= d->min)
        return;
    x = d->min + (int32_t)((uint32_t)(d->max - d->min) * v / full);
    if (pid >= PID_TESS(0)) {
        tessera_param_set(tr, pid - PID_TESS(0), x);
        return;
    }
    x = enum_orig(d, x);
    if (pid >= PID_GLOBAL(0)) {
        if (pid != PID_GLOBAL(G_MIDI) && pid != PID_GLOBAL(G_CLOCK) && pid != PID_GLOBAL(G_ROUTE))
            song.g[pid - PID_GLOBAL(0)] = (int16_t)x;
        return;
    }
    if (((chain.running || chain.armed) && pid >= P_SLEN && pid <= P_SGATE)   /* (a chain plays: as the editor's SET) */
        || pid == P_SCALE || pid == P_QUANT || pid == P_MPCDEG || (pid == P_ROOT && tessera.key_link))
        return;
    t->p[pid] = (int16_t)x;
    motion_capture(t, pid, (int16_t)x);
}

/* CC cc = v on channel ch, whose track is t (midi_control.c, unless MIDI CC IN is OFF) */
static __attribute__((noinline)) void midi_cc(track_t *t, uint32_t ch, uint32_t cc, uint32_t v)
{
    uint32_t pid = PID_NONE, tr = trk_index(t), k;
    if (cc == 99u) {
        midi_nrpn[ch].msb = (uint8_t)(v + 1u);
        midi_nrpn[ch].lsb = 0;
    } else if (cc == 98u) {
        midi_nrpn[ch].lsb = (uint8_t)(v + 1u);
    } else if (cc == 101u || cc == 100u) {             /* an RPN chosen: no NRPN */
        midi_nrpn[ch].lsb = 0;
    } else if ((cc == 6u || cc == 38u) && midi_nrpn[ch].lsb) {
        uint32_t id = (midi_nrpn[ch].msb ? midi_nrpn[ch].msb - 1u : 0u) << 7 | (midi_nrpn[ch].lsb - 1u);
        if (cc == 6u)
            midi_nrpn[ch].data = (uint8_t)v;
        if (id != 0x3FFFu)                             /* (127 / 127: the null NRPN) */
            midi_pid(t, id, cc == 6u ? v : (uint32_t)midi_nrpn[ch].data << 7 | v, cc == 6u ? 127u : 16383u);
        return;
    }
    if (cc >= 16u && cc <= 19u) {
        k = ENGINES[t->eng_req % NENGINES]->knob[cc - 16u];
        pid = k < P_COUNT ? PID_TRACK(k) : PID_NONE;
    } else if (cc >= 20u && cc <= 27u)
        pid = PID_TRACK(P_E0 + cc - 20u);
    else if (cc >= 76u && cc <= 79u)
        pid = PID_TRACK(((const uint8_t[]){P_LRATE, P_LD_PIT, P_LD_FLT, P_SUS})[cc - 76u]);
    else if (cc == 91u || cc == 93u || cc == 94u)
        pid = PID_TRACK(cc == 91u ? P_FXSEND : cc == 93u ? P_FXSEND2 : P_FXSEND3);
    else if (cc == 102u)
        pid = PID_TESS(TX_MODE);
    else if (cc >= 103u && cc <= 105u && tr < TESSERA_TRACKS) {
        k = GEN_KNOB_TX[tessera.mode[tr] % TESS_ENG_COUNT][cc - 103u];
        pid = k != 0xFFu ? PID_TESS(k) : PID_NONE;
    }
    if (pid != PID_NONE)
        midi_pid(t, pid, v, 127u);
}
