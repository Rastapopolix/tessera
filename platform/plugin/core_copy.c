/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* One private copy of the core (core_copies.h): the core as the web simulator builds it (web/sim/sim.c, over
 * platform/core/core_unit.c), with every global it defines renamed TESS_PFX + name (build/gen/core_syms.h, made by
 * gen.py), so several copies link into one plugin, each with its own state. CMakeLists.txt compiles this file once
 * per copy, with -DTESS_PFX=tc0_, tc1_, .. Its one name outside: TESS_PFX + tess_core_vt, the table of its API. */
#ifndef TESS_PFX
#error "core_copy.c: compile with -DTESS_PFX=<prefix> (CMakeLists.txt)"
#endif
#define TESS_CAT2(a, b) a##b
#define TESS_CAT(a, b) TESS_CAT2(a, b)
#define TESS_NS(x) TESS_CAT(TESS_PFX, x)
#include "core_syms.h"
#include "../../web/sim/sim.c"
#include "core_copies.h"

#define TESS_CORE_VT_INIT(n) tess_core_##n,
__attribute__((visibility("hidden"))) const tess_core_vt TESS_NS(tess_core_vt) = {TESS_CORE_API(TESS_CORE_VT_INIT)};
