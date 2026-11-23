/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* A Standard MIDI File from the notes Tessera played: format 1, 480 ticks a quarter, 4/4; a tempo track (the project's
 * name, its tempo) and a track per MIDI channel that has notes, named T1..T16, in channel order. No allocation, no
 * 64-bit division, no library: the plugin (platform/plugin/midifile.cpp) and Andamento (tess_core_smf, MELODEE_SMF)
 * write the same file. Valid C and C++ (the plugin includes it in a C++ file). Not on the FM-1: it has no files.
 *
 * In: messages in tick order (ticks: 480 a quarter from the song's top), notes only (others are left out); a note-on of
 * velocity 0 is an off. Two ways to cut the clip:
 *   bars 0   from the bar (1,920 ticks) of the first note to the last message kept; an off of a note that began
 *            before the clip is left out, a note still on ends with the last message;
 *   bars N   from the song's top for N bars exactly: messages from the end on are left out, a note still on ends at
 *            the end, and every track ends there (a DAW takes the clip as N bars long). */

#ifndef SMF_API
#define SMF_API static
#endif
#define SMF_PPQ 480u
#define SMF_BAR (4u * SMF_PPQ)

typedef struct smf_ev {
    uint32_t tick;
    uint8_t status, d1, d2, pad;
} smf_ev;

typedef struct smf_buf {
    uint8_t *p;      /* NULL: count only */
    uint32_t n, cap;
} smf_buf;

static void smf_b(smf_buf *b, uint32_t v)
{
    if (b->p && b->n < b->cap)
        b->p[b->n] = (uint8_t)v;
    b->n++;
}
static void smf_be(smf_buf *b, uint32_t v, int bytes)
{
    while (bytes--)
        smf_b(b, v >> (bytes * 8) & 0xFFu);
}
static void smf_vlq(smf_buf *b, uint32_t v)
{
    uint8_t t[5];
    int k = 0;
    do {
        t[k++] = (uint8_t)(v & 0x7Fu);
        v >>= 7;
    } while (v);
    while (k--)
        smf_b(b, t[k] | (k ? 0x80u : 0u));
}
static void smf_name(smf_buf *b, const char *s)      /* a track name (meta 03) at delta 0 */
{
    uint32_t l = 0, i;
    while (s[l])
        l++;
    smf_b(b, 0x00);
    smf_b(b, 0xFF);
    smf_b(b, 0x03);
    smf_vlq(b, l);
    for (i = 0; i < l; i++)
        smf_b(b, (uint8_t)s[i]);
}

/* channel ch's track body (its name, the notes, end-of-track) into b, with ticks from start; messages at or after
 * stop left out (bars N), held notes ended at close, end-of-track at eot (0: with the last message). The messages
 * kept; *last: the latest kept tick, if later */
static uint32_t smf_chan(smf_buf *b, const smf_ev *ev, uint32_t n, uint32_t ch, uint32_t start, uint32_t stop,
                         uint32_t close, uint32_t eot, uint32_t *last)
{
    uint8_t on[128];
    uint32_t i, k, now = 0, kept = 0;
    char nm[4] = {'T', 0, 0, 0};
    if (ch < 9u)
        nm[1] = (char)('1' + ch);
    else
        nm[1] = '1', nm[2] = (char)('0' + ch - 9u);
    for (k = 0; k < 128u; k++)
        on[k] = 0;
    smf_name(b, nm);
    for (i = 0; i < n; i++) {
        const smf_ev *e = &ev[i];
        uint32_t kind = e->status & 0xF0u, key = e->d1 & 127u, t;
        int is_on;
        if ((kind != 0x80u && kind != 0x90u) || (e->status & 15u) != ch || e->tick < start || e->tick >= stop)
            continue;
        is_on = kind == 0x90u && e->d2;
        if (!is_on && !on[key])
            continue;                                /* (its note began before the clip) */
        if (is_on) {
            if (on[key] < 255u)
                on[key]++;
        } else {
            on[key]--;
        }
        t = e->tick - start;
        smf_vlq(b, t - now);
        now = t;
        smf_b(b, (is_on ? 0x90u : 0x80u) | ch);
        smf_b(b, key);
        smf_b(b, is_on ? (e->d2 & 127u) : 64u);
        if (e->tick > *last)
            *last = e->tick;
        kept++;
    }
    for (k = 0; k < 128u; k++)                       /* (notes still on: ended at close) */
        while (on[k]) {
            smf_vlq(b, close > now ? close - now : 0u);
            if (close > now)
                now = close;
            smf_b(b, 0x80u | ch);
            smf_b(b, k);
            smf_b(b, 64u);
            on[k]--;
        }
    smf_vlq(b, eot > now ? eot - now : 0u);
    smf_b(b, 0xFF);
    smf_b(b, 0x2F);
    smf_b(b, 0x00);
    return kept;
}

/* the file into out (cap bytes); its length (more than cap: out holds its start only; call again with that much).
 * bpm: the tempo (0: 120); name: the first track's name ("" or NULL: none); bars: 0 or N (above). 0: no notes */
SMF_API uint32_t smf_write(const smf_ev *ev, uint32_t n, uint32_t bpm, const char *name, uint32_t bars, uint8_t *out,
                           uint32_t cap)
{
    smf_buf b, d;
    uint32_t i, ch, first = 0xFFFFFFFFu, start, stop, last = 0, close, eot, tracks = 1, us;
    for (i = 0; i < n; i++)
        if ((ev[i].status & 0xF0u) == 0x90u && ev[i].d2 && ev[i].tick < first)
            first = ev[i].tick;
    stop = bars ? bars * SMF_BAR : 0xFFFFFFFFu;
    if (first == 0xFFFFFFFFu || first >= stop)
        return 0;
    start = bars ? 0u : first / SMF_BAR * SMF_BAR;
    for (ch = 0; ch < 16u; ch++) {                   /* (the last message kept, and the tracks with any) */
        d.p = 0;
        d.n = d.cap = 0;
        if (smf_chan(&d, ev, n, ch, start, stop, 0, 0, &last))
            tracks++;
    }
    close = bars ? stop - start : last - start;
    eot = bars ? stop - start : 0u;

    b.p = out;
    b.n = 0;
    b.cap = out ? cap : 0u;
    smf_be(&b, 0x4D546864u, 4);                      /* MThd */
    smf_be(&b, 6, 4);
    smf_be(&b, 1, 2);
    smf_be(&b, tracks, 2);
    smf_be(&b, SMF_PPQ, 2);
    d.p = 0;                                         /* the tempo track, measured first */
    for (i = 0; i < 2u; i++) {
        smf_buf *w = i ? &b : &d;
        uint32_t len = d.n;
        d.n = 0;
        if (i) {
            smf_be(w, 0x4D54726Bu, 4);               /* MTrk */
            smf_be(w, len, 4);
        }
        if (name && *name)
            smf_name(w, name);
        us = (60000000u + (bpm ? bpm : 120u) / 2u) / (bpm ? bpm : 120u);
        smf_b(w, 0x00);
        smf_b(w, 0xFF);
        smf_b(w, 0x51);
        smf_b(w, 0x03);
        smf_be(w, us, 3);
        smf_b(w, 0x00);                              /* 4/4, 24 clocks a click, 8 32nds a quarter */
        smf_b(w, 0xFF);
        smf_b(w, 0x58);
        smf_b(w, 0x04);
        smf_be(w, 0x04021808u, 4);
        smf_vlq(w, eot);
        smf_b(w, 0xFF);
        smf_b(w, 0x2F);
        smf_b(w, 0x00);
    }
    for (ch = 0; ch < 16u; ch++) {
        uint32_t len;
        d.p = 0;
        d.n = d.cap = 0;
        if (!smf_chan(&d, ev, n, ch, start, stop, close, eot, &last))
            continue;
        len = d.n;
        smf_be(&b, 0x4D54726Bu, 4);
        smf_be(&b, len, 4);
        smf_chan(&b, ev, n, ch, start, stop, close, eot, &last);
    }
    return b.n;
}
