/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* LISTEN (the window's menu): the notes on one MIDI channel steer Tessera instead of playing it.
 *   - from SPLIT up (note 48 unless set: a C, 12..108; given as a note number, since DAWs name octaves differently,
 *     note 48 C2 in some and C3 in others): what is held sets the key. One note: its ROOT, the scale kept. A chord:
 *     its root (the held note with a fifth or a third above it, the lowest such), and a major or minor third above
 *     that root turns the song's scale to its major or minor twin (MAJ <-> MIN, PEN <-> MPEN: tess_core_key);
 *   - under SPLIT: GENERATE, the strip's VARIATION, the seed from the note and its velocity, so a clip that plays the
 *     same note makes the same piece each time. A white key picks the style: the octave just under SPLIT C..B the
 *     styles 1..7, the octave under that 8..14, and so on down; a black key (or a white one past the last style)
 *     the style the strip shows. With SPLIT 48 a 49 or 61-key keyboard's bottom octave (36..47) is styles 1..7.
 * Nothing here touches the core: Listen says what a message asks for. On the audio thread, no allocation. */
#pragma once
#include <cstdint>

struct Listen
{
    enum { NONE, KEY, GENERATE };
    struct Ask
    {
        int what = NONE;
        int root = 0, minor = -1;   // KEY: 0..11; 1 minor, 0 major, -1 the scale as it is
        uint32_t seed = 0;          // GENERATE: 1..
        int style = -1;             // GENERATE: the style's index (0..), -1 the strip's
    };
    static const int SPLIT = 48, SPLIT_MIN = 12, SPLIT_MAX = 108;   // (SPLIT: the first key note, a C)

    int channel = 0;   // 0 off, 1..16
    int split = SPLIT;
    static bool split_ok(int s) { return s >= SPLIT_MIN && s <= SPLIT_MAX && s % 12 == 0; }
    // a GENERATE note's style: white keys from the octave under the split down, 7 an octave; -1 a black key
    int style_of(int key) const
    {
        static const int WHITE[12] = {0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6};
        int d = split - 1 - key, oct = d / 12, w = WHITE[11 - d % 12];
        return w < 0 ? -1 : oct * 7 + w;
    }
    uint32_t held[4] = {};

    bool mine(int ch) const { return channel > 0 && ch == channel - 1; }
    void clear() { held[0] = held[1] = held[2] = held[3] = 0; }

    // a note on (vel 1..127) or off (vel 0) on the LISTEN channel: what it asks for
    Ask note(int key, int vel)
    {
        Ask a;
        if (key < 0 || key > 127)
            return a;
        if (key < split)
        {
            if (vel > 0)
            {
                a.what = GENERATE;
                a.seed = 1u + (uint32_t)key * 128u + (uint32_t)vel;
                a.style = style_of(key);
            }
            return a;
        }
        uint32_t bit = 1u << (key & 31);
        if (vel <= 0)
        {
            held[key >> 5] &= ~bit;
            return a;   // (letting go keeps the key)
        }
        held[key >> 5] |= bit;
        return chord();
    }

    // the key the held notes make
    Ask chord() const
    {
        Ask a;
        int pcs = 0, lowest = -1;
        for (int k = split; k < 128; k++)
            if (held[k >> 5] >> (k & 31) & 1u)
            {
                pcs |= 1 << (k % 12);
                if (lowest < 0) lowest = k;
            }
        if (lowest < 0)
            return a;
        a.what = KEY;
        a.root = lowest % 12;
        if ((pcs & (pcs - 1)) == 0)
            return a;   // (one pitch: its root, the scale kept)
        auto has = [&](int r, int i) { return (pcs >> ((r + i) % 12) & 1) != 0; };
        int best = -1, best_score = 0;
        for (int i = 0; i < 12; i++)   // (from the lowest note up: the first root that scores best wins)
        {
            int r = (a.root + i) % 12;
            if (!(pcs >> r & 1)) continue;
            int score = (has(r, 7) ? 2 : 0) + (has(r, 4) || has(r, 3) ? 2 : 0);
            if (score) score += has(r, 10) || has(r, 11) ? 1 : 0;   // (a seventh: only beside a fifth or a third)
            if (score > best_score) best = r, best_score = score;
        }
        if (best < 0)
            return a;   // (no fifth, no third: the lowest note, the scale kept)
        a.root = best;
        if (has(best, 4) != has(best, 3))
            a.minor = has(best, 3) ? 1 : 0;
        return a;
    }
};
