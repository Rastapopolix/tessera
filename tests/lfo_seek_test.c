/* SPDX-License-Identifier: GPL-3.0-only */
/* LFO 2 SYNC + FREE after a Song Position (voice.c lfo_seek, seq.c seq_seek): the LFO's phase where playing from the
 * top would have put it, so it stays on the DAW's bar when the DAW starts mid-song, loops or locates (a MIDI master's
 * Song Position too). Checked against the per-block sum (lfo_sync_inc) for every SYNC value, through seq_start +
 * seq_seek, and that NOTE and SYNC OFF are left alone. Built twice by run_tests.sh: 4 tracks and the plugin's 8 */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>

/* the phase after playing UNITS from the top, in blocks of BLOCK units, as track_lfo_tick advances it */
static uint32_t lfo_played(track_t *t, uint32_t units, uint32_t block)
{
    uint32_t ph = (uint32_t)t->p[P_LPHASE] << 25;
    lfo_sync_rem[(uint32_t)(t - trk) % NTRK] = 0;
    while (units) {
        uint32_t n = units < block ? units : block;
        lfo_block_units = n;
        ph += lfo_sync_inc(t);
        units -= n;
    }
    return ph;
}
static uint32_t dist(uint32_t a, uint32_t b) { uint32_t d = a - b; return d < 0x80000000u ? d : 0u - d; }

int main(void)
{
    host_tracks_init();
    int bad = 0;
    track_t *t = &trk[NTRK - 1u];
    static const uint32_t AT[] = {1u, 777u, MIDI_BEAT_UNITS, 3u * MIDI_BEAT_UNITS + 4321u, 17u * MIDI_BEAT_UNITS,
                                  4u * 4u * MIDI_BEAT_UNITS - 1u, 65u * 4u * MIDI_BEAT_UNITS + 12345u, 6000000u};
    uint32_t sync, k, worst = 0;
    t->p[P_LTRIG] = 1;                                   /* FREE */
    for (sync = 1; sync <= (uint32_t)TP[P_LSYNC].max; sync++)
        for (k = 0; k < NELEM(AT); k++) {
            t->p[P_LSYNC] = (int16_t)sync;
            t->p[P_LPHASE] = (int16_t)(k * 37u % 128u);
            uint32_t want = lfo_played(t, AT[k], 700u);
            t->lfo_ph = 0xDEADBEEFu;
            lfo_seek(t, AT[k]);
            uint32_t d = dist(t->lfo_ph, want);
            if (d > worst) worst = d;
            if (d > 64u) {                               /* (64 / 2^32 of a cycle: nothing an ear could tell) */
                printf("FAIL: SYNC %u at %u units: %08x, playing from the top %08x\n", sync, AT[k], t->lfo_ph, want);
                bad++;
            }
        }
    printf("lfo_seek: %u SYNC values x %u places, the worst %u / 2^32 of a cycle from playing from the top\n",
           (uint32_t)TP[P_LSYNC].max, (uint32_t)NELEM(AT), worst);

    /* through the transport: PLAY then a Song Position, as host_locate and midi_clock_seek do */
    t->p[P_LSYNC] = 3; t->p[P_LPHASE] = 32;              /* 1/1: a bar a cycle, a quarter in */
    seq_start();
    seq_seek(2u * 4u * MIDI_BEAT_UNITS + 2u * MIDI_BEAT_UNITS);   /* bar 3, beat 3: half a bar on */
    if (dist(t->lfo_ph, 0xC0000000u) > 64u) { printf("FAIL: bar 3 beat 3, 1/1 from a quarter: %08x\n", t->lfo_ph); bad++; }
    seq_start();
    if (t->lfo_ph != 0x40000000u) { printf("FAIL: PLAY from the top: %08x\n", t->lfo_ph); bad++; }

    t->p[P_LTRIG] = 0;                                   /* NOTE: each phrase starts it again, the seek leaves it */
    t->lfo_ph = 0x12345678u;
    lfo_seek(t, 5u * MIDI_BEAT_UNITS);
    if (t->lfo_ph != 0x12345678u) { printf("FAIL: TRIG NOTE moved\n"); bad++; }
    t->p[P_LTRIG] = 1; t->p[P_LSYNC] = 0;                /* SYNC OFF: RATE's, free */
    lfo_seek(t, 5u * MIDI_BEAT_UNITS);
    if (t->lfo_ph != 0x12345678u) { printf("FAIL: SYNC OFF moved\n"); bad++; }

    printf(bad ? "lfo_seek: %d FAILED\n" : "lfo_seek: ok\n", bad);
    return bad != 0;
}
