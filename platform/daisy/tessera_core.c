/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Andamento: Tessera's portable core on a Daisy Seed (STM32H750, libDaisy), first stage.
 *
 * A unity build, as firmware/src/melodee.c and tests/hostsim.c are: the engines, voices, mods, the mixer, the
 * sequencer and the generators, with the FM-1's hardware names (fm1_*) given plain stand-ins here, as the host
 * tests give them. No UI, storage or USB yet: main.cpp hands this file one block of CTL samples at a time from
 * libDaisy's audio callback, and a short figure is played on track 1 so the board makes sound on day one. The
 * screen (platform/andamento: a framebuffer, the SSD1322 driver) shows a bring-up card until the UI comes.
 *
 * Built at FS = 48000 (platform/daisy/Makefile: MELODEE_FS for tools/gen_tables.py), the Daisy's codec rate.
 * The FM-1's placements are kept (section, noinline, always_inline): tessera_daisy.lds puts its .audio_text and
 * .dsp_text in ITCM and its .dsp_tables in DTCM, with the rest of the audio path, and .pool and .noinit in .bss.
 * The include list follows tests/hostsim.c; keep the two in step when a source is added there. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
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
/* The main loop's guard against the audio callback (an interrupt): PRIMASK off around a change the callback must
 * not see half made, nesting, back to what it was at the outermost ON. Only tessera_core_tick_ms runs in the main
 * loop yet; the UI will need this. */
static uint32_t daisy_irq_depth, daisy_irq_primask;
static inline __attribute__((always_inline)) void daisy_irq_off(void)
{
    uint32_t pm;
    __asm__ volatile("mrs %0, primask\n\tcpsid i" : "=r"(pm) :: "memory");
    if (!daisy_irq_depth++)
        daisy_irq_primask = pm;
}
static inline __attribute__((always_inline)) void daisy_irq_on(void)
{
    if (daisy_irq_depth && !--daisy_irq_depth && !daisy_irq_primask)
        __asm__ volatile("cpsie i" ::: "memory");
}
#define TESS_IRQ_OFF() daisy_irq_off()
#define TESS_IRQ_ON() daisy_irq_on()
#define TESS_ENTROPY() (fm1_ms * 2654435761u)
#include "../../firmware/src/tessera.c"
#include "../../firmware/src/pat_gen.c"
#include "../../firmware/src/pid.c"
#include "../../firmware/src/cord.c"
#include "../../firmware/src/mscope.c"
#include "../../firmware/src/mo_font.h"
#include "../andamento/and_fb.c"                  /* the screen: a framebuffer of 16 levels */
#include "../andamento/ssd1322.c"                 /* .. as the SSD1322's bytes (and_spi_cmd / and_spi_data: main.cpp) */

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

_Static_assert(TESSERA_SCREEN_BYTES == SSD_FRAME, "tessera_core.h: a frame of the SSD1322");

void tessera_core_screen_init(void)
{
    ssd1322_init();
}

void tessera_core_screen(uint8_t *buf)
{
    static uint8_t on;
    and_testcard();                              /* (no UI yet: platform/andamento/and_ui.c comes next) */
    ssd1322_pack(and_fb, buf);
    ssd1322_frame(buf);
    if (!on)
        ssd1322_on(on = 1);
}
