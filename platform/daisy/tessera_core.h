/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Andamento: what main.cpp (C++, libDaisy) calls in tessera_core.c (C, Tessera's core) */
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define TESSERA_CORE_BLOCK 32           /* frames a call to tessera_core_block renders: Tessera's CTL */

void tessera_core_init(void);           /* tables, track 1 on FM6; before the audio starts */
void tessera_core_block(int32_t *out);  /* TESSERA_CORE_BLOCK stereo frames, interleaved L R, Q15 */
void tessera_core_tick_ms(void);        /* once a millisecond: Tessera's clock (fm1_ms) */

#ifdef __cplusplus
}
#endif
