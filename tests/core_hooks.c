/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The core's output hooks (CORE_OUT_HOOKS, core.h): a platform's own outputs besides USB (Andamento's CV / gate pairs
 * and DIN MIDI). Built as the Daisy builds the core (tests/tsp_cross.sh's daisy options) with CORE_OUT_HOOKS=1; plays
 * a demo and checks the hooks see its notes: every note started ends, none from a muted track, the MIDI OUT packets
 * come (NOTES OUT ALL) and are note on / off. Exit 0: passed.
 *   core_hooks DEMO.tsp */
#include "../platform/core/core_unit.c"

#if !CORE_OUT_HOOKS
#error "core_hooks.c: build with -DCORE_OUT_HOOKS=1"
#endif

static uint32_t ons, offs, muted_on, held[NTRK][128], pkts, bad_pkts, max_held;

static void note_hook(const track_t *t, uint32_t note, uint32_t vel)
{
    uint32_t tr = (uint32_t)(t - trk) % NTRK;
    if (vel) {
        ons++;
        muted_on += t->p[P_MUTE] != 0;
        held[tr][note & 127u]++;
    } else {
        offs++;
        if (held[tr][note & 127u]) held[tr][note & 127u]--;
    }
}

static void midi_hook(uint32_t pkt)
{
    uint32_t st = pkt >> 8 & 0xF0u;
    pkts++;
    bad_pkts += st != 0x90u && st != 0x80u && st != 0xB0u;   /* (notes; CC for the all-notes-off at a stop) */
}

static uint8_t buf[65536];

int main(int argc, char **argv)
{
    static float l[512], r[512];
    FILE *f = argc > 1 ? fopen(argv[1], "rb") : 0;
    uint32_t n, k, tr, still = 0;
    if (!f) {
        fprintf(stderr, "usage: core_hooks DEMO.tsp\n");
        return 3;
    }
    n = (uint32_t)fread(buf, 1, sizeof buf, f);
    fclose(f);
    tess_core_boot();
    if (tess_core_load(buf, n)) {
        fprintf(stderr, "core_hooks: %s refused\n", argv[1]);
        return 3;
    }
    settings_midi_out = MIDI_OUT_ALL;             /* (the generators' notes too, as the plugin and Andamento) */
    out_note_hook = note_hook;
    out_midi_hook = midi_hook;
    tess_core_transport(1);
    for (k = 0; k < 8u * tess_core_rate(); k += 512u)
        tess_core_render(l, r, 0, 512u);
    tess_core_transport(0);
    for (k = 0; k < 2u * tess_core_rate(); k += 512u)   /* (the stop taken, every note released) */
        tess_core_render(l, r, 0, 512u);
    for (tr = 0; tr < NTRK; tr++)
        for (k = 0; k < 128u; k++)
            still += held[tr][k];
    printf("core_hooks %s: %u notes on, %u off, %u still held, %u from a muted track; %u MIDI OUT packets (%u not notes)\n",
           argv[1], (unsigned)ons, (unsigned)offs, (unsigned)still, (unsigned)muted_on, (unsigned)pkts, (unsigned)bad_pkts);
    if (!ons || still || muted_on || pkts < ons || bad_pkts) {   /* (a sparse demo: a few notes in 8 s) */
        printf("core_hooks: FAILED\n");
        return 1;
    }
    printf("core_hooks: ok\n");
    return 0;
}
