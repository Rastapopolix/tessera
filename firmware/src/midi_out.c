/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* MIDI OUT over USB (usb.c midi_out_event): what the tracks play, each on its own channel, and the clock.
 * MIDI OUT (Settings): KEYS the notes played on the keys (as Melodee always sent them), ALL also the steps, the
 * ARP, recordings and every generator, OFF nothing. Only notes started inside the sequencer (seq_advance) are sent
 * as ALL's extra ones, so MIDI IN is never echoed back. A track's channel is its own (GLO > MIDI OUT, tessera.midi_ch:
 * 1..16, or OFF: none of its notes), or AUTO: ROUT's (CH1-4: 1..4, CH5-8: 5..8, ..; SEL: 1..4; the plugin: always the
 * track's own, 1..8), a drum track's the
 * SYSTEM DRUM channel. A track's TRANSPOSE (OCT+ held on GLO > MIDI OUT, tessera.midi_tr) moves its notes on the way
 * out only; one moved past 0..127 is not sent. Every note sent is remembered until its note-off went out, so
 * a STOP, a panic, a channel or a TRANSPOSE change never leaves a note hanging on the other side.
 * CLOCK OUT (Settings): PPQN pulses a beat (G_PPQN, 24 by default) with START and STOP, while CLK is INT (an external
 * clock is not sent on). */
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
#ifdef TESS_PLUGIN_IO   /* (the plugin: AUTO is the track's own 1..8, whatever ROUT takes in, so a DAW's routing holds) */
    uint32_t first = 0, k = trk_index(t);
#else
    uint32_t route = (uint32_t)song.g[G_ROUTE], first = route > 1u ? (route - 1u) * 4u : 0u, k = trk_index(t);
#endif
    if (k < TESSERA_TRACKS && tessera.midi_ch[k])
        return tessera.midi_ch[k] >= TESS_MIDI_OFF ? MIDI_OUT_NONE : tessera.midi_ch[k] - 1u;
    if (settings_drumch && drum_track(t))
        return settings_drumch - 1u;
    return (first + trk_index(t)) & 15u;
}

#if TESSERA_MIDI_ONLY
/* A note-off is never lost. The USB queue (usb.c midi_out_q, MQ 64) drops what does not fit, and in Tessera MIDI eight
 * tracks of chords put a step's offs and ons into it in one block, while a device on the host port takes a packet at a
 * time and may NAK for a while (Theo on tm2: PLAY|STOP left notes on a Digitone). So an off that does not fit is owed:
 * it goes out first, as soon as there is room (each block, mfx_tick, and before any other note), and the note does not
 * start again until it did (its on is dropped instead). STOP then ends with All Notes Off (CC 123) on each channel the
 * tracks played on since START, owed the same way. */
static uint32_t mo_owed[16][4];                         /* the offs owed, a bit each, by channel */
static uint16_t mo_owed_n, mo_owed_ano, mo_used;       /* how many; CC 123 owed; the channels played on, a bit each */
static int mo_room(void) { return !usb_midi_out_ready() || mo_w - mo_r < MQ; }   /* (nowhere to go: nothing owed) */

static void mo_owed_drain(void)
{
    uint32_t c, w;
    for (c = 0; c < 16u && mo_owed_n; c++)
        for (w = 0; w < 4u; w++)
            while (mo_owed[c][w]) {
                uint32_t b = (uint32_t)__builtin_ctz(mo_owed[c][w]);
                if (!mo_room()) return;
                midi_out_event(0x08u | (0x80u | c) << 8 | (w * 32u + b) << 16);
                mo_owed[c][w] &= mo_owed[c][w] - 1u;
                mo_owed_n--;
            }
    for (c = 0; c < 16u && mo_owed_ano; c++)
        if (mo_owed_ano >> c & 1u) {
            if (!mo_room()) return;
            midi_out_event(0x0Bu | (0xB0u | c) << 8 | 123u << 16);
            mo_owed_ano &= (uint16_t)~(1u << c);
        }
}

