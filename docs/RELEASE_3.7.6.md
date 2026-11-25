**Tessera 3.7.6** follows [Tessera 3.7.5](https://github.com/Rastapopolix/tessera/releases/tag/v3.7.5): navigation that acts the same on every page, a calmer LANDSCAPE, and synced LFOs that land on the bar after a DAW starts mid-song. Everything in the [3.7](https://github.com/Rastapopolix/tessera/releases/tag/v3.7), [3.7.1](https://github.com/Rastapopolix/tessera/releases/tag/v3.7.1), [3.7.2](https://github.com/Rastapopolix/tessera/releases/tag/v3.7.2), [3.7.3](https://github.com/Rastapopolix/tessera/releases/tag/v3.7.3), [3.7.4](https://github.com/Rastapopolix/tessera/releases/tag/v3.7.4) and [3.7.5](https://github.com/Rastapopolix/tessera/releases/tag/v3.7.5) notes still applies.

### 🧭 Navigation
* **The page tabs show on every page**, now also on PATTERNS, SONG, MOTION, MODS, PRESETS and SCALES, and the big knob readout sits above them instead of covering them.
* **SELECT stops at a group's first and last page.** The group buttons still wrap: after the last page comes the first.
* **No more GLO / SEL double-tap lock.** Two quick taps step two pages, as on every other button. The layer is there while you hold the button, and MIXER, PERF and LANDSCAPE still give the GLO keys without holding.
* **GLO reopens the GLO page you last used**, as FX, ENV, LFO, SEL, EDIT, SEQ and ARP already did. SAVE still opens USER.
* **A hold is a modifier, never a page:** SEQ held no longer jumps to PATTERNS (SEQ held + keys still pick patterns; tap SEQ or turn SELECT to get there).
* **An EDIT tap always opens EDIT.** What it did on some pages is now on the sheet (OCT+ held): **Page map** (the sound sheet's last row), **Delete note** / **Delete selected** on NOTES and STEP, **Clear hit** on the drum grid, **Rename** on the USER and PROJECT slots. EDIT held (undo, redo, clear) is unchanged.

### 💡 Lights
* **LANDSCAPE (and IDLE's LANDSCAPE) no longer flashes the keys.** They follow **MENU > LIGHTS** (dark at OFF); only a solo, FILL or HOLD stays bright. MIXER and PERF are unchanged.

### 🌊 LFOs
* **A SYNC + FREE LFO stays on the bar when the clock jumps.** With CLK on USB or TRS, a Song Position from a DAW (PLAY from mid-song, a loop, a locate) puts the LFO where it would be had it played from the top. Before, it started from its PHASE at the jump, off the bar. Cords and the matrix that use the LFO (S&H, SLEW, EOC) follow it.
* **LFO WAVE has no list picker any more:** turn the knob and the shape shows under it.

### 🖥️ Screens
* **Slot and list pages** (PROJECT, SAVE > USER, FM6's STORE, VOICE, SCL and the other lists) **get the ordinary page header and their family's tabs**, so FM6's EDIT pages keep one shape through STORE. Slot pages still show the track's sound under the rows; the lists no longer show the tempo and its beat triangle (HOME does).
* In MOSAIC, the sound browser's picked row and card are white, as in every other list.

### 📦 Installation
Open **[rastapopolix.github.io/tessera](https://rastapopolix.github.io/tessera/)** in Chrome or Edge, connect the FM-1 by USB and press **Install**, then keep the tab open and the FM-1 connected until the install window says Done. The installer can also put the official M-VAVE firmware back. Or take `tessera-3.7.6.fwsc` (dual core) or `tessera-3.7.6-1core.fwsc` from this release.

### ⚠️ Good to know
* **Coming from 3.7.5:** SEQ held, an EDIT tap and a GLO / SEL double tap now do the things above; the [manual](https://rastapopolix.github.io/tessera/manual/) and the cheatsheet are updated.
* **Projects from 3.7.5 load and play the same**, and a project saved on 3.7.6 opens on 3.7.5. No new settings or project parts.
* The identity stays **FM-1_837**: M-UPGRADE 3.11 and later recognise it, and the web installer offers 3.7.6 as an update.
