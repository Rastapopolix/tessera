/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The core's audio (44.1 kHz, its own rate) to the host's: a windowed-sinc interpolator, the same for every channel of
 * a frame. Kernel: TAPS core frames (TAPS / 2 each side of the point played), a sinc low-passed below the lower of the
 * two Nyquists (0.95 of it: the transition band between there and the Nyquist), under a Kaiser window (beta 8.6:
 * about 90 dB down in the stop band); a table of PHASES fractional positions, the two nearest blended. The point played
 * is DELAY core frames behind the newest frame in (the host's latency: delay_host). At the core's own rate it is a
 * plain copy, no delay. Header only: the plugin (tessera_clap.cpp) and its test (resample_test.cpp). Not thread-safe:
 * the audio thread only. */
#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

template <unsigned NCH>
class Resampler
{
  public:
    static constexpr int TAPS = 64, DELAY = TAPS / 2, PHASES = 256;
    /* in_rate the core's, out_rate the host's */
    void setup(double in_rate, double out_rate)
    {
        step_ = in_rate / out_rate;
        same_ = std::fabs(step_ - 1.0) < 1e-12;
        double fc = (step_ > 1.0 ? 1.0 / step_ : 1.0) * 0.95;   // (the cutoff, as a fraction of the core's Nyquist)
        coef_.assign((size_t)(PHASES + 1) * TAPS, 0.0f);
        const double kPi = 3.14159265358979323846, beta = 8.6, i0b = bessel_i0(beta);
        for (int p = 0; p <= PHASES; p++)
        {
            double frac = (double)p / PHASES, sum = 0;
            float *c = &coef_[(size_t)p * TAPS];
            for (int j = 0; j < TAPS; j++)
            {
                double u = j - DELAY + frac;                     // (core frames from the point played)
                double x = u / DELAY, w = std::fabs(x) >= 1.0 ? 0.0 : bessel_i0(beta * std::sqrt(1.0 - x * x)) / i0b;
                double s = u == 0 ? 1.0 : std::sin(kPi * fc * u) / (kPi * fc * u);
                c[j] = (float)(fc * s * w);
                sum += c[j];
            }
            for (int j = 0; j < TAPS; j++) c[j] = (float)(c[j] / sum);   // (each phase passes DC whole)
        }
        reset();
    }
    void reset()
    {
        std::memset(hist_, 0, sizeof hist_);
        head_ = 0;
        pos_ = 1.0;
    }
    double step() const { return step_; }
    bool same() const { return same_; }
    /* the host's latency in its frames (the point played behind the newest frame in), 0 at the core's own rate */
    uint32_t delay_host() const { return same_ ? 0u : (uint32_t)std::lround(DELAY / step_); }
    /* core frames the point played lags the newest one in */
    uint32_t delay_core() const { return same_ ? 0u : (uint32_t)DELAY; }
    /* one host frame: more core frames from next(frame) while they are needed (it fills frame[NCH] and returns), then
     * each channel into out[c][k] (out[c] null: a channel nobody listens to, not worked out) */
    template <class Next>
    void frame(float *const *out, uint32_t k, Next &&next)
    {
        while (pos_ >= 1.0)
        {
            float f[NCH];
            next(f);
            head_ = (head_ + 1) % TAPS;
            for (unsigned c = 0; c < NCH; c++) hist_[c][head_] = hist_[c][head_ + TAPS] = f[c];   // (twice: no wrap)
            pos_ -= 1.0;
        }
        if (same_)
        {
            for (unsigned c = 0; c < NCH; c++)
                if (out[c]) out[c][k] = hist_[c][head_];
            pos_ += step_;
            return;
        }
        // the kernel at this fraction: the two nearest phases blended
        double ph = pos_ * PHASES;
        int p = (int)ph;
        float b = (float)(ph - p), a = 1.0f - b, kern[TAPS];
        const float *c0 = &coef_[(size_t)p * TAPS], *c1 = c0 + TAPS;
        for (int j = 0; j < TAPS; j++) kern[j] = a * c0[j] + b * c1[j];
        // hist_[c][head_ + TAPS - j] is the frame j back from the newest: tap j weighs it (u = j - DELAY + pos)
        for (unsigned c = 0; c < NCH; c++)
        {
            if (!out[c]) continue;
            const float *h = &hist_[c][head_ + TAPS];
            float acc = 0;
            for (int j = 0; j < TAPS; j++) acc += kern[j] * h[-j];
            out[c][k] = acc;
        }
        pos_ += step_;
    }

  private:
    static double bessel_i0(double x)
    {
        double s = 1, t = 1;
        for (int k = 1; k < 40; k++)
        {
            t *= (x / (2 * k)) * (x / (2 * k));
            s += t;
        }
        return s;
    }
    double step_ = 1, pos_ = 1;
    bool same_ = true;
    std::vector<float> coef_;
    float hist_[NCH][2 * TAPS] = {};
    int head_ = 0;
};
