# Smalti: Tessera as a plugin

**Smalti** is Tessera inside a DAW (called Tessera until 0.1-beta4; Audio Damage has a plugin of that name). The visible
names changed; the CLAP id, VST3 class ids, AU codes and the settings folders stay `tessera`, so saved songs still load: the same firmware as the FM-1, built as a CLAP instrument and, through
[clap-wrapper](https://github.com/free-audio/clap-wrapper), as VST3, an Audio Unit (macOS) and a standalone app (Windows, macOS). It plays the whole firmware at up to 8
tracks, in TESSERA's own window or the FM-1's.

## What works

- **Sound:** the whole firmware (sequencer, generators, voices, FX) as the web simulator runs it (`web/sim/sim.c`),
  at 44.1 kHz inside, resampled to the host's rate by a 64-tap windowed sinc (`resample.h`: flat to 20 kHz, the error ~90 dB under the signal, nothing folded down at lower rates; ~0.4 % of a core). It plays 32 core frames (0.7 ms) behind, reported to the host as latency; at 44.1 kHz it is a plain copy, no latency.
- **Engines the FM-1 has no room for:** SID (a whole MOS 6581 / 8580 per note), DRUM's SID KIT and SID DIGI, and
  MACRO (Plaits' synthesis), PHASE (the CZ-1's phase distortion, CrispyZebra's waveforms), besides the FM-1's own.
  A project with them opens on the FM-1 too: its SID, MACRO and PHASE tracks play FM6's INIT there, a SID kit the 909. The Daisy build (Andamento) has them as well.
- **Start:** a new project (`project_new_blank`, as an FM-1 with nothing saved: the power-on sounds, every pattern
  empty, no generator), so a new instance plays nothing until it gets notes; STILL TIDE and KOTO GARDEN wait in slots
  A and B (`web/sim/sim.c`). The host's PLAY / STOP start and stop the song. The standalone app has no transport and
  no DAW to send notes: it loads STILL TIDE (slot A) at its first block, unless a state was loaded, and plays at once.
- **MIDI in:** notes and MIDI messages reach Tessera as USB MIDI does on the FM-1, at their exact sample. ROUT starts
  at SEL (a DAW sends on channel 1: the notes play the selected track); a note-off for every key (key -1) is All
  Notes Off, a choke All Sound Off. **Program Change** (`TESS_PC_IN`, `form_chain.c tessera_pc_in`), on any channel:
  0..2 cue the CHAIN's section A..C for the next phrase line (the next PLAY while stopped), 8..15 every track's bank
  1..8 through PATTERNS' queue (`pattern_request`). CLK stays INT and CLOCK OUT off (the DAW keeps time). The device's MIDI setup
  (ROUT, DRUM's channel, NOTES OUT, TUNE: `tess_core_setup_save`) goes in the DAW state as its bytes, their count and
  `TGLO`, before `TMST`, never in the `.tsp`; a reused core copy gets its power-on setup back.
- **MPE and note expression** (`TESS_MPE`): each note from MIDI remembers its channel and key, and keeps its own
  pressure and slide: an MPE member channel's pressure and CC74, Poly AT, and a CLAP note's PRESSURE and BRIGHTNESS
  (`CLAP_EVENT_NOTE_EXPRESSION` -> `tess_core_note_expr`, queued with the notes). The note port offers the CLAP, MIDI
  and MIDI MPE dialects. In the mods, AT on a voice destination (PITCH, CUTOFF, SHAPE, GAIN) of its own track is each
  note's own, and the new source SLIDE (cord code 35) too; a note not from MIDI reads the track's. **Pitch bend per
  note:** a note from MIDI bends by its own channel's bend (`voice_t.mbt`, smoothed in `mbq8` as the track's) plus a
  CLAP note's TUNING (`tess_core_note_tune`, a queue of its own taken after the block's MIDI); MCM (RPN 6 on channel
  1 or 16) sets the members' range to 48, RPN 0 reaches 96. FM6 bends by its own DX7 range, a track at a time.
- **Saving:** the plugin's state in the DAW project is the Tessera project as a `.tsp` record, the same bytes SAVE >
  PROJECT writes on the FM-1.
- **Presets in the DAW's browser** (CLAP: CLAP preset discovery and preset-load, for the hosts that read them, e.g.
  Bitwig): the eight demos inside the plugin (factory content, each described as `demos/README.md` has it) and every
  `.tsp` in **Documents/Tessera/Presets** (your own; the folder is made at the first scan). A preset is a whole
  project, loaded as OPEN loads one, and the DAW is told which. SAVE under the keys offers that folder, so what you
  save there shows in the browser. (The VST3 and AU builds: not through their own preset lists yet.)
