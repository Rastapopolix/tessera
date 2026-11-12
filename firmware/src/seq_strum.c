/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* STRUM (CHORD, OCT- held on GEN; Tessera, included by seq.c before seq_sub.c): a CHORD strike's notes STRUM ms apart,
 * as a strummed chord, instead of together. DIR: UP the lowest first, DOWN the highest, ALT turns round each strike
 * (from UP, at PLAY). The strike (tessera.c tess_chord_play) holds the whole chord in seq_notes as ever (ascending:
 * chord_make sorts it) and queues its notes in DIR's order; each block (seq_grid_tick) the next one sounds when its
 * time has come, the first on the next block (under a ms late). All of them within half the chord's length (its
 * seq_off), so a short stab keeps every note; they end together with the chord. The queue is dropped when the chord
 * ends (seq_n 0: its gate, a new strike, STOP), when a RATCH repeat strikes the chord again (seq_sub_tick), and at
 * PLAY and STOP (seq_sub_clear). In MIDI clock mode the ms are the sequencer's units, as HUMAN's. STRUM 0: nothing
 * queued, every note at once (as before) */
static struct {
    uint32_t left, gap;             /* to the next queued note, between notes (the sequencer's units) */
    uint8_t n, k, vel;              /* queued notes, the next one, their velocity */
    uint8_t note[CHORD_MAX];
} seq_st[NTRK];
static uint8_t seq_st_alt;          /* bit per track: ALT's next strike goes DOWN */
_Static_assert(TESS_STR_UP == 0 && TESS_STR_DOWN == 1, "seq_strum_strike: DIR as the strike's down");
#ifndef SUB_NOTE_ON
#define SUB_NOTE_ON trk_note_on                         /* (tests/hostsim.c: logged as a generator's notes; seq_sub.c) */
#endif

/* a strike of the chord in t's seq_notes (not sounding yet): queued in DIR's order, STRUM ms apart */
static void seq_strum_strike(track_t *t, uint32_t tr, uint32_t vel)
{
    uint32_t n = t->seq_n, i, gap, down = tessera.ch_sdir[tr];
    if (down == TESS_STR_ALT) {
        down = (seq_st_alt >> tr) & 1u;
        seq_st_alt ^= (uint8_t)(1u << tr);
    }
    gap = seq_units_of(441u * tessera.ch_strum[tr] / 10u);   /* (44.1 samples a ms) */
    if (n > 1u && gap * (n - 1u) > t->seq_off / 2u) gap = t->seq_off / 2u / (n - 1u);
    for (i = 0; i < n; i++) seq_st[tr].note[i] = t->seq_notes[down ? n - 1u - i : i];   /* (down: TESS_STR_DOWN, 1) */
    seq_st[tr].n = (uint8_t)n;
    seq_st[tr].k = 0;
    seq_st[tr].vel = (uint8_t)vel;
    seq_st[tr].gap = gap;
    seq_st[tr].left = 0;
}

/* each block, n units on: the next queued note when it is due, while the chord still sounds */
static void seq_strum_tick(track_t *t, uint32_t n)
{
    uint32_t tr = trk_index(t);
    if (seq_st[tr].k >= seq_st[tr].n) return;
    if (!t->seq_n) seq_st[tr].n = 0;
    else if (seq_st[tr].left > n) seq_st[tr].left -= n;
    else {
        uint32_t x = seq_st[tr].note[seq_st[tr].k++];  /* (first: SUB_NOTE_ON may name its arguments twice) */
        SUB_NOTE_ON(t, x, seq_st[tr].vel);
        seq_st[tr].left = seq_st[tr].gap;
    }
}
