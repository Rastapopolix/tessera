/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Andamento: Tessera's portable core on a Daisy Seed (STM32H750, libDaisy), first stage.
 *
 * A unity build, as firmware/src/melodee.c and tests/hostsim.c are: the engines, voices, mods, the mixer, the
 * sequencer and the generators, with the FM-1's hardware names (fm1_*) given plain stand-ins here, as the host
 * tests give them. No UI, screen, storage or USB yet: main.cpp hands this file one block of CTL samples at a time
 * from libDaisy's audio callback, and a short figure is played on track 1 so the board makes sound on day one.
 *
 * Built at FS = 48000 (platform/daisy/Makefile: MELODEE_FS for tools/gen_tables.py), the Daisy's codec rate.
 * The include list follows tests/hostsim.c; keep the two in step when a source is added there. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define __attribute__(x)                         /* (the FM-1's section placements: libDaisy's linker script has none) */
#define memset melodee_memset
#define memcpy melodee_memcpy
#define memcmp melodee_memcmp
#include "melodee_tables.h"
#include "../../firmware/src/libc.c"
#undef memset
#undef memcpy
#undef memcmp
static struct { volatile uint32_t notes, buttons; } fm1_in;   /* the keys: bit k = note key k (7 = C4) */
#include "../../firmware/src/core.h"
#include "../../firmware/src/engines.c"
#include "../../firmware/src/params.c"
#include "../../firmware/src/mod.c"
#include "../../firmware/src/voice.c"
#include "../../firmware/src/mix.c"
static void fm1_delay_ms(uint32_t ms) { (void)ms; }
#if MELODEE_USB_HOST
static void fm1_wdt_feed(void) {}
#endif
#include "../../firmware/src/usb.c"
#include "../../firmware/src/midi_uart.c"
#include "../../firmware/src/tessera.h"
#include "../../firmware/src/gen.c"
#include "../../firmware/src/song_chain.c"
#include "../../firmware/src/seq.c"
#define TESS_IRQ_OFF() ((void)0)                  /* (the audio callback and the main loop share nothing yet) */
#define TESS_IRQ_ON() ((void)0)
#define TESS_ENTROPY() (fm1_ms * 2654435761u)
#include "../../firmware/src/tessera.c"
#include "../../firmware/src/pat_gen.c"
#include "../../firmware/src/pid.c"
#include "../../firmware/src/cord.c"
#include "../../firmware/src/mscope.c"
#undef __attribute__

#include "tessera_core.h"

_Static_assert(FS == 48000, "platform/daisy: build the tables at MELODEE_FS=48000");

static uint32_t daisy_frame;                     /* samples played since tessera_core_init */

void tessera_core_init(void)
{
    uint32_t i;
    track_t *t = &trk[0];
    dsp_tables_init();                           /* the RAM tables (fm1_main on the FM-1) */
    for (i = 0; i < G_COUNT; i++)
        song.g[i] = GP[i].def;
    for (i = 0; i < P_E0; i++)
        t->p[i] = TP[i].def;
    t->eng_req = t->engine = ENGI_FM6;           /* FM6, its first preset, as tests/hostsim.c */
    for (i = 0; i < 8; i++)
        t->p[P_E0 + i] = ENGINES[ENGI_FM6]->edit[i].def;
    if (ENGINES[ENGI_FM6]->npresets) {
        const preset_t *p = eng_preset(ENGINES[ENGI_FM6], 0);
        for (i = 0; i < 8; i++)
            t->p[P_E0 + i] = p->e[i];
        t->p[P_ATK] = p->env[0];
        t->p[P_DEC] = p->env[1];
        t->p[P_SUS] = p->env[2];
        t->p[P_REL] = p->env[3];
        t->p[P_ED_FLT] = p->fenv;
    }
    fm6_track_loaded(t);
    t->p[P_VOICE] = 1;
    t->p[P_RESERVED34] = 24;                     /* the sends, as melodee_init */
    t->p[P_RESERVED35] = 28;
    t->p[P_RESERVED36] = 36;
    song.master_q12 = 4096;
    daisy_frame = 0;
}

void tessera_core_block(int32_t *out)
{
    uint32_t f = daisy_frame % (2u * FS), n = 0;  /* tests/hostsim.c's 2 s figure: C4, E4, G4 */
    if (f > FS / 10 && f < FS / 2)
        n |= 1u << 7;
    if (f > FS * 45 / 100 && f < FS * 8 / 10)
        n |= 1u << 11;
    if (f > FS * 9 / 10 && f < FS * 12 / 10)
        n |= 1u << 14;
    fm1_in.notes = n;
    mix_block(out, CTL);
    daisy_frame += CTL;
}

void tessera_core_tick_ms(void)
{
    fm1_ms++;
}
