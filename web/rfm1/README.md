# Tessera as an .rfm1 (RemoteFM1)

An `.rfm1` is an FM-1 firmware built to run inside a host app, RemoteFM1 for iOS first: the firmware's host build
compiled to WebAssembly and run by the app's interpreter (wasm3). FM1VST (github.com/Quixotic7/FM1VST) builds it:
its `docs/RFM1.md` has the format and the tools.

This folder is Tessera's core glue for FM1VST:

| file | what |
| --- | --- |
| `core_tessera.c` | the firmware unit: power-on as main.c does it, and the loop behind FM1VST's core ABI (`core-api/fm1core.h`) |
| `tessera_firmware.h` | the include list: the host build (`tests/hostsim.c`, `tests/ui_test.c`'s HAL stubs) with the flash (`storage.c` on FM1VST's NOR image) |
| `core_tessera_desc.c` | the descriptor, the module's one export |
| `core.cmake` | `fm1_add_core` and `fm1_add_rfm1` |
| `build.sh` | builds, checks and runs the .rfm1 |

```
web/rfm1/build.sh [--version X.Y[.Z]]
```

It needs an FM1VST checkout with `fm1_add_rfm1` (`$FM1VST`, default `~/src/FM1VST`), Emscripten 6.0.13 (`emcc` on the
PATH, or emsdk in `$EMSDK`), CMake 3.22 or newer and the Python that runs `tools/build.py`'s generate step. FM1VST is
not changed: `build/rfm1/tree/` links to its folders, with this folder as `core-glue/tessera` and this repository as
`cores/tessera`. The file is `build/rfm1/tessera-<version>.rfm1`; the script checks it (`tools/rfm1_check.py`) and
plays it in wasm3 (`rfm1_run`). The version is the tag at HEAD, else the newest tag with `-dev`.

Each call returns: `tick` sets the device's millisecond, `frame` is one pass of main.c's loop (input, the settings
save, LEDs, screen), `idle` reads the input, `audio` runs `mix_block` 32 frames at a time.

The flash is the 1 MiB image the app keeps: settings, projects and user presets survive the app, as on the device. The
first run (the image erased) puts STILL TIDE in slot A and KOTO GARDEN in slot B and loads A, as the web simulator
starts; later runs boot as the device does (the BOOT project, or a new one).

Left out: USB audio, the USB host, the update, the editor's SysEx, the CPU budget (`stats` reports 0), and
Calibration (MENU > PANEL waits in loops for the buttons; the app's panel table is fixed, so it says ON THE FM-1).
