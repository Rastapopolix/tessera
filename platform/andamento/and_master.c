/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Andamento's master bus: a one-knob tilt EQ, then a stereo-linked compressor, after Tessera's own output stage
 * (mix_block's master_out: DC blocker, limiter, soft clip, in mix.c) and before the board's DAC or the browser's
 * speakers. Not part of the FM-1 build: and_master_block() runs on mix_block's finished samples (int32, the Q15-ish
 * scale output_clip leaves them in), called once a control block by web/andamento/sim.c's web_ms() and, later,
 * platform/daisy's audio callback. Settings are an Andamento-only pid namespace (AM_PID + AM_*, like and_ext.c's
 * AE_PID), on the FX page's second sub-page (press FX again): song-wide, so every track's, not track 1's alone
 * (and_ui.c also shows it on an external track's FX page, which otherwise has no voice to send through fx.c).
 *
 *   TILT      a shelf down below 1 kHz and the complementary shelf up above it, so turning it barely moves the
 *             overall loudness; 0 flat, + brighter, - darker (named after the classic one-knob mastering EQ)
 *   THRESH,
 *   RATIO,
 *   ATTACK,
 *   RELEASE   a feedforward compressor on the peak of both channels together (one envelope, so a panned source
 *             doesn't pull the stereo image), after the tilt
 *   MAKEUP    trim after the gain reduction
 *   MIX       the whole stage (tilt then compressor) blended back with the dry signal; 0 is the default and
 *             leaves mix_block's output bit-identical (and_master_block returns at once, computing nothing),
 *             matching fx.c's own "OFF / MIX 0 changes nothing" rule
 *
 * Not saved in a project yet, as and_ext.c's settings aren't: a project format part is for later.
 * Included by and_ui.c after and_ext.c, so the FX page's cells can read and set AM_* through and_desc/and_get/
 * and_set; and_master_block() is called directly by the platform (web/andamento/sim.c, platform/daisy), not
 * through and_ui.c. */

#include <math.h>

#define AM_PID 0xE100u                       /* settings' pids: AM_PID + AM_* (and_ext.c's AE_PID is 0xE000, 12 wide) */

enum { AM_TILT, AM_THRESH, AM_RATIO, AM_ATTACK, AM_RELEASE, AM_MAKEUP, AM_MIX, AM_NPRM };

static const param_desc_t AM_DESC[AM_NPRM] = {
    [AM_TILT] = {"TILT", F_BIPCT, -64, 63, 0, 0, 0},            /* a bipolar %, as P_ED_FLT and the other mod amounts */
    [AM_THRESH] = {"THRESH", F_INT, -40, 0, -12, 0, "dB"},
    [AM_RATIO] = {"RATIO", F_INT, 1, 20, 4, 0, ":1"},
    [AM_ATTACK] = {"ATTACK", F_INT, 1, 200, 10, 0, "ms"},
    [AM_RELEASE] = {"RELEASE", F_INT, 10, 1000, 150, 0, "ms"},
    [AM_MAKEUP] = {"MAKEUP", F_INT, -12, 24, 0, 0, "dB"},
    [AM_MIX] = {"MIX", F_PCT, 0, 100, 0, 0, 0},                 /* 0: off, bit-identical (the default) */
};
static const param_desc_t AM_NONE_DESC = {"-", F_INT, 0, 0, 0, 0, 0};

static struct {
    int16_t v[AM_NPRM];
    float tilt_zl, tilt_zr;                  /* the tilt's low-shelf one-pole state, a channel each */
    float env;                               /* the compressor's envelope follower (linear, both channels) */
} and_master = {.v = {[AM_THRESH] = -12, [AM_RATIO] = 4, [AM_ATTACK] = 10, [AM_RELEASE] = 150}};

static const param_desc_t *am_desc(uint32_t x) { return x < AM_NPRM ? &AM_DESC[x] : &AM_NONE_DESC; }
static int32_t am_get(uint32_t x) { return x < AM_NPRM ? and_master.v[x] : 0; }
static void am_set(uint32_t x, int32_t v)
{
    if (x < AM_NPRM)
        and_master.v[x] = (int16_t)clamp(v, AM_DESC[x].min, AM_DESC[x].max);
}

#define AM_TILT_HZ 1000.0f                   /* the pivot: low enough to miss a bass voice, high enough to miss a lead */

/* a one-pole low-pass coefficient for a cutoff of hz at the platform's sample rate (FS: gen_tables.py, core.h) */
static inline float am_coef_hz(float hz) { return 1.0f - expf(-2.0f * 3.14159265f * hz / (float)FS); }
/* an envelope follower's coefficient for a time constant of ms (how much of the gap it closes each sample) */
static inline float am_coef_ms(float ms) { return 1.0f - expf(-1000.0f / (ms * (float)FS)); }
static inline int32_t am_clip16(int32_t x) { return x < -32768 ? -32768 : x > 32767 ? 32767 : x; }

/* mix_block's n samples (int32, interleaved L/R, out[2i]/out[2i+1]), processed in place. Called once a control
 * block: the coefficients below are recomputed every call (n is small, CTL, so the cost is trivial) rather than
 * cached against a changed setting, which the keys and ENERGY modules already do differently; here it is simpler
 * and no less correct, since MIX = 0 skips all of it anyway. */
static void and_master_block(int32_t *out, uint32_t n)
{
    uint32_t i;
    float mix, tilt, thresh, ratio, att, rel, makeup;
    if (!and_master.v[AM_MIX])                            /* the default: leave out[] exactly as mix_block left it */
        return;
    mix = (float)and_master.v[AM_MIX] * 0.01f;
    tilt = (float)and_master.v[AM_TILT] * (1.0f / 64.0f);  /* -1 .. ~0.98, as F_BIPCT reads it */
    thresh = (float)and_master.v[AM_THRESH];
    ratio = (float)and_master.v[AM_RATIO];
    att = am_coef_ms((float)and_master.v[AM_ATTACK]);
    rel = am_coef_ms((float)and_master.v[AM_RELEASE]);
    makeup = powf(10.0f, (float)and_master.v[AM_MAKEUP] * 0.05f);
    for (i = 0; i < n; i++) {
        float l = (float)out[2u * i] * (1.0f / 32768.0f), r = (float)out[2u * i + 1u] * (1.0f / 32768.0f);
        float dry_l = l, dry_r = r, peak, env_db, over, gain;
        if (tilt != 0.0f) {
            /* each channel's own low shelf (a one-pole low-pass); the high part is what's left of the signal.
             * Adding tilt * (high - low) back in boosts one side and cuts the other by the same amount, so the
             * two barely move the total level; tilt = 0 skips this (both channels pass through exactly). */
            and_master.tilt_zl += am_coef_hz(AM_TILT_HZ) * (l - and_master.tilt_zl);
            and_master.tilt_zr += am_coef_hz(AM_TILT_HZ) * (r - and_master.tilt_zr);
            l += tilt * (l - 2.0f * and_master.tilt_zl);
            r += tilt * (r - 2.0f * and_master.tilt_zr);
        }
        peak = fabsf(l) > fabsf(r) ? fabsf(l) : fabsf(r);
        and_master.env += (peak > and_master.env ? att : rel) * (peak - and_master.env);
        env_db = 20.0f * log10f(and_master.env < 0.000001f ? 0.000001f : and_master.env);
        over = env_db - thresh;
        gain = (over > 0.0f ? powf(10.0f, -(over - over / ratio) * 0.05f) : 1.0f) * makeup;
        l *= gain;
        r *= gain;
        out[2u * i] = am_clip16((int32_t)((dry_l * (1.0f - mix) + l * mix) * 32768.0f));
        out[2u * i + 1u] = am_clip16((int32_t)((dry_r * (1.0f - mix) + r * mix) * 32768.0f));
    }
}
