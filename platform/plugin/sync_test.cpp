/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The DAW sync test (tess_core_host), without a host: blocks of 10 ms with the DAW's position, as the plugin passes it.
 *   - PLAY at beat 8 starts Tessera at beat 8 (Song Position), and it stays within 1/50 of a beat of the DAW for a minute
 *     at 97.3 BPM (a fractional tempo the FM-1's integer BPM cannot hold), then after a change to 133.7;
 *   - a loop back to beat 4 seeks there; STOP stops; the BPM shown follows the DAW's, rounded.
 * usage: tessera-sync-test */
#include <cmath>
#include <cstdio>

#include "tess_core.h"

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

static float L[1024], R[1024];
static double daw = 0;   // the DAW's position, beats

// one 10 ms block of the DAW at bpm: its position to the core, then the core's audio for it
static void block(bool playing, double bpm)
{
    tess_core_host(playing, 1, bpm, daw);
    tess_core_render(L, R, nullptr, tess_core_rate() / 100);
    if (playing)
        daw += bpm / 60.0 / 100.0;
}

// beats apart, ignoring where the song wraps (the bar counter is the sequencer's own; compare within 64 bars)
static double gap(double a, double b) { return a - b; }

int main()
{
    tess_core_boot();
    for (int i = 0; i < 100; i++) block(false, 97.3);
    CHECK(!tess_core_playing(), "stopped at first");

    daw = 8.0;
    double worst = 0;
    for (int i = 0; i < 6000; i++)   // 60 s
    {
        block(true, 97.3);
        if (i > 10)
            worst = std::fmax(worst, std::fabs(gap(tess_core_position(), daw)));
    }
    std::printf("97.3 BPM, 60 s from beat 8: DAW at %.3f, Tessera at %.3f, worst gap %.4f beats, BPM shown %u\n", daw,
                tess_core_position(), worst, tess_core_bpm());
    CHECK(tess_core_playing(), "the DAW's PLAY plays");
    CHECK(worst < 0.02, "Tessera should stay within 1/50 beat of the DAW (worst %.4f)", worst);
    CHECK(tess_core_bpm() == 97, "the BPM shown (%u)", tess_core_bpm());

    worst = 0;
    for (int i = 0; i < 3000; i++)   // 30 s at a new tempo
    {
        block(true, 133.7);
        if (i > 10)
            worst = std::fmax(worst, std::fabs(gap(tess_core_position(), daw)));
    }
    std::printf("133.7 BPM, 30 s: worst gap %.4f beats\n", worst);
    CHECK(worst < 0.02, "Tessera should follow a tempo change (worst %.4f)", worst);

    daw = 4.0;   // a loop back
    block(true, 133.7);
    block(true, 133.7);
    std::printf("loop to 4: Tessera at %.3f\n", tess_core_position());
    CHECK(std::fabs(gap(tess_core_position(), daw)) < 0.05, "a loop should seek (%.3f vs %.3f)", tess_core_position(), daw);

    block(false, 133.7);
    for (int i = 0; i < 10; i++) block(false, 133.7);
    CHECK(!tess_core_playing(), "the DAW's STOP stops");

    std::printf(fails ? "sync test: %d FAILED\n" : "sync test: ok\n", fails);
    return fails ? 1 : 0;
}
