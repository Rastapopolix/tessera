// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Theo Dainis
// Andamento: Tessera on a Daisy Seed, first stage. libDaisy starts the board and the codec at 48 kHz in blocks
// of 32 frames, Tessera's control block, and each audio callback asks tessera_core.c for one block. The main
// loop keeps Tessera's millisecond clock and blinks the Seed's LED, so a board that runs but stays silent says so.
#include "daisy_seed.h"
#include "tessera_core.h"

using namespace daisy;

static DaisySeed hw;
static CpuLoadMeter load;               // the audio callback's share of the time: the first number to measure

// The screen: the yellow NHD-2.7-12864WDY3-M (SSD1322) in 4-wire SPI. tessera_core.c draws and packs a frame; these
// two calls are all it needs from the board. Its packed frame lives in memory the SPI's DMA can read (not cached).
// Left for the board (DAISY_PORT.md task 5): SPI1 with SCK on D8 (pin 9), MOSI on D10 (pin 11), CS on D7 (pin 8),
// at most 10 MHz, mode 0; DC and RESET on two free GPIO, chosen when it is wired. RESET low for at least 2 us, then
// a wait before tessera_core_screen_init. Until then the calls do nothing, and the build checks the rest compiles.
static uint8_t DMA_BUFFER_MEM_SECTION screen_buf[TESSERA_SCREEN_BYTES];

extern "C" void and_spi_cmd(uint8_t c)
{
    (void)c;                             // the board: DC low, the byte out on SPI1
}

extern "C" void and_spi_data(const uint8_t *p, uint32_t n)
{
    (void)p;                             // the board: DC high, the bytes out on SPI1 (a frame by DMA)
    (void)n;
}

static void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    (void)in;
    load.OnBlockStart();
    int32_t q15[2 * TESSERA_CORE_BLOCK];
    for (size_t i = 0; i < size; i += TESSERA_CORE_BLOCK) {
        tessera_core_block(q15);
        for (size_t k = 0; k < TESSERA_CORE_BLOCK && i + k < size; k++) {
            out[0][i + k] = (float)q15[2 * k] * (1.0f / 32768.0f);       // Q15 -> -1..1
            out[1][i + k] = (float)q15[2 * k + 1] * (1.0f / 32768.0f);
        }
    }
    load.OnBlockEnd();
}

int main(void)
{
    hw.Init(true);                       // true: the CPU at its full 480 MHz
    hw.SetAudioBlockSize(TESSERA_CORE_BLOCK);
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    load.Init(hw.AudioSampleRate(), hw.AudioBlockSize());
    tessera_core_init();
    tessera_core_screen_init();
    tessera_core_screen(screen_buf);     // the bring-up card (and_testcard)
    hw.StartAudio(AudioCallback);

    uint32_t last = System::GetNow();
    for (;;) {
        uint32_t now = System::GetNow();
        while (last != now) {            // Tessera's clock, one tick a millisecond
            tessera_core_tick_ms();
            last++;
        }
        hw.SetLed((now / 500u) & 1u);    // alive: a slow blink
    }
}