static void midi_out_send(uint32_t ch, uint32_t note, uint32_t vel)   /* vel 0: the note-off */
{
    uint32_t *w = &mo_owed[ch & 15u][(note >> 5) & 3u], bit = 1u << (note & 31u);
    if (mo_owed_n | mo_owed_ano) mo_owed_drain();
    if (!vel) {
        if (*w & bit) return;                           /* (owed already) */
        if (mo_room() && !mo_owed_n) { midi_out_event(0x08u | (0x80u | ch) << 8 | note << 16); return; }
        *w |= bit;
        mo_owed_n++;
        return;
    }
    if (*w & bit) return;                               /* its off not out yet: it does not start again before it */
    mo_used |= (uint16_t)(1u << (ch & 15u));
    midi_out_event(0x09u | (0x90u | ch) << 8 | note << 16 | vel << 24);
}

static void midi_out_all_notes_off(void)                /* (STOP: after the offs) */
{
    mo_owed_ano |= mo_used;
    mo_used = 0;
    mo_owed_drain();
}
#else
static void midi_out_send(uint32_t ch, uint32_t note, uint32_t vel)   /* vel 0: the note-off */
{
    midi_out_event(vel ? 0x09u | (0x90u | ch) << 8 | note << 16 | vel << 24 : 0x08u | (0x80u | ch) << 8 | note << 16);
}
#endif

#if TESSERA_MIDI_ONLY
/* Tessera MIDI's MIDI FX, a track's (MIDI FX / MIDI FX 2 under the FX button; core.h P_MECHO ..). Every note the
 * track sends passes here (midi_out_note): VEL fixes its velocity, HUMAN plays it up to 20 ms late and up to 20
 * softer or harder, STRUM spreads a chord's notes (those starting in one block) by its ms, UP, DOWN or at random, and
 * ECHO repeats it every TIME (a beat's 1/32 .. 1/4), each repeat FADE % softer and PITCH semitones on, a repeat as
 * long as half the TIME. A note's off goes out after its on, as late as the on went; a note started again right
 * after its off (a repeated or tied step) waits 1.5 ms after it, so the envelope on the other side fires again (an
 * off and an on in one USB packet are one event to many synths). The queue: 96 events; full, a
 * repeat is dropped, never an off. STOP, a channel or TRANSPOSE change (midi_out_flush) sends the offs waiting and
 * drops the notes not out yet */
#define MFX_Q 96u
typedef struct { uint32_t due; uint8_t k, ch, note, vel; } mfx_ev_t;   /* vel 0: an off */
static mfx_ev_t mfx_q[MFX_Q];
static uint32_t mfx_n, mfx_now;                         /* events waiting; the samples played (mfx_tick) */
static uint16_t mfx_late[NTRK][128];                    /* each note's on went out this late, ms, rounded up (its off too) */
typedef struct { uint8_t ch, note, vel, off; } mfx_pend_t;
static mfx_pend_t mfx_pend[NTRK][8];                    /* STRUM: this block's new notes, spread at its end */
static uint8_t mfx_npend[NTRK], mfx_legato;           /* mfx_legato: the off being sent is LEGATO's, ending the
                                                         * note before the one just started (midi_out.c mo_mono_to) */
static uint32_t mfx_gone[2][NTRK][4];                  /* the notes whose off went out this block and the one
                                                         * before, a bit each: their next on waits (MFX_GAP) */
#define MFX_GAP 66u                                     /* 1.5 ms between a note's off and its next on */
static uint32_t mfx_last[NTRK];                         /* the latest note on went out this late, samples (HUMAN's) */
static uint32_t mfx_rnd(void) { static uint32_t x = 0x2545F491u; x ^= x << 13; x ^= x >> 17; x ^= x << 5; return x; }
static uint32_t mfx_ms(uint32_t ms) { return ms * 441u / 10u; }   /* (44.1 kHz) */
static uint16_t mfx_late_ms(uint32_t late) { late = (late * 10u + 440u) / 441u; return (uint16_t)(late > 65535u ? 65535u : late); }
static uint32_t mfx_strum(int32_t v)                    /* STRUM: 0 off, else GLIDE's curve (F_TIME), as samples */
{ return v > 0 ? time_ms_x10((uint32_t)clamp(v, 1, 84)) * 441u / 100u : 0u; }

