/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* DIR (TX_SEQ_DIR, each STEP pattern's: pattern.c pattern_dir): the order the steps play in. Included by seq.c.
 *
 * A pass is still LEN steps of time: seq_slot counts them (0..LEN-1) and keeps the bar (the pass, a pattern change,
 * punch-in, SWING's pairs), seq_idx is the step playing on it. FWD: the two are the same and nothing here runs, as
 * before DIR. The others choose the next step a step ahead (seq_nxt), so a TIE and an early NUDGE see the step that
 * comes; RAND and DRUNK by a hash of the track's SEED and its step since PLAY (seq_run_step), so PLAY replays them. A
 * new DIR (the sheet, a mod) goes on from the step playing: the next step is chosen again, the playhead stays. */
#define SEQ_NXT_FRESH 0xFFFFu           /* seq_nxt: the pattern from its top (PLAY, a pattern change): DIR's first step */
#define SEQ_NXT_AGAIN 0xFFFEu           /* .. chosen again from seq_idx (moved by hand: song position) */

static uint32_t seq_dir_of(const track_t *t) { return pattern_dir[(t - trk) % NTRK][t->pattern % NPAT]; }
void cord_hold_bar(uint32_t tr);                         /* (cord.c: a bar of the track, its HOLD mods) */
static int32_t cord_tess_get(uint32_t tr, uint32_t x);  /* (cord.c: the pattern's DIR as its mods make it) */
static uint32_t seq_dir_now(const track_t *t) { return (uint32_t)cord_tess_get((uint32_t)(t - trk), TX_SEQ_DIR); }

/* step k of track tr since PLAY: its RAND / DRUNK draw */
static uint32_t seq_dir_hash(uint32_t tr, uint32_t k)
{
    uint32_t x = ((uint32_t)tessera.seed[tr % TESSERA_TRACKS] << 16 ^ tr << 12 ^ 0xD1u) + k * 0x9E3779B1u;
    x ^= x >> 15; x *= 0x85EBCA77u; x ^= x >> 13; x *= 0xC2B2AE35u;
    return x ^ x >> 16;
}
/* the step after idx going d, for step k since PLAY (fresh: d's first step); PEND / PONG turn at the ends */
static uint32_t seq_dir_pick(track_t *t, uint32_t d, uint32_t idx, uint32_t len, uint32_t k, int fresh)
{
    uint32_t h = seq_dir_hash((uint32_t)(t - trk), k);
    idx %= len;
    switch (d) {
    case SEQ_BACK: return fresh ? len - 1u : (idx ? idx : len) - 1u;
    case SEQ_PEND: case SEQ_PONG:
        if (fresh) { SEQ_D(t).back = 0; return 0; }
        if (SEQ_D(t).back ? !idx : idx + 1u >= len) {     /* (an end: back the other way; PONG plays the end again) */
            SEQ_D(t).back ^= 1u;
            if (d == SEQ_PONG || len < 2u) return idx;
        }
        return SEQ_D(t).back ? idx - 1u : idx + 1u;
    case SEQ_RAND: return h % len;
    case SEQ_DRUNK: return fresh ? h % len : (idx + ((h >> 8) & 1u ? 1u : len - 1u)) % len;
    default: return fresh ? 0u : (idx + 1u) % len;
    }
}
/* the step after the one playing (a TIE, NUDGE, motion's next step) */
static uint32_t seq_dir_next(const track_t *t, uint32_t len)
{
    if (!len) len = 1u;
    return SEQ_D(t).ndir != SEQ_FWD && SEQ_D(t).nxt < len ? SEQ_D(t).nxt : (t->seq_idx + 1u) % len;
}
/* the transport, a pattern change: the pattern from its top, its last step of time playing */
static void seq_dir_top(track_t *t)
{
    t->seq_idx = SEQ_D(t).slot = (uint16_t)(t->p[P_SLEN] - 1);
    SEQ_D(t).nxt = SEQ_NXT_FRESH;
}
/* seq_grid_tick: on to the next step (slot and step); len the pattern's, k this step since PLAY */
static void seq_dir_step(track_t *t, uint32_t len, uint32_t k)
{
    uint32_t d = seq_dir_now(t), was = SEQ_D(t).nxt;
    if (d == SEQ_FWD) {                                 /* (as before DIR) */
        t->seq_idx = (uint16_t)((t->seq_idx + 1u) % len);
        SEQ_D(t).slot = (uint16_t)((SEQ_D(t).slot + 1u) % len);   /* (always FWD: the same as seq_idx) */
        SEQ_D(t).ndir = SEQ_FWD;
        SEQ_D(t).nxt = SEQ_NXT_AGAIN;                     /* (another DIR later: on from here) */
        return;
    }
    SEQ_D(t).slot = (uint16_t)(was == SEQ_NXT_FRESH ? 0u : (SEQ_D(t).slot + 1u) % len);
    if (was < len && (d == SEQ_D(t).ndir || (seq_nudge_early & (1u << (t - trk))))) t->seq_idx = (uint16_t)was;
    else t->seq_idx = (uint16_t)seq_dir_pick(t, d, t->seq_idx, len, k, was == SEQ_NXT_FRESH);
    SEQ_D(t).ndir = (uint8_t)d;
    SEQ_D(t).nxt = (uint16_t)seq_dir_pick(t, d, t->seq_idx, len, k + 1u, 0);
}
/* song position: step k since PLAY, as the pattern's DIR would have it from the top (DRUNK: RAND's step, a walk is
 * not replayed) */
static void seq_dir_seek(track_t *t, uint32_t len, uint32_t k)
{
    uint32_t d = seq_dir_of(t), s = k % len, p, i;
    SEQ_D(t).slot = (uint16_t)s;
    SEQ_D(t).nxt = SEQ_NXT_AGAIN;
    SEQ_D(t).back = 0;
    if (d == SEQ_BACK) i = len - 1u - s;
    else if ((d == SEQ_PEND || d == SEQ_PONG) && len > 1u) {
        p = d == SEQ_PEND ? 2u * len - 2u : 2u * len;
        p = k % p;
        SEQ_D(t).back = (uint8_t)(p >= len - (d == SEQ_PEND));
        i = p < len ? p : (d == SEQ_PEND ? 2u * len - 2u : 2u * len - 1u) - p;
    } else if (d >= SEQ_RAND) i = seq_dir_hash((uint32_t)(t - trk), k) % len;
    else i = s;
    t->seq_idx = (uint16_t)i;
}
/* a new pattern on the bar line (seq_grid_tick): its first step, by its own DIR */
static void seq_dir_first(track_t *t, uint32_t len, uint32_t k)
{
    seq_dir_top(t);
    seq_dir_step(t, len ? len : 1u, k);
}
