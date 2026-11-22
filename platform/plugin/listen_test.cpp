/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* LISTEN (listen.h), alone: what the notes on its channel ask for.
 *   - only its own channel; OFF hears none;
 *   - one note: its ROOT, the scale kept; a chord: its root (inversions too) and major or minor; a fifth alone: the
 *     root, the scale kept; letting go keeps the key;
 *   - under C1: GENERATE, the same note and velocity the same seed, another a different one; its note-off nothing.
 * usage: tessera-listen-test */
#include <cstdio>
#include <initializer_list>

#include "listen.h"

static int fails = 0;
#define CHECK(c, ...)                          \
    do                                         \
    {                                          \
        if (!(c))                              \
        {                                      \
            std::printf("FAIL: " __VA_ARGS__); \
            std::printf("\n");                 \
            fails++;                           \
        }                                      \
    } while (0)

// a chord played (all on), what the last note asks; let go after
static Listen::Ask play(Listen &l, std::initializer_list<int> keys)
{
    Listen::Ask a;
    for (int k : keys) a = l.note(k, 100);
    for (int k : keys) l.note(k, 0);
    return a;
}

int main()
{
    Listen l;
    CHECK(!l.mine(0) && !l.mine(15), "OFF hears no channel");
    l.channel = 16;
    CHECK(l.mine(15) && !l.mine(0), "16 hears channel 16 only");

    Listen::Ask a = play(l, {62});
    CHECK(a.what == Listen::KEY && a.root == 2 && a.minor == -1, "D alone: ROOT D, the scale kept (%d %d)", a.root, a.minor);
    a = play(l, {60, 64, 67});
    CHECK(a.what == Listen::KEY && a.root == 0 && a.minor == 0, "C E G: C major (%d %d)", a.root, a.minor);
    a = play(l, {57, 60, 64});
    CHECK(a.root == 9 && a.minor == 1, "A C E: A minor (%d %d)", a.root, a.minor);
    a = play(l, {52, 55, 60});
    CHECK(a.root == 0 && a.minor == 0, "E G C (first inversion): C major (%d %d)", a.root, a.minor);
    a = play(l, {55, 60, 63});
    CHECK(a.root == 0 && a.minor == 1, "G C Eb (second inversion): C minor (%d %d)", a.root, a.minor);
    a = play(l, {43, 50, 53, 59});
    CHECK(a.root == 7 && a.minor == 0, "G D F B (G7): G major (%d %d)", a.root, a.minor);
    a = play(l, {45, 52});
    CHECK(a.root == 9 && a.minor == -1, "A E (a fifth): A, the scale kept (%d %d)", a.root, a.minor);
    a = play(l, {60, 61});
    CHECK(a.root == 0 && a.minor == -1, "C C# (no fifth, no third): the lowest, the scale kept (%d %d)", a.root, a.minor);
    a = l.note(64, 0);
    CHECK(a.what == Listen::NONE, "a note let go asks nothing");

    a = l.note(12, 90);
    Listen::Ask b = l.note(12, 90), c = l.note(13, 90), d = l.note(12, 91);
    CHECK(a.what == Listen::GENERATE && a.seed && a.seed == b.seed, "under C1: GENERATE, the same note the same seed");
    CHECK(c.seed != a.seed && d.seed != a.seed, "another note or velocity: another seed");
    CHECK(l.note(12, 0).what == Listen::NONE, "its note-off: nothing");
    CHECK(l.note(23, 1).what == Listen::GENERATE && l.note(24, 1).what == Listen::KEY, "C1 is the first key note");
    CHECK(l.note(-1, 1).what == Listen::NONE && l.note(128, 1).what == Listen::NONE, "out of range: nothing");

    std::printf(fails ? "listen test: %d FAILED\n" : "listen test: ok\n", fails);
    return fails ? 1 : 0;
}
