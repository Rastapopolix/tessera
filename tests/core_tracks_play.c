/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* CORE_TRACKS 8 (core.h): a project (.tsp) holds tracks 1..4, and loading one must leave tracks 5..8 as they were,
 * playable: SOUND ON, no TRANSPOSE, full SOLO and ENTRIES gain. Built as tests/core_tracks.sh builds its 8-track
 * core (the plugin's options); loads a demo, stopped, plays a note on each of tracks 5..8 and checks each sounds on
 * its own stem and the other tracks stay quiet. (Bytes past a record's four tracks read as SOUND OFF once.)
 *   core_tracks_play DEMO.tsp      exit 0: passed */
#include "../platform/core/core_unit.c"

#include <stdio.h>

#if CORE_TRACKS != 8
#error "core_tracks_play.c: build with -DCORE_TRACKS=8"
#endif

static float l[256], r[256], s[NTRK][256];
static float *st[NTRK];

static void run(int blocks, double *e)
{
    for (int b = 0; b < blocks; b++) {
        tess_core_render(l, r, st, 256);
        if (e)
            for (int k = 0; k < NTRK; k++)
                for (int i = 0; i < 256; i++) e[k] += (double)s[k][i] * s[k][i];
    }
}

int main(int argc, char **argv)
{
    static uint8_t buf[TSP_MAX + 64];
    FILE *f = argc > 1 ? fopen(argv[1], "rb") : 0;
    size_t n = f ? fread(buf, 1, sizeof buf, f) : 0;
    int fails = 0;
    if (f) fclose(f);
    for (int k = 0; k < NTRK; k++) st[k] = s[k];
    tess_core_boot();
    if (!n || tess_core_load(buf, (uint32_t)n)) {
        printf("core_tracks_play: %s did not load\n", argc > 1 ? argv[1] : "(no file)");
        return 1;
    }
    run(400, 0);                                         /* (stopped: any tails die) */
    for (int k = 4; k < NTRK; k++) {
        double e[NTRK] = {0}, others = 0;
        if (tessera.sound_off[k] || tessera.midi_tr[k]) {
            printf("FAIL T%d: SOUND %s, TRANSPOSE %d after the load\n", k + 1, tessera.sound_off[k] ? "OFF" : "ON",
                   (int)tessera.midi_tr[k]);
            fails++;
        }
        input_on(&trk[k], 60, 100);
        run(20, e);
        input_off(&trk[k], 60);
        run(300, 0);
        for (int j = 0; j < NTRK; j++)
            if (j != k) others += e[j];
        if (!(e[k] > 1e-3 && e[k] > others * 4)) {
            printf("FAIL T%d: a note gave %.4f on its stem, %.4f on the others\n", k + 1, e[k], others);
            fails++;
        }
    }
    return fails ? 1 : 0;
}
