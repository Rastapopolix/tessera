/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* MIDI OUT over USB (usb.c midi_out_event): what the tracks play, each on its own channel, and the clock.
 * MIDI OUT (Settings): KEYS the notes played on the keys (as Melodee always sent them), ALL also the steps, the
 * ARP, recordings and every generator, OFF nothing. Only notes started inside the sequencer (seq_advance) are sent
 * as ALL's extra ones, so MIDI IN is never echoed back. A track's channel is its own (GLO > MIDI OUT, tessera.midi_ch:
 * 1..16, or OFF: none of its notes), or AUTO: ROUT's (CH1-4: 1..4, CH5-8: 5..8, ..; SEL: 1..4), a drum track's the
 * SYSTEM DRUM channel. A track's TRANSPOSE (OCT+ held on GLO > MIDI OUT, tessera.midi_tr) moves its notes on the way
 * out only; one moved past 0..127 is not sent. Every note sent is remembered until its note-off went out, so
 * a STOP, a panic, a channel or a TRANSPOSE change never leaves a note hanging on the other side.
 * CLOCK OUT (Settings): 24 pulses a beat with START and STOP, while CLK is INT (an external clock is not sent on). */
enum { MIDI_OUT_KEYS, MIDI_OUT_ALL, MIDI_OUT_OFF, MIDI_OUT_N };
static uint8_t settings_midi_out, settings_clock_out;   /* saved: favorites.c row 15 byte 30 (settings_persist.c) */
static uint8_t midi_out_seq;                     /* 1 inside seq_advance: the sequencer is playing these notes */
static uint32_t midi_out_held[NTRK][4];          /* the notes sent on, a bit each */
static uint8_t midi_out_chan[NTRK];              /* the channel they went out on */
static int8_t midi_out_tr[NTRK];                 /* and the TRANSPOSE (the bits are the notes as played) */
static uint8_t midi_out_clk_live;               /* CLOCK OUT sent START, not STOP yet */
static uint32_t midi_out_clk;                    /* CLOCK OUT: 24 x the samples since the last pulse; ~0u: none yet */

#define MIDI_OUT_NONE 16u                        /* midi_out_channel: the track's MIDI OUT is OFF */
static uint32_t midi_out_channel(const track_t *t)
{
    uint32_t route = (uint32_t)song.g[G_ROUTE], first = route > 1u ? (route - 1u) * 4u : 0u, k = trk_index(t);
    if (k < TESSERA_TRACKS && tessera.midi_ch[k])
        return tessera.midi_ch[k] >= TESS_MIDI_OFF ? MIDI_OUT_NONE : tessera.midi_ch[k] - 1u;
    if (settings_drumch && drum_track(t))
        return settings_drumch - 1u;
    return (first + trk_index(t)) & 15u;
}

static void midi_out_send(uint32_t ch, uint32_t note, uint32_t vel)   /* vel 0: the note-off */
{
    midi_out_event(vel ? 0x09u | (0x90u | ch) << 8 | note << 16 | vel << 24 : 0x08u | (0x80u | ch) << 8 | note << 16);
}

/* every note track k still has on goes off */
static void midi_out_flush(uint32_t k)
{
    for (uint32_t w = 0; w < 4u; w++)
        for (uint32_t b = midi_out_held[k][w]; b; b &= b - 1u)
            midi_out_send(midi_out_chan[k], (uint32_t)((int32_t)(w * 32u + (uint32_t)__builtin_ctz(b)) + midi_out_tr[k]), 0);
    memset(midi_out_held[k], 0, sizeof midi_out_held[k]);
}

/* DRUM MAP ROW: a General MIDI drum note (the drum grid's and ATLAS's: eng_drum.c DRUM_LANE_NOTE) as one note a drum
 * in a row from 36, in a drum machine's track order (BD SD RS CP BT LT MT HT CH OH CY CB); any other note as it is */
static uint32_t midi_out_map(uint32_t dm, uint32_t note)
{
    static const uint8_t GM[8] = {36, 38, 37, 39, 45, 42, 46, 56}, ROW[8] = {0, 1, 2, 3, 5, 8, 9, 11};
    if (dm == TESS_DM_ROW)
        for (uint32_t i = 0; i < 8u; i++)
            if (note == GM[i]) return 36u + ROW[i];
    return note;
}
static uint8_t midi_out_dm[NTRK];                        /* the DRUM MAP the notes held went out with */

