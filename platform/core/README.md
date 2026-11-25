# platform/core: Tessera's core API

One front-end interface for every build of Tessera that is not the FM-1's own: the web simulator (`web/sim/`), the
plugin (`platform/plugin/`), the Daisy for Andamento (`platform/daisy/`) and the cross-target save test
(`tests/tsp_cross.sh`). The FM-1 itself still builds from `firmware/src/melodee.c` with its hardware; none of this
changes its image.

| File | What |
| --- | --- |
| `tess_core.h` | the API: events in (keys, buttons, knobs, MIDI, the transport, a DAW's clock), audio out (float, or fixed Q15 blocks), the screen and the panel's LEDs, the project as a `.tsp` record and the four slots, parameters by central ID (`pid.h`), MIDI out, the screen's zones for a mouse (`tess_view_t.zones`, `tess_core_zone_do`; a profile with `TESS_HAS_POINTER`), a second screen, TRACKS (`tess_core_tracks_view`, `tess_core_tracks_do`; a profile with `TESS_SCR_COUNT` 2) |
| `tess_core.c` | its implementation over the shared build. Audio time drives the clock: each millisecond of samples runs the FM-1's main loop (a UI frame every 15 ms) |
| `core_unit.c` | the shared build: the whole firmware (sound, sequencer, generators, the FM-1's UI, the projects) on the host HAL, with `tess_core.c`. Each front end includes it with its own options (listed at its top) |
| `host_hal.h` | the FM-1's hardware without the FM-1: the stand-ins for `firmware/hal/` (buttons, keys, encoders, LEDs, a 240 x 240 screen, no flash) |
| `host_boot.h` | power-on (`ui_power_on`: `main.c` melodee_init's state) and `screen_clear` |

## One include list

The firmware's sources are listed once for all of these builds and the host tests, in two layers:

- `firmware/src/core_sources.h`: the core (tables, engines, voices, mods, mixer, sequencer, generators, parameters,
  cords). `tests/hostsim.c` includes it, and so everything built on it.
- `firmware/src/core_ui_sources.h`: the FM-1's UI, the user presets and the projects (`.tsp`). `tests/ui_test.c`
  includes it after `host_hal.h`.

`platform/core/core_unit.c` includes both, so the web simulator, the plugin and the Daisy build exactly what the host
tests build. A new source goes into `firmware/src/melodee.c` (the FM-1) and into one of the two lists.

## One save format

`tests/tsp_cross.sh` (and CI: `.github/workflows/core-api.yml`) builds the API four times, with the FM-1's options,
the web simulator's, the plugin's and the Daisy's (tables at 48 kHz, no FM-1 UI, no screen copy). Each loads every
demo in `demos/`, plays it a few seconds and saves it; every target then loads each save and saves it again at once,
and the bytes must not change.

## Targets

| | rate | FM-1 UI runs | screen copy | audio out | notes |
| --- | --- | --- | --- | --- | --- |
| web simulator | 44,100 | yes | yes | `tess_core_render` (via `web_render`) | Felucca's browser interface on top (`web/sim/sim.c`) |
| plugin | 44,100 | yes | yes | `tess_core_render` | `TESS_HOST_CLOCK`, `TESS_PLUGIN_IO`: the DAW's clock, track outputs, MIDI out |
| Daisy | 48,000 | no (`TESS_CORE_UI 0`) | no (`HOST_HAL_SCREEN 0`) | `tess_core_block` in the audio callback | keeps the firmware's placements (`CORE_KEEP_ATTRIBUTES`) for ITCM / DTCM |

One core per process: the firmware keeps its state in file-level globals (a plugin instance per loaded copy of the
code first; a state struct later, as its own refactor).
