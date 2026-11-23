/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments; Theo Dainis (melodee-tess) */
/* Tessera's core, the shared include list (1 of 2): the tables, the engines, voices, mods, the mixer, the sequencer,
 * the generators, the parameter list and the cords. Every build that is not the FM-1's own unity build includes it:
 * the host tests (tests/hostsim.c), and through platform/core/core_unit.c the web simulator, the plugin and the Daisy
 * build. The FM-1 (melodee.c) keeps its own list, with its hardware between the same files: a source added here
 * goes there too. The second list, the FM-1's UI and the projects: core_ui_sources.h.
 *
 * Before it: <stdint.h>, <stdlib.h>, <string.h>, <math.h>. Hooks a build may define first:
 *   TESS_IRQ_OFF() / TESS_IRQ_ON()   the guard against the audio interrupt (none: no interrupt, the host)
 *   TESS_ENTROPY()                   a fresh Turing register's seed (none: fixed, repeatable tests)
 *   TESSERA_NOTE_HOOK                every note a generator plays, on its way (tests/tessera_test.c)
 * The FM-1's hardware names it needs (fm1_in, fm1_delay_ms) get plain stand-ins here. */
#define memset melodee_memset
#define memcpy melodee_memcpy
#define memcmp melodee_memcmp
#include "melodee_tables.h"
#include "libc.c"
#undef memset
#undef memcpy
#undef memcmp
static struct { volatile uint32_t notes, buttons; } fm1_in;   /* the keys (bit k = note key k, 7 = C4) and buttons */
#include "core.h"
#include "engines.c"
#include "params.c"
#include "mod.c"
#include "voice.c"
#include "mix.c"
static void fm1_delay_ms(uint32_t ms) { (void)ms; }
#if MELODEE_USB_HOST
static void fm1_wdt_feed(void);                       /* (usb_host.c's waits; the build defines it) */
#endif
#include "usb.c"
#include "midi_uart.c"                                /* TRS MIDI IN: its parser (um_byte) feeds midi_in_q */
#include "tessera.h"
#include "gen.c"
#include "song_chain.c"
#ifdef TESSERA_NOTE_HOOK                              /* (SUB's repeats, seq_sub.c: a generator's notes too) */
static void TESSERA_NOTE_HOOK(track_t *t, uint32_t note, uint32_t vel);
#define SUB_NOTE_ON(t, n, v) (TESSERA_NOTE_HOOK((t), (n), (v)), trk_note_on((t), (n), (v)))
#endif
#include "seq.c"
#ifdef TESSERA_NOTE_HOOK                              /* tests/tessera_test.c: every note a generator plays, logged */
#define trk_note_on(t, n, v) (TESSERA_NOTE_HOOK((t), (n), (v)), trk_note_on((t), (n), (v)))
#endif
#ifndef TESS_IRQ_OFF
#define TESS_IRQ_OFF() ((void)0)                      /* (no audio interrupt on the host) */
#define TESS_IRQ_ON() ((void)0)
#endif
#ifndef TESS_ENTROPY
#define TESS_ENTROPY() 0x5EEDu                        /* (a fresh Turing register: fixed on the host, repeatable tests) */
#endif
#include "tessera.c"
#include "pat_gen.c"
#include "pid.c"
#include "cord.c"
#include "mscope.c"
#if MELODEE_SMF
#include "smf.c"                                     /* a .mid of the notes played (Andamento) */
#endif
#ifdef TESSERA_NOTE_HOOK
#undef trk_note_on
#endif
