**Tessera 3.1** follows [Tessera 3.0.1](https://github.com/Rastapopolix/tessera/releases/tag/v3.0.1): the FM-1 can now
be the host of a USB-MIDI keyboard, controller, synth or MIDI-to-CV module, and external gear can set Tessera's
parameters by MIDI CC. Projects, presets and settings from 2.x, 3.0 and 3.0.1 carry over as they are.

**Both new features are untested on the device as yet.** They pass the host tests, but nobody has tried them on an
FM-1 with Tessera. If you try them, please say how it went in an
[issue](https://github.com/Rastapopolix/tessera/issues).

### 🔌 USB host (untested on the device)
* **MENU > USB HOST**: with it **ON**, the USB-C port hosts one class-compliant USB-MIDI device, plugged in directly or
  behind **one hub**, and the screen says **USB MIDI READY**.
* **In**: its notes, CCs, bend and clock arrive as USB MIDI from a computer would: MIDI ROUTING picks the tracks, CLK
  on USB follows its clock, and the new CC map works from its knobs.
* **Out**: MIDI OUT and CLOCK OUT go to the device, so a USB synth or a USB-MIDI-to-CV module plays Tessera's tracks
  with no computer in between.
* **Power**: the FM-1 does not power its USB port, so a bus-powered device needs a **powered hub** (or its own power
  supply).
* **Never saved**: every start is a normal USB device, as before, so the computer link, the web editor and updates
  work as they did. While USB HOST is ON there is no computer link; turn it OFF or restart to get it back.
* Full-speed devices only (**LOW SPEED** otherwise); a device without USB-MIDI shows **NO MIDI DEVICE**. Unplugging
  shows **DEVICE LOST** and ends the notes it held.
* USB host mode is Melodee's, by Kerem Kilic and recursivefieldinstruments.

### 🎛️ MIDI CC control (untested on the device)
* **A fixed CC map**, with General MIDI's meanings where it has one: CC 16–19 the engine's HOME knobs, 20–27 E1–E8, 76
  LFO RATE, 77 / 78 LFO to pitch / filter, 79 SUSTAIN, 91 / 93 / 94 FX SEND 1–3, 102 MODE, 103–105 the GEN page's
  KNOB 2–4. The CCs that already worked are unchanged. The
  [MANUAL](https://github.com/Rastapopolix/tessera/blob/HEAD/MANUAL.md#midi-cc-control) has the table.
* **NRPN by parameter ID** reaches every track, generator and project parameter
  ([PARAMETERS.md](https://github.com/Rastapopolix/tessera/blob/HEAD/docs/PARAMETERS.md) lists the IDs), in 7 or 14
  bits.
* **MENU > MIDI CC IN** ON (the default) / OFF, for a sequencer that sends CCs to other gear on the same channels.
* The knobs are endless, so a CC sets the value and the knob carries on from there. MOTION records CC moves like knob
  turns.

### 🔧 Under the hood
* The host tests and the web simulator build the Mono look, as the device does.

### 📦 Installation
Open **[rastapopolix.github.io/tessera](https://rastapopolix.github.io/tessera/)** in Chrome or Edge, connect the
FM-1 by USB and press **Install**, then keep the tab open and the FM-1 connected until the install window says Done.
The installer can also put the official M-VAVE firmware back. Or take `tessera-3.1.fwsc` (dual core) or
`tessera-3.1-1core.fwsc` from this release.

### ⚠️ Good to know
* **Projects from 2.x, 3.0 and 3.0.1 load** and play as before. MIDI CC IN starts ON, so CCs that your gear already
  sends on Tessera's channels now move the mapped parameters: turn MIDI CC IN off if they should not.