static void mfx_put(uint32_t k, uint32_t ch, uint32_t note, uint32_t vel, uint32_t late)
{
    uint32_t i;
    if (!late) { midi_out_send(ch, note, vel); return; }
    if (mfx_n >= MFX_Q) {                               /* full: an off goes now, an on is lost */
        if (!vel) midi_out_send(ch, note, 0);
        return;
    }
    i = mfx_n++;
    mfx_q[i].due = mfx_now + late;
    mfx_q[i].k = (uint8_t)k; mfx_q[i].ch = (uint8_t)ch; mfx_q[i].note = (uint8_t)note; mfx_q[i].vel = (uint8_t)vel;
}

/* a note's on, late by `late` samples, and its echoes */
static void mfx_on_at(uint32_t k, uint32_t ch, uint32_t note, uint32_t vel, uint32_t late)
{
    const track_t *t = &trk[k];
    uint32_t r = (uint32_t)clamp(t->p[P_MECHO], 0, 8), i;
    static const uint8_t TIME24[8] = {3, 4, 6, 8, 9, 12, 18, 24};   /* a beat's 24ths: 1/32 16T 1/16 8T 16D 1/8 8D 1/4 */
    uint32_t step = seq_beat_samples() * TIME24[clamp(t->p[P_METIME], 0, 7)] / 24u;
    int32_t v = (int32_t)vel, n = (int32_t)note;
    mfx_late[k][note & 127u] = mfx_late_ms(late);
    mfx_put(k, ch, note, vel, late);
    for (i = 1; i <= r && step; i++) {
        v = v * (100 - clamp(t->p[P_MEFADE], 0, 100)) / 100;
        n += t->p[P_MEPIT];
        if (v < 1 || n < 0 || n > 127) break;
        mfx_put(k, ch, (uint32_t)n, (uint32_t)v, late + i * step);
        mfx_put(k, ch, (uint32_t)n, 0, late + i * step + step / 2u);
    }
}

static void mfx_on(uint32_t k, uint32_t ch, uint32_t note, uint32_t vel)
{
    const track_t *t = &trk[k];
    uint32_t h = (uint32_t)clamp(t->p[P_MHUMAN], 0, 100), late = 0;
    int32_t v = t->p[P_MVEL] > 0 ? t->p[P_MVEL] : (int32_t)vel;
    if (h) {
        late = mfx_ms(mfx_rnd() % (h * 20u / 100u + 1u));
        v += (int32_t)(mfx_rnd() % (2u * (h * 20u / 100u) + 1u)) - (int32_t)(h * 20u / 100u);
    }
    v = clamp(v, 1, 127);
    if ((mfx_gone[0][k][(note >> 5) & 3u] | mfx_gone[1][k][(note >> 5) & 3u]) >> (note & 31u) & 1u
        && late < mfx_ms(mfx_late[k][note & 127u]) + MFX_GAP)   /* its off just went: a gap, so the envelope
                                                                  * on the other side starts again */
        late = mfx_ms(mfx_late[k][note & 127u]) + MFX_GAP;
    mfx_last[k] = late;
    if (t->p[P_MSTRUM] > 0 && mfx_npend[k] < 8u) {     /* STRUM: spread at the block's end (mfx_tick) */
        mfx_pend_t *p = &mfx_pend[k][mfx_npend[k]++];
        p->ch = (uint8_t)ch; p->note = (uint8_t)note; p->vel = (uint8_t)v; p->off = 0;
        mfx_late[k][note & 127u] = mfx_late_ms(late);   /* (HUMAN's part; STRUM adds its) */
        return;
    }
    mfx_on_at(k, ch, note, (uint32_t)v, late);
}

