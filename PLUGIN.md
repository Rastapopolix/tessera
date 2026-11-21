# Tessera as a plugin

Tessera also runs inside a DAW. The same firmware the FM-1 plays is built as a **CLAP** instrument, a **VST3**
instrument, an **Audio Unit** on macOS and a **standalone app**. Its window is the FM-1: the same screen, buttons, knobs and keys, so everything
in the [manual](MANUAL.md) works the same way here. This page covers only what is different in a DAW.

- [Install](#install)
- [First sound](#first-sound)
- [The window](#the-window)
- [Playing it from your FM-1](#playing-it-from-your-fm-1)
- [Playing with the DAW](#playing-with-the-daw)
- [Saving, and project files](#saving-and-project-files)
- [Automation: the parameters](#automation-the-parameters)
- [Track outputs and MIDI out](#track-outputs-and-midi-out)
- [Several Tesseras in one song](#several-tesseras-in-one-song)
- [Sounds only the plugin has](#sounds-only-the-plugin-has)
- [How much CPU it takes](#how-much-cpu-it-takes)
- [Not there yet](#not-there-yet)

## Install

Copy the plugin where your DAW looks for plugins, then rescan in the DAW.

Download the zip for your system from the [plugin's releases](https://github.com/Rastapopolix/tessera/releases)
(the betas are marked *Pre-release*). Each zip has an INSTALL text with these steps and the beta's notes.

| | CLAP (`Tessera.clap`) | VST3 (`Tessera.vst3`) | Audio Unit (`Tessera.component`) |
|---|---|---|---|
| Windows | `C:\Program Files\Common Files\CLAP\` | `C:\Program Files\Common Files\VST3\` | |
| Linux | `~/.clap/` | `~/.vst3/` | |
| macOS | `~/Library/Audio/Plug-Ins/CLAP/` | `~/Library/Audio/Plug-Ins/VST3/` | `~/Library/Audio/Plug-Ins/Components/` |

Use CLAP if your DAW has it (Bitwig, Reaper, FL Studio and others): it is Tessera's own format, and VST3 and the Audio
Unit are wrappers around it. Logic and GarageBand use the Audio Unit. The standalone app (Windows and macOS) needs no
DAW: start it, pick an audio output, and it plays at once.

**macOS:** the beta isn't signed with an Apple Developer ID yet, so macOS calls it "damaged" or says it "can't be
verified". After copying, clear the download's quarantine once, in Terminal:

```
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/CLAP/Tessera.clap ~/Library/Audio/Plug-Ins/VST3/Tessera.vst3 ~/Library/Audio/Plug-Ins/Components/Tessera.component
```

(and the same for `Tessera.app` where you put it). **Windows:** SmartScreen may warn about the unsigned `Tessera.exe`
the first time: click *More info*, then *Run anyway*.

## First sound

A new Tessera opens with **STILL TIDE** loaded (and KOTO GARDEN in slot B). Press PLAY in the DAW and it plays. Play
notes from a MIDI track or a keyboard routed to Tessera: they go to the selected track, as the FM-1's keys do.

## The window

The window shows the FM-1, 1:1. Click the buttons and keys (drag across the keys to glide). Drag a knob up and down
or left and right, or turn the mouse wheel over it.

**The computer keyboard** plays it too, as in the web simulator (click the window first):

- **Z** to **'** and **Q** to **O**: the keys.
- **1** to **=**: the twelve buttons, in the panel's order.
- **[** and **]**: OCT- and OCT+.
- **Up / Down**: turn SELECT. **Left / Right**: turn ALGORITHM.

Keys go by where they sit, not what they print, so any keyboard layout works. With Ctrl or Alt held, keys go to the
DAW. Keys reach Tessera only while its window has focus; click back in the DAW and its own shortcuts work as usual.
To leave every key to the DAW, set **KEYS** to OFF in Tessera's menu (it applies to every Tessera window).

**Button combos with the mouse:** right-click a button or key (Ctrl-click on a Mac) to latch it: it stays held, drawn
as held, while you click the second one, so GLO + a button or SEL + a key work with one mouse. Right-click it again,
click it, or press **Esc** to let go.

**Shells:** the window opens in Tessera's own look. Click the Tessera mark at the top left for its menu:

- **SHELL:** Tessera's look, or one of the eight FM-1 colours. Each shell sets its own screen ink.
- **INK:** the screen's ink, the same 13 choices as MENU > COLOR (nine inks, then MOSAIC, SUNSET, OCEAN and NEON,
  which colour each track). Hover over a swatch to see its name.
- **MODE** and **SIZE:** FM-1 at 100% for now; EXPANDED and the larger sizes are coming.
- **FM-1 USB:** OFF or ON. With it on, an FM-1 on USB plays the plugin (see
  [Playing it from your FM-1](#playing-it-from-your-fm-1)); the line under it says what the FM-1 is doing.
- **ABOUT:** the plugin's version.

Esc, or a click outside, closes the menu. In an FM-1 shell, the swatches under the keys pick the shell (Tessera's is
the first) and the row labelled INK, between the buttons and the keys, picks the ink.

Picking a shell sets that shell's ink; an ink you pick afterwards, there or in MENU > COLOR, stays until you pick a
shell again. The **MIDI** lamp beside the mark lights when notes come in from the DAW.

Tessera remembers the last shell and the last ink for every new window, in any DAW. They are your settings, not the
project's: a project opens in your shell and ink.

The window moves only while the DAW is processing audio. If it looks frozen, check that the DAW's audio engine is on.

## Playing it from your FM-1

An FM-1 running Tessera 3.5.1 or later can play the plugin from its own knobs, buttons and keys, as it can play the
[web simulator](https://rastapopolix.github.io/tessera/webapp/sim/). Connect it by USB and turn on **FM-1 USB** (Tessera's
menu, or the **FM-1 USB** button under the keys in an FM-1 shell). When the FM-1 answers, the menu says
**FM-1 CONNECTED**.

- **Turn on SIM CONTROL** on the FM-1 (its menu) so it plays only the plugin. Without it, the FM-1 plays its own
  sounds as well. FX and GLO pressed together turn SIM CONTROL off again.
- **Windows:** only one program can use the FM-1's MIDI port at a time. Turn the FM-1 off in the DAW's MIDI
  settings, and close the editor if it's open in a browser. Otherwise the menu says **PORT IN USE**.
- **macOS:** the DAW can keep the FM-1 on, but then its keys also reach whatever track the DAW sends them to. Turning
  it off in the DAW is simplest.
- **Several Tesseras:** the FM-1 plays one of them at a time. Turning FM-1 USB on in a Tessera's window takes the FM-1
  for that one; the others say **IN ANOTHER TESSERA**. Turning it off turns it off for all of them.
- **Other messages:** **NO FM-1 FOUND** means no FM-1 is plugged in, or its port has another name. **UPDATE THE FM-1**
  means it doesn't answer: it runs Tessera 3.5 or older, or other firmware.

Tessera remembers FM-1 USB for every new Tessera, like the shell and the ink. The FM-1 has no 1-4 / 5-8 switch: use
the window's to reach tracks 5 to 8.

## Playing with the DAW

Tessera follows the DAW's transport:

- **PLAY** starts every track where it would be if the song had played from the DAW's zero. Starting at bar 9 sounds
  like the song has been playing for eight bars.
- **The DAW's tempo** drives Tessera's clock, tempo changes included. The BPM on the screen is the DAW's, rounded. A
  TEMPO mod has no effect while the DAW plays.
- **A loop or a jump** in the DAW moves Tessera to that point. **STOP** stops it.
- With **CLK** set to MIDI, an outside MIDI clock leads instead, and the DAW only starts and stops it.

## Saving, and project files

**The DAW saves Tessera with your song.** The saved state is the Tessera project, the same bytes SAVE > PROJECT
writes on the FM-1, plus the MASTER volume. Open the song again and every setting comes back as it was, each Tessera
with its own project.

**OPEN** and **SAVE**, under the keys, work with files:

- **OPEN** loads a `.tsp` project (from the editor, from the `demos/` folder, or saved by another Tessera) or an
  **FM-1 backup** (the `.json` from the backup page or the Hub). A backup's slots A to D go into the plugin's slots,
  then the music it was playing loads. On Windows and macOS you can also drop a file on the window.
- **SAVE** writes the project as a `.tsp` under its name, ready for the FM-1's editor or another Tessera.

A `.tsp` from the plugin opens on the FM-1 too. Tracks on the plugin-only sounds play a stand-in there (see
[Sounds only the plugin has](#sounds-only-the-plugin-has)).

On Linux the file dialogs need `zenity` or `kdialog`.

## Tracks 1 to 4, and 5 to 8

The plugin has eight tracks; the panel works on four at a time, as the FM-1's does. The **1-4 / 5-8** switch beside
SAVE picks the bank: 1-4 selects T1, 5-8 selects T5 (**Tab** on the computer keyboard swaps them). The lit half is the
bank of the track selected now, so it follows SELECT too. While a track of 5 to 8 is selected the screen's top line
shows **5-8**, inverted, before the track dots.

## Automation: the parameters

Almost every project setting is a DAW parameter you can automate, about 1,700 in all:

- **MASTER** and PERF's **MACRO 1** to **MACRO 4** come first.
- **Each track's** settings, named like `T1 LEVEL` or `T3 TURING LENGTH`, grouped under *Track 1/Sound and sequence*
  and *Track 1/Generator*, and so on.
- **The song's** settings, under *Song*.

Values read as the FM-1 shows them ("SAW", "1/16", "-6dB"), and you can type them that way. ENGINE 1 to 8 mean what
the track's engine makes of them, as on the FM-1, so their names change when the engine does.

When a setting changes on the panel, by loading a project or by Tessera itself (a scale shared by every track, a
generator moving), the DAW hears about it: MASTER and the MACROs at once, the rest within a fraction of a second.

Not parameters: the device's own settings (A4, the MIDI routing), and the MOD cords. Make those on the panel; the
project saves them.

## Track outputs and MIDI out

- **Track outputs:** besides the main stereo mix, Tessera has eight more outputs, TRACK 1 to TRACK 8, each track on
  its own, as the FM-1's USB stems are: after the track's level, before pan and the bus effects. Route them to their
  own mixer channels in the DAW to process each track apart.
- **MIDI out:** every track's notes, each on its own channel (GLO > MIDI OUT; by default channels 1 to 8), so
  Tessera's generators can play other instruments. NOTES OUT starts at ALL in the plugin (the generators' notes too).
  MENU can set it to KEYS or OFF.

## Several Tesseras in one song

You can use up to **8 Tesseras** in one DAW song. Each is wholly its own: its project, its transport, its window and
its parameters. A ninth won't open. Each new Tessera starts from STILL TIDE.

## Sounds only the plugin has

The plugin has room for engines the FM-1 can't fit:

- **SID**: a whole MOS 6581 or 8580 chip per note, with 32 presets.
- **DRUM: SID KIT** and **SID DIGI**: drums from one SID chip.
- **MACRO**: a macro-oscillator with many synthesis models.
- **PHASE**: phase distortion in the manner of Casio's CZ-1: two lines of bent-phase waves (saw, square, pulse,
  double sine, saw-pulse and three resonant ones), mixed or ring-modulated, with 33 presets. EDIT 2's EG at SPLIT gives
  DCW 1, DCW 2, DCA 2 and pitch their own envelopes (5 more pages).

The Daisy build (Andamento) has them too. On the FM-1, a SID, MACRO or PHASE track plays FM6's INIT sound instead, and
a SID kit plays the 909, so a project still opens and plays there.

## Echoes on the beat

FX's **SYNC ECHO** repeats on a note value (1/32 to 1/1, triplets and dotted) at the DAW's tempo. The plugin's echo
holds about 3 s, so a 1/2 or a 1/1 rings as set; on the FM-1 it holds 371 ms and a longer value halves until it fits.

## How much CPU it takes

On average, one Tessera playing a project takes about 2 % of one core of a 2.1 GHz desktop processor, at 48 kHz.
Eight of them on one core take about 20 %, and less when the DAW spreads them over several cores. Very small audio
buffers (64 samples) leave less room for the occasional slower block. If you hear clicks with several Tesseras, raise
the buffer to 256.

## Not there yet

- **Tracks 5 to 8's steps** aren't saved yet. Their settings are saved with the DAW song, but a `.tsp` holds tracks 1
  to 4 only.
- **PHASE** has 33 presets of its own (brass, pads, basses, keys, bells, sweeps, ring sounds), not the CZ-1's original tones.
- **One window size**, the FM-1's. A larger, wider window is planned.
- **No presets in the DAW's browser** yet: use OPEN, or the DAW's own way of saving a plugin's state.
- **No standalone app on Linux** yet.
- **Not signed** on macOS or Windows yet (see [Install](#install)).