- **Project files:** OPEN and SAVE, under the keys, open a `.tsp` (the editor's "project file", the demos in
  `demos/`) or an **FM-1 backup** (`.json` from the backup page or the Hub: its slots A..D go into the plugin's
  slots, then the music it was playing loads), and save the project as `.tsp` under its name, for the FM-1's editor or
  another Tessera. On Windows and macOS the system's dialogs (a file dropped on the window opens too); on Linux
  zenity or kdialog, whichever is installed. The DAW is told its saved state changed.
- **MIDI clip:** MIDI, beside OPEN and SAVE, writes the notes Tessera played since PLAY (or since the song last jumped
  back: a loop, a locate) as a Standard MIDI File in `Documents/Tessera/MIDI` (`<project> <date time>.mid`), to drag
  into the DAW: format 1, the project's tempo, a track per channel (T1..T8, each track on its MIDI OUT channel), from
  the bar of the first note. Only notes (no CCs, no clock); up to 65,536 messages, the oldest dropped past that
  (`midifile.h`; kept on the audio thread in a fixed ring, nothing allocated there).
- **LISTEN** (the menu: `< OFF >`, then CH 1..16, and SPLIT under it): the notes on that MIDI channel steer Tessera
  instead of playing it (`listen.h`). From SPLIT up (note 48 unless moved, an octave at a time from 12 to 108; shown
  as a note number, since DAWs name that note C2 or C3), what is held sets the key: one note its ROOT, a chord its
  root (inversions too) on every melodic track (drum tracks keep theirs, as KEY LINK leaves them), and a major or
  minor third above that root turns a MAJ / MIN (or PEN / MPEN) song to its twin; other scales keep theirs, and
  letting go keeps the key. Under SPLIT a note GENERATEs a piece at the strip's VARIATION, its seed from the note and
  velocity, so a clip that plays the same note makes the same piece. A white key also picks the style: the octave
  just under SPLIT, C to B, the strip's first seven styles, the octave under that the next seven, and so on; a black
  key (or a white one past the last style) keeps the style the strip shows. With SPLIT 48 the bottom octave of a 49
  or 61-key keyboard (36..47) is the first seven styles. The strip shows the piece's style after. Channel and SPLIT
  are the instance's, saved in the DAW's state as `TLSN`, the channel and SPLIT when LISTEN is on or SPLIT moved (a
  plugin build before LISTEN cannot open such a state; with LISTEN OFF at SPLIT 48 the state is as before).
