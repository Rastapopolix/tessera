/* SPDX-License-Identifier: GPL-3.0-only */
/* SYNC ECHO (fx.c FX_SYNC): TIME steps through note values at the tempo playing; a value longer than the ring halves
 * until it fits (the FM-1's 4096-sample ring: 371 ms; the plugin's MELODEE_ECHO_RING=32768: ~3 s); TIME's names; the
 * ring's allocation. Built twice by run_tests.sh: at the FM-1's ring and at the plugin's */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>

static int rate_of(uint32_t div)                         /* the lowest RATE that picks note value div (0..13) */
{
    for (int r = 0; r < 128; r++) if (((uint32_t)r * 14u >> 7) == div) return r;
    return -1;
}
/* an impulse through track 1's FX 1 (no feedback, full wet): the first echo's place, in samples at FS */
static uint32_t echo_at(int bpm, uint32_t div)
{
    fx_clear_all(); memset(fx_state, 0, sizeof fx_state);
    for (uint32_t k = 0; k < FX_STATE_COUNT; k++) resource_release(RES_FX0 + k);
    memset(trk, 0, sizeof trk);
    song.g[G_BPM] = (int16_t)bpm;
    track_t *t = &trk[0];
    t->p[P_FXTYPE] = FX_SYNC; t->p[P_FXMIX] = 127; t->p[P_FXRATE] = (int16_t)rate_of(div); t->p[P_FXCHAR] = 0;
    fx_state[0].mix = 32768;
    int32_t b[CTL];
    uint32_t best = 0, n = 0, len = (uint32_t)FS * 4u;
    int32_t peak = 0;
    for (uint32_t frame = 0; frame < len / CTL; frame++) {
        memset(b, 0, sizeof b);
        if (!frame) b[0] = b[1] = b[2] = b[3] = 20000;   /* (four samples: one quarter-rate tick) */
        assert(fx_process(t, b, CTL));
        for (uint32_t i = 0; i < CTL; i++, n++) {
            int32_t v = fx_left[i] < 0 ? -fx_left[i] : fx_left[i];
            if (n > 64u && v > peak) peak = v, best = n;
        }
    }
    assert(peak > 1000);
    assert(resource[RES_FX0].size == MELODEE_ECHO_RING * 2u);
    return best;
}
static void expect(int bpm, uint32_t div, const char *name)
{
    char v[8]; const char *u = 0;
    const param_desc_t *d = track_desc(&trk[0], P_FXRATE);
    trk[0].p[P_FXTYPE] = FX_SYNC;
    d = track_desc(&trk[0], P_FXRATE);
    param_format(d, rate_of(div), v, &u);
    assert(str_eq(d->label, "TIME") && str_eq(v, name));
    uint32_t want = (uint32_t)FS * 60u / (uint32_t)bpm * FX_SYNC_DIV[div] / 96u;   /* quarter-rate samples */
    while (want > MELODEE_ECHO_RING - 2u) want >>= 1;
    uint32_t at = echo_at(bpm, div), w = want * 4u;
    printf("echo_sync: %3d BPM %-4s -> %6u samples (%4u ms), the note value's %6u%s\n", bpm, name, at,
           at * 1000u / (uint32_t)FS, (uint32_t)FS * 60u / (uint32_t)bpm * FX_SYNC_DIV[div] / 24u,
           w < (uint32_t)FS * 60u / (uint32_t)bpm * FX_SYNC_DIV[div] / 24u - 8u ? " (folded to fit the ring)" : "");
    assert(at + 8u >= w && at <= w + 8u);
}

int main(void)
{
    host_tracks_init();
    assert(str_eq(N_FX[FX_SYNC], "SYNC ECHO") && NELEM(N_FX) == FX_COUNT);
    expect(120, 5, "1/8");                              /* 250 ms: fits either ring */
    expect(120, 2, "1/16");
    expect(120, 1, "16T");
    expect(120, 6, "4T");
    expect(120, 7, "8D");
    expect(90, 5, "1/8");                               /* the tempo moves it */
    expect(120, 8, "1/4");                              /* 500 ms: the FM-1 folds it to a 1/8 */
    expect(120, 13, "1/1");                             /* 2 s: the plugin's ring holds it */
    expect(40, 13, "1/1");                              /* 6 s: folds in either */
    {                                                   /* every FX slot a SYNC ECHO at once: the arena holds them all */
        fx_clear_all(); memset(fx_state, 0, sizeof fx_state);
        for (uint32_t k = 0; k < RES_COUNT; k++) resource_release(k);
        int16_t p[FX_PARAMS] = {FX_SYNC, 127, 60, 64};
        fx_plan_t plan;
        for (uint32_t si = 0; si < FX_STATE_COUNT; si++) {
            fx_slot_prepare(&plan, si, p);
            assert(plan.active && !fx_state[si].failed && resource[RES_FX0 + si].size == MELODEE_ECHO_RING * 2u);
        }
        printf("echo_sync: %u FX slots, all SYNC ECHO: %u KB, allocated\n", (unsigned)FX_STATE_COUNT,
               (unsigned)(FX_STATE_COUNT * MELODEE_ECHO_RING * 2u / 1024u));
    }
    printf("echo_sync: ring %u (%u ms): OK\n", (unsigned)MELODEE_ECHO_RING,
           (unsigned)(MELODEE_ECHO_RING * 4000u / (uint32_t)FS));
    return 0;
}
