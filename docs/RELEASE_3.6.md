**Tessera 3.6** follows [Tessera 3.5.3](https://github.com/Rastapopolix/tessera/releases/tag/v3.5.3): everything from the seven 3.6 betas. A harmonizer, a delay with a mind of its own on ANSWER, 22 more mod destinations in a grouped list, QNT per track, MIDI clock at other PPQNs, and the FM-1 as a controller for the web simulator. Around it: a fairer voice ceiling, faster FM6, and more room in the flash.

> ⚠️ **The project format has changed: a project saved on 3.6 does not load on 3.5.3 or older.** Keep a backup before you update if you may go back. Projects, presets and settings from 3.5.3 load on 3.6 and play as before (but for FM6 AEG turned right: see below).

### 🎼 Harmony
* **HARM, a harmonizer** (after Dreamsequence's): with a CHORD track playing, each note of the scale up plays the chord's next note up, so a run up the keys is an arpeggio that follows the chords. **SCL > QNT HARM** does this for the keys, MIDI IN and the generators; FOLLOW's new **HOW HARM** does it to another track's line, SHIFT moving by chord notes. A cord into SHIFT on a HOW HARM track picks chord notes: any modulation, in harmony ([RECIPES](https://github.com/Rastapopolix/tessera/blob/main/RECIPES.md)).
* **QNT LINK** (SCL, the row after QNT): **LINKED** keeps one QNT for every track, as before; **OWN** gives each track its own QNT and MPC degree, so one track can take MPC pads while others follow the chords (CHRD) or HARM (#755). The MANUAL now has a table of QNT's modes, MPC included.
* **Microtonal tunings with QNT OFF**: microtonal scales and your own Scala tunings now play with QNT OFF too. Each key and MIDI note is the next degree, so 24EDO and the maqam scales keep their quarter tones. A project saved with a microtonal scale and QNT OFF now plays in its tuning instead of ordinary semitones.

### 🔁 ANSWER
* **BEND**, a delay with a mind of its own (after Qu-Bit's Data Bender, the idea only): GEN sheet (OCT+ held), **Mode** REPLY (as before) or BEND. Every note comes back **TIME** steps later (1 to 16) while you are still playing, then again, drifting with VARY and SHIFT; **BREAK** is the chance a repeat glitches (slips, skips, stutters, jumps back). With VARY, SHIFT, ECHO and BREAK at 0 it is a plain tempo-synced delay. A STEP pattern as SOURCE is not heard in BEND yet (the keys, MIDI IN and a generator SOURCE are).
* ANSWER holds each note as long as you held its key in the call (with LENGTH on STEP, the default), and captures up to **16 notes** a call (was 8).
* **ANSWER CHORD** (GEN sheet): ARP spreads notes played together one step apart, as before; TRUE answers them as a real chord.

### 🔌 MODS
* **22 more destinations**: STRUM, STR DIR, RATCH, R CHANC, GATE, N LEN, LOCK, VELO, SWING, GLIDE, GRAVITY, DEVELOP, AN ECHO, AN SHFT, AN SPCE, BREATH, ACC LIM, TIDE SP, SWRM RT, ARP RTE, ARP GTE and ARP OCT.
* KNOB 3's destinations come **in groups** (SOUND, ENV, LFO, MIX, FX, GEN, FEEL, EVOLVE, ARP, MODS), and the list's title names the group under the cursor, for example T3 LFO. A track lists only the destinations that act on what it plays; a mod already set to one keeps it.
* KNOB 1's and KNOB 3's lists start at the **selected track**: on track 3, the first turn lands on T3 LFO, or T3 PITCH (#724).

### 🎛️ MIDI, the simulator and the installer
* **PPQN**, the MIDI clock's pulses per beat (GLO > MIDI IN, hold OCT-, KNOB 4): 24 (the default, the MIDI standard), 48, 4, 8 or 12, for the clock the FM-1 follows and for CLOCK OUT. If the FM-1 plays twice as fast as the gear it follows, try 48 (#747).
* **SIM CONTROL** (in the menu: hold HOME): with the [web simulator](https://rastapopolix.github.io/tessera/webapp/sim/) open and the FM-1 connected by USB, the FM-1's knobs, buttons and keys play only the simulator. Hold FX + GLO for a second to get the panel back; closing the simulator or unplugging ends it too. It is off every time the FM-1 starts.
* The installer waits a few seconds for the FM-1, and when another app or tab holds the FM-1's MIDI port it names the port instead of saying FM-1 not found (#743).
* The FM-1's update mode is now called **Tessera Update** on the computer (it was Melodee Update). A computer that has seen the old name may keep showing it.
* After a project loads, a track playing one of your own FM6 voices finds its user slot again: HOME shows its name and PRESETS starts from it (#744).

### ⚙️ Sound, CPU and space
* **A fairer voice ceiling**: after an overload (a crackle) the voices come back within a few seconds while you play, and one overload no longer holds every track to one or two voices. When a sound has to be cut early, the quietest fading drum (a hat's tail) or the quietest released voice goes first, and a drum hit that has not sounded yet is never taken by a later one (ATLAS kept losing its 909 snare).
* **FM6 is faster** with MARK I (upstream Melodee's exact fast paths), the sound bit-identical. **ECHO rests** while nothing reaches it and its tail has died away, freeing CPU when tracks are quiet.
* **MENU > ABOUT** shows **SPACE**: how much flash and RAM this firmware leaves free. The flash has room for new features again (3.5.3 had about 1 KB left): among other trims, the sound designer is gone; saving and loading user sounds from the SOUND sheet works as before.
* **FM6 AEG** (EDIT 2, KNOB 4): the slow side is usable all the way up. It slows the envelopes of the operators you hear by up to 24 rate steps (was 40) and never to a release longer than about 15 s, unless the patch's own is longer. A project with AEG turned right has slightly shorter tails than before; AEG 0 and the left side are unchanged.

### 🔧 Fixed
* LFO FADE 0: a new phrase no longer drops the LFO for a moment (a click with LFO onto the filter). Thanks to Thomas Bradley's LEMONADE SANDWICH, where this and the quietest-voice choice come from.
* Web simulator with an FM-1 on USB: a click of the FM-1's knobs is one step again where Windows listed the FM-1 twice.
* SYSTEM: the CPU dial no longer shows its own jittering number; the CPU bar is the one readout.
* Help on the device for ATLAS DENSITY, ATLAS PART4, CHORD PROG, FOLLOW RHYTHM and CONSTELLATION SPEED says to hold OCT-, as the keys work.

### 📦 Installation
Open **[rastapopolix.github.io/tessera](https://rastapopolix.github.io/tessera/)** in Chrome or Edge, connect the FM-1 by USB and press **Install**, then keep the tab open and the FM-1 connected until the install window says Done. The installer can also put the official M-VAVE firmware back. Or take `tessera-3.6.fwsc` (dual core) or `tessera-3.6-1core.fwsc` from this release.

### ⚠️ Good to know
* **On 3.5.3 and older, a project saved on 3.6 does not load** (the project format changed for ANSWER's held lengths, 16 notes, CHORD and BEND). Keep a copy if you may go back.
* A project that uses one of the new MODS destinations, FOLLOW HOW HARM or QNT HARM needs 3.6. A project saved with QNT OWN loads on older firmware with each track's QNT as saved.
* Patterns (PAT GEN) keep ANSWER's first 8 notes; CHORD, held lengths and notes 9 to 16 belong to the track, not the pattern.
* A project saved with a microtonal scale and QNT OFF now plays in its tuning (see Harmony).
* PPQN, like CLK, is kept through power-off and project loads.
* The identity is **FM-1_836**: M-UPGRADE 3.11 and later recognise it, and the web installer offers 3.6 as an update.

### 🙏 Thanks
To everyone who tried the 3.6 betas and wrote back, to BobFromson (#755) and the reporters of #724, #743, #744 and #747, and to Thomas Bradley for the fixes from LEMONADE SANDWICH.
