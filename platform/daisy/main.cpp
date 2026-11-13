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