static void mfx_off(uint32_t k, uint32_t ch, uint32_t note)
{
    uint32_t i;
    for (i = 0; i < mfx_npend[k]; i++)                   /* its on not out yet (STRUM): it ends right after it */
        if (mfx_pend[k][i].note == note) { mfx_pend[k][i].off = 1; return; }
    i = mfx_ms(mfx_late[k][note & 127u]);
    mfx_gone[0][k][(note >> 5) & 3u] |= 1u << (note & 31u);
    if (mfx_legato && i < mfx_last[k] + mfx_ms(1u))     /* LEGATO: after the new note's on, whatever HUMAN did */
        i = mfx_last[k] + mfx_ms(1u);
    mfx_put(k, ch, note, 0, i);
}

/* a track's waiting notes: the offs out now, the ons dropped (midi_out_flush) */
static void mfx_purge(uint32_t k)
{
    uint32_t i, j = 0;
    mfx_npend[k] = 0;
    for (i = 0; i < mfx_n; i++) {
        if (mfx_q[i].k != k) { mfx_q[j++] = mfx_q[i]; continue; }
        if (!mfx_q[i].vel) midi_out_send(mfx_q[i].ch, mfx_q[i].note, 0);
    }
    mfx_n = j;
    memset(mfx_late[k], 0, sizeof mfx_late[k]);
}

/* mix_block, each block after its notes: STRUM's spread, then the time on and what is due out */
static void mfx_tick(uint32_t n)
{
    uint32_t k, i, j;
    if (mo_owed_n | mo_owed_ano) mo_owed_drain();      /* (the offs owed first) */
    for (k = 0; k < NTRK; k++) {
        uint32_t c = mfx_npend[k], gap = mfx_strum(trk[k].p[P_MSTRUM]), dir = (uint32_t)trk[k].p[P_MSDIR];
        uint8_t ord[8];
        if (!c) continue;
        for (i = 0; i < c; i++) ord[i] = (uint8_t)i;
        for (i = 1; i < c; i++)                          /* low to high (UP), high to low (DOWN), or shuffled */
            for (j = i; j > 0 && (dir == 1u ? mfx_pend[k][ord[j]].note > mfx_pend[k][ord[j - 1]].note
                                            : mfx_pend[k][ord[j]].note < mfx_pend[k][ord[j - 1]].note); j--) {
                uint8_t x = ord[j]; ord[j] = ord[j - 1]; ord[j - 1] = x;
            }
        if (dir == 2u)
            for (i = c - 1u; i > 0; i--) { uint8_t x; j = mfx_rnd() % (i + 1u); x = ord[i]; ord[i] = ord[j]; ord[j] = x; }
        mfx_npend[k] = 0;
        for (i = 0; i < c; i++) {
            const mfx_pend_t *p = &mfx_pend[k][ord[i]];
            uint32_t late = mfx_ms(mfx_late[k][p->note]) + i * gap;
            mfx_on_at(k, p->ch, p->note, p->vel, late);
            if (p->off) mfx_put(k, p->ch, p->note, 0, late + mfx_ms(10u));
        }
    }
    memcpy(mfx_gone[1], mfx_gone[0], sizeof mfx_gone[0]);
    memset(mfx_gone[0], 0, sizeof mfx_gone[0]);
    mfx_now += n;
    for (i = 0, j = 0; i < mfx_n; i++) {                /* due: out, in the order they were queued */
        if ((int32_t)(mfx_now - mfx_q[i].due) >= 0) midi_out_send(mfx_q[i].ch, mfx_q[i].note, mfx_q[i].vel);
        else mfx_q[j++] = mfx_q[i];
    }
    mfx_n = j;
}
#define MFX_ON(k, ch, note, vel) mfx_on(k, ch, note, vel)
#define MFX_OFF(k, ch, note) mfx_off(k, ch, note)

