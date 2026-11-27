/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* CLOCK's pulses (pulse.h), alone:
 *   - from the top at 120 BPM, 48 kHz, blocks of 256: a pulse every 6000 frames (a sixteenth) to the frame, each
 *     240 frames (5 ms) high, and one reset, with the first pulse;
 *   - a start mid-sixteenth waits for the next line (no short first step), the reset with it;
 *   - a loop back: a reset with the pulse after the jump; stopped: silence;
 *   - odd block sizes give the same frames as one long block.
 * usage: tessera-pulse-test */
#include <cstdio>
#include <vector>

#include "pulse.h"

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

static const double RATE = 48000, BPM = 120, STEP = BPM / 60.0 / RATE;

// frames total from beat b0, in blocks of `block` (a list of sizes, cycled): the clock and the reset
static void run(PulseClock &pc, double b0, size_t frames, const std::vector<uint32_t> &blocks, std::vector<float> &l,
                std::vector<float> &r, bool playing = true)
{
    l.assign(frames, 0);
    r.assign(frames, 0);
    size_t at = 0, i = 0;
    while (at < frames)
    {
        uint32_t n = blocks[i++ % blocks.size()];
        if (at + n > frames) n = (uint32_t)(frames - at);
        pc.run(&l[at], &r[at], n, playing, b0 + at * STEP, STEP, RATE);
        at += n;
    }
}

// the frames each run of 1s starts at, and the length of the first
static std::vector<size_t> rises(const std::vector<float> &v, size_t *width = nullptr)
{
    std::vector<size_t> out;
    for (size_t k = 0; k < v.size(); k++)
        if (v[k] > 0.5f && (k == 0 || v[k - 1] < 0.5f))
            out.push_back(k);
    if (width && !out.empty())
    {
        size_t k = out[0];
        while (k < v.size() && v[k] > 0.5f) k++;
        *width = k - out[0];
    }
    return out;
}

int main()
{
    std::vector<float> l, r;
    {   // from the top
        PulseClock pc;
        run(pc, 0.0, 48000, {256}, l, r);
        size_t w = 0;
        auto c = rises(l, &w), z = rises(r);
        bool even = c.size() == 8;
        for (size_t k = 0; even && k < c.size(); k++) even = c[k] == k * 6000;
        std::printf("from the top: %zu pulses (first at %zu, %zu frames high), %zu resets\n", c.size(),
                    c.empty() ? 0 : c[0], w, z.size());
        CHECK(even, "a pulse every 6000 frames from frame 0");
        CHECK(w == 240, "5 ms high (%zu)", w);
        CHECK(z.size() == 1 && z[0] == 0, "one reset, with the first pulse");
    }
    {   // odd blocks: the same frames
        PulseClock a, b;
        std::vector<float> l2, r2;
        run(a, 0.0, 30000, {30000}, l, r);
        run(b, 0.0, 30000, {1, 7, 333, 64, 1024, 5}, l2, r2);
        CHECK(l == l2 && r == r2, "block sizes change nothing");
    }
    {   // a start mid-sixteenth (beat 3.1: the next line is 3.25, 3600 frames on)
        PulseClock pc;
        run(pc, 3.1, 12000, {256}, l, r);
        auto c = rises(l), z = rises(r);
        size_t first = (size_t)((3.25 - 3.1) / STEP + 0.5);
        CHECK(!c.empty() && (c[0] == first || c[0] + 1 == first || c[0] == first + 1),
              "mid-sixteenth: the first pulse on the next line (%zu, not %zu)", c.empty() ? 0 : c[0], first);
        CHECK(z.size() == 1 && !c.empty() && z[0] == c[0], "the reset with it");
    }
    {   // a loop: beat 0..1 twice
        PulseClock pc;
        std::vector<float> l1, r1, l2, r2;
        run(pc, 0.0, 24000, {256}, l1, r1);
        run(pc, 0.0, 24000, {256}, l2, r2);
        auto z = rises(r2), c = rises(l2);
        CHECK(z.size() == 1 && z[0] == 0 && !c.empty() && c[0] == 0, "back to the top: the pulse and a reset at once");
    }
    {   // stopped
        PulseClock pc;
        run(pc, 0.0, 12000, {256}, l, r, false);
        CHECK(rises(l).empty() && rises(r).empty(), "stopped: silence");
    }
    std::printf("pulse test: %s\n", fails ? "FAILED" : "ok");
    return fails ? 1 : 0;
}
