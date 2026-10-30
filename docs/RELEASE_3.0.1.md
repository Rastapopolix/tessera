**Tessera 3.0.1** follows [Tessera 3.0](https://github.com/Rastapopolix/tessera/releases/tag/v3.0): a new colour, the tempo
under SELECT, and fixes to the new look and to TURING played from a STEP pattern. Projects, presets and settings from
2.x and 3.0 carry over as they are.

### 🖥️ The look
* **MOSAIC**, a tenth MENU > COLOR: each track has a colour of its own (T1 yellow, T2 blue, T3 hot pink, T4 green), the
  screen takes the selected track's colour, and the value you turn and the picked row show in white. Wherever the four
  tracks show together (HOME, MIXER, PATTERNS, SONG, QUAD, MATRIX, MIDI OUT, the cords, the GLO layer, the header's
  track dots) each is drawn in its own colour. The nine single inks look as before.
* **The page's place on every page.** The header's place (e.g. **4/10**) now runs through the button's whole cycle: SEQ
  counts its GEN pages and the sequencer's pages together (it used to restart halfway, at 3/5), and ENV, LFO and SEQ
  number every page they visit, on every engine. FM6's and ORACLE's own ENV and LFO pages no longer count EDIT's pages
  (FM LFO showed 13/15), and MODS, the list pages and SONG now show the place too. A section whose tabs all fit the
  footer shows no number.
* **Knob values look the same at any length.** A dial value is plain text and a list value is boxed, however many
  letters it has; neighbouring values keep a clear gap, so `TURNG` and `16/16`, or `CHORD` and `DRONE`, no longer run
  into one block.

### 🎹 Playing
* **SELECT on HOME sets the tempo**, and so does SELECT with **GLO** held on any page, with SEQ > TEMPO's range and the
  BPM overlay. With CLK on USB or TRS it says the tempo is the clock's. (Issue #296.)
* **NOTES, KNOB 2 moves by scale notes while QNT is on**: a note off the scale goes to the next scale note in the
  direction of the turn, and with **QNT CHRD** and a CHORD track playing it steps through the chord's notes. Hold **ENV**
  for a semitone, for passing notes. With QNT OFF it moves a semitone, as before.

### 🐛 Fixes
* **TURING from a STEP pattern** (SOURCE): with PROB turned up, the changed steps piled up on one or two notes, often an
  octave above the root, because a changed step copied another note of the loop and a rest that became a hit took the
  note before it. A changed step now gets a new pitch: half the time another of your notes as written, otherwise a
  note of the track's scale within **SPAN** octaves of your lowest note. KEEP PITCH, PROB 0 and GRAVITY work as before.
  MORPH's B, which drifts the same way, spreads the same way.
* **The filter picture follows TRIO's MODE**: TONE drew a low-pass whatever the MODE; it now draws the band-pass, the
  high-pass and the notch too, on the page and on Stage's overlay. (Issue #299.)
* **The LFO picture's dot crosses the whole wave**: it stopped in the middle. With POL UNI it now sits on the curve, and
  S&H draws the value it holds.

### 🔧 Under the hood
* `./build.sh` builds the Mono look by default, as the releases do. `MELODEE_LOOK=0` builds the detailed look, which
  no longer fits the device's flash.

### 📦 Installation
Open **[rastapopolix.github.io/tessera](https://rastapopolix.github.io/tessera/)** in Chrome or Edge, connect the
FM-1 by USB and press **Install**, then keep the tab open and the FM-1 connected until the install window says Done.
The installer can also put the official M-VAVE firmware back. Or take `tessera-3.0.1.fwsc` (dual core) or
`tessera-3.0.1-1core.fwsc` from this release.

### ⚠️ Good to know
* **Projects from 2.x and 3.0 load** and play as before, except that a TURING or MORPH playing from a STEP pattern with
  PROB above 0 now draws its changed notes as above.
* With COLOR on MOSAIC, going back to 3.0 shows the default YELLOW.
