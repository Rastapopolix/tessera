**Tessera 3.7** follows [Tessera 3.6.2](https://github.com/Rastapopolix/tessera/releases/tag/v3.6.2): play a phrase and let a generator carry it on. Capture-to-loop turns what you play on the keys into the seed for TURING, EUCLID, MARKOV and MORPH, and LANDSCAPE gives every named project a place of its own, with a thing on each shore. Everything in the [3.6](https://github.com/Rastapopolix/tessera/releases/tag/v3.6), [3.6.1](https://github.com/Rastapopolix/tessera/releases/tag/v3.6.1) and [3.6.2](https://github.com/Rastapopolix/tessera/releases/tag/v3.6.2) notes still applies.

> ⚠️ **A project with a track set to SOURCE KEYS does not load on 3.6.2 or older.** Projects that don't use KEYS load on 3.6.2 as before. Keep a backup before you update if you may go back.

### 🎹 Capture-to-loop
* **SOURCE KEYS**: TURING, EUCLID, MARKOV and MORPH have a new SOURCE (OCT- held, KNOB 2). Play a phrase on the track's keys (or MIDI IN), up to 16 notes; a bar without a key ends the take, and the generator then plays and varies what you played, with KEEP and GRAVITY as for a STEP source. It is always listening, as ANSWER is: the next phrase you play replaces the last.
* **REC held prints the take into STEP**, so you can fix a note by hand, and the generator plays on from that pattern.
* The take is saved with the project.

### 🖥️ On the screen
* **LANDSCAPE places**: a named project is islands (as before), land cut by narrow channels with an arched bridge, or all land. Each CHAIN section's shore has one thing of its own: a torii on A (on all land, a seated Buddha), and on B and C a pagoda, a lit stone lantern or a pine (now and then a cherry, its petals falling slowly), some shores bare. Each is reflected in the water, and the lantern glows with the notes.

### ⚙️ Space
* About 2.6 KB of flash freed by drawing the knob cards through shared helpers (the screens are exactly as before) and leaving developer diagnostics out of release builds.

### 📦 Installation
Open **[rastapopolix.github.io/tessera](https://rastapopolix.github.io/tessera/)** in Chrome or Edge, connect the FM-1 by USB and press **Install**, then keep the tab open and the FM-1 connected until the install window says Done. The installer can also put the official M-VAVE firmware back. Or take `tessera-3.7.fwsc` (dual core) or `tessera-3.7-1core.fwsc` from this release.

### ⚠️ Good to know
* **A project with a track set to SOURCE KEYS does not load on 3.6.2 or older firmware.** Other projects saved on 3.7 load on 3.6.x as before, and every 3.6.x project loads and plays the same on 3.7. As with 3.6, a project saved on 3.6 or later does not load on 3.5.3 or older.
* Named projects whose place is channels or all land look different from 3.6.2 (their hills are new); islands places keep their hills and the torii where it stood (B and C gain a little ground under their thing).
* The identity is **FM-1_837**: M-UPGRADE 3.11 and later recognise it, and the web installer offers 3.7 as an update.
