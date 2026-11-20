/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Andamento: Tessera's portable core on a Daisy Seed (STM32H750, libDaisy), first stage.
 *
 * The shared build (platform/core/core_unit.c: the same two include lists as the host tests, the web simulator and
 * the plugin, the host HAL) with Tessera's core API (platform/core/tess_core.h) over it. main.cpp hands
 * tess_core_block one block of CTL samples at a time from libDaisy's audio callback; audio time drives Tessera's
 * clock, as in the simulator. The FM-1's UI is built but never runs (TESS_CORE_UI 0) and keeps no copy of its screen:
 * Andamento draws its own (platform/andamento: a framebuffer, the SSD1322 driver), a bring-up card until its UI comes.
 * Storage and USB: none yet (projects live in RAM: tess_core_save / tess_core_load move .tsp records).
 *
 * Built at FS = 48000 (platform/daisy/Makefile: MELODEE_FS for tools/gen_tables.py), the Daisy's codec rate.
 * The FM-1's placements are kept (CORE_KEEP_ATTRIBUTES: section, noinline, always_inline): tessera_daisy.lds puts its
 * .audio_text and .dsp_text in ITCM and its .dsp_tables in DTCM, with the rest of the audio path, and .pool and
 * .noinit in .bss; the host's big buffers (the sample slots' stand-in) go to the SDRAM. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
/* The main loop's guard against the audio callback (an interrupt): PRIMASK off around a change the callback must
 * not see half made, nesting, back to what it was at the outermost ON. The main loop's calls into the core (a project
 * load, the UI to come) need it. */
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
#define TESS_IRQ_OFF() daisy_irq_off()
#define TESS_IRQ_ON() daisy_irq_on()
#define TESS_ENTROPY() (fm1_ms * 2654435761u)
#define CORE_KEEP_ATTRIBUTES 1
#define CORE_BIG __attribute__((section(".sdram_bss")))
#define HOST_HAL_SCREEN 0                         /* (the FM-1's screen: drawn nowhere) */
#define TESS_CORE_UI 0                            /* (Andamento's own UI comes: platform/andamento/and_ui.c) */
#include "../core/core_unit.c"
#include "../../firmware/src/mo_font.h"
#include "../andamento/and_fb.c"                  /* the screen: a framebuffer of 16 levels */
#include "../andamento/ssd1322.c"                 /* .. as the SSD1322's bytes (and_spi_cmd / and_spi_data: main.cpp) */

#include "tessera_core.h"

_Static_assert(FS == 48000, "platform/daisy: build the tables at MELODEE_FS=48000");

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
