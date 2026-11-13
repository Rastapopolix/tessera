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
| `main.cpp` | libDaisy: the board, the codec at 48 kHz in blocks of 32 frames, the audio callback, the fast memories filled at boot, the LED, the screen's frame buffer and its two SPI calls (empty until the board is here) |
| `tessera_daisy.lds` | the memory map: libDaisy's QSPI one, with the audio path in ITCM and DTCM (below) |
| `itcm_pick.py` | at build time, chooses the code that goes to ITCM, as much of the audio path as fits |
| `memcheck.py` | after the link: fails on a section nothing would copy, and prints where the audio path landed |
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
bootloader (`APP_TYPE = BOOT_QSPI`). The link prints the memory use, then `memcheck.py` where the audio path went.
`.github/workflows/daisy-build.yml` builds it on every pull request that touches the core.

## Where things live

The STM32H750 has memories of very different speeds, and where the audio path sits decides how many voices fit.
`tessera_daisy.lds` is libDaisy's QSPI memory map with Tessera's hot parts moved into the two fast ones:

| Memory | Size | Speed | What goes there |
| --- | --- | --- | --- |
| ITCM | 64 KiB | the CPU's own, no wait | the code that runs for every sample: the FM-1's own choice (`.audio_text`, `.dsp_text`), then the mixer and the engines' render loops, as many as fit (`itcm_pick.py`) |
| DTCM | 128 KiB | the CPU's own, no wait | the tables those read (`.dsp_tables`, the sine, pitch and FM6 tables), the state they write (tracks, effects, FM6's RAM tables, the mix batch, the first banks of the audio arena), and the stack (32 KiB kept for it, from the top) |
| AXI SRAM | 512 KiB | fast, through the cache | everything else: the main audio arena, patterns, the FM-1's `.pool` and `.noinit` |
| RAM_D2_DMA | 32 KiB | not cached | what DMA reads or writes: libDaisy's audio buffers (16 KiB) and the screen's packed frame (8 KiB) |
| SDRAM | 64 MiB | slow, through the cache | nothing of Tessera's yet: long buffers later (Beads, samples), `DSY_SDRAM_BSS` |
| QSPI flash | 8 MiB | slow on a cache miss | the rest of the code and the constants |

Why: code in QSPI flash runs through a 16 KiB instruction cache, and every miss waits on the flash; ITCM never
misses. The DMA rule is the classic first bug on this chip: the CPU's data cache doesn't see what DMA does, so a
buffer DMA touches must sit where nothing is cached (or be cleaned and invalidated by hand). On 2026-11-13
(libDaisy 1289a00, arm-none-eabi-gcc 13.2): ITCM 58.5 of 64 KiB, DTCM 82.8 KiB plus the stack's 32, AXI SRAM
276 KiB, RAM_D2_DMA 24 of 32 KiB, QSPI 387 KiB. Only `mix_block` and `cord_block` (once a block each) stay in
QSPI for lack of room.

libDaisy's startup copies `.data` and zeroes `.bss`, nothing else, so `main.cpp` fills ITCM and DTCM first thing
(`tessera_memory_init`) and zeroes the SDRAM's `.sdram_bss` once `hw.Init` has started it. A section name the
linker script doesn't know would get a place in RAM and its bytes in flash, and nothing would ever copy them:
`memcheck.py` fails the build if that happens. Moving the FM-1's placements across (rather than dropping them, as
the first build did) also keeps its `noinline` and `always_inline`, which the compiler now honours.

## Flash (once the board is here)

1. Once: `make -C ~/src/libDaisy/... program-boot` puts the Daisy bootloader in the Seed's internal flash (see
   Electrosmith's docs for the current command).
2. Press RESET; while the LED pulses, `make -C platform/daisy LIBDAISY_DIR=... program-dfu`.
3. The LED blinks slowly once Tessera runs; the figure plays on the audio outputs.

Rescue: hold BOOT, tap RESET, release BOOT, and flash again.

## The sample rate

The FM-1 runs at 44.1 kHz and the Seed's codec at 48 kHz. `tools/gen_tables.py` reads `MELODEE_FS` and every
rate-dependent number in the firmware is derived from `FS`, so this build plays in tune (`tests/fs_test.py`
checks it on the host). Since 2026-11-13 that includes the 808 kit (its fixed filters come from `gen_tables.py`;
its metal oscillators, which were 9 % sharp at 48 kHz, and its short envelopes are rescaled) and PHYS (the modes'
Q scale, Dust's density, the string's dispersion), each identical at 44.1 kHz. `fs_test.py` checks the cowbell's
pitch and the decays of four PHYS sounds. The 808's noise sounds (hats, snare, clap) and PHYS' plucks and bows
were compared by their spectra, octave by octave: what still differs comes from the random noise running a
different course at another rate, not from a number made for 44.1 kHz.

## Next

The plan is in the private notes (tessera-notes `andamento/DAISY_PORT.md`): a demo project instead of the fixed
figure, the screen's SPI (below), encoders and keys, MIDI, storage on SD. On the board, the first number to read is
`tessera_core_load` (the audio callback's peak share of the time, 0..1) with the debugger.

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
