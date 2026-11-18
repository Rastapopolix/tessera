# Tessera as a plugin

Tessera inside a DAW: the same firmware as the FM-1, built as a CLAP instrument and, through
[clap-wrapper](https://github.com/free-audio/clap-wrapper), as VST3 and a standalone app. This is the first step
(phases 0 and 1): it plays, and its window is the FM-1.

## What works

- **Sound:** the whole firmware (sequencer, generators, voices, FX) as the web simulator runs it (`web/sim/sim.c`),
  at 44.1 kHz inside, resampled to the host's rate.
- **Start:** STILL TIDE is loaded (KOTO GARDEN in slot B), as in the simulator. The host's PLAY / STOP start and stop
  it; the standalone app has no transport, so it plays at once.
- **MIDI in:** notes and MIDI messages reach Tessera as USB MIDI does on the FM-1, at their exact sample.
- **Saving:** the plugin's state in the DAW project is the Tessera project as a `.tsp` record, the same bytes SAVE >
  PROJECT writes on the FM-1.
- **The window** (Windows and Linux): the FM-1 as the web simulator draws it, black shell. The screen at 1:1;
  click the buttons and keys (drag across the keys to glide); drag a knob up/down or left/right, or use the mouse
  wheel over it. The LEDs light, dim and breathe as on the device. The window talks to the core through the audio
  thread, so it only moves while the host is processing audio.

- **Following the DAW** (CLK INT): the DAW's PLAY starts every track where it would be after playing from the DAW's
  zero (as a MIDI Song Position), its beat position drives Tessera's clock (fractional tempos and tempo changes, no
  drift), a loop or a locate seeks, STOP stops. The BPM on the screen is the DAW's, rounded; a TEMPO mod has no effect
  while the DAW plays. With CLK on MIDI, the MIDI master's clock rules and the DAW only starts and stops it.
- **Parameters** (automatable): MASTER and PERF's MACRO 1..4 (the project's, so they are saved with it too). Turned on
  the panel or on PERF, the new value is sent back to the DAW.

## Not yet

- No window on macOS (it plays as in phase 0).
- No computer-keyboard shortcuts, no shell or ink picker (the ink is MENU > COLOR, as on the device).
- One Tessera per DAW session: the firmware keeps its state in globals, so a second instance is refused, and a new
  instance carries on from the last one's state.
- No multi-out, no MIDI out.

## Build

Needs CMake 3.21+, clang, Python 3 with Pillow and fontTools (as `./build.sh` does), and network access the first
time (the CLAP SDK, clap-wrapper and the VST3 SDK are downloaded at configure time).

```
cmake -S platform/plugin -B build/plugin -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build/plugin -j
build/plugin/tessera-panel-test shots/                                             # the window's test: PPM pictures in shots/
build/plugin/tessera-sync-test                                                     # following the DAW
xvfb-run build/plugin/tessera-render-test build/plugin/out/Tessera.clap 12 still-tide.wav window.ppm   # Linux / macOS
```

Windows (Visual Studio 2022 with its "C++ Clang tools for Windows" component). Windows' Pillow has no Raqm for the
UI font, so make the generated headers in WSL first (`tools/wsl.sh python3 platform/plugin/gen.py`), then:

```
cmake -S platform/plugin -B build/plugin -G "Visual Studio 17 2022" -A x64 -T ClangCL -DTESS_GEN=OFF
cmake --build build/plugin --config Release
```

The outputs are in `build/plugin/out/`: `Tessera.clap`, `Tessera.vst3` and (Windows, macOS) the standalone
`Tessera` app. The GitHub workflow `plugin.yml` builds them on every pull request that touches the firmware and keeps
them as the run's artifact.

## Licences

Tessera is GPL-3.0-only. The CLAP SDK and clap-wrapper are MIT; the VST3 SDK is MIT since 3.8. VST is a trademark
of Steinberg Media Technologies GmbH.