static void midi_out_note(const track_t *t, uint32_t note, uint32_t vel)
{
    uint32_t k = trk_index(t), ch = midi_out_channel(t), bit, *w;
    uint32_t dm = k < TESSERA_TRACKS && (drum_track(t) || tessera.mode[k] == TESS_ENG_GRIDS) ? tessera.midi_dmap[k] : 0u;
    int32_t tr = k < TESSERA_TRACKS ? tessera.midi_tr[k] : 0, s;
    note = midi_out_map(vel ? dm : midi_out_dm[k], note & 127u);
    bit = 1u << (note & 31u);
    w = &midi_out_held[k][(note >> 5) & 3u];
    if (!vel) {
        if (*w & bit)
            midi_out_send(midi_out_chan[k], (uint32_t)((int32_t)note + midi_out_tr[k]), 0);
        *w &= ~bit;
        return;
    }
    if (ch != midi_out_chan[k] || tr != midi_out_tr[k] || dm != midi_out_dm[k]) {   /* the channel, TRANSPOSE or DRUM
                                                                                      * MAP changed: end the notes sent */
        midi_out_flush(k);                                    /* with the old ones */
        midi_out_chan[k] = (uint8_t)ch;
        midi_out_tr[k] = (int8_t)tr;
        midi_out_dm[k] = (uint8_t)dm;
    }
    s = (int32_t)note + tr;
    if (ch == MIDI_OUT_NONE || s < 0 || s > 127)  /* (OFF, or moved out of MIDI's range: nothing held to end later) */
        return;
    if (*w & bit)                                 /* sounding already: it starts again */
        midi_out_send(ch, (uint32_t)s, 0);
    midi_out_send(ch, (uint32_t)s, vel > 127u ? 127u : vel);
    *w |= bit;
}

/* voice.c trk_note_on / trk_note_off: the sequencer's notes (ALL) */
static void midi_out_on(const track_t *t, uint32_t note, uint32_t vel)
{
    if (midi_out_seq && settings_midi_out == MIDI_OUT_ALL && vel && !t->p[P_MUTE])
        midi_out_note(t, note, vel);
}

static void midi_out_off(const track_t *t, uint32_t note)
{
    midi_out_note(t, note, 0);
}

/* seq.c key_on / key_off: the keys' notes (KEYS and ALL; with ALL and the ARP on, the ARP's notes go out instead) */
static void midi_out_key(const track_t *t, uint32_t note, uint32_t vel)
{
    if (!vel)
        midi_out_note(t, note, 0);
    else if (settings_midi_out == MIDI_OUT_KEYS || (settings_midi_out == MIDI_OUT_ALL && !t->p[P_AMODE]))
        midi_out_note(t, note, vel);
}

static int midi_out_clock_on(void) { return settings_clock_out && !midi_clock.mode; }

static void midi_out_start(void)
{
    midi_out_clk = ~0u;
    midi_out_clk_live = (uint8_t)midi_out_clock_on();
    if (midi_out_clk_live)
        midi_out_event(0x0Fu | 0xFAu << 8);
}

static void midi_out_stop(void)
{
    uint32_t k;
    for (k = 0; k < NTRK; k++)
        midi_out_flush(k);
    if (midi_out_clk_live)
        midi_out_event(0x0Fu | 0xFCu << 8);
    midi_out_clk_live = 0;
}

/* seq_advance: n samples played (beat: a beat's); the first pulse on START's block, then every 1/24 beat.
 * CLOCK OUT turned off while playing sends STOP; turned on, it waits for the next START */
static void midi_out_clock(uint32_t n, uint32_t beat)
{
    if (!midi_out_clk_live || !song.playing || !beat)
        return;
    if (!midi_out_clock_on()) {
        midi_out_event(0x0Fu | 0xFCu << 8);
        midi_out_clk_live = 0;
        return;
    }
    if (midi_out_clk == ~0u) {
        midi_out_event(0x0Fu | 0xF8u << 8);
        midi_out_clk = 0;
    }
    for (midi_out_clk += n * 24u; midi_out_clk >= beat; midi_out_clk -= beat)
        midi_out_event(0x0Fu | 0xF8u << 8);
}
