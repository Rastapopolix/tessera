/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* MOD SCOPE (gap G14, tessera-notes docs/design/mod-scope.md): the picked mod's move over time and its source, for
 * MODS to draw (ui_mono.c mo_scope) while OCT- is held. Sampled by the UI once a frame while MODS is shown
 * (ui_cords.c cu_scope_draw; nothing off MODS): for each of MSC_W columns the lowest and highest of the cord's move
 * (cord_share, a share of the destination's range), their mean, whether the move turned back in it more than once
 * (busy: a fast mod, drawn by its mean alone, not a smear) and its source as the cord reads it (before a GATE or a
 * fold), all -127..127. The columns sweep left to right on the musical clock (voice.c lfo_units_run: the LFOs' units,
 * running when stopped too), a window of 1 .. 64 bars chosen by the source (mscope_bars). Included after cord.c. */
#define MSC_W 112u                                      /* columns: the Mono screen's 120 cells less the margins */
#define MSC_BAR 98304u                                  /* a bar of four beats in the sequencer's units (mix.c) */
static struct {
    int8_t lo[MSC_W], hi[MSC_W], mv[MSC_W], sv[MSC_W];  /* the move's lowest, highest and mean, the source last */
    uint8_t busy[(MSC_W + 7u) / 8u];                    /* a bit a column: the move turned back twice or more in it */
    int32_t sum;                                        /* the column now: its samples' sum and count, the move's */
    uint16_t n;                                         /* turns, its last sample and way (-1, 0, 1) */
    uint8_t turns;
    int8_t last, dir;
    uint32_t at, units, per;                            /* the clock last seen, units into the column, a column's */
    uint8_t cord, src, col, bars, full;                 /* full: a sweep is done (every column holds a sample) */
    uint8_t cross, cross_was;                           /* the source's rising crossings of its middle, this sweep and
                                                         * the last (the LFO's window, the reader's "A BAR") */
    int8_t sgn;
} mscope;

static __attribute__((noinline)) int8_t msc_q(int32_t v)   /* Q15 to -127..127 */
{
    return (int8_t)(clamp(v, -32767, 32767) / 258);
}

/* the window for a source, in bars: what it takes to see its shape (a TIDE drawn over a second is a flat line) */
static uint32_t mscope_bars(uint32_t s)
{
    if (s == CS_DRIFT || (s >= CS_OCHD1 && s <= CS_OCHD8)) return 16u;
    if ((s >= CS_TORPOR && s <= CS_INERTIA) || s == CS_ENERGY) return 64u;
    if (s >= CS_MACRO1 && s < CS_EOC) return 4u;
    if (s == CS_LFO || s == CS_SLEW) return 2u;         /* (then two to six cycles: mscope_take) */
    return 1u;                                          /* the notes' own: ENV, VEL, GATE, NOTE, CV, S&H .. */
}
static void mscope_reset(uint32_t i, uint32_t bars)
{
    memset(&mscope, 0, sizeof mscope);
    mscope.cord = (uint8_t)i;
    mscope.src = cords[i].src;
    mscope.bars = (uint8_t)bars;
    mscope.per = bars * (MSC_BAR / MSC_W);
    mscope.at = lfo_units_run;
}
static void msc_col(int8_t v)                           /* the column now starts at this sample */
{
    uint32_t x = mscope.col;
    mscope.lo[x] = mscope.hi[x] = mscope.mv[x] = v;
    mscope.busy[x >> 3] &= (uint8_t)~(1u << (x & 7u));
    mscope.sum = v;
    mscope.n = 1;
    mscope.turns = 0;
}
/* cord i (< CORD_LIVE) now, into its column; a column passed, the next; a sweep done, an LFO's window follows its
 * rate (two to six cycles in view) */
static void mscope_take(uint32_t i)
{
    const cord_t *c = &cords[i];
    cord_t t;
    uint32_t now = lfo_units_run, d = now - mscope.at, b;
    int8_t v, s, g, dd;
    int32_t mid;
    int fresh;
    if (i != mscope.cord || c->src != mscope.src || !mscope.per || d >= MSC_W * mscope.per)   /* (another mod or */
        mscope_reset(i, mscope_bars(c->src)), d = 0;    /* source; back on MODS after a while) */
    mscope.at = now;
    t = *c;                                             /* (the source as it is: a copy read +-, cord.c's cord_val */
    t.uni = CU_BI;                                      /* out of line, not cord_src again) */
    s = msc_q(cord_val(&t, 32767));
    v = cord_live(c) ? msc_q(cord_share(c)) : cord_src_only(c) ? msc_q(cord_val(c, 32767)) : 0;   /* (no destination
                                                         * yet: the source as the move, cord.c cord_src_only) */
    mid = cord_bipolar(c->src, c->src_trk) ? 0 : 64;    /* rising through the source's middle (0 of a bipolar one, */
    g = (int8_t)(s > mid + 8 ? 1 : s < mid - 8 ? -1 : mscope.sgn);   /* half of a 0..1 one), a little hysteresis */
    mscope.cross += (uint8_t)(g > 0 && mscope.sgn < 0 && mscope.cross < 255u);
    mscope.sgn = g;
    fresh = !d && !mscope.units && !mscope.col && !mscope.full;   /* (a fresh window: its first column, this sample) */
    if (fresh) msc_col(v);
    for (mscope.units += d; mscope.units >= mscope.per; mscope.units -= mscope.per) {   /* the columns passed */
        if (++mscope.col >= MSC_W) {
            mscope.col = 0;
            mscope.full = 1;
            mscope.cross_was = mscope.cross;
            mscope.cross = 0;
            b = mscope.bars;
            if (c->src == CS_LFO || c->src == CS_SLEW)
                b = mscope.cross_was > 6u && b > 1u ? b >> 1 : mscope.cross_was < 2u && b < 16u ? b << 1 : b;
            if (b != mscope.bars) {
                mscope_reset(i, b);
                return;
            }
        }
        msc_col(v);                                     /* (a new column starts at this sample) */
        fresh = 1;
    }
    dd = (int8_t)(v > mscope.last ? 1 : v < mscope.last ? -1 : 0);
    if (dd) {                                           /* (the move turned back) */
        mscope.turns += (uint8_t)(mscope.dir && dd != mscope.dir && mscope.turns < 255u);
        mscope.dir = dd;
    }
    mscope.last = v;
    if (mscope.turns > 1u) mscope.busy[mscope.col >> 3] |= (uint8_t)(1u << (mscope.col & 7u));
    if (!fresh) {
        if (v < mscope.lo[mscope.col]) mscope.lo[mscope.col] = v;
        if (v > mscope.hi[mscope.col]) mscope.hi[mscope.col] = v;
        mscope.sum += v;
        mscope.n++;
        mscope.mv[mscope.col] = (int8_t)(mscope.sum / (int32_t)mscope.n);
    }
    mscope.sv[mscope.col] = s;
}
