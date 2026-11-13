# platform/daisy: Tessera's core on a Daisy Seed

The first stage of Andamento, Tessera on hardware of its own around an Electrosmith Daisy Seed (STM32H750,
480 MHz Cortex-M7). This builds Tessera's engines, voices, mods, mixer, sequencer and generators for the Seed with
libDaisy, and plays a short C4-E4-G4 figure on FM6 through the Seed's codec at 48 kHz. There is no UI, screen,
storage or MIDI yet: those come next, behind the same stand-ins.

| File | What |
| --- | --- |
| `tessera_core.c` | the unity build of the portable core (the include list of `tests/hostsim.c`), with plain stand-ins for the FM-1's `fm1_*` names |
| `tessera_core.h` | what `main.cpp` calls: init, one 32-frame block, the millisecond tick |
| `main.cpp` | libDaisy: the board, the codec at 48 kHz in blocks of 32 frames, the audio callback, the LED |
| `Makefile` | libDaisy's build, plus Tessera's generated headers with the tables at `MELODEE_FS=48000` |

## Build

Needs `arm-none-eabi-gcc` (Ubuntu: `sudo apt install gcc-arm-none-eabi libnewlib-arm-none-eabi
libstdc++-arm-none-eabi-newlib`), Python 3, and a built libDaisy:

```
git clone --recursive https://github.com/electro-smith/libDaisy ~/src/libDaisy
make -C ~/src/libDaisy
make -C platform/daisy LIBDAISY_DIR=~/src/libDaisy
```

The result is `platform/daisy/build/tessera_daisy.bin`, built to run from the Seed's QSPI flash with the Daisy
bootloader (`APP_TYPE = BOOT_QSPI`). The link prints the memory use: on 2026-11-13 (libDaisy 1289a00,
arm-none-eabi-gcc 13.2), about 373 KiB of code in QSPI and 328 KiB of RAM in the AXI SRAM, most of it the
host-sized resource arena (`resource_host`).
`.github/workflows/daisy-build.yml` builds it on every pull request that touches the core.

## Flash (once the board is here)

1. Once: `make -C ~/src/libDaisy/... program-boot` puts the Daisy bootloader in the Seed's internal flash (see
   Electrosmith's docs for the current command).
2. Press RESET; while the LED pulses, `make -C platform/daisy LIBDAISY_DIR=... program-dfu`.
3. The LED blinks slowly once Tessera runs; the figure plays on the audio outputs.

Rescue: hold BOOT, tap RESET, release BOOT, and flash again.

## The sample rate

The FM-1 runs at 44.1 kHz and the Seed's codec at 48 kHz. `tools/gen_tables.py` reads `MELODEE_FS` and every
rate-dependent number in the firmware is derived from `FS`, so this build plays in tune (`tests/fs_test.py`
checks it on the host). Some filters still hold coefficients made at 44.1 kHz and sound slightly brighter at
48 kHz: the 808 kit's `DRF_*` filters (`drum_808.c`) and PHYS' damping and brightness.

## Next

The plan is in the private notes (tessera-notes `andamento/DAISY_PORT.md`): the audio path in fast RAM (ITCM,
DTCM), a demo project instead of the fixed figure, the SSD1322 screen, encoders and keys, MIDI, storage on SD.