/* ENV CC and LFO CC (core.h P_ECC ..): with no voices, each track's envelope runs here, at control rate: ATK DEC SUS
 * REL as straight segments (ENV_LIN, the times the voices' have), every note starting it again from where it is,
 * the release once the track's last note ended (tm_gate: notes played, MIDI OUT's or not). It is the track's ENV for
 * the mods too (t->m_env), and the LFO ticks here (voice.c track_lfo_tick; it restarts with a phrase there). Each
 * goes out as its CC on the track's MIDI channel when DEPTH is not 0: BASE + 2 x DEPTH x ENV (0..1: 100 % the whole
 * 0..127), BASE + DEPTH x the LFO (-1..1, or 0..1 UNI: 100 % from 0 to 127 around 64), when the value changed and at most every 12 blocks (~9 ms) each, as a mod onto CC */
enum { TM_IDLE, TM_ATK, TM_DEC, TM_SUS, TM_REL };
static uint32_t tm_held[NTRK][4];                       /* the notes playing on each track, a bit each */
static uint8_t tm_stage[NTRK], tm_peak;                /* tm_peak bit k: its attack just ended (its top goes out
                                                         * at once, not 12 blocks on) */
static int32_t tm_lvl[NTRK];                            /* the envelope, Q24 */
static uint8_t tm_cc_was[NTRK][2], tm_cc_num[NTRK][2];  /* the value sent last (0xFF: none) and on which CC */
static uint16_t tm_cc_at[NTRK][2], tm_blocks;

static void tm_gate(const track_t *t, uint32_t note, uint32_t on)   /* (voice.c trk_note_on / trk_note_off; note
                                                                      * past 127, off: all, trk_all_off) */
{
    uint32_t k = trk_index(t) % NTRK, bit = 1u << (note & 31u), *w = &tm_held[k][(note >> 5) & 3u];
    if (note > 127u) memset(tm_held[k], 0, sizeof tm_held[k]);
    else if (on) {
        *w |= bit;
        tm_stage[k] = TM_ATK;
        return;
    }
    *w &= ~bit;
    if (!(tm_held[k][0] | tm_held[k][1] | tm_held[k][2] | tm_held[k][3]) && tm_stage[k] != TM_IDLE)
        tm_stage[k] = TM_REL;
}

static void tm_env_tick(track_t *t, uint32_t k, uint32_t n)
{
#define TM_STEP(x) ((int32_t)(ENV_LIN[(uint32_t)(x) & 127u] >> 5) * (int32_t)n)   /* (ENV_LIN: Q24 a CTL = 32) */
    int32_t v = tm_lvl[k], sus = clamp(t->p[P_SUS], 0, 127) * 132104;
    switch (tm_stage[k]) {
    case TM_ATK:
        v += TM_STEP(t->p[P_ATK]);
        if (v >= 1 << 24) v = 1 << 24, tm_stage[k] = TM_DEC, tm_peak |= (uint8_t)(1u << k);
        break;
    case TM_DEC:
        v -= TM_STEP(t->p[P_DEC]);
        if (v <= sus) v = sus, tm_stage[k] = TM_SUS;
        break;
    case TM_SUS:
        v = sus;
        break;
    case TM_REL:
        v -= TM_STEP(t->p[P_REL]);
        if (v <= 0) v = 0, tm_stage[k] = TM_IDLE;
        break;
    default:
        v = 0;
    }
#undef TM_STEP
    tm_lvl[k] = v;
    t->m_env = v >> 9;
}

