**Tessera 3.2.1** follows [Tessera 3.2](https://github.com/Rastapopolix/tessera/releases/tag/v3.2): your own tunings
from Scala files, a transpose for each track's MIDI out, the battery always in the header, and a USB host that finds
more hubs. Projects, presets and settings from 2.x, 3.0, 3.0.1, 3.1 and 3.2 carry over as they are.

### 🎼 Your own tunings (Scala files)
* Up to **four tunings of your own, USR1 to USR4**, made from [Scala](https://www.huygens-fokker.org/scala/) `.scl`
  files, the format of the Scala archive's thousands of scales and of most microtonal tools.
* Load them in the web editor: **Projects > Tunings (Scala)**, **Load .scl** on a slot. Pick one on the device in
  **SCL > SCALES**, the new **USER** family (or **Play** in the editor).
* Up to **64 notes** per period. The file's last pitch is the period, so non-octave tunings work too (up to four
  octaves). Notes below the root or past the period are folded in.
* The keys, MIDI input, the generators, QNT, chords and recordings all play its notes; ROOT and TRN move it. Picking a
  built-in scale goes back to it.
* The tunings are **saved with the project**, so a project plays the same on any FM-1.

### 🎚️ MIDI OUT TRANSPOSE
* Each track's notes can be moved up or down **on their way out over MIDI**, -48 to +48 semitones, for an instrument
  or a MIDI-to-CV module that plays them too high or too low. The track's own sound does not change.
* Hold **OCT+** on **GLO > MIDI OUT** and turn **KNOB 1** to **4**. Saved with the project.

### 🔋 Battery
* The Mono header **always shows the battery**: its bars on the battery, a bolt on external power.
* **MENU > BATTERY**: ON (the default) or WHEN LOW. **ABOUT > POWER** shows the battery's reading.

### 🔌 USB host
* Hubs with **a second hub inside** (common in USB-C hubs) are followed one level down.
* A device that does not answer at first is **tried again**, up to four times.
* **ABOUT > USB HOST SCAN** lists what each hub port had and why a scan failed; the header points to it.
* The header's USB messages fit the screen.

### 📦 Installation
Open **[rastapopolix.github.io/tessera](https://rastapopolix.github.io/tessera/)** in Chrome or Edge, connect the
FM-1 by USB and press **Install**, then keep the tab open and the FM-1 connected until the install window says Done.
The installer can also put the official M-VAVE firmware back. Or take `tessera-3.2.1.fwsc` (dual core) or
`tessera-3.2.1-1core.fwsc` from this release.

### ⚠️ Good to know
* **Projects from 2.x, 3.0, 3.0.1, 3.1 and 3.2 load** and play as before. A project saved with tunings or a MIDI OUT
  TRANSPOSE in 3.2.1 opens in an older version without them.
* `.kbm` keyboard maps are not read: ROOT places the tuning's first note. MIDI OUT sends no tuning messages.
* User tunings, MIDI OUT TRANSPOSE and the USB host changes are new and not yet tried on many devices: if something
  is off, please say so in an [issue](https://github.com/Rastapopolix/tessera/issues).
