# Smalti: what's in the beta

Smalti is Tessera as a plugin: the same firmware the FM-1 plays, as a CLAP, VST3 and (on macOS) Audio
Unit instrument, and as a standalone app. The manual for the plugin is
[PLUGIN.md](https://github.com/Rastapopolix/tessera/blob/main/PLUGIN.md) (also on the
[plugin's page](https://rastapopolix.github.io/tessera/plugin/)); everything in the
[manual](https://github.com/Rastapopolix/tessera/blob/main/MANUAL.md) works the same way.

**Until beta 4 it was called Tessera** (Audio Damage makes a plugin of that name). Songs saved with the old name open in
Smalti; delete the old `Tessera.clap`, `Tessera.vst3`, `Tessera.component` and app first, so your DAW doesn't find it
twice (the install notes say where).

It's a beta: expect some rough edges. Found a problem? Open an issue:
<https://github.com/Rastapopolix/tessera/issues> (the plugin's version, your DAW and its version, your system, the
steps, what you expected and what happened).

## In this beta
- Windows (64-bit): CLAP, VST3 and the standalone app.
- macOS 10.15 or later, Apple silicon and Intel: CLAP, VST3, Audio Unit and the standalone app.
- Linux (x86-64): CLAP and VST3.
- **Tessera's own look** (the window opens in it), or any of the FM-1's eight colours: the Tessera mark at the top left
  opens the menu (SHELL, INK, MODE, SIZE, FM-1 USB, ABOUT).
- **EXPANDED:** a wider window with a second screen, TRACKS, beside the FM-1's: every track on a row of its own (its
  steps, level, M and S). It opens by itself when your screen has room; MODE in the menu switches it.
- **SIZE:** 100, 150 or 200% (EXPANDED: 100 or 125%); drag the window's corner and it snaps from one size to the next.
- Eight tracks. In FM-1 mode the 1-4 / 5-8 switch (or Tab) picks which four the panel works on. All eight are saved,
  with the DAW song and in a `.tsp`.
- **The mouse on the screen:** turn the wheel or drag a value; the wheel scrolls lists and texts; click a row, a slot,
  a pattern, a section, an ink or a mixer strip's MUTE / SOLO; double-click for OK or a default.
- **The computer keyboard** plays it, as in the web simulator, and types names (NAME) directly.
- A new Smalti opens on an empty project and plays nothing until it gets notes; the demos are in slots A and B, in
  the DAW's preset list, and in the `demos/` folder. The standalone app opens with STILL TIDE playing.
- The DAW's transport, tempo, loops and jumps drive Smalti; its undo takes back GENERATE and preset loads.
- Nine stereo outputs: the main mix, then TRACK 1 to TRACK 8 on their own.
- MIDI in plays the selected track (ROUT SEL) or tracks 1 to 8 on channels 1 to 8; Program Change picks sections and
  banks; an **MPE** controller plays with per-note bends and pressure. MIDI out sends each track's notes on its own
  channel, and MIDI under the keys writes what played as a MIDI clip.
- About 1,700 settings as DAW parameters you can automate, with the FM-1's names and values.
- **Presets in the DAW's list** (CLAP, VST3 and Audio Unit): the demos, and what you SAVE into Documents/Tessera/Presets.
- Up to 8 Smaltis in one song, each with its own project.
- OPEN and SAVE: `.tsp` projects and FM-1 backups (the `.json` from the Hub or the backup page). A `.tsp` from the
  plugin opens on the FM-1 too.
- **FM-1 USB:** an FM-1 on USB plays the plugin, its knobs and keys and screen.
- Sounds only the plugin has: SID, the SID drum kit and SID DIGI, MACRO, and PHASE (phase distortion in the manner of
  the CZ-1). On the FM-1 those tracks play FM6's INIT sound instead.
- FX **SYNC ECHO**: an echo on note values (1/32 to 1/1, triplets, dotted) at the DAW's tempo, up to about 3 s.

## Not there yet
- PHASE comes with 33 presets of its own, not the CZ-1's original tones.
- No standalone app on Linux.
- Not signed on macOS or Windows (see below).
- DRUM, MOTION and MODS don't take the mouse yet: use the panel's knobs and keys there.
- The device's own settings (A4, the MIDI routing) and the MOD cords aren't DAW parameters; set them on the panel.

## Good to know
- **Unsigned on macOS:** macOS will call the plugins "damaged" or say they "can't be verified" until you clear the
  download's quarantine. INSTALL-macos.txt has the one Terminal line.
- **Unsigned on Windows:** SmartScreen may warn about Smalti.exe the first time: More info, then Run anyway.
- Signing would take both away; it costs about USD 99 a year with Apple, plus a Windows certificate. If you'd like to
  help get Smalti signed: [ko-fi.com/Rastapopolix](https://ko-fi.com/Rastapopolix).
- The window moves only while the DAW's audio engine runs.
- Some DAWs keep Tab for themselves; click the 1-4 / 5-8 switch instead.
- If you hear clicks with several Smaltis, raise the audio buffer to 256 samples.
- With a busy jam on all eight tracks, capture-to-loop may not hold everything you played.

## Please try
- Your DAW: does Smalti load, play, and come back as you left it after saving and reopening the song?
- On a Mac: Logic or GarageBand (Audio Unit), Live (VST3), Bitwig or Reaper (CLAP); a Retina and a non-Retina screen.
- EXPANDED on your screen, and SIZE; drag the window's corner in the standalone app.
- The presets in your DAW's list, in each format you use.
- Route TRACK 5 to 8 to their own channels and play MIDI on channels 5 to 8; an MPE controller if you have one.
- Two or three Smaltis in one song, each with its own project.
- OPEN an FM-1 backup and a demo `.tsp`; SAVE one and open it on your FM-1.