static void tm_mod_tick(uint32_t n)                     /* (mix.c mix_block, each block) */
{
    uint32_t k, j;
    tm_blocks++;
    for (k = 0; k < NTRK; k++) {
        track_t *t = &trk[k];
        uint32_t ch = midi_out_channel(t);
        track_lfo_tick(t);
        tm_env_tick(t, k, n);
        if (settings_midi_out == MIDI_OUT_OFF || ch == MIDI_OUT_NONE || t->p[P_MUTE]) continue;
        for (j = 0; j < 2u; j++) {
            int32_t depth = t->p[j ? P_LDEPTH : P_EDEPTH], src, v;
            uint32_t cc = (uint32_t)clamp(t->p[j ? P_LCC : P_ECC], 1, 119);
            if (!depth || ((uint16_t)(tm_blocks - tm_cc_at[k][j]) < 12u && (j || !(tm_peak >> k & 1u)))) continue;
            src = j ? mulq15(t->lfo_val, t->lfo_fade) : t->m_env;
            v = clamp(t->p[j ? P_LBASE : P_EBASE] + (((j ? 1 : 2) * depth * src) >> 15), 0, 127);
            if (cc == tm_cc_num[k][j] && (uint32_t)v == tm_cc_was[k][j]) continue;
            tm_cc_was[k][j] = (uint8_t)v;
            tm_cc_num[k][j] = (uint8_t)cc;
            tm_cc_at[k][j] = tm_blocks;
            midi_out_event(0x0Bu | (0xB0u | ch) << 8 | cc << 16 | (uint32_t)v << 24);
        }
    }
    tm_peak = 0;
}
#else
#define MFX_ON(k, ch, note, vel) midi_out_send(ch, note, vel)
#define MFX_OFF(k, ch, note) midi_out_send(ch, note, 0)
#endif

#if TESSERA_MIDI_ONLY
static uint8_t mo_mono[NTRK][8], mo_mono_n[NTRK], mo_mono_out[NTRK], mo_mono_vel[NTRK];   /* VOICE MONO over MIDI
                                                         * (midi_out_note): the notes held, the one out + 1, its velocity */
#endif

