/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* UAC1 stereo playback and selectable stereo master / four-track capture.
 * Callers serialize USB service and each short audio block. Capture precedes
 * computer playback, so DAW monitoring cannot feed itself through capture.
 *
 * The I2S clock is independent of USB SOF. A low-pass ring-fill servo adjusts
 * capture packet lengths and the playback endpoint's explicit 10.14 feedback.
 * USB and I2S both run at the native 44.1 kHz rate; no resampler is needed.
 *
 * Once a stream runs, a ring that runs short or full is mended, not reset (#586): an
 * underrun sends what it has and pads only the missing frames with the last sample
 * decaying; an overrun drops the oldest frames back to the target. Either way the
 * stream crossfades back in over UA_FADE frames. A reset cost 11.6 ms of silence. */
#include <stdint.h>
#define UA_RATE 44100u
#define UA_CAP_CHANNELS 4u                    /* maximum; active stride follows the enumerated mode */
static uint8_t ua_cap_channels = 2u;
#define UA_MAX_FRAMES 45u                      /* ceil(44.1 + maximum feedback correction) */
#define UA_PLAY_PACKET (UA_MAX_FRAMES * 2u * 3u)
#define UA_PACKET (UA_MAX_FRAMES * UA_CAP_CHANNELS * 3u)
#define UA_RING 1024u
#define UA_TARGET 512u
#define UA_FADE 32u                             /* frames: a mended gap's crossfade back into the stream */
#define UA_NOMINAL ((UA_RATE * 16384u) / 1000u)
#ifdef __APPLE__
#define UA_POOL                                 /* (the host tests: Mach-O has no such section) */
#else
#define UA_POOL __attribute__((section(".pool")))   /* the rings, 12 KiB: the pool (zeroed at boot), not RAM */
#endif

static struct {
    uint8_t play_alt, cap_alt, play_ready, cap_ready;
    uint32_t pw, pr, cw, cr, cap_frac;
    int32_t play_fill_q8, cap_fill_q8;
    uint32_t play_underruns, play_overruns, cap_underruns, cap_overruns, bad_packets;
    uint32_t rx_packets, tx_packets, missed_frames;
    uint32_t poll_max_ticks, service_max_ticks;
    uint8_t play_ramp, cap_ramp;               /* frames of crossfade left after a mended gap */
    int16_t play_last[2], cap_last[UA_CAP_CHANNELS];
    int16_t play[UA_RING * 2u], cap[UA_RING * UA_CAP_CHANNELS];
} ua UA_POOL;

static int32_t ua_clip(int32_t x)
{
    return x > 32767 ? 32767 : x < -32768 ? -32768 : x;
}

static uint32_t ua_sample_bytes(uint8_t alt) { return alt == 2u ? 3u : 2u; }

/* Packed little-endian PCM24 uses the same full-scale level as PCM16. The
 * low byte is discarded on playback; recording pads it with zero. */
static int16_t ua_decode(const uint8_t *p, uint32_t width)
{
    p += width - 2u;
    return (int16_t)(p[0] | (uint16_t)p[1] << 8);
}

static void ua_encode(uint8_t *p, uint32_t width, int16_t sample)
{
    if (width == 3u)
        *p++ = 0;
    p[0] = (uint8_t)sample;
    p[1] = (uint8_t)((uint16_t)sample >> 8);
}

#define UA_VOLUME_DEFAULT 50u                  /* USB VOLUME: the old level at the default MASTER (0.5) */
static uint8_t settings_usb_volume = UA_VOLUME_DEFAULT;   /* MENU > USB VOLUME, 0..100 % (settings_persist.c) */

static void ua_play_reset(void)
{
    ua.pw = ua.pr = 0;
    ua.play_ready = ua.play_ramp = 0;
    ua.play_last[0] = ua.play_last[1] = 0;
    ua.play_fill_q8 = UA_TARGET * 256;
}

static void ua_cap_reset(void)
{
    uint32_t ch;
    ua.cw = ua.cr = ua.cap_frac = 0;
    ua.cap_ready = ua.cap_ramp = 0;
    for (ch = 0; ch < UA_CAP_CHANNELS; ch++)
        ua.cap_last[ch] = 0;
    ua.cap_fill_q8 = UA_TARGET * 256;
}

/* One sample of a mended gap: the last sample decays by 7/8 a frame and fades out as
 * the stream's sample s fades in over UA_FADE frames (ramp UA_FADE: all tail, a pad). */
static int16_t ua_mend(int16_t *last, int32_t s, uint32_t ramp)
{
    int32_t tail = *last * 7 / 8;
    s = (s * (int32_t)(UA_FADE - ramp) + tail * (int32_t)ramp) / (int32_t)UA_FADE;
    return *last = (int16_t)s;
}

static void ua_reset(void)
{
    ua.play_alt = ua.cap_alt = 0;
    ua_play_reset();
    ua_cap_reset();
}

static void ua_receive(const uint8_t *p, uint32_t bytes)
{
    uint32_t i, width = ua_sample_bytes(ua.play_alt), frame = width * 2u;
    uint32_t n = bytes / frame;
    if (!ua.play_alt)
        return;
    if (n > UA_MAX_FRAMES || bytes % frame) {
        ua.bad_packets++;
        return;
    }
    if (ua.pw - ua.pr + n > UA_RING) {
        ua.play_overruns++;
        ua.pr = ua.pw + n - UA_TARGET;         /* drop the oldest, never replay stale data */
        ua.play_ramp = UA_FADE;
    }
    for (i = 0; i < n; i++) {
        uint32_t at = (ua.pw & (UA_RING - 1u)) * 2u;
        ua.play[at] = ua_decode(p + frame * i, width);
        ua.play[at + 1u] = ua_decode(p + frame * i + width, width);
        ua.pw++;
    }
    ua.rx_packets++;
}

/* Called once per observed USB frame, not once per poll or audio callback. */
static void ua_sof(void)
{
    if (ua.play_ready)
        ua.play_fill_q8 += ((int32_t)(ua.pw - ua.pr) * 256 - ua.play_fill_q8) / 32;
    if (ua.cap_ready)
        ua.cap_fill_q8 += ((int32_t)(ua.cw - ua.cr) * 256 - ua.cap_fill_q8) / 32;
}

static uint32_t ua_rate(int32_t error_q8)
{
    int32_t correction = error_q8 / 16;         /* fill error / 1024, in 10.14 */
    if (correction > 8192)
        correction = 8192;
    if (correction < -8192)
        correction = -8192;
    return (uint32_t)((int32_t)UA_NOMINAL + correction);
}

static uint32_t ua_feedback(void)
{
    return ua_rate(UA_TARGET * 256 - ua.play_fill_q8);
}

static uint32_t ua_transmit(uint8_t *p)
{
    uint32_t i, n, have = 0, width = ua_sample_bytes(ua.cap_alt), gap = 0;
    ua.cap_frac += ua_rate(ua.cap_fill_q8 - UA_TARGET * 256);
    n = ua.cap_frac >> 14;
    ua.cap_frac &= 16383u;
    if (!ua.cap_ready && ua.cw - ua.cr >= UA_TARGET)
        ua.cap_ready = 1;
    if (ua.cap_ready) {
        have = ua.cw - ua.cr;
        if (have < n) {
            ua.cap_underruns++;                 /* send what there is, pad the rest */
            gap = 1;
        } else
            have = n;
    }
    for (i = 0; i < n; i++) {
        int16_t sample[UA_CAP_CHANNELS] = {0};
        uint32_t ch, at = (ua.cr & (UA_RING - 1u)) * ua_cap_channels;
        for (ch = 0; ch < ua_cap_channels; ch++) {
            if (i < have)
                sample[ch] = ua.cap_ramp ? ua_mend(&ua.cap_last[ch], ua.cap[at + ch], ua.cap_ramp)
                                         : (ua.cap_last[ch] = ua.cap[at + ch]);
            else if (gap)
                sample[ch] = ua_mend(&ua.cap_last[ch], 0, UA_FADE);
            ua_encode(p + (i * ua_cap_channels + ch) * width, width, sample[ch]);
        }
        if (i < have) {
            ua.cr++;
            if (ua.cap_ramp)
                ua.cap_ramp--;
        }
    }
    if (gap)
        ua.cap_ramp = UA_FADE;
    return n * width * ua_cap_channels;
}

/* Stereo Q15 post-FX/limiter mix, or four mono stems, both before MASTER: the
 * volume pot never sets the recording's level (upstream 1d02852). Stereo reads the
 * mixer's copy at a fixed MASTER (tracks: L, R a frame), or the output for a block
 * collected while capture was off. Playback has its own level (volume_q12: USB
 * VOLUME, apart from MASTER; upstream cc28729).
 * Call in blocks of CTL (32), with USB service excluded during this copy.
 * In track mode a null stem pointer skips capture for a block collected while
 * capture was disabled. */
static void ua_audio(int32_t *out, const int32_t *tracks, uint32_t n, uint32_t volume_q12)
{
    uint32_t i, capture = ua.cap_alt && (ua_cap_channels == 2u || tracks), playback = 0, have = n;
    if (capture && ua.cw - ua.cr + n > UA_RING) {
        ua.cap_overruns++;
        ua.cr = ua.cw + n - UA_TARGET;          /* drop the oldest, keep the stream */
        ua.cap_ramp = UA_FADE;
    }
    if (ua.play_alt) {
        if (!ua.play_ready && ua.pw - ua.pr >= UA_TARGET)
            ua.play_ready = 1;
        if (ua.play_ready) {
            playback = 1;
            if (ua.pw - ua.pr < n) {
                ua.play_underruns++;            /* play what there is, pad the rest */
                have = ua.pw - ua.pr;
            }
        }
    }
    for (i = 0; i < n; i++) {
        uint32_t ci = ((ua.cw + i) & (UA_RING - 1u)) * ua_cap_channels;
        uint32_t pi = ((ua.pr + i) & (UA_RING - 1u)) * 2u;
        int32_t l = out[2u * i], r = out[2u * i + 1u];
        if (capture) {
            uint32_t ch;
            if (ua_cap_channels == 2u) {
                ua.cap[ci] = (int16_t)(tracks ? ua_clip(tracks[2u * i]) : l);
                ua.cap[ci + 1u] = (int16_t)(tracks ? ua_clip(tracks[2u * i + 1u]) : r);
            } else
                for (ch = 0; ch < UA_CAP_CHANNELS; ch++)
                    ua.cap[ci + ch] = (int16_t)ua_clip(tracks[i * UA_CAP_CHANNELS + ch]);
        }
        if (playback) {
            int32_t pl = 0, pr = 0;
            if (i < have) {
                pl = ua.play[pi];
                pr = ua.play[pi + 1u];
                if (ua.play_ramp) {
                    pl = ua_mend(&ua.play_last[0], pl, ua.play_ramp);
                    pr = ua_mend(&ua.play_last[1], pr, ua.play_ramp--);
                } else {
                    ua.play_last[0] = (int16_t)pl;
                    ua.play_last[1] = (int16_t)pr;
                }
            } else {
                pl = ua_mend(&ua.play_last[0], 0, UA_FADE);
                pr = ua_mend(&ua.play_last[1], 0, UA_FADE);
            }
            l += (pl * (int32_t)volume_q12) >> 12;
            r += (pr * (int32_t)volume_q12) >> 12;
            out[2u * i] = ua_clip(l);
            out[2u * i + 1u] = ua_clip(r);
        }
    }
    if (capture)
        ua.cw += n;
    if (playback) {
        ua.pr += have;
        if (have < n)
            ua.play_ramp = UA_FADE;
    }
}
