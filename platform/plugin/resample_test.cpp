/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The resampler (resample.h), alone: the core's 44.1 kHz to a host's rate.
 *   - at 44.1 kHz a plain copy, no delay;
 *   - DC passes whole at every rate;
 *   - 44.1 -> 48 and -> 96 kHz: sines at 1, 10 and 18 kHz come out as the same sine DELAY core frames later, the error
 *     far under what the linear interpolation before it made (both printed);
 *   - 44.1 -> 32 kHz: a tone above the host's Nyquist is filtered out, not folded down.
 * usage: tessera-resample-test */
#include <cmath>
#include <cstdio>
#include <vector>

#include "resample.h"

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

static const double FS = 44100.0, kPi = 3.14159265358979323846;

// a sine at hz through the resampler to rate: the error against the ideal sine (dB under the sine), and the same
// for the linear interpolation the plugin had (its point played the frame between the last two)
static void sine(double rate, double hz, double &err_db, double &lin_db, double &out_db)
{
    Resampler<1> r;
    r.setup(FS, rate);
    uint64_t n = 0;
    double prev = 0, cur = 0, pos = 1.0, step = FS / rate;
    double e = 0, el = 0, s = 0, o = 0;
    const int N = (int)rate * 2, SKIP = (int)rate / 4;
    float y[1];
    float *out[1] = {y};
    uint64_t lin_n = 0;
    for (int k = 0; k < N; k++)
    {
        r.frame(out, 0, [&](float *f) { f[0] = (float)(0.5 * std::sin(2 * kPi * hz * (double)n++ / FS)); });
        while (pos >= 1.0) { prev = cur; cur = 0.5 * std::sin(2 * kPi * hz * (double)lin_n++ / FS); pos -= 1.0; }
        double lin = prev + (cur - prev) * pos;
        // the time played: the sinc's DELAY behind the newest frame; the linear one's between frames lin_n-2 and -1
        double t = (double)(n - 1) - Resampler<1>::DELAY + pos, tl = (double)(lin_n - 2) + pos;   // (the same fraction)
        pos += step;
        if (k < SKIP) continue;
        double want = 0.5 * std::sin(2 * kPi * hz * t / FS), want_l = 0.5 * std::sin(2 * kPi * hz * tl / FS);
        e += (y[0] - want) * (y[0] - want);
        el += (lin - want_l) * (lin - want_l);
        s += want * want;
        o += (double)y[0] * y[0];
    }
    err_db = 10 * std::log10(e / s + 1e-30);
    lin_db = 10 * std::log10(el / s + 1e-30);
    out_db = 10 * std::log10(o / s + 1e-30);
}
int main()
{
    {   // the core's own rate: a copy
        Resampler<2> r;
        r.setup(FS, FS);
        float a[64], b[64];
        float *out[2] = {a, b};
        int n = 0;
        for (int k = 0; k < 64; k++) r.frame(out, (uint32_t)k, [&](float *f) { f[0] = (float)n; f[1] = (float)-n; n++; });
        bool same = true;
        for (int k = 0; k < 64; k++) same = same && a[k] == (float)k && b[k] == (float)-k;
        CHECK(same && r.delay_host() == 0, "at 44.1 kHz a plain copy, no delay");
    }
    for (double rate : {32000.0, 48000.0, 88200.0, 96000.0})
    {
        Resampler<1> r;
        r.setup(FS, rate);
        float y[1];
        float *out[1] = {y};
        double worst = 0;
        for (int k = 0; k < 4000; k++)
        {
            r.frame(out, 0, [](float *f) { f[0] = 1.0f; });
            if (k > 1000) worst = std::fmax(worst, std::fabs(y[0] - 1.0));
        }
        CHECK(worst < 1e-5, "DC whole at %.0f Hz (off by %.2g)", rate, worst);
    }
    for (double rate : {48000.0, 96000.0})
        for (double hz : {1000.0, 10000.0, 18000.0})
        {
            double e, l, o;
            sine(rate, hz, e, l, o);
            std::printf("%5.0f Hz -> %.0f Hz: error %6.1f dB (linear %6.1f dB), level %+.2f dB\n", hz, rate, e, l, o);
            CHECK(e < -70 && e < l - 20, "%.0f Hz at %.0f: the error %.1f dB (linear %.1f)", hz, rate, e, l);
            CHECK(std::fabs(o) < 0.1, "%.0f Hz at %.0f: its level kept (%.2f dB)", hz, rate, o);
        }
    {   // 44.1 -> 32 kHz: 20 kHz is above the host's Nyquist (16 kHz): filtered, not folded to 12 kHz
        double e, l, o;
        sine(32000.0, 20000.0, e, l, o);
        std::printf("20000 Hz -> 32000 Hz: level %.1f dB\n", o);
        CHECK(o < -60, "20 kHz into 32 kHz filtered out (%.1f dB)", o);
    }
    std::printf(fails ? "resample test: %d FAILED\n" : "resample test: ok\n", fails);
    return fails ? 1 : 0;
}
