# Tessera plugin: what's in the beta

The first public beta of Tessera as a plugin: the same firmware the FM-1 plays, as a CLAP, VST3 and (on macOS) Audio
Unit instrument, and as a standalone app. The manual for the plugin is
[PLUGIN.md](https://github.com/Rastapopolix/tessera/blob/main/PLUGIN.md); everything in the
[manual](https://github.com/Rastapopolix/tessera/blob/main/MANUAL.md) works the same way.

Found a problem? Open an issue: <https://github.com/Rastapopolix/tessera/issues> (the plugin's version, your DAW and
its version, your system, the steps, what you expected and what happened).

## In this beta
- Windows (64-bit): CLAP, VST3 and the standalone app.
- macOS 10.15 or later, Apple silicon and Intel: CLAP, VST3, Audio Unit and the standalone app.
- Linux (x86-64): CLAP and VST3.
- The window is the FM-1, 1:1: click the buttons and keys, drag the knobs, or play from the computer keyboard.
- Eight tracks. The 1-4 / 5-8 switch (or Tab) picks which four the panel works on; the screen shows 5-8 while you're
  there.
- The DAW's transport, tempo, loops and jumps drive Tessera.
- Nine stereo outputs: the main mix, then TRACK 1 to TRACK 8 on their own.
- MIDI in on channels 1 to 8 plays tracks 1 to 8; MIDI out sends each track's notes on its own channel.
- About 1,700 settings as DAW parameters you can automate, with the FM-1's names and values.
- Up to 8 Tesseras in one song, each with its own project.
- OPEN and SAVE: `.tsp` projects and FM-1 backups (the `.json` from the Hub or the backup page). A `.tsp` from the
  plugin opens on the FM-1 too.
- Sounds only the plugin has: SID, the SID drum kit and SID DIGI, MACRO, and PHASE (phase distortion in the manner of
  the CZ-1). On the FM-1 those tracks play FM6's INIT sound instead.
- Shell and ink pickers: your choice is remembered for every new window.

## Not there yet
- Tracks 5 to 8's settings are saved with your DAW song, but **their steps aren't saved yet**, and a `.tsp` holds
  tracks 1 to 4 only.
- PHASE comes with 33 presets of its own, not the CZ-1's original tones.
- One window size (the FM-1's). A larger, wider window is planned.
- No presets in the DAW's own browser: use OPEN, or the DAW's way of saving a plugin's state.
- No standalone app on Linux.
- The device's own settings (A4, the MIDI routing) and the MOD cords aren't DAW parameters; set them on the panel.

## Good to know
- **Unsigned on macOS:** macOS will call the plugins "damaged" or say they "can't be verified" until you clear the
  download's quarantine. INSTALL-macos.txt has the one Terminal line.
- **Unsigned on Windows:** SmartScreen may warn about Tessera.exe the first time: More info, then Run anyway.
- The window moves only while the DAW's audio engine runs.
- Some DAWs keep Tab for themselves; click the 1-4 / 5-8 switch instead.
- If you hear clicks with several Tesseras, raise the audio buffer to 256 samples.
- A DAW song saved with this beta won't open in Tessera builds from before it (only test builds existed before).
- With a busy jam on all eight tracks, capture-to-loop may not hold everything you played.

## Please try
- Your DAW: does Tessera load, play, and come back as you left it after saving and reopening the song?
- On a Mac: Logic or GarageBand (Audio Unit), Live (VST3), Bitwig or Reaper (CLAP); a Retina and a non-Retina screen.
- Route TRACK 5 to 8 to their own channels and play MIDI on channels 5 to 8.
- Automate a setting, then change the track's engine: does the lane's name follow?
- Two or three Tesseras in one song, each with its own project.
- OPEN an FM-1 backup and a demo `.tsp`; SAVE one and open it on your FM-1.
