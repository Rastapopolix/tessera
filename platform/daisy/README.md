# platform/daisy: Tessera's core on a Daisy Seed

The first stage of Andamento, Tessera on hardware of its own around an Electrosmith Daisy Seed (STM32H750,
480 MHz Cortex-M7). This builds Tessera's engines, voices, mods, mixer, sequencer and generators for the Seed with
libDaisy, and plays a short C4-E4-G4 figure on FM6 through the Seed's codec at 48 kHz. The screen's driver
(`platform/andamento/`) draws a bring-up card, but its SPI is not wired yet. There is no UI, storage or MIDI yet:
those come next, behind the same stand-ins.

| File | What |
| --- | --- |
| `tessera_core.c` | the unity build of the portable core (the include list of `tests/hostsim.c`), with plain stand-ins for the FM-1's `fm1_*` names |
| `tessera_core.h` | what `main.cpp` calls: init, one 32-frame block, the millisecond tick |
| `main.cpp` | libDaisy: the board, the codec at 48 kHz in blocks of 32 frames, the audio callback, the LED, the screen's frame buffer and its two SPI calls (empty until the board is here) |
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
host-sized resource arena (`resource_host`). The screen added 1.4 KiB of code, its 8 KiB framebuffer, and an 8 KiB
packed frame in the DMA RAM (`RAM_D2_DMA`, 24 of 32 KiB used, the rest libDaisy's).
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
DTCM), a demo project instead of the fixed figure, the screen's SPI (below), encoders and keys, MIDI, storage on SD.

## The screen

Andamento's screen is the yellow NHD-2.7-12864WDY3-M: 128 x 64 pixels, 16 levels, an SSD1322 controller in 4-wire
SPI. `platform/andamento/and_fb.c` is the screen as memory (a byte a pixel, 0..15) with the drawing calls;
`platform/andamento/ssd1322.c` sets the controller up and turns a frame into its 8,192 bytes. Both are plain C, so
`tests/oled_render.c` checks them on the PC through a model of the controller and makes PNGs in `build/oled/`.

Left for the board: `and_spi_cmd` and `and_spi_data` in `main.cpp`. SPI1 with SCK on D8 (pin 9), MOSI on D10
(pin 11), CS on D7 (pin 8), mode 0, at most 10 MHz; DC (low: a command, high: its arguments and the pixels) and
RESET on two free GPIO; a frame by DMA from `screen_buf`. The first picture should be the bring-up card: a border
all round, TL TR BL BR in their corners and 16 steps of brightness. A missing edge means the column offset
(`SSD_COL0`) is wrong, mirrored text the remap (`0xA0`).
