**Tessera 3.0** follows [Tessera 2.1.1](https://github.com/Rastapopolix/tessera/releases/tag/v2.1.1). The whole screen is
new: every page is drawn in one ink on black, in Tessera's own 5 x 7 font, and you pick the ink. A voice is back
(**ORACLE**, a classic polyphonic analogue voice), and the generators and STEP tracks have grown a lot: STEP patterns as
seeds for the generators, GRAVITY, conditional steps, a pitch accumulator, FORM sections that develop, a FILTER effect
and effects as cord destinations. Projects, presets and settings from 2.x carry over.

### 🖥️ A new look
* **One ink on black, one font.** Every screen, from the boot mark to the help card, is drawn in a single ink with a
  5 x 7 pixel font: the selected item is inverted, quieter things are dimmer. Each page has its own picture of what it
  does (the generator, the envelope, the filter curve, the arp, the cords).
* **MENU > COLOR** picks the ink: **YELLOW** (the default), WHITE, GREEN, BLUE, RED, and ORANGE, PINK, HOT PINK and
  LILAC to match the FM-1's shells.
* **Where you are among the pages.** When a section has more pages than the footer's tabs show (a generator track's
  SEQ pages), the header shows the page's place, e.g. **4/10**, and arrows at the footer's ends say more pages lie
  that way.
* The boot mark dissolves into grains and sweeps away.

### 🎹 ORACLE
* **ORACLE** is a two-oscillator polyphonic analogue voice with a resonant four-pole low-pass filter: oscillator A
  (saw, pulse) and B (saw, triangle, pulse) and noise, **POLY MOD** (the filter envelope and oscillator B bend A's
  pitch, pulse width or the cutoff, note by note), sync, two envelopes, an LFO, and EARLY / LATE filter models. Warm
  pads, brass, sync leads, basses, plucks: **24 sounds of its own**, and each track's sound is kept in the project.
  Its pages are on **EDIT**. ORACLE costs about one and a half FM6 voices: one busy ORACLE track is fine; with two,
  keep BUDGET in mind.

### 🌱 Generators
* **STEP as seed.** TURING, EUCLID, MARKOV and MORPH can play from a STEP pattern you wrote (**SOURCE**, OCT+ held on
  GEN): Turing drifts it, Euclid gives it a new rhythm, Markov learns what follows what, Morph plays it against a
  variation. **KEEP** holds its rhythm or its pitches, **GRAVITY** pulls it back to as written.
* **GRAVITY on EVOLVE** (OCT+ held on EVOLVE): a phrase drifts, then comes home to the generator as you set it.
* **FORM DEVELOP** (OCT+ held on FORM): a section that comes back varies a little each time, but the section itself is
  kept, so the form never drifts away, even over hours. 0 (the default) returns it exactly, as before.
* **STEP EVOLVE HOW**: besides DRIFT, **ADORN** (ornaments that only add: ratchets, octave jumps, passing notes) and
  **SERIAL** (the whole phrase backwards, upside down, rotated or at half speed for a round).
* **CHORD HIT OFFBEAT**: short stabs on the off-beats.
* **ATLAS** starts somewhere new: a random X / Y and DENSITY 30–50 % the first time it is used.

### 🥁 STEP tracks
* **Conditions.** A step's sheet (OCT+ on a note or a hit) has a **Condition**: which passes it plays in (1:2, 3:4,
  1:8, FIRST, !FIRST …), or **FILL** / **!FILL**, played while **GLO + E4** is held (on MIXER, E4 alone).
* **Accumulate**: a step's notes climb or fall a few scale degrees each time the pattern comes round, within the
  track's limit, so a short loop moves on its own.

### 🎛️ Sound and modulation
* **FILTER effect**: a resonant low-pass as an insert or a send (CUT, RES), on any track. FM6 has none of its own,
  so this is FM6's filter.
* **Effects as cord destinations**: the insert and send effects' knobs (ECHO TIME, the FILTER's cutoff …) can be moved
  by cords.
* **ENERGY part 2**: **LEVEL** sets the energy by hand and the tide swings round it, through both halves (busier and
  louder, sparser and softer); **DEPTH** (OCT+ held) says how much each track follows, 0 an anchor that plays as set.
* **FM6 AEG** (EDIT 2, KNOB 4): one knob for the amp envelope, all the carriers' envelopes slower or faster at once.
* The FM6 ENV pages stay ENV's when you turn SELECT; ANSWER's FADE moves a count a click.
* **PRESETS** has a **LIST** knob: ALL, the favourites, RECENT or a category.
* Power-on track 2 is TRIO's STRING MACH (in place of FM6 STRINGS), so generators on both pads stay within what the
  FM-1 can play.
* The metronome follows MASTER.

### 🎵 Demos
Three demo projects in [demos/](demos/README.md): **STILL TIDE** and **KOTO GARDEN** remade with the new options, and
**LOW HARBOUR**, a slow ambient dub. `tools/demo_backup.py` puts them into slots B, C and D of your own backup. And a
new demo video on the [project page](https://rastapopolix.github.io/tessera/).

### 🐛 Fixes
* **SWING turned while playing stays in time.** Turning a track's SWING (or the global one) during a late step used to
  leave the track off the beat until the next PLAY, even after turning it back to 0. It now goes straight back onto the
  grid, as it already did for DIV and tempo.
* The boot screen no longer flashes yellow before your COLOR: it starts black and the logo comes in your ink.
* NOTES shows Accumulate: a small caret over a note, pointing up when it climbs, down when it falls.

### 🔧 Under the hood
* **More audio memory**: the stacks were cut to what they really use (about 14 KB more for the audio), and the boot
  images are compressed (7 KB less flash).
* **Steadier under load**: the voices are shed sooner when the FM-1 runs out of time, so it stays clean instead of
  crackling.
* **Safer updates**: an interrupted update resumes only in the package's own loader.
* Flash room for the new features: help texts and readouts packed smaller, and DRUM's SID KIT left out (below).
* Each release package is attested by GitHub, so you can check where it was built.

### 📦 Installation
Open **[rastapopolix.github.io/tessera](https://rastapopolix.github.io/tessera/)** in Chrome or Edge, connect the
FM-1 by USB and press **Install**, then keep the tab open and the FM-1 connected until the install window says Done.
The installer can also put the official M-VAVE firmware back. Or take `tessera-3.0.fwsc` (dual core) or
`tessera-3.0-1core.fwsc` from this release.

### ⚠️ Good to know
* **Projects from 2.x load.** The new settings start off, so an older project plays as it did. A track saved with
  DRUM's **SID KIT** (or SID DIGI) loads as the **909**: they are out of the device build to make room.
* 3.0 has only the new look. The old detailed look stays in the source on branch `detailed-look`.
* A project saved with 3.0 may not play the same in 2.x, which does not know the new settings.
