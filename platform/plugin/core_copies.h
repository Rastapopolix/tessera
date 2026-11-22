/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Several Tesseras in one DAW project: the firmware keeps its state in file-level globals, so the plugin is built with
 * several private copies of the core (core_copy.c, each compiled with its own names) and each instance takes one.
 * A copy is the whole core API (tess_core.h) as a table of its functions: copy->render(...) for tess_core_render(...).
 * Not thread-safe within a copy (tess_core.h); two copies are wholly apart and may run on two threads at once. */
#pragma once
#include "tess_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* the API, by name: X(n) for each tess_core_n */
#define TESS_CORE_API(X)                                                                                              \
    X(rate) X(boot) X(render) X(block) X(frames) X(keys) X(buttons) X(enc) X(master) X(master_get) X(midi)           \
    X(transport) X(playing) X(midi_out) X(screen) X(view) X(save) X(load) X(tsp_max) X(slot_get) X(slot_put)         \
    X(slot_load) X(param_count) X(param_at) X(param_info) X(param_get) X(param_set) X(param_text) X(host) X(macro)   \
    X(macro_set) X(position) X(bpm) X(ink) X(ink_at) X(tracks) X(track) X(track_select)  \
    X(setup_save) X(setup_load) X(notes_off) X(zone_do) X(gen_styles) X(gen_style) X(generate) X(gen_swap)           \
    X(gen_last) X(note_expr) X(note_tune)

#define TESS_CORE_VT_MEMBER(n) __typeof__(tess_core_##n) *n;
typedef struct tess_core_vt {
    TESS_CORE_API(TESS_CORE_VT_MEMBER)
} tess_core_vt;

/* the copies built in (CMake's TESS_PLUGIN_COPIES): how many, and copy k (0 .. count - 1) */
uint32_t tess_copy_count(void);
const tess_core_vt *tess_copy(uint32_t k);

#ifdef __cplusplus
}
#endif