/* every note track k still has on goes off */
static void midi_out_flush(uint32_t k)
{
#if TESSERA_MIDI_ONLY
    mfx_purge(k);
    mo_mono_n[k % NTRK] = 0; mo_mono_out[k % NTRK] = 0;
#endif
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

#if TESSERA_MIDI_ONLY
static void midi_out_note1(const track_t *t, uint32_t note, uint32_t vel)   /* (midi_out_note: MONO's choice first) */
#else
static void midi_out_note(const track_t *t, uint32_t note, uint32_t vel)
#endif
{
    uint32_t k = trk_index(t), ch = midi_out_channel(t), bit, *w;
    uint32_t dm = k < TESSERA_TRACKS && (drum_track(t) || tessera.mode[k] == TESS_ENG_GRIDS) ? tessera.midi_dmap[k] : 0u;
    int32_t tr = k < TESSERA_TRACKS ? tessera.midi_tr[k] : 0, s;
    note = midi_out_map(vel ? dm : midi_out_dm[k], note & 127u);
    bit = 1u << (note & 31u);
    w = &midi_out_held[k][(note >> 5) & 3u];
    if (!vel) {
        if (*w & bit)
            MFX_OFF(k, midi_out_chan[k], (uint32_t)((int32_t)note + midi_out_tr[k]));
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
        MFX_OFF(k, ch, (uint32_t)s);
    MFX_ON(k, ch, (uint32_t)s, vel > 127u ? 127u : vel);
    *w |= bit;
}

#if TESSERA_MIDI_ONLY
/* VOICE MONO / LEGATO / UNISON over MIDI: one note at a time. The notes held are a stack (as voice.c's mono_stack, kept
 * here: MIDI OUT's notes are not the voices'); PRIO LAST / LOW / HIGH picks the one that sounds. A new winner ends the
 * old note first in MONO, just after its own on in LEGATO and UNISON (an overlap: a mono synth glides, its envelope
 * goes on); a released winner hands over to the next held note. A drum track (VOICE does not apply) plays
 * every note */
static uint32_t mo_mono_pick(uint32_t k, uint32_t prio)
{
    uint32_t i, n = mo_mono[k][mo_mono_n[k] - 1u];
    for (i = 0; i < mo_mono_n[k]; i++)
        if (prio == 1u ? mo_mono[k][i] < n : prio == 2u && mo_mono[k][i] > n)
            n = mo_mono[k][i];
    return n;
}

static void mo_mono_drop(uint32_t k, uint32_t note)
{
    uint32_t i, j = 0;
    for (i = 0; i < mo_mono_n[k]; i++)
        if (mo_mono[k][i] != note) mo_mono[k][j++] = mo_mono[k][i];
    mo_mono_n[k] = (uint8_t)j;
}

static void mo_mono_to(const track_t *t, uint32_t k, uint32_t from, uint32_t to, uint32_t vel)   /* from + 1, 0: none */
{
    if (from && trk_vmode(t) != V_MONO) {               /* LEGATO / UNISON: the new note on first, the old one's off a
                                                         * millisecond after it, so a mono synth on the other side glides */
        midi_out_note1(t, to, vel);
        mfx_legato = 1;
        midi_out_note1(t, from - 1u, 0);
        mfx_legato = 0;
        return;
    }
    if (from) midi_out_note1(t, from - 1u, 0);
    midi_out_note1(t, to, vel);
}

static void midi_out_note(const track_t *t, uint32_t note, uint32_t vel)
{
    uint32_t k = trk_index(t) % NTRK, out, want;
    if (trk_vmode(t) == V_POLY) {                       /* (a mode change: the stack starts afresh) */
        mo_mono_n[k] = 0; mo_mono_out[k] = 0;
        midi_out_note1(t, note, vel);
        return;
    }
    note &= 127u;
    out = mo_mono_out[k];
    if (vel) {
        mo_mono_drop(k, note);
        if (mo_mono_n[k] >= sizeof mo_mono[k]) mo_mono_drop(k, mo_mono[k][0]);   /* (full: the oldest goes) */
        mo_mono[k][mo_mono_n[k]++] = (uint8_t)note;
        mo_mono_vel[k] = (uint8_t)(vel > 127u ? 127u : vel);
        want = mo_mono_pick(k, (uint32_t)t->p[P_PRIO]);
        if (out == want + 1u && want != note)
            return;                                     /* LOW / HIGH: this note does not win */
        mo_mono_to(t, k, out == want + 1u ? 0u : out, want, vel);   /* (the same note again: it starts again) */
        mo_mono_out[k] = (uint8_t)(want + 1u);
        return;
    }
    mo_mono_drop(k, note);
    if (out != note + 1u) {
        midi_out_note1(t, note, 0);                     /* (not the one sounding: nothing out, or a note from before) */
        return;
    }
    if (!mo_mono_n[k]) {
        midi_out_note1(t, note, 0);
        mo_mono_out[k] = 0;
        return;
    }
    want = mo_mono_pick(k, (uint32_t)t->p[P_PRIO]);     /* back to a note still held */
    mo_mono_to(t, k, out, want, mo_mono_vel[k]);
    mo_mono_out[k] = (uint8_t)(want + 1u);
}
#endif

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

#if TESSERA_MIDI_ONLY
static void cord_mix_resend(void);                      /* cord.c: MIXER LEVEL / PAN out again (CC 7 / 10) */
#endif
static void midi_out_start(void)
{
#if TESSERA_MIDI_ONLY
    cord_mix_resend();
#endif
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
#if TESSERA_MIDI_ONLY
    midi_out_all_notes_off();
#endif
    if (midi_out_clk_live)
        midi_out_event(0x0Fu | 0xFCu << 8);
    midi_out_clk_live = 0;
}

/* seq_advance: n samples played (beat: a beat's); the first pulse on START's block, then every 1/PPQN beat.
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
    for (midi_out_clk += n * midi_ppqn(); midi_out_clk >= beat; midi_out_clk -= beat)
        midi_out_event(0x0Fu | 0xF8u << 8);
}
