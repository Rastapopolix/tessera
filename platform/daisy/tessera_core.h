/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Andamento: what main.cpp (C++, libDaisy) calls in tessera_core.c besides Tessera's core API
 * (platform/core/tess_core.h: tess_core_boot, tess_core_block, ...): the screen. */
#pragma once
#include <stdint.h>
#include "tess_core.h"
#ifdef __cplusplus
extern "C" {
#endif

#define TESSERA_SCREEN_BYTES 8192       /* a frame of the 128 x 64 screen as the SSD1322 takes it */
void tessera_core_screen_init(void);    /* the SSD1322 set up, display off: after its reset pulse */
void tessera_core_screen(uint8_t *buf); /* the screen drawn, packed into buf and sent; the display on after the first */

/* main.cpp provides these to the screen driver (platform/andamento/ssd1322.c): its SPI, DC low / DC high */
void and_spi_cmd(uint8_t c);
void and_spi_data(const uint8_t *p, uint32_t n);

#ifdef __cplusplus
}
#endif
