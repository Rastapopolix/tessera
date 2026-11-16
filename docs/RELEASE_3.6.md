**Tessera 3.6** follows [Tessera 3.5.3](https://github.com/Rastapopolix/tessera/releases/tag/v3.5.3): everything from the eight 3.6 betas, and more since the last one. A harmonizer, a delay with a mind of its own on ANSWER, 22 more mod destinations in a grouped list, QNT per track, MIDI clock at other PPQNs, the FM-1 as a controller for the web simulator, a place of its own for every project on LANDSCAPE, HOLD for the generators, Program Changes that follow the form, ENTRIES that know a bass from a melody and fade your outside synths too, keys that light up with the notes, and a dozen touches on the screen. Around it: a fairer and earlier voice ceiling, faster FM6 and TRIO, and more room in the flash.

> ⚠️ **The project format has changed: a project saved on 3.6 does not load on 3.5.3 or older.** Keep a backup before you update if you may go back. Projects, presets and settings from 3.5.3 load on 3.6 and play as before (but for FM6 AEG turned right: see below).

### 🎼 Harmony
* **HARM, a harmonizer** (after Dreamsequence's): with a CHORD track playing, each note of the scale up plays the chord's next note up, so a run up the keys is an arpeggio that follows the chords. **SCL > QNT HARM** does this for the keys, MIDI IN and the generators; FOLLOW's new **HOW HARM** does it to another track's line, SHIFT moving by chord notes. A cord into SHIFT on a HOW HARM track picks chord notes: any modulation, in harmony ([RECIPES](https://github.com/Rastapopolix/tessera/blob/main/RECIPES.md)).
* **QNT LINK** (SCL, the row after QNT): **LINKED** keeps one QNT for every track, as before; **OWN** gives each track its own QNT and MPC degree, so one track can take MPC pads while others follow the chords (CHRD) or HARM (#755). The MANUAL now has a table of QNT's modes, MPC included.
* **Microtonal tunings with QNT OFF**: microtonal scales and your own Scala tunings now play with QNT OFF too. A scale of up to 12 degrees an octave (the maqamat RAST, BAYATI, HIJAZ, SABA, HUSAYN, SIKAH, the just and meantone scales, the Japanese pentatonics) keeps the keyboard a piano's: every key plays its own note, and the keys nearest the scale's degrees take its tuning (RAST on C: E and B play half flat), so you play a maqam as on a piano and can still reach every note off it. A bigger tuning (24EDO, 31EDO, a large Scala file) plays every degree in a row, one a key and one a MIDI note. A project saved with a microtonal scale and QNT OFF now plays in its tuning instead of ordinary semitones.

### 🔁 ANSWER
* **BEND**, a delay with a mind of its own (after Qu-Bit's Data Bender, the idea only): GEN sheet (OCT+ held), **Mode** REPLY (as before) or BEND. Every note comes back **TIME** steps later (1 to 16) while you are still playing, then again, drifting with VARY and SHIFT; **BREAK** is the chance a repeat glitches (slips, skips, stutters, jumps back). With VARY, SHIFT, ECHO and BREAK at 0 it is a plain tempo-synced delay. A STEP pattern as SOURCE is not heard in BEND yet (the keys, MIDI IN and a generator SOURCE are).
* ANSWER holds each note as long as you held its key in the call (with LENGTH on STEP, the default), and captures up to **16 notes** a call (was 8).
* **ANSWER CHORD** (GEN sheet): ARP spreads notes played together one step apart, as before; TRUE answers them as a real chord.

### 🔌 MODS
* **22 more destinations**: STRUM, STR DIR, RATCH, R CHANC, GATE, N LEN, LOCK, VELO, SWING, GLIDE, GRAVITY, DEVELOP, AN ECHO, AN SHFT, AN SPCE, BREATH, ACC LIM, TIDE SP, SWRM RT, ARP RTE, ARP GTE and ARP OCT.
* KNOB 3's destinations come **in groups** (SOUND, ENV, LFO, MIX, FX, GEN, FEEL, EVOLVE, ARP, MODS), and the list's title names the group under the cursor, for example T3 LFO. A track lists only the destinations that act on what it plays; a mod already set to one keeps it.
* KNOB 1's and KNOB 3's lists start at the **selected track**: on track 3, the first turn lands on T3 LFO, or T3 PITCH (#724).

### 🖥️ On the screen
* **LANDSCAPE gets a place per project**: a named project's name makes its hills or islands, its stars, where the moon rests and a torii gate on A's shore; an unnamed project keeps the lake as before. The layers you switch on show in the scene: a PIECE's shape is the moon's phase (waxing, lit on the left, as the shape rises; waning, lit on the right, as it falls), with dusk (or the stars coming out) over its last tenth; each CHAIN section has its own shore, sliding in from the right, and a shooting star when the form comes round; ENERGY blows the clouds right as it builds and left as it thins, and a rest lays mist on the water.
* **MENU > IDLE LANDSCAPE**: after two minutes without a touch while playing, the screen moves to LANDSCAPE. The first touch only takes you back to the page you were on. OFF (the default) as before.
* **MENU > TEXT BOLD**: the small text drawn heavier on every screen, for reading at a distance. NORMAL (the default) as before.
* **LARGE VALUE ON** keeps the large value about a second after the knob stops.
* **A PIECE's progress in the header**: while a PIECE plays, the line under every header is solid as far as the piece has got and dotted after.
* **The beat in the PLAY triangle** (HOME and the list headers): solid on the 1, hollow on the other beats, blinking on the 1 of a phrase line (CHAIN's 4, 8 or 16 bars).
* **MIXER, a waiting mute** (MENU MUTES BAR or PHRASE): a thin line under the strip's MUTE box counts down to the bar or phrase line where it lands.
* **ROLL**: while playing, a turned seed that waits for the bar line shows dotted under the one playing, until it lands. **ROLL ALL** now goes back two levels: turn it left once for the seeds before the last ROLL ALL, twice for the ones before that.
* **MENU > NOTE LIGHTS**: the keys light up while a sequence plays their notes, for as long as each note lasts. OFF (the default) as before; STEP lights a STEP pattern's notes; ALL the generators' too.
* **PRESETS by engine**: LIST has the engines after the categories (FM6, ORACLE, TRIO, PHYS, DRUM), each listing only that engine's sounds, your user presets included. **EDIT > VOICE** starts with the track's Sound: KNOB 2 steps through the sounds, OCT+ opens PRESETS right there, and OK or BACK bring you back to VOICE.
* **QUAD's DIR and RANGE** are drawn as pictures (arrows, two dots for RAND, a bar for HOLD; one to three ticks for the range) instead of single punctuation signs.

### 🎹 Playing
* **HOLD, GLO + D4**: while held, every generator keeps the line it is playing (EVOLVE and STEP EVOLVE skip their rounds, TURING plays as PROB 0, a recorded MARKOV or MORPH phrase and CONSTELLATION's loop as LOCK 100, ATLAS keeps its bar). Let go and they carry on. A double tap latches it, one more tap lets it go. On MIXER, PERF and LANDSCAPE, D4 alone does the same. MARKOV and MORPH at LOCK 0 keep the pass they last played, so HOLD (or LOCK turned up while playing) loops what you just heard at once.
* **SECTION PC** (MIDI OUT's sheet, one row a track, OFF by default): when the CHAIN goes to section A, B or C, the track sends Program Change 0, 1 or 2 on its MIDI OUT channel, before that bar's notes, so a drum machine or groovebox follows the form. A repeated section sends nothing.
* **ENV + OCT- / OCT+ on HOME**: the selected track an octave down or up (its TRN, up to two octaves either way); ENV + both OCTs puts it back to 0. Plain OCT- and OCT+ still move the keys' octave.
* **ENTRIES' ROLE AUTO** works out each track's part before it plays, from its generator, sound and register, judging the four tracks together: CHORD and chord keys are HARMONY; GRIDS, a kit or NOISE RHYTHM; the lowest track clearly below the others BASS. The ROLE knob shows the guess.
* **MODS**: a STEP track's **GATE** and **NOTE** sources now follow its pattern, as a generator track's do (they stayed flat before).

### 🎛️ MIDI, the simulator and the installer
* **PPQN**, the MIDI clock's pulses per beat (GLO > MIDI IN, hold OCT-, KNOB 4): 24 (the default, the MIDI standard), 48, 4, 8 or 12, for the clock the FM-1 follows and for CLOCK OUT. If the FM-1 plays twice as fast as the gear it follows, try 48 (#747).
* **ENTRIES with STYLE FADE fades MIDI OUT too**: while a track fades out or in, its notes go out with their velocity scaled by its level (a synth that ignores velocity still hears a cut). A track at SOUND OFF fades over MIDI as well.
* **SIM CONTROL** (in the menu: hold HOME): with the [web simulator](https://rastapopolix.github.io/tessera/webapp/sim/) open and the FM-1 connected by USB, the FM-1's knobs, buttons and keys play only the simulator. Hold FX + GLO for a second to get the panel back; closing the simulator or unplugging ends it too. It is off every time the FM-1 starts.
* The installer waits a few seconds for the FM-1, and when another app or tab holds the FM-1's MIDI port it names the port instead of saying FM-1 not found (#743).
* The FM-1's update mode is now called **Tessera Update** on the computer (it was Melodee Update). A computer that has seen the old name may keep showing it.
* After a project loads, a track playing one of your own FM6 voices finds its user slot again: HOME shows its name and PRESETS starts from it (#744).

### ⚙️ Sound, CPU and space
* **A fairer voice ceiling**: after an overload (a crackle) the voices come back within a few seconds while you play, and one overload no longer holds every track to one or two voices. When a sound has to be cut early, the quietest fading drum (a hat's tail) or the quietest released voice goes first, and a drum hit that has not sounded yet is never taken by a later one (ATLAS kept losing its 909 snare).
* **An earlier ceiling**: past 70 % CPU the FM-1 holds back new voices (a new note takes over a fading one, the bass kept) before it has to cut a sound, so fewer clicks. A dense chord may lose a released tail a little earlier than in 3.5.3.
* **TRIO on the second core**: TRIO sounds (STRING MACH, WARM PAD ...) now share the work with CPU1, as FM6, ORACLE and DRUM already did. The sound is exactly the same.
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
* A project with SECTION PC on loads on older firmware with SECTION PC simply off. MENU > TEXT and MENU > IDLE are kept with the settings; older firmware ignores them.
* A project with its ENTRIES ROLEs on AUTO may get different roles than on 3.5.3, so tracks may come in and leave in another order. Set a ROLE (or ORDER) by hand to keep it as it was.
* PPQN, like CLK, is kept through power-off and project loads.
* The identity is **FM-1_836**: M-UPGRADE 3.11 and later recognise it, and the web installer offers 3.6 as an update.

### 🙏 Thanks
To everyone who tried the 3.6 betas and wrote back, to BobFromson (#755) and the reporters of #724, #743, #744 and #747, to the maqam player whose request shaped QNT OFF, and to Thomas Bradley for the fixes from LEMONADE SANDWICH.
