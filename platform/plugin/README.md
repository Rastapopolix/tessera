# Tessera as a plugin (spike)

Tessera inside a DAW: the same firmware as the FM-1, built as a CLAP instrument and, through
[clap-wrapper](https://github.com/free-audio/clap-wrapper), as VST3 and a standalone app. This is the first step
(phase 0): it plays, but has no window yet.

## What works

- **Sound:** the whole firmware (sequencer, generators, voices, FX) as the web simulator runs it (`web/sim/sim.c`),
  at 44.1 kHz inside, resampled to the host's rate.
- **Start:** STILL TIDE is loaded (KOTO GARDEN in slot B), as in the simulator. The host's PLAY / STOP start and stop
  it; the standalone app has no transport, so it plays at once.
- **MIDI in:** notes and MIDI messages reach Tessera as USB MIDI does on the FM-1, at their exact sample.
- **Saving:** the plugin's state in the DAW project is the Tessera project as a `.tsp` record, the same bytes SAVE >
  PROJECT writes on the FM-1.

## Not yet

- No window: no screen, panel or knobs (next phase).
- No tempo sync: Tessera keeps its own BPM; the host's tempo and bar position are ignored.
- One Tessera per DAW session: the firmware keeps its state in globals, so a second instance is refused, and a new
  instance carries on from the last one's state.
- No multi-out, no MIDI out.

## Build

Needs CMake 3.21+, clang, Python 3 with Pillow and fontTools (as `./build.sh` does), and network access the first
time (the CLAP SDK, clap-wrapper and the VST3 SDK are downloaded at configure time).

```
cmake -S platform/plugin -B build/plugin -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build/plugin -j
build/plugin/tessera-render-test build/plugin/out/Tessera.clap 12 still-tide.wav   # Linux / macOS
```

Windows (Visual Studio 2022 with its "C++ Clang tools for Windows" component):

```
cmake -S platform/plugin -B build/plugin -G "Visual Studio 17 2022" -A x64 -T ClangCL -DTESS_PLUGIN_TESTS=OFF
cmake --build build/plugin --config Release
```

The outputs are in `build/plugin/out/`: `Tessera.clap`, `Tessera.vst3` and (Windows, macOS) the standalone
`Tessera` app. The GitHub workflow `plugin.yml` builds them on every pull request that touches the firmware and keeps
them as the run's artifact.

## Licences

Tessera is GPL-3.0-only. The CLAP SDK and clap-wrapper are MIT; the VST3 SDK is MIT since 3.8. VST is a trademark
of Steinberg Media Technologies GmbH.
