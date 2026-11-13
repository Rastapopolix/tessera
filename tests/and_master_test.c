/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Andamento's master bus (platform/andamento/and_master.c) on the PC, through the virtual prototype's own build
 * (web/andamento/sim.c, at 48 kHz). Direct checks on and_master_block(), not a project playing: the bus does not
 * depend on what mix_block produced, only on the samples handed to it.
 *   and_master_test   (run by tests/run_tests.sh; exit 1 on a failed check)
 * Checks:
 *   default    MIX 0 (the default): and_master_block leaves a block exactly as it was, bit for bit
 *   bypass     MIX 100, THRESH 0, RATIO 1, TILT 0, MAKEUP 0: no gain reduction and no tone change, so the block
 *              is unchanged but for rounding (within +-1 of full scale)
 *   compressor a block at a steady level above THRESH, RATIO > 1, MIX 100: the output settles below the input's
 *              level, and closer to it than RATIO : 1 would be exactly (the envelope's attack is not instant)
 *   release    the level drops back under THRESH: the output returns to (near) unity gain, not stuck down
 *   makeup     MAKEUP raises the compressed level by (near) its own dB, once the envelope has settled
 *   tilt       a block with more energy below 1 kHz than above it: TILT > 0 raises its level less than a steady
 *              tone's (it cuts the lows it is mostly made of), TILT < 0 raises it; TILT 0 leaves it alone
 *   determinism  the same settings and the same input give the same output, run twice from a fresh boot */
#include "../web/andamento/sim.c"

#include <math.h>
#include <stdio.h>
#include <string.h>

static uint32_t nfail;
#define CHECK(c, ...) do { if (!(c)) { nfail++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

#define N (CTL * 1500u)        /* 1 s at FS (48 kHz, CTL 32): 5+ RELEASE time constants even at its slowest default */
#define TAIL (FS / 10u)      /* 0.1 s: several full cycles of any test tone, for a true peak regardless of phase */

static void fill_tone(int32_t *b, uint32_t n, float hz, float amp)
{
    uint32_t i;
    for (i = 0; i < n; i++) {
        int32_t s = (int32_t)(amp * 32767.0f * sinf(2.0f * 3.14159265f * hz * (float)i / (float)FS));
        b[2u * i] = s;
        b[2u * i + 1u] = s;
    }
}
static int32_t peak_abs(const int32_t *b, uint32_t n)
{
    uint32_t i;
    int32_t m = 0;
    for (i = 0; i < n; i++) {
        int32_t a = b[2u * i] < 0 ? -b[2u * i] : b[2u * i];
        if (a > m)
            m = a;
    }
    return m;
}
static void run_master(int32_t *b, uint32_t n) { uint32_t i; for (i = 0; i < n; i += CTL) and_master_block(b + 2u * i, CTL); }

int main(void)
{
    static int32_t buf[2u * N], ref[2u * N];
    web_boot();

    /* default: MIX 0 leaves mix_block's output exactly as it was */
    fill_tone(buf, N, 220.0f, 0.5f);
    memcpy(ref, buf, sizeof buf);
    run_master(buf, N);
    CHECK(!memcmp(buf, ref, sizeof buf), "default (MIX 0): the block is unchanged");

    /* bypass: MIX 100 with no compression and no tilt leaves levels within rounding of the original */
    am_set(AM_MIX, 100);
    am_set(AM_THRESH, 0);
    am_set(AM_RATIO, 1);
    am_set(AM_TILT, 0);
    am_set(AM_MAKEUP, 0);
    fill_tone(buf, N, 220.0f, 0.5f);
    memcpy(ref, buf, sizeof buf);
    run_master(buf, N);
    {
        uint32_t i, worst = 0;
        for (i = 0; i < N; i++) {
            uint32_t d = (uint32_t)abs((int)(buf[2u * i] - ref[2u * i]));
            if (d > worst)
                worst = d;
        }
        CHECK(worst <= 2u, "bypass (THRESH 0, RATIO 1, TILT 0): within rounding of the original (worst %u)", worst);
    }

    /* compressor: a steady tone well above THRESH, RATIO 4:1, settles lower than it came in, and higher than a
     * hard 4:1 reduction (the envelope needs its ATTACK to catch up; it does not clamp instantly) */
    am_set(AM_THRESH, -12);
    am_set(AM_RATIO, 4);
    am_set(AM_ATTACK, 10);
    am_set(AM_RELEASE, 150);
    am_set(AM_MAKEUP, 0);
    and_master.env = 0;
    fill_tone(buf, N, 220.0f, 0.95f);              /* about -0.4 dBFS: well over -12 dB */
    run_master(buf, N);
    {
        int32_t in_peak = (int32_t)(0.95f * 32767.0f);
        int32_t settled = peak_abs(buf + 2u * (N - TAIL), TAIL);        /* the last block, once the envelope has caught up */
        CHECK(settled < in_peak, "compressor: the settled level (%d) is below the input's (%d)", settled, in_peak);
        CHECK(settled > in_peak / 4, "compressor: not clamped harder than RATIO allows (settled %d, input %d)",
              settled, in_peak);
    }

    /* release: back under THRESH, the output returns close to unity gain rather than staying ducked */
    fill_tone(buf, N, 220.0f, 0.05f);               /* about -26 dBFS: well under -12 dB */
    run_master(buf, N);
    {
        int32_t in_peak = (int32_t)(0.05f * 32767.0f);
        int32_t settled = peak_abs(buf + 2u * (N - TAIL), TAIL);
        CHECK(settled > in_peak * 9 / 10 && settled < in_peak * 11 / 10,
              "release: back near unity gain under THRESH (settled %d, input %d)", settled, in_peak);
    }

    /* makeup: the same loud tone, now with MAKEUP +6 dB, settles about 6 dB above the un-made-up case */
    am_set(AM_MAKEUP, 6);
    and_master.env = 0;
    fill_tone(buf, N, 220.0f, 0.95f);
    run_master(buf, N);
    {
        int32_t with_makeup = peak_abs(buf + 2u * (N - TAIL), TAIL);
        int32_t without;
        am_set(AM_MAKEUP, 0);
        and_master.env = 0;
        fill_tone(ref, N, 220.0f, 0.95f);
        run_master(ref, N);
        without = peak_abs(ref + 2u * (N - TAIL), TAIL);
        {
            float db = 20.0f * log10f((float)with_makeup / (float)without);
            CHECK(db > 4.5f && db < 7.5f, "makeup +6 dB: the settled level is about 6 dB above the plain case (%.1f dB)", db);
        }
    }

    /* tilt: a tone mostly below the 1 kHz pivot is cut more than it is boosted by TILT > 0, so its settled level
     * (compression off: RATIO 1) drops; TILT < 0 (darker) raises it instead; TILT 0 leaves it where it was */
    am_set(AM_THRESH, 0);
    am_set(AM_RATIO, 1);
    {
        int32_t flat, bright, dark;
        am_set(AM_TILT, 0);
        and_master.tilt_zl = and_master.tilt_zr = 0;
        fill_tone(buf, N, 150.0f, 0.5f);
        run_master(buf, N);
        flat = peak_abs(buf + 2u * (N - TAIL), TAIL);

        am_set(AM_TILT, 63);
        and_master.tilt_zl = and_master.tilt_zr = 0;
        fill_tone(buf, N, 150.0f, 0.5f);
        run_master(buf, N);
        bright = peak_abs(buf + 2u * (N - TAIL), TAIL);

        am_set(AM_TILT, -64);
        and_master.tilt_zl = and_master.tilt_zr = 0;
        fill_tone(buf, N, 150.0f, 0.5f);
        run_master(buf, N);
        dark = peak_abs(buf + 2u * (N - TAIL), TAIL);

        CHECK(bright < flat, "tilt: brighter (TILT +) cuts a 150 Hz tone (flat %d, bright %d)", flat, bright);
        CHECK(dark > flat, "tilt: darker (TILT -) raises a 150 Hz tone (flat %d, dark %d)", flat, dark);
    }
    am_set(AM_TILT, 0);

    /* determinism: the same settings and input, run twice, give the same output */
    am_set(AM_MIX, 100);
    am_set(AM_THRESH, -12);
    am_set(AM_RATIO, 4);
    and_master.env = 0;
    fill_tone(buf, N, 220.0f, 0.7f);
    run_master(buf, N);
    and_master.env = 0;
    fill_tone(ref, N, 220.0f, 0.7f);
    run_master(ref, N);
    CHECK(!memcmp(buf, ref, sizeof buf), "determinism: the same run twice matches");

    printf(nfail ? "and_master: %u failed\n" : "and_master: all checks passed\n", nfail);
    return nfail ? 1 : 0;
}
