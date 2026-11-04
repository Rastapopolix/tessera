# Tessera as an .rfm1 (RemoteFM1)

An `.rfm1` is an FM-1 firmware built to run inside a host app, RemoteFM1 for iOS first: the firmware's host build
compiled to WebAssembly and run by the app's interpreter (wasm3). FM1VST (github.com/Quixotic7/FM1VST) builds it:
its `docs/RFM1.md` has the format and the tools.

This folder is Tessera's core glue for FM1VST:

| file | what |
| --- | --- |
| `core_tessera.c` | the firmware unit: the web simulator's firmware (`web/sim/sim.c`, on `tests/ui_test.c`'s HAL stubs) behind FM1VST's core ABI (`core-api/fm1core.h`) |
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

Each call returns: `tick` sets the device's millisecond, `frame` is one pass of main.c's loop (input, LEDs, screen),
`idle` reads the input, `audio` runs `mix_block` 32 frames at a time. It starts as the web simulator does, with STILL
TIDE in slot A and KOTO GARDEN in slot B.

Left out, as in the web simulator: the flash (project slots are RAM, so saves are lost when the app closes), USB
audio, the USB host, the update, the editor's SysEx, the CPU budget (`stats` reports 0).
