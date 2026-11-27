/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* CLOCK's pulses (the plugin's CLOCK port, CLOCK OUT PULSE): a clock and a reset for a modular, as audio, so they
 * land on the sample and the host lines them up with the music (a DC-coupled interface output into a clock input).
 * The clock: a pulse every 1/PPQN beat from the song's zero, MS long, at +1. A start (or a jump back: a loop, a
 * locate) pulses at once if it is on a pulse's line, else on the next one; the reset (the right) goes with that pulse.
 * Stopped: nothing (a pulse already high finishes). Real-time safe: no allocation. */
#pragma once
#include <cmath>
#include <cstdint>

struct PulseClock
{
    static constexpr double PPQN = 4, MS = 5;   // (sixteenths, as most modular clocks count; 5 ms high)
    int64_t last = INT64_MIN;   // the last pulse's number (beats x PPQN); INT64_MIN: none since the song (re)started
    bool reset_due = false;     // the reset waits for the start's first pulse
    uint32_t clk = 0, rst = 0;  // frames left high

    // n frames into l (the clock) and r (the reset; may be null): playing, the beat at frame 0, beats a frame, the rate
    void run(float *l, float *r, uint32_t n, bool playing, double b0, double step, double rate)
    {
        if (!playing)
            last = INT64_MIN;
        const uint32_t width = (uint32_t)(rate * MS / 1000.0);
        for (uint32_t k = 0; k < n; k++)
        {
            if (playing)
            {
                double b = b0 + k * step, x = b * PPQN + 1e-9;
                int64_t q = (int64_t)std::floor(x);
                if (q < last)   // (back: a loop or a locate, as a new start)
                    last = INT64_MIN;
                if (b >= -1e-9 && last == INT64_MIN)
                {   // on a line if it was crossed since the frame before
                    last = x - (double)q < step * PPQN + 2e-9 ? q - 1 : q;
                    reset_due = true;
                }
                if (b >= -1e-9 && q > last)
                {
                    clk = width;
                    if (reset_due)
                        rst = width;
                    reset_due = false;
                    last = q;
                }
            }
            l[k] = clk ? 1.0f : 0.0f;
            if (r)
                r[k] = rst ? 1.0f : 0.0f;
            if (clk) clk--;
            if (rst) rst--;
        }
    }
};
