/* SPDX-License-Identifier: GPL-3.0-only */
/* MELODEE_VOICE_BUDGET=0 (the plugin): no shared voice budget. Every track plays up to its engine's own voice count,
 * whatever the others play: nothing is stolen for another track, and a track's own cap still holds */
#define MELODEE_VOICE_BUDGET 0
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>

int main(void)
{
    memset(trk, 0, sizeof trk); memset(&song, 0, sizeof song); memset(resource, 0, sizeof resource);
    host_tracks_init();
    voice_kills = 0;
    for (unsigned p = 0; p < NPART; p++) {
        host_preset(&trk[p], HOST_SYNTH, HOST_PLAIN);
        trk[p].p[P_VOICE] = V_POLY;
    }
    uint32_t cap = trk_nvoice(&trk[0]), units = voice_units(&trk[0]);
    assert(cap > 1u && voice_budget() >= NPART * NVOICE * units);
    for (unsigned p = 0; p < NPART; p++)                 /* every track full: past the FM-1's 16 points */
        for (unsigned n = 0; n < cap; n++)
            trk_note_on(&trk[p], 40u + 3u * n, 100);
    assert(!voice_kills && voices_busy() == NPART * cap * units && voices_busy() > VBUDGET);
    for (unsigned p = 0; p < NPART; p++) {                /* each track still at its own cap: one more replaces one */
        trk_note_on(&trk[p], 100u, 100);
        uint32_t n = 0;
        for (unsigned i = 0; i < NVOICE; i++) n += trk[p].v[i].active && trk[p].v[i].stage != 4u;
        assert(n <= cap);
    }
    printf("voice_free: %u tracks x %u voices, %u units sounding (the FM-1's budget %u), none stolen across tracks\n",
           (unsigned)NPART, (unsigned)cap, (unsigned)voices_busy(), (unsigned)VBUDGET);
    return 0;
}
