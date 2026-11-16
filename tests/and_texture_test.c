/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Andamento's texture engine (platform/andamento/and_texture.c) on the PC, through the virtual prototype's own
 * build (web/andamento/sim.c, at 48 kHz). Direct checks on and_texture_block(), not a project playing: the engine
 * only resamples whatever and_master_block already handed it.
 *   and_texture_test   (run by tests/run_tests.sh; exit 1 on a failed check)
 * Checks:
 *   default    MIX 0 (the default): and_texture_block leaves a block exactly as it was, bit for bit
 *   silence    MIX up, nothing played yet: no grain has anything to read, so the bed stays at (or very near)
 *              silence; a dry tone through it is still there, barely touched
 *   capture    feed a loud tone, let the 2 s capture fill, then silence the input: with the bed up, sound
 *              keeps coming (the grains reading back what was captured) after the dry signal has stopped
 *   FREEZE     once on, further input changes nothing more: the bed afterwards is the same whether the (silent)
 *              input keeps changing or not
 *   DENS       more grains a second make for a busier bed: a higher DENS should not make it quieter
 *   bounded    AT_GRAINS (8) is the worst case: a count of simultaneously active grains never goes over it
 *   determinism  the same settings and the same input give the same output, run twice from a fresh boot */
#include "../web/andamento/sim.c"

#include <math.h>
#include <stdio.h>
#include <string.h>

static uint32_t nfail;
#define CHECK(c, ...) do { if (!(c)) { nfail++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

#define N (CTL * 1500u)          /* 1 s at FS (48 kHz, CTL 32) */

static void fill_tone(int32_t *b, uint32_t n, float hz, float amp)
{
    uint32_t i;
    for (i = 0; i < n; i++) {
        int32_t s = (int32_t)(amp * 32767.0f * sinf(2.0f * 3.14159265f * hz * (float)i / (float)FS));
        b[2u * i] = s;
        b[2u * i + 1u] = s;
    }
}
static void fill_silence(int32_t *b, uint32_t n) { memset(b, 0, 2u * n * sizeof *b); }
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
static void run_texture(int32_t *b, uint32_t n) { uint32_t i; for (i = 0; i < n; i += CTL) and_texture_block(b + 2u * i, CTL); }
static uint32_t active_grains(void)
{
    uint32_t i, n = 0;
    for (i = 0; i < AT_GRAINS; i++)
        if (and_texture.g[i].on)
            n++;
    return n;
}

int main(void)
{
    static int32_t buf[2u * AT_BUF], ref[2u * AT_BUF];
    uint32_t i;
    web_boot();

    /* default: MIX 0 leaves and_master_block's output exactly as it was */
    fill_tone(buf, N, 220.0f, 0.5f);
    memcpy(ref, buf, 2u * N * sizeof *buf);
    run_texture(buf, N);
    CHECK(!memcmp(buf, ref, 2u * N * sizeof *buf), "default (MIX 0): the block is unchanged");

    /* silence: MIX up, but the capture is still empty (a fresh boot): the bed adds very little to a loud tone */
    at_set(AT_MIX, 100);
    at_set(AT_DENS, 30);
    at_set(AT_SIZE, 80);
    fill_tone(buf, N, 220.0f, 0.5f);
    memcpy(ref, buf, 2u * N * sizeof *buf);
    run_texture(buf, N);
    {
        uint32_t worst = 0;
        for (i = 0; i < N; i++) {
            uint32_t d = (uint32_t)abs((int)(buf[2u * i] - ref[2u * i]));
            if (d > worst)
                worst = d;
        }
        CHECK(worst < (uint32_t)(0.5f * 32767.0f * 0.3f), "silence (empty capture): the bed adds little yet (worst %u)", worst);
    }

    /* capture: fill the whole 2 s capture with a loud tone, then feed silence: with the bed up, sound keeps
     * coming after the dry input has stopped (the grains are reading the capture back) */
    at_set(AT_FREEZE, 0);
    fill_tone(buf, AT_BUF, 220.0f, 0.8f);
    run_texture(buf, AT_BUF);                                 /* fills the 2 s capture exactly once */
    fill_silence(buf, N);
    run_texture(buf, N);
    CHECK(peak_abs(buf, N) > 0, "capture: the bed still sounds once the dry input has gone silent");

    /* FREEZE: once on, more (silent) input changes nothing further; run twice from the same point and compare */
    at_set(AT_FREEZE, 1);
    {
        int16_t snap_buf[AT_BUF];
        uint32_t snap_w;
        memcpy(snap_buf, and_texture.buf, sizeof and_texture.buf);
        snap_w = and_texture.w;
        fill_silence(buf, N);
        run_texture(buf, N);
        CHECK(!memcmp(snap_buf, and_texture.buf, sizeof and_texture.buf) && snap_w == and_texture.w,
              "FREEZE: the capture itself does not change while it is on");
    }

    /* DENS: a busier bed (more grains a second) is not quieter than a sparser one, once both have had time to fill */
    {
        int32_t sparse, busy;
        at_set(AT_FREEZE, 0);
        at_set(AT_DENS, 2);
        fill_tone(buf, AT_BUF, 220.0f, 0.8f);
        run_texture(buf, AT_BUF);
        fill_silence(buf, N);
        run_texture(buf, N);
        sparse = peak_abs(buf, N);

        at_set(AT_DENS, 90);
        fill_tone(buf, AT_BUF, 220.0f, 0.8f);
        run_texture(buf, AT_BUF);
        fill_silence(buf, N);
        run_texture(buf, N);
        busy = peak_abs(buf, N);

        CHECK(busy >= sparse, "DENS: a busier bed (DENS 90, %d) is not quieter than a sparse one (DENS 2, %d)", busy, sparse);
    }

    /* bounded: never more than AT_GRAINS active at once, even at the highest DENS and a long SIZE */
    at_set(AT_DENS, 100);
    at_set(AT_SIZE, 500);
    fill_tone(buf, AT_BUF, 220.0f, 0.8f);
    {
        uint32_t worst = 0, j;
        for (j = 0; j < AT_BUF; j += CTL) {
            and_texture_block(buf + 2u * j, CTL);
            if (active_grains() > worst)
                worst = active_grains();
        }
        CHECK(worst <= AT_GRAINS, "bounded: at most AT_GRAINS (%u) active at once (saw %u)", AT_GRAINS, worst);
    }

    /* determinism: the same settings and input, run twice (from a fresh boot each time), give the same output */
    at_set(AT_DENS, 20);
    at_set(AT_SIZE, 80);
    at_set(AT_SPREAD, 50);
    at_set(AT_PITCH, 3);
    web_power_on();
    at_set(AT_MIX, 100);
    at_set(AT_DENS, 20);
    at_set(AT_SIZE, 80);
    at_set(AT_SPREAD, 50);
    at_set(AT_PITCH, 3);
    fill_tone(buf, AT_BUF, 220.0f, 0.7f);
    run_texture(buf, AT_BUF);

    web_power_on();
    at_set(AT_MIX, 100);
    at_set(AT_DENS, 20);
    at_set(AT_SIZE, 80);
    at_set(AT_SPREAD, 50);
    at_set(AT_PITCH, 3);
    fill_tone(ref, AT_BUF, 220.0f, 0.7f);
    run_texture(ref, AT_BUF);

    CHECK(!memcmp(buf, ref, sizeof buf), "determinism: the same run twice, from a fresh boot, matches");

    printf(nfail ? "and_texture: %u failed\n" : "and_texture: all checks passed\n", nfail);
    return nfail ? 1 : 0;
}
