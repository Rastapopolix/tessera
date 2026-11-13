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

#define TESSERA_SCREEN_BYTES 8192       /* a frame of the 128 x 64 screen as the SSD1322 takes it */
void tessera_core_screen_init(void);    /* the SSD1322 set up, display off: after its reset pulse */
void tessera_core_screen(uint8_t *buf); /* the screen drawn, packed into buf and sent; the display on after the first */

/* main.cpp provides these to the screen driver (platform/andamento/ssd1322.c): its SPI, DC low / DC high */
void and_spi_cmd(uint8_t c);
void and_spi_data(const uint8_t *p, uint32_t n);

#ifdef __cplusplus
}
#endif
