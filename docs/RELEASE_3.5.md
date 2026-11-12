**Tessera 3.5** follows [Tessera 3.4.1](https://github.com/Rastapopolix/tessera/releases/tag/v3.4.1): modular-style patching on the FM-1. Mods can now be mixed, gated, clocked and braked, and MOD SCOPE shows what a cord does. Around it: more from the generators, a direction for every pattern, one key rule on every page, a warm paper screen, and projects you can move between the web simulator and the FM-1 as files.

> ⚠️ **A project that uses the new mod features does not load on 3.4.1**: MIX buses, EOC, CLOCK, PLAY or a mod onto MOD DEP. Projects, presets and settings from 3.4.1 load on 3.5 and play as before. Before you connect the web editor for the first time, see *Web editor* below.

### 🔌 Patch like a modular
* **MIX buses**: BUS A and BUS B, two mixers for mods. Send any mods into a bus (the end of KNOB 3's list on MODS) and use the bus as a source (the end of KNOB 1's list). On MODS' sheet (OCT+ held): each bus's **Mode** (SUM, MAX, MIN, DIFF; with gates OR, AND, XOR) and **Lag** (OFF, 10 ms to 20 s: a gate into a lagged bus is an envelope). A bus can feed itself, one block late, safely.
* **EOC**: a new source on every track, the LFO's cycle start as a gate (high from each wrap for an eighth of the cycle). Use it as a GATE mod onto FILL, a bus or a mod's amount.
* **CLOCK**: a new destination. A generator track with a mod onto CLOCK steps on the source's edges instead of its DIV. EOC as the source gives a note each LFO cycle (add S&H onto the LFO's rate for a Krell). AMOUNT is the division (50 % = every second edge); a GATE source counts hits.
* **PLAY**: a destination that decides whether a track is heard. A GATE mod onto PLAY is a gated voice in one cord; a shut track plays on unheard, so the tracks that follow it keep going.
* **Mods of mods can scale**: hold OCT- on M1 AMT .. M16 AMT and turn KNOB 3 right. The mod then multiplies the other mod's move by 0 to 1 instead of adding to it (M3* on the row), so a GATE or an envelope opens and closes a mod that keeps its own amount.
* **HOLD mods**: OCT+ on MODS cycles +- / + / GATE / HOLD. HOLD takes the source at the start of each bar, or on each note of a track you pick (hold OCT-, KNOB 1), and keeps it until the next.
* **MOD DEPTH**: one control for how much of the whole patch plays, 0 to 100 % (GLO > PERF's sheet). It scales every mod's movement, never an OFFSET: at 0 the patch is dry. MOD DEP is also a destination, so a MACRO can ride it. The brake for a feedback patch.
* **MOD SCOPE**: hold OCT- on MODS to see the picked mod move over time (a line) over its source (faint dots), a GATE's threshold as a wedge. Hold OCT- and tap OCT+ to pin it (SCOPE PINNED), so the knobs work while you watch. The screen reader says its shape.
* **Clear mod**: the first row of MODS' sheet empties the picked mod.
* New to this? [GENERATIVE.md](https://github.com/Rastapopolix/tessera/blob/main/GENERATIVE.md) starts with **New to patching?** and has long patches (Krell, long gates, stepped chaos, the brake); [RECIPES.md](https://github.com/Rastapopolix/tessera/blob/main/RECIPES.md) has short ones.

### 🎲 More from the generators
* **PAT GEN**: each of a track's 8 patterns can keep its own generator setup, so changing pattern changes the generator (EVOLVE, OCT- held, KNOB 3; off by default).
* **ROLL ALL**: a new seed on every generator track at once, with one step back (EVOLVE, OCT- held, KNOB 1). ROLL and ROLL ALL say NEW LINE.
* **RANDOM**: hold OCT- on GEN or TWEAK and turn KNOB 1 right for new values on the page's knobs, each inside its range; turn left to put them back.
* **ENTRIES ORDER**: choose the order tracks come in (AUTO, 1ST to 4TH); they leave in reverse (ENTRIES' sheet).
* **CHORD STRUM**: a CHORD track's notes start 1 to 250 ms apart, UP, DOWN or ALT, like a played chord (GEN, OCT- held). MIDI OUT gets them strummed too.
* **New chord kinds** SUS2, 7SUS4, ADD9 and MAJ6 on CHRD, for the chord keys, MIDI in, step entry, the arp and the CHORD generator.
* **USER scales**: four 12-note scales of your own, USER1 to USER4 on SCL > SCALES, made on the keys. Every generator, QNT and FOLLOW play them.
* **ARP modes** DNUP and RND1 (one random order, repeated until the held notes change), and **PRINT ARP**: hold REC on an ARP track to print what it played into STEP.

### ↔️ STEP direction
* Each pattern plays **FWD, BACK, PEND, FOLD, RAND or DRUNK** (the Direction row on the pattern's sheet). FOLD goes from the ends inwards: 1 8 2 7 3 6 4 5. RAND and DRUNK follow the SEED, so PLAY replays them.
* **DIR** is a mod destination: a cord moves the direction at each step, and a GATE mod switches it while its source is high.

### 🎹 One key rule everywhere
* **OCT+ goes in, OCT- goes back**, on every page. Tap OCT+: Enter (the row, slot, note or mod under the cursor). Hold OCT+: the page's sheet (a page without one says NO SHEET). Tap OCT-: Back. Hold OCT-: the page's second set of knobs, only while held. The OCT- LED is lit where that does something.
* Moved from OCT+ held to OCT- held, same knobs: FORM, TIMING, ENERGY, CHAIN, MIXER (PAN), MODS (threshold, CC number), GEN's second layer, EVOLVE, TWEAK (FOLLOW), MIDI OUT (TRANSPOSE), ENTRIES (ROLE). Settings that didn't fit four knobs moved onto sheets: GEN's sheet, MIDI OUT's (SOUND, DRUM MAP), ENTRIES' (ORDER), MODS' (the MIX buses), MIXER's (Mutes).
* **Values under the knob**: DIV, ARP MODE, LFO SYNC, TEMPO's QNT, CHRD, VOIC and the FX types step under their knob, no list pops up; the ones in time order go slow to fast. The turned value is inverted on every page.
* NOTES: OCT+ on an empty step no longer places a note; it says NOTHING THERE.
* The [cheatsheet](https://github.com/Rastapopolix/tessera/blob/main/docs/cheatsheet/tessera-cheatsheet.pdf) has every key.

### 🌅 GROUND LIGHT
* **MENU > GROUND > LIGHT** puts the screen on warm paper. Each ink becomes a darker one that reads well on paper; the MOSAIC sets keep four different inks.
* **LANDSCAPE by day**: on LIGHT, a sun takes the moon's place and the stars stay out.

### 💾 Projects, the web editor and the simulator
* **Project files**: save a slot's project as a `.tsp` file, or load one into a slot, in the web editor (Projects tab) and in the [web simulator](https://rastapopolix.github.io/tessera/webapp/sim/) (under the FM-1). Move songs between the simulator and the FM-1 this way.
* **The web editor only reads the FM-1** until you tick **Edit the FM-1** beside Disconnect, and ticking it offers a complete backup first. It is off again at every connection (#609).
* **OLD SAVE LOADED**: each slot keeps its last two saves. If the newest is damaged, the FM-1 loads the one before and now says so (#609).
* **The installer, the web editor, the simulator and the project page** come in English, Spanish, Italian, German, French, Brazilian Portuguese, Korean and Japanese. The translations are machine-made: corrections are welcome.
* Web editor: user presets show in the Sound tab's Presets list (#526); **Health** has six USB audio rows, so a click on the USB side can be told from an overloaded FM-1 (#586).

### ⚙️ Sound and CPU
* **An honest CPU meter**: GLO > SYSTEM counts everything (USB and MIDI too) and shows the average and the worst moment of the last second, as 55/92. When the second number reaches 99, the sound may break up.
* **A track nobody can hear stops costing CPU**: at LEVEL 0, muted, left out by SOLO or by ENTRIES, its voices stop after about 46 ms, while its sequencer, LFO and followers go on. It fades back in when heard (#586).
* **USB OUTPUT 4 TRACKS**: each track's stem has the stereo recording's level and soft clip, so a loud chord no longer crackles. The MANUAL covers recording stems on macOS and Windows.
* **MIDI OUT DRUM MAP**: GM or ROW, for drum machines that take their tracks as a row of notes.
* **New projects** start with CEDAR RAIN's sounds (KUGO HARP, KOTO WARM, FLUTE and the 808 KIT in KUMOI on E at 80 BPM, REVERB and ECHO on the sends). Saved projects are unchanged.

### 🔧 Fixed
* ARP HOLD with an external clock and the transport stopped: a latched chord keeps playing at the local tempo.
* Web editor on macOS: found when CoreMIDI still names the port "FM-1" (#527).
* Rename and SAVE while playing: the name screen opens; only writing it needs STOP (#602).
* STEP direction backwards plays each step's own recorded notes; a tied note plays once.
* USER scales: the keys light and play right with QNT on (#559).
* ARP ORD plays the notes in the order the keys were pressed.
* ANSWER follows a SOURCE track that runs a generator.
* Muting a generator track no longer stops the tracks that follow it.
* OCT- and OCT+: a long press never goes back, a short look at the second knobs never goes HOME (#519), and a knob turned while an OCT key is held is not a tap.
* FM6's EDIT map lands on the page last used on an operator's row; KNOBS BARS keep a gap at the top.
* Web simulator: the same sounds as the device (no SID KIT) (#593).

### 📦 Installation
Open **[rastapopolix.github.io/tessera](https://rastapopolix.github.io/tessera/)** in Chrome or Edge, connect the FM-1 by USB and press **Install**, then keep the tab open and the FM-1 connected until the install window says Done. The installer can also put the official M-VAVE firmware back. Or take `tessera-3.5.fwsc` (dual core) or `tessera-3.5-1core.fwsc` from this release.

### ⚠️ Good to know
* **On 3.4.1, these projects do not load**: one with a mod into or out of a MIX bus, EOC as a source, or a mod onto CLOCK, PLAY or MOD DEP. Keep a copy saved without them if you go back.
* On 3.4.1 a project from 3.5 plays these the old way: a direction as FWD, a HOLD mod as +-, a scaling mod of mods as one that adds, MOD DEPTH at 100 %, a strum as a plain chord, SUS2 / 7SUS4 / ADD9 / MAJ6 as POW, ARP DNUP or RND1 as REPEAT, a HOLD clocked by notes by the bar. A cord or MIDI CC on CHRD now spreads over 14 kinds, not 10.
* **GROUND** is kept with the settings: older firmware keeps your ink and draws it on black.
* From the 3.5 betas: STEP direction PONG is now FOLD; 3.5-beta4 and beta5 don't load a project with STRUM over 60 ms.
* **The keys moved** (see above): what was on OCT+ held is on OCT- held or on a sheet, and OCT- no longer goes HOME.
* The identity is **FM-1_835**: M-UPGRADE 3.11 and later recognise it, and the web installer offers 3.5 as an update.

### 🙏 Thanks
To everyone who tried the 3.5 betas and release candidates and wrote back, and to resocut (#586), bambalaya (#593, #602, #609), BobFromson (#519, #559) and tv-glotzer (#527) for their reports.
