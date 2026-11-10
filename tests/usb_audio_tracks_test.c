/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Real render path (mix.c mix_part -> track_capture -> usb_audio_stream.c ua_audio): Melodee In's four channels are
 * the four tracks, each isolated, after DIST, the SLICER and LEVEL, independent of the monitor's pan, sends and
 * MASTER; a silent block clears its channel. */
#include <assert.h>
#define MELODEE_USB_AUDIO 1
#define main hostsim_main
#include "hostsim.c"
#undef main

struct result { uint64_t hash, energy, monitor, out_energy, differ; };

static struct result render(uint32_t channel, int sliced, int variant)
{
    int fd[2], status;
    struct result result = {2166136261u, 0, 0};
    assert(pipe(fd) == 0);
    pid_t child = fork();                     /* fresh DSP state for each comparison */
    assert(child >= 0);
    if (!child) {
        int32_t out[CTL * 2];
        track_t *t = &trk[channel];
        close(fd[0]);
        usb.up=usb.config=1;usb.suspended=0;ua.cap_alt=1;ua_cap_channels=4;
        host_tracks_init();
        for (uint32_t k = 0; k < NPART; k++) host_preset(&trk[k], HOST_SYNTH, HOST_PLAIN);
        t->p[P_RESERVED33] = 60;
        t->p[P_RESERVED45] = sliced ? 2 : 0;
        t->p[P_RESERVED46] = 1;
        t->p[P_RESERVED48] = 127;
        t->p[P_PAN] = variant == 1 ? -64 : variant == 2 ? 64 : 0;
        t->p[P_RESERVED34] = t->p[P_RESERVED35] = t->p[P_RESERVED36] = variant == 1 ? 127 : 0;
        song.master_q12 = variant == 2 ? 0 : 4096;
        if (variant == 3)
            t->p[P_LEVEL] = 0;
        trk_note_on(t, 60, 100);
        for (uint32_t block = 0; block < 2048; block++) {
            mix_block(out, CTL);
            ua.cw = ua.cr = 0;
            ua_audio(out, track_capture, CTL, song.master_q12);
            for (uint32_t i = 0; i < CTL; i++) {
                for (uint32_t ch = 0; ch < NTRK; ch++) {
                    int32_t x = track_capture[i * NTRK + ch];
                    assert(ua.cap[i * NTRK + ch] == ua_clip(x));
                    if (ch != channel) assert(x == 0);
                    else {
                        result.hash = (result.hash ^ (uint32_t)x) * 16777619u;
                        result.energy += (int64_t)x * x;
                    }
                }
                result.monitor += (int64_t)out[i * 2] * out[i * 2];
                result.monitor += (int64_t)out[i * 2 + 1] * out[i * 2 + 1];
            }
        }
        memset(trk, 0, sizeof trk);
        mix_block(out, CTL);
        for (uint32_t i = 0; i < CTL * NTRK; i++) assert(track_capture[i] == 0);
        assert(write(fd[1], &result, sizeof result) == sizeof result);
        _exit(0);
    }
    close(fd[1]);
    assert(read(fd[0], &result, sizeof result) == sizeof result);
    close(fd[0]);
    assert(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0);
    return result;
}

/* Compare every captured sample with the real finished mixer output. Separate
 * processes keep effect tails and limiter history identical between variants. */
static struct result stereo_render(int insert, int send, uint32_t master)
{
    int fd[2], status;
    struct result result = {2166136261u, 0, 0};
    assert(pipe(fd) == 0);
    pid_t child = fork();
    assert(child >= 0);
    if (!child) {
        int32_t out[CTL * 2];
        close(fd[0]);
        host_tracks_init();
        for (uint32_t k = 0; k < NPART; k++) host_preset(&trk[k], HOST_SYNTH, HOST_PLAIN);
        trk[0].p[P_PAN] = -40;
        trk[0].p[P_FXTYPE] = insert ? FX_ECHO : FX_OFF;
        trk[0].p[P_FXMIX] = 100;
        trk[0].p[P_FXRATE] = 30;
        trk[0].p[P_FXCHAR] = 80;
        trk[0].p[P_FXSEND] = send ? 127 : 0;
        song.g[G_MFXTYPE] = send ? FX_REVERB : FX_OFF;
        song.g[G_MFXMIX] = 127;
        song.g[G_FXRETURN] = 127;
        song.master_q12 = master;
        usb.up = usb.config = 1; usb.suspended = 0;
        ua.cap_alt = 1; ua_cap_channels = 2;
        for (uint32_t i = 0; i < CTL * NTRK; i++) track_capture[i] = 123456;
        trk_note_on(&trk[0], 60, 100);
        for (uint32_t block = 0; block < 2048; block++) {
            mix_block(out, CTL);
            assert(!track_capture_on && master_capture_on);
            for (uint32_t i = CTL * 2; i < CTL * NTRK; i++) assert(track_capture[i] == 123456);   /* (stereo: 2 a frame) */
            ua.cw = ua.cr = 0;
            ua_audio(out, track_capture, CTL, 4096);
            assert(ua.cw == CTL);
            for (uint32_t i = 0; i < CTL * 2; i++) {
                int32_t c = ua.cap[i];
                assert(out[i] >= -32768 && out[i] <= 32767);
                assert(c == ua_clip(track_capture[i]));
                result.differ += c != out[i];
                result.out_energy += (int64_t)out[i] * out[i];
                result.hash = (result.hash ^ (uint32_t)c) * 16777619u;
                result.energy += (int64_t)c * c;
                if (!(i & 1u)) result.monitor += abs(c - ua.cap[i + 1u]);
            }
        }
        assert(write(fd[1], &result, sizeof result) == sizeof result);
        _exit(0);
    }
    close(fd[1]);
    assert(read(fd[0], &result, sizeof result) == sizeof result);
    close(fd[0]);
    assert(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0);
    return result;
}

