**Tessera 3.3** follows [Tessera 3.2.2](https://github.com/Rastapopolix/tessera/releases/tag/v3.2.2): long form.
A project can now arrange itself into a whole piece, with sections that change together, parts that drop out and
come back, and a shape from the first note to the last, over minutes or hours. Around it: macros, mods of mods, CC
out, MIDI LEARN that stays, TIMING SPLIT, clearer SEQ and ARP buttons, new palettes and a landscape to watch.
Projects, presets and settings from 2.x and 3.x carry over.

### 🧭 Long form
* **CHAIN** (GLO > CHAIN): one FORM for the whole project. On each phrase line (4, 8 or 16 bars) every track that
  follows moves to its A, B or C together. **WANDER** chooses the next section instead of reading it, always coming
  home to A; **REPEAT** holds a place for 1 to 8 phrases. Each track can **FOLLOW** or keep its **OWN** FORM, and
  **SKIP** lets it lag a phrase now and then.
* **KEY B / KEY C** (SCL's last rows): a key for each section, so B can go a fourth up and C a tone down.
* **ENTRIES** (GLO > ENTRIES): tracks drop out and come back on the phrase lines, in a musical order (the melody
  leaves first, the bass comes in first), fading or cut. Each track has a **ROLE**; an **ANCHOR** never leaves.
* **PIECE** (GLO > PIECE): a length (1 minute to 4 hours) and a shape (ARCH, RISE, FALL, PLATEAU, VALLEY, WAVES,
  BUILDS, STEPS, FLAT or your own). It steers ENERGY, ENTRIES and CHAIN along the shape, and at the end stops, loops
  or starts a new piece.
* **FORM's DEVELOP**: a section varies a little each time it returns, without drifting away.
* The manual has a new page on **how these layers work together**, with a diagram, and one on **PATTERNS and SONG**.
* **Five new demos** made with them: SLOW LIGHT, CEDAR RAIN, UNDERTOW, EMBER FIELD and PAPER KITES (web editor > Demo
  projects).

### 🎛️ Mods and performance
* **PERF** (GLO > PERF, after MIXER): four macros, **MCR1 to MCR4**, as mod sources kept in the project.
* **Mods of mods**: a mod's amount (**M1 to M16 AMT**) is a destination, so one mod can swell another.
* **GATE** mods (open only while a source is over a threshold), **FILL** and **TEMPO** as destinations. TEMPO speeds
  the whole project up and down without skipping a step (thanks to BobFromson for the idea).
* **FOLLOW HOLD**: a track takes the followed note only when a third track plays, a sample and hold.
* **SOUND OFF** and **TRANSPOSE** per track on MIDI OUT: a track can play only an outside synth, octaves away.
* **MUTES** on the menu: NOW, BAR or PHRASE, so a mute lands on the line.

### 🎹 MIDI and USB
* **CC out from MODS**: a mod can send a CC on its track's channel, only when its value changes.
* **MIDI LEARN** maps a controller knob to any knob, and the maps now stay after power-off. FORGET asks first.
* **Clock**: an exact internal clock, synced LFOs that stay on the beat, and Song Position from a DAW.
* **USB VOLUME** (menu): the level of audio from the computer, apart from the volume knob.
* **USB host** finds more devices, including ones with audio and MIDI together.

### ⏱️ Sequencing
* **SEQ is what plays, ARP is how it's shaped.** SEQ holds GEN, NOTES, PATTERN, TEMPO, PATTERNS and SONG; the
  **ARP** button holds the pages that shape the notes: EVOLVE, FORM, TWEAK, TWEAK 2, LENGTH, TIMING and ENERGY on a
  generator, STEP EVOLVE, TWEAK and TIMING on STEP, ARP 2 and TWEAK on an arpeggio. A tap of ARP never changes the MODE.
* **The arpeggio** takes TWEAK's SHIFT and HUMAN, and its RANDOM order and PROB come from the track's SEED, so PLAY
  plays it the same again.
* **TIMING SPLIT** (TIMING, OCT+ held): a chance that a step plays at half its division, as two notes (DOUBLE), one
  note late (PUSH) or either (MIX). SPLIT is also a mod destination.
* **NOTES**: no large read-out over the steps; the pitch bars follow pitch even with one note raised high; turning
  **KNOB 2** sounds the note's new pitch.
* **NOTE PREVIEW** is now **on by default**, and sounds while playing too (not while recording).
* The manual compares RATCH, Ratchet, ADORN, SPLIT, HUMAN, Nudge and SWING in one table.

### 🖥️ Screens and sounds
* **SUNSET, OCEAN and NEON** palettes, and a **COLOR** sheet on the menu that shows each one. **CONSTELLATION** and
  **LANDSCAPE** draw in all four colours of these palettes and MOSAIC.
* **LANDSCAPE** (GLO, last page): rain on water, drawn by the music. It changes nothing; it's just nice to watch.
* **KNOBS**: dials or bars. **LARGE VALUE**: the knob you turn shown large across the foot of the screen, on HOME and
  pages without a picture of their own (**AUTO**, the default), so a picture (a generator's, an envelope, the LFO)
  stays in view while you turn; or on every page (**ON**), or never (**OFF**).
* The battery shows on every page, and **BATTERY LOW** shows once, then the next STOP saves at once.
* **ORACLE** sounds save to USER.
* **ECHO and ROOM** no longer add a high whine. **TRIO** no longer clicks when a note is struck again while it sounds.

### 🛠️ Under the hood
* **New identity, FM-1_833**: M-VAVE's own updater (M-UPGRADE 3.11 and later) recognises Tessera again, and can
  take the FM-1 back to the official firmware. The web installer's way back works as before.
* A fault on the second core no longer stops the FM-1: it carries on with one core until power-off.
* A step longer than a bar no longer sends the CHAIN back to A.
* The web editor's **Health** shows live CPU and voice counts; Demo projects holds the CHK test projects.
* The recovery tool asks before writing to a UC25HQ80 flash chip.

### 📦 Installation
Open **[rastapopolix.github.io/tessera](https://rastapopolix.github.io/tessera/)** in Chrome or Edge, connect the
FM-1 by USB and press **Install**, then keep the tab open and the FM-1 connected until the install window says Done.
The installer can also put the official M-VAVE firmware back. Or take `tessera-3.3.fwsc` (dual core) or
`tessera-3.3-1core.fwsc` from this release.

### ⚠️ Good to know
* **Projects from 2.x to 3.2.2 load** and play as before. The new parts start off: CHAIN OFF, ENTRIES 0, PIECE FREE,
  SPLIT 0, every track SOUND ON, macros at 0.
* **Going back to 3.2.x**: a project saved on 3.3 loses what 3.2.x doesn't know (CHAIN, ENTRIES, PIECE, KEY B and C,
  SPLIT, PERF, FOLLOW HOLD, SOUND OFF, CC numbers). **A project with a mod on a destination new in 3.3** (M1 to M16
  AMT, FILL, TEMPO, CC, SPLIT) or a GATE mod **can't be read by 3.2.x at all.** Older firmware also clears ORACLE user presets at
  power-on and ignores MIDI LEARN maps.
* **NOTE PREVIEW** comes up ON once after the update, even if you had turned it off: turn it off again in the menu.
* **Not yet tried on many devices**: PERF macros, MIDI LEARN, the clock and Song Position, USB VOLUME, USB host with
  composite devices, MUTES, KNOBS, LARGE VALUE, the palettes and COLOR, BATTERY LOW, LANDSCAPE, ORACLE to USER,
  M-UPGRADE 3.11, the second-core fault fix, TIMING SPLIT and the NOTES changes. If you try them, please say how it went
  in an [issue](https://github.com/Rastapopolix/tessera/issues).

### 🙏 Thanks
To the beta testers: pavig-coder, alxantdroid and BobFromson (for the TEMPO idea, and for asking why SEQ and ARP
overlapped), and @nrittamurti on YouTube for the ideas behind KNOBS, the inverted value and the large read-out.
