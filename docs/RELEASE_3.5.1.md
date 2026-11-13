**Tessera 3.5.1** follows [Tessera 3.5](https://github.com/Rastapopolix/tessera/releases/tag/v3.5): ANSWER and FOLLOW hear what you play, the GLO pages start on the MIXER, and the FM-1's own panel can play the web simulator. Everything in the [3.5 notes](https://github.com/Rastapopolix/tessera/releases/tag/v3.5) still applies.

### 🎹 ANSWER and FOLLOW hear your keys
* **ANSWER** answers the notes you play on its SOURCE track's keys (or over that track's MIDI IN), not only that track's pattern or generator. A STEP pattern playing on the SOURCE still comes first.
* **FOLLOW** follows the notes you play on the followed track's keys too, and so do CANON, RHYTHM SOURCE and HOLD. A STEP pattern playing there still comes first; against a generator, the newer note wins.
* With ARP on the played track, both hear the held keys, not the ARP's notes.

### 🎚️ GLO starts on the MIXER
* **MIXER is GLO's first page** (1/9), with GLO's tabs and a key legend: a black key MUTE, a white key SOLO, E FILL.
* **PERF and LANDSCAPE keep the MIXER's keys**: mute, solo, FILL, tap and the key lights work there too, so the first three GLO pages are the play pages. The keys play notes again from PIECE on.
* **GLO's order**: MIXER, PERF, LANDSCAPE, PIECE, CHAIN, ENTRIES, SYSTEM, MIDI IN, MIDI OUT. SELECT turns through them but skips MIXER, where SELECT stays the tempo.
* **SYSTEM** has BUDGET, CPU, A4 and TUNE, with a VOICES bar and a CPU bar (its tick is the worst moment). **MIDI IN** has MIDI, DRUM, ROUT and CLK, and lists the channel each track hears.
* LEVEL reads in whole dB everywhere, the GLO map too. The hint says **HOLD [GLO] SET**, and only off the play pages.
* Tapping GLO or SCL twice locks its keys open ([GLO] LOCK, [SCL] LOCK). This isn't new; it's now in the MANUAL and on the cheatsheet.

### 🔌 The FM-1 plays the web simulator
* Plug the FM-1 in by USB and open the [web simulator](https://rastapopolix.github.io/tessera/webapp/sim/) in Chrome or Edge. **An FM-1 on USB** (under the panel) finds it and says **FM-1 connected**; the FM-1's knobs, buttons, keys and MASTER then play the simulator, holds and shortcuts included.
* While the simulator asks, the FM-1 also sends its panel on MIDI channel 16. It stops about 3 s after the tab closes. Its usual MIDI OUT is unchanged.
* Close the web editor or a DAW that holds the FM-1's MIDI port first. An FM-1 on 3.5 or older gets a message to update.

### 📖 Docs
* **RECIPES** starts from a demo: what each demo is good for, what to change, and the recipe that goes with it.
* The MANUAL's glossary takes the place of GLOSSARY and FEATURES.

### 📦 Installation
Open **[rastapopolix.github.io/tessera](https://rastapopolix.github.io/tessera/)** in Chrome or Edge, connect the FM-1 by USB and press **Install**, then keep the tab open and the FM-1 connected until the install window says Done. The installer can also put the official M-VAVE firmware back. Or take `tessera-3.5.1.fwsc` (dual core) or `tessera-3.5.1-1core.fwsc` from this release.

### ⚠️ Good to know
* **The project format has not changed** since 3.5: projects, presets, settings and learned MIDI CCs carry over both ways, and what the 3.5 notes say about going back to 3.4.1 still applies.
* **Two GLO pages have new names**: GLOBAL is now SYSTEM, and SYSTEM is now MIDI IN.
* The identity stays **FM-1_835**: M-UPGRADE 3.11 and later recognise it, and the web installer offers 3.5.1 as an update.

### 🙏 Thanks
To the Reddit user who found that ANSWER didn't hear its SOURCE's keys, and to bitmap303 for the tagline, a pocket rabbit hole.