/* Issue #586: a six-note chord on the power-on TRIO track (track 2) peaked at +4 dBFS in its stem and the 16-bit
 * capture cut it hard. The stem now has the stereo recording's gain (half) and the outputs' soft clip: no sample at
 * the hard ceiling, the loud chord still loud, a single note at the stereo recording's level. */
static void stem_headroom(void)
{
    static const uint8_t chord[6] = {48, 55, 60, 64, 67, 71};
    int32_t out[CTL * 2];
    for (uint32_t notes = 1; notes <= 6; notes += 5) {
        int32_t stem_peak = 0, stereo_peak = 0;
        for (uint32_t stems = 0; stems <= 1; stems++) {
            memset(&ua, 0, sizeof ua);
            host_tracks_init();
            for (uint32_t k = 0; k < NPART; k++) host_power_on_sound(&trk[k], k);
            for (uint32_t k = 0; k < NPART; k++) for (uint32_t v = 0; v < NVOICE; v++) trk[k].v[v].active = 0;
            usb.up = usb.config = 1; usb.suspended = 0; ua.cap_alt = 1; ua_cap_channels = stems ? 4 : 2;
            for (uint32_t i = 0; i < notes; i++) trk_note_on(&trk[1], chord[i], 100);
            for (uint32_t block = 0; block < 1500; block++) {
                mix_block(out, CTL);
                ua.cw = ua.cr = 0;
                ua_audio(out, track_capture, CTL, 4096);
                for (uint32_t i = 0; i < CTL; i++) {
                    int32_t c = stems ? ua.cap[i * NTRK + 1u] : ua.cap[2u * i];
                    c = c < 0 ? -c : c;
                    assert(c < 32767);
                    if (stems && c > stem_peak) stem_peak = c;
                    if (!stems && c > stereo_peak) stereo_peak = c;
                }
            }
        }
        if (notes == 6)
            assert(stem_peak > 20000);                      /* (still loud: soft, not quietened) */
        else                                                /* a lone centred note: the stereo recording's level */
            assert(stem_peak > stereo_peak * 9 / 10 && stem_peak < stereo_peak * 11 / 10);
    }
    puts("USB track capture: a loud chord's stem soft-clipped, not cut; a single note at the stereo level: OK");
}

int main(void)
{
    struct result dry = stereo_render(0, 0, 4096);
    struct result insert = stereo_render(1, 0, 4096);
    struct result wet = stereo_render(1, 1, 4096);
    assert(dry.energy && dry.monitor && insert.energy && wet.energy);
    assert(dry.hash != insert.hash && insert.hash != wet.hash);
    struct result half = stereo_render(1, 1, 2048), off = stereo_render(1, 1, 0);
    assert(!half.differ && wet.differ);                     /* at MASTER's default the recording is the output */
    assert(off.out_energy == 0 && half.out_energy < wet.out_energy);   /* MASTER sets the output ... */
    assert(off.hash == wet.hash && half.hash == wet.hash);   /* ... and never the recording */
    puts("USB stereo: post-insert/send/pan/limiter copy before MASTER, no stem collection: OK");
    for (uint32_t ch = 0; ch < NTRK; ch++) {
        struct result dry = render(ch, 0, 0);
        assert(dry.energy && dry.monitor);
        for (int sliced = 0; sliced <= 1; sliced++) {
            struct result base = render(ch, sliced, 0);
            assert(base.energy && base.monitor);
            if (sliced) assert(base.hash == dry.hash);
            for (int variant = 1; variant <= 2; variant++) {
                struct result changed = render(ch, sliced, variant);
                assert(changed.hash == base.hash && changed.energy == base.energy);
                if (variant == 2) assert(changed.monitor == 0);
            }
            assert(render(ch, sliced, 3).energy == 0);
        }
    }
    puts("USB track capture: tracks 1..4 isolated, ignored legacy effects, level, pan/master isolation, stale blocks: OK");
    stem_headroom();                                        /* (last: it leaves voices sounding) */
    return 0;
}