- **The window** (Windows, Linux and macOS): TESSERA's own shell by default (shell 0: flat, the project page's
  colours, a menu under the mark for SHELL / INK / MODE / SIZE / ABOUT, a MIDI lamp), or the FM-1 as the web
  simulator draws it, in any of its eight shells (the swatches
  under the keys, at the right; each sets its own Mono ink, as the web simulator's picker does, and MENU > COLOR still
  changes the ink after), and MENU > COLOR's 13 inks as a row of swatches between the buttons and the keys (the one
  the screen has ringed, its name on hover; tess_core_ink_at lists them, tess_view_t.ink is the one now). The shell
  picked and the ink last shown (a pick there, a shell's ink, or MENU > COLOR) are remembered for every window, in any
  DAW (files `shell` and `ink` in `%APPDATA%\Tessera`, `~/.config/tessera` or
  `~/Library/Application Support/Tessera`; the ink over the shell's own), not in the project. **SIZE** (TESSERA's menu) 100, 150 or 200 % (EXPANDED: 100 or 125, `Panel::size_fit`): the whole window larger (200 % each pixel doubled, 125 and 150 % smoothed), remembered for every window (file `size`); until one is picked, the host's scale for the screen (a HiDPI display) picks the nearest; a host that lets its frame be dragged snaps it to the nearest the mode has. **MODE** (TESSERA's menu, file `mode`): FM-1, or EXPANDED (TESSERA's shell only; tessera-notes docs/design/PLUGIN_EXPANDED.md): 1500 x 624, the TRACKS screen (`tess_core_tracks_view`, firmware/src/ui_tracks.c, drawn by the audio thread with the view while `tracks_on`) at 568,52 beside the FM-1's, KNOB 1-4 and the buttons right of it, the keys across the window, no bank switch; TRACKS' zones (a row, M, S, a level) go to `tess_core_tracks_do` (`PanelInput::tzones`); each window follows `Panel::width()`. The screen at 1:1 within it;
  click the buttons and keys (drag across the keys to glide); drag a knob up/down or left/right, or use the mouse
  wheel over it. **The screen's zones** (`tess_view_t.zones`, `ui_zones.c`, profile fact `TESS_HAS_POINTER`): the
  Mono pages leave what a mouse can use as they draw it (a value's column, a list's row, OK / BACK / YES / NO, the
  browser's and SCALES' two lists, HOME's tracks); the panel hit-tests them before its own knobs (wheel and drag on a
  value, click and double-click on a row or a key) and sends `tess_core_zone_do`, which turns the knobs and presses the
  keys from a queue (a row's detents never accelerated); the zone under the mouse outlined, its name and value large
  under the screen. **The computer keyboard** too, as in the web simulator (click the window first): Z..' and Q..O play
  the keys, 1..= are the twelve buttons in the panel's order and [ ] OCT- / OCT+ (each held while its key is), up /
  down turn SELECT, left / right ALGORITHM. Keys go by where they are, not what they print, so any layout works;
  with Ctrl or Alt held they are the DAW's. The LEDs light, dim and breathe as on the device. The window talks to the core through the audio
  thread, so it only moves while the host is processing audio.

- **An FM-1 on USB plays the plugin** (FM-1 USB, in TESSERA's menu or under the keys in an FM-1 shell): the web
  simulator's "An FM-1 on USB" (`fm1_link.h`). The plugin opens the FM-1's own MIDI ports (WinMM; ALSA's raw MIDI
  devices, no library; CoreMIDI), not the DAW's, asks with PANEL (editor SysEx 75) about once a second, and decodes
  channel 16's buttons, keys, knobs and MASTER into the same input the window gives the core (the two held together).
  One FM-1 for the process, one instance at a time: a click on ON takes it; a new instance with the setting on waits
  its turn. States: NO FM-1 FOUND, PORT IN USE (Windows: the DAW holds it), UPDATE THE FM-1 (no answer: 3.5 or older),
  FM-1 CONNECTED, IN ANOTHER TESSERA. The setting is the file `fm1_usb` beside `shell` and `ink`; the link starts at
  an instance's first activate, not at init, so a DAW's plugin scan opens no ports. No firmware change.
  `tessera-fm1-test`: the decoder over captured byte streams, the port names, the link with a stand-in FM-1.

- **GENERATE** (TESSERA's shell, the strip at the top): one click, a whole piece (LEMONADE SANDWICH's fifteen
  styles, `firmware/src/generate.c`): the four tracks'
  sounds, generators, key, cords and the long-form layers, from a new project, all ordinary settings. Now and then a
  track takes a preset of the plugin's own engines (PHASE, SID, MACRO: a few picked a role, never a patch rolled), and
  in the styles that move their parts (ENTRIES) up to three of tracks 5..8 are alternates of tracks 1..4: the same part
  with a SEED of its own and another sound, taking turns with it a section at a time (ENTRIES ROLE TURNS), so a piece
  has more sounds over its length, never more parts at once. GENERATE makes a new piece in the style shown from a new seed; **held** half a
  second, the project before the piece comes back (held again: the piece). The style: click its name for the list, or
  the arrows / the wheel. **VAR** 0..100: how wide each pick is in its range (100 the styles as tuned, 0 the middle of
  every range; drag or wheel, double-click 100). The seed shows the piece's; the wheel over it makes the pieces made
  before again, exactly (the last 16 of this instance). While a DAW drives, the piece keeps the DAW's tempo; the
  standalone app takes the style's. The style and VAR picked are remembered for every window (files `gen_style`,
  `gen_variation` beside `shell`), not in the project; a piece is saved as any project is. A press marks the DAW's
  state changed. Not yet: KEEP a track, make one track again, typing a seed.
- **Following the DAW** (CLK INT): the DAW's PLAY starts every track where it would be after playing from the DAW's
  zero (as a MIDI Song Position), its beat position drives Tessera's clock (fractional tempos and tempo changes, no
  drift), a loop or a locate seeks, STOP stops. The BPM on the screen is the DAW's, rounded; a TEMPO mod has no effect
  while the DAW plays. CLK stays INT in the plugin (not a parameter), so the DAW is always the clock.
- **Undo in the DAW:** a knob turned in the window (or on an FM-1 playing the plugin) sends its parameters' changes
  between a gesture's begin and end, so the DAW makes each turn one undo step (and writes automation in its touch or
  latch modes); the gesture ends once the knobs have been still half a second. A whole project changed (OPEN, a
  backup, GENERATE and its BEFORE / AFTER) is told to a host with CLAP's undo extension (a draft: few hosts yet) as a
  named step; elsewhere hold GENERATE to go back to the project before a piece.
