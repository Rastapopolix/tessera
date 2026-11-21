# Tessera as a plugin

Tessera inside a DAW: the same firmware as the FM-1, built as a CLAP instrument and, through
[clap-wrapper](https://github.com/free-audio/clap-wrapper), as VST3 and a standalone app. This is the first step
(phases 0 and 1): it plays, and its window is the FM-1.

## What works

- **Sound:** the whole firmware (sequencer, generators, voices, FX) as the web simulator runs it (`web/sim/sim.c`),
  at 44.1 kHz inside, resampled to the host's rate.
- **Engines the FM-1 has no room for:** SID (a whole MOS 6581 / 8580 per note), DRUM's SID KIT and SID DIGI, and
  MACRO (Plaits' synthesis), PHASE (the CZ-1's phase distortion, CrispyZebra's waveforms), besides the FM-1's own.
  A project with them opens on the FM-1 too: its SID, MACRO and PHASE tracks play FM6's INIT there, a SID kit the 909. The Daisy build (Andamento) has them as well.
- **Start:** STILL TIDE is loaded (KOTO GARDEN in slot B), as in the simulator. The host's PLAY / STOP start and stop
  it; the standalone app has no transport, so it plays at once.
- **MIDI in:** notes and MIDI messages reach Tessera as USB MIDI does on the FM-1, at their exact sample.
- **Saving:** the plugin's state in the DAW project is the Tessera project as a `.tsp` record, the same bytes SAVE >
  PROJECT writes on the FM-1.
- **Project files:** OPEN and SAVE, under the keys, open a `.tsp` (the editor's "project file", the demos in
  `demos/`) or an **FM-1 backup** (`.json` from the backup page or the Hub: its slots A..D go into the plugin's
  slots, then the music it was playing loads), and save the project as `.tsp` under its name, for the FM-1's editor or
  another Tessera. On Windows the system's dialogs (a file dropped on the window opens too); on Linux zenity or
  kdialog, whichever is installed. The DAW is told its saved state changed. Not on macOS yet (no window).
- **The window** (Windows and Linux): the FM-1 as the web simulator draws it, in any of its eight shells (the swatches
  under the keys, at the right; each sets its own Mono ink, as the web simulator's picker does, and MENU > COLOR still
  changes the ink after), and MENU > COLOR's 13 inks as a row of swatches between the buttons and the keys (the one
  the screen has ringed, its name on hover; tess_core_ink_at lists them, tess_view_t.ink is the one now). The shell
  picked and the ink last shown (a pick there, a shell's ink, or MENU > COLOR) are remembered for every window, in any
  DAW (files `shell` and `ink` in `%APPDATA%\Tessera`, `~/.config/tessera` or
  `~/Library/Application Support/Tessera`; the ink over the shell's own), not in the project. The screen at 1:1;
  click the buttons and keys (drag across the keys to glide); drag a knob up/down or left/right, or use the mouse
  wheel over it. **The computer keyboard** too, as in the web simulator (click the window first): Z..' and Q..O play
  the keys, 1..= are the twelve buttons in the panel's order and [ ] OCT- / OCT+ (each held while its key is), up /
  down turn SELECT, left / right ALGORITHM. Keys go by where they are, not what they print, so any layout works;
  with Ctrl or Alt held they are the DAW's. The LEDs light, dim and breathe as on the device. The window talks to the core through the audio
  thread, so it only moves while the host is processing audio.

- **Following the DAW** (CLK INT): the DAW's PLAY starts every track where it would be after playing from the DAW's
  zero (as a MIDI Song Position), its beat position drives Tessera's clock (fractional tempos and tempo changes, no
  drift), a loop or a locate seeks, STOP stops. The BPM on the screen is the DAW's, rounded; a TEMPO mod has no effect
  while the DAW plays. With CLK on MIDI, the MIDI master's clock rules and the DAW only starts and stops it.
- **Parameters** (automatable): MASTER and PERF's MACRO 1..4 (the project's, so they are saved with it too), then
  **every project setting** by its central ID (about 930): each track's ("T1 LEVEL", "T3 TURING LENGTH", in the
  modules "Track n/Sound and sequence" and "Track n/Generator") and the song's ("Song"); not the device's own (A4, the
  MIDI routing). Values read as the FM-1 shows them ("SAW", "1/16", "-6dB") and can be typed that way. ENGINE 1..8 mean
  what the track's engine makes them (their range is the widest any engine has). Changed on the panel, by a project or
  by the core, a value is sent back to the DAW (MASTER and the MACROs at once, the rest within ~20 blocks). Ids are
  stable: 0..4 as before, then 0x10000 + slot * 0x1000 + pid (slot 0..3 a track, 4 the song).

- **Track outputs:** besides the main mix, TRACK 1..4 each alone (the FM-1's USB stems: the track after its level,
  before pan and the bus FX, mono on both sides). The core makes them once the DAW connects one.
- **MIDI out:** every track's notes, each on its own channel (GLO > MIDI OUT; by default ROUT's 1..4), to drive other
  instruments. NOTES OUT starts at ALL in the plugin (the generators' notes too); MENU can set KEYS or OFF.
- **Several instances:** up to 8 Tesseras in one DAW project, each wholly its own (its project, transport, window,
  parameters), and they may run on different threads. The firmware keeps its state in globals, so the plugin carries
  8 private copies of the core (`core_copy.c`: the core compiled once per copy, each of its shared names prefixed
  `tc<k>_` from `build/gen/core_syms.h`) and each instance takes a free one; a 9th is refused. A new instance starts
  from STILL TIDE; one put in a copy another instance had used starts from STILL TIDE again too, but that copy's
  project slots A..D keep what the last one saved in them. The cost: the plugin file is about 16 MB (8 copies of the
  core's code and tables) and each instance about 1 MB of memory more. `-DTESS_PLUGIN_COPIES=n` (1..16) builds
  another number.

## Not yet

- No window on macOS (it plays as in phase 0).

## Build

Needs CMake 3.21+, clang and nm (or llvm-nm), Python 3 with Pillow and fontTools (as `./build.sh` does), and network
access the first time (the CLAP SDK, clap-wrapper and the VST3 SDK are downloaded at configure time).

```
cmake -S platform/plugin -B build/plugin -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build/plugin -j
build/plugin/tessera-panel-test shots/                                             # the window's test: PPM pictures in shots/
build/plugin/tessera-sync-test                                                     # following the DAW
build/plugin/tessera-copies-test                                                   # several instances: the copies apart
xvfb-run build/plugin/tessera-render-test build/plugin/out/Tessera.clap 12 still-tide.wav window.ppm   # Linux / macOS
build/plugin/tessera-session-test build/plugin/out/Tessera.clap                     # a DAW session saved and opened again
build/plugin/tessera-cpu-bench build/plugin/out/Tessera.clap 10 48000 256          # what 1, 4 and 8 instances cost (measures only)
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
