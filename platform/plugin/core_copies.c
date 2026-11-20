/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The copies of the core built into the plugin (core_copies.h): TESS_PLUGIN_COPIES of them (CMakeLists.txt), each
 * core_copy.c compiled with the prefix tc<k>_ */
#include "core_copies.h"

#ifndef TESS_PLUGIN_COPIES
#define TESS_PLUGIN_COPIES 8
#endif
#if TESS_PLUGIN_COPIES < 1 || TESS_PLUGIN_COPIES > 16
#error "TESS_PLUGIN_COPIES: 1 to 16"
#endif

#define TESS_COPY_EXTERN(k) extern const tess_core_vt tc##k##_tess_core_vt;
#define TESS_COPY_REF(k) &tc##k##_tess_core_vt,
#define TESS_COPIES_EACH(X)                                                                                           \
    X(0) TESS_COPIES_1(X)
#if TESS_PLUGIN_COPIES > 1
#define TESS_COPIES_1(X) X(1) TESS_COPIES_2(X)
#else
#define TESS_COPIES_1(X)
#endif
#if TESS_PLUGIN_COPIES > 2
#define TESS_COPIES_2(X) X(2) TESS_COPIES_3(X)
#else
#define TESS_COPIES_2(X)
#endif
#if TESS_PLUGIN_COPIES > 3
#define TESS_COPIES_3(X) X(3) TESS_COPIES_4(X)
#else
#define TESS_COPIES_3(X)
#endif
#if TESS_PLUGIN_COPIES > 4
#define TESS_COPIES_4(X) X(4) TESS_COPIES_5(X)
#else
#define TESS_COPIES_4(X)
#endif
#if TESS_PLUGIN_COPIES > 5
#define TESS_COPIES_5(X) X(5) TESS_COPIES_6(X)
#else
#define TESS_COPIES_5(X)
#endif
#if TESS_PLUGIN_COPIES > 6
#define TESS_COPIES_6(X) X(6) TESS_COPIES_7(X)
#else
#define TESS_COPIES_6(X)
#endif
#if TESS_PLUGIN_COPIES > 7
#define TESS_COPIES_7(X) X(7) TESS_COPIES_8(X)
#else
#define TESS_COPIES_7(X)
#endif
#if TESS_PLUGIN_COPIES > 8
#define TESS_COPIES_8(X) X(8) TESS_COPIES_9(X)
#else
#define TESS_COPIES_8(X)
#endif
#if TESS_PLUGIN_COPIES > 9
#define TESS_COPIES_9(X) X(9) TESS_COPIES_10(X)
#else
#define TESS_COPIES_9(X)
#endif
#if TESS_PLUGIN_COPIES > 10
#define TESS_COPIES_10(X) X(10) TESS_COPIES_11(X)
#else
#define TESS_COPIES_10(X)
#endif
#if TESS_PLUGIN_COPIES > 11
#define TESS_COPIES_11(X) X(11) TESS_COPIES_12(X)
#else
#define TESS_COPIES_11(X)
#endif
#if TESS_PLUGIN_COPIES > 12
#define TESS_COPIES_12(X) X(12) TESS_COPIES_13(X)
#else
#define TESS_COPIES_12(X)
#endif
#if TESS_PLUGIN_COPIES > 13
#define TESS_COPIES_13(X) X(13) TESS_COPIES_14(X)
#else
#define TESS_COPIES_13(X)
#endif
#if TESS_PLUGIN_COPIES > 14
#define TESS_COPIES_14(X) X(14) TESS_COPIES_15(X)
#else
#define TESS_COPIES_14(X)
#endif
#if TESS_PLUGIN_COPIES > 15
#define TESS_COPIES_15(X) X(15)
#else
#define TESS_COPIES_15(X)
#endif

TESS_COPIES_EACH(TESS_COPY_EXTERN)
static const tess_core_vt *const copies[] = {TESS_COPIES_EACH(TESS_COPY_REF)};

uint32_t tess_copy_count(void) { return (uint32_t)(sizeof copies / sizeof copies[0]); }
const tess_core_vt *tess_copy(uint32_t k) { return k < tess_copy_count() ? copies[k] : 0; }
