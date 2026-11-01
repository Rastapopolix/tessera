**Tessera 3.2** follows [Tessera 3.1](https://github.com/Rastapopolix/tessera/releases/tag/v3.1): PHYS, the
physical-modelling engine, is back and friendlier. Recording live into STEP now layers pass after pass, tracks you
start while others play join in time, and MODS says more clearly what each cord does. Projects, presets and settings
from 2.x, 3.0, 3.0.1 and 3.1 carry over as they are.

### 🥁 PHYS is back
* **Physical models** of struck, plucked and bowed things: **MODAL** (bars, bells, plates), **STRNG** (a plucked or
  bowed string), **MEMB** (drum heads) and **SYMP** (a string with three more ringing along).
* **17 sounds**: BELL TREE, MARIMBA, PLUCK, BOWED METAL, KALIMBA, HAND DRUM, TOMS, DRONE STRING, HARP, VIBES, GONG,
  SINGING BOWL, DULCIMER, SITAR, BOWED STRING, TIMPANI and SYMPATHY.
* **Friendlier knobs**: each covers only its musical range, with wide sweet spots, and a dull, short or bowed sound is
  as loud as a bright one. Its own picture names where STRC is (in tune, a chime, a marimba).
* No click when a note is struck again while it still rings.
* An old project's PHYS track (one not saved again since 2.0, which turned it into FM6) plays PHYS again.

### 🎹 Recording live into STEP
* **REC records pass after pass**, each on top of the last, until you tap REC again.
* **EDIT + REC** on NOTES: **CLEAR PATTERN?**, OCT+ clears it and REC stays on for a new take.
* **OCT- / OCT+** on NOTES while recording move the keys an octave (both: back to the middle).
* **NOTES comes right after GEN** on STEP and ARP tracks; GEN holds MODE, LEN, DIV and GATE there, and SWING is on
  the pattern's sheet.
* **ANSWER SOURCE** (OCT+ held on ANSWER, KNOB 1): T1 to T4 makes a STEP pattern the call, so a sequence you record
  gets answered, and answered anew when you change it.
* The header's messages fit the screen (no more WAITING FOR LOO).

### ⏱️ Tracks join in time
* A generator picked, or a track unmuted, while the others play starts where it would be had it played since PLAY:
  on the same steps and bars. An ATLAS groove picked under a playing chord lands on the beat.

### 🎛️ Sounds from any page, clearer MODS
* **Hold GLO and turn PRESETS** on any page to browse the selected track's sounds, with the sound's name large. Let go
  of GLO and you are back where you were. Without GLO, PRESETS does what it did before.
* **MODS**: a mod's direction reads **+-** (both ways) or **+** (one way) instead of BI / UNI, and a +- mod's bar
  reaches as far below the middle as above. It is the mod's own setting, not the LFO's POL.
* The source and destination lists name each entry with its track (**T2 GATE**); on **MOSAIC** each takes its track's
  colour.
* An old project's LFO route on a track with POL UNI loads as a + mod, so it moves as it did.

### 🔵 Generators
* **BOUNCE**: the balls touch the walls on the step they hit.

### 📦 Installation
Open **[rastapopolix.github.io/tessera](https://rastapopolix.github.io/tessera/)** in Chrome or Edge, connect the
FM-1 by USB and press **Install**, then keep the tab open and the FM-1 connected until the install window says Done.
The installer can also put the official M-VAVE firmware back. Or take `tessera-3.2.fwsc` (dual core) or
`tessera-3.2-1core.fwsc` from this release.

### ⚠️ Good to know
* **Projects from 2.x, 3.0, 3.0.1 and 3.1 load** and play as before. A track that had a SOURCE under TURING, EUCLID,
  MARKOV or MORPH and was later switched to ANSWER now answers that pattern.
* 3.1's **USB host** and **MIDI CC control** are still untested on the device: if you try them, please say how it went
  in an [issue](https://github.com/Rastapopolix/tessera/issues).