- **Parameters** (automatable): MASTER and PERF's MACRO 1..4 (the project's, so they are saved with it too), then
  **every project setting** by its central ID (about 1,700): each of the 8 tracks' ("T1 LEVEL", "T3 TURING LENGTH",
  in the modules "Track n/Sound and sequence" and "Track n/Generator") and the song's ("Song", TUNE, ROUT and MIDI
  INPUT among them; not CLK, held at INT). Values read as the FM-1 shows them ("SAW", "1/16", "-6dB") and can be typed that way. ENGINE 1..8 mean
  what the track's engine makes them (their range is the widest any engine has). Changed on the panel, by a project or
  by the core, a value is sent back to the DAW (MASTER and the MACROs at once, the rest within ~20 blocks). Ids are
  stable: 0..4 as before, then 0x10000 + slot * 0x1000 + pid (slot 0..3 tracks 1..4, 4 the song, 5..8 tracks 5..8).

- **Track outputs:** besides the main mix, TRACK 1..8 each alone (`NSTEMS = NTRACKS`: the FM-1's USB stems, the
  track after its level, before pan and the bus FX, mono on both sides). The core makes them once the DAW connects one.
- **MIDI out:** every track's notes, each on its own channel (GLO > MIDI OUT; AUTO is the track's own 1..8 here,
  whatever ROUT takes in), to drive other instruments. NOTES OUT starts at ALL in the plugin (the generators' notes
  too); MENU can set KEYS or OFF. Channel messages only (no clock or system messages). The notes left on when the host
  stops processing (stop_processing, deactivate, reset) get their note-offs in the next block (`tess_core_notes_off`).
- **Several instances:** up to 8 Tesseras in one DAW project, each wholly its own (its project, transport, window,
  parameters), and they may run on different threads. The firmware keeps its state in globals, so the plugin carries
  8 private copies of the core (`core_copy.c`: the core compiled once per copy, each of its shared names prefixed
  `tc<k>_` from `build/gen/core_syms.h`) and each instance takes a free one; a 9th is refused. A new instance starts
  from a new project; one put in a copy another instance had used starts from a new project too
  (`tess_core_project_new`), but that copy's project slots A..D keep what the last one saved in them. The cost: the plugin file is about 16 MB (8 copies of the
  core's code and tables) and each instance about 1 MB of memory more. `-DTESS_PLUGIN_COPIES=n` (1..16) builds
  another number.

## Not yet

The list for players is PLUGIN.md's "Not there yet". Here: the window (Windows, Linux, macOS: `gui_win32.cpp`,
`gui_x11.cpp`, `gui_mac.mm`) has no free resize (SIZE's steps only); the standalone app is Windows and macOS only; the builds are
not signed.

## Build

Needs CMake 3.21+, clang and nm (or llvm-nm), Python 3 with Pillow and fontTools (as `./build.sh` does), and network
access the first time (the CLAP SDK, clap-wrapper and the VST3 SDK are downloaded at configure time).

```
cmake -S platform/plugin -B build/plugin -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build/plugin -j
build/plugin/tessera-panel-test shots/                                             # the window's test: PPM pictures in shots/
build/plugin/tessera-sync-test                                                     # following the DAW
build/plugin/tessera-copies-test                                                   # several instances: the copies apart
xvfb-run build/plugin/tessera-render-test build/plugin/out/Smalti.clap 12 still-tide.wav window.ppm   # Linux / macOS
build/plugin/tessera-session-test build/plugin/out/Smalti.clap                     # a DAW session saved and opened again
build/plugin/tessera-cpu-bench build/plugin/out/Smalti.clap 10 48000 256          # what 1, 4 and 8 instances cost (measures only)
```

Windows (Visual Studio 2022 with its "C++ Clang tools for Windows" component). Windows' Pillow has no Raqm for the
UI font, so make the generated headers in WSL first (`tools/wsl.sh python3 platform/plugin/gen.py`), then:

```
cmake -S platform/plugin -B build/plugin -G "Visual Studio 17 2022" -A x64 -T ClangCL -DTESS_GEN=OFF
cmake --build build/plugin --config Release
```

The outputs are in `build/plugin/out/`: `Smalti.clap`, `Smalti.vst3` and (Windows, macOS) the standalone
`Smalti` app. The GitHub workflow `plugin.yml` builds them on every pull request that touches the firmware and keeps
them as the run's artifact.

## Licences

Tessera is GPL-3.0-only. The CLAP SDK and clap-wrapper are MIT; the VST3 SDK is MIT since 3.8. VST is a trademark
of Steinberg Media Technologies GmbH.
