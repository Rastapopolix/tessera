**Tessera 3.2.2** follows [Tessera 3.2.1](https://github.com/Rastapopolix/tessera/releases/tag/v3.2.1) with one addition: **Snap to scale**, which moves a pattern's written notes onto the scale. Projects, presets and settings from 2.x, 3.0, 3.0.1, 3.1, 3.2 and 3.2.1 carry over as they are.

### 🎯 Snap to scale
* QNT works on notes as they come in, so notes written with QNT OFF or before SCL changed stay off the scale. This brings them onto it, once.
* Hold **OCT+** on **GEN (STEP)**, **NOTES** or **PATTERN** for the pattern's sheet, go to **Snap to scale** and press **OCT+**.
* It moves every written note of the pattern on a melodic track: each note of a chord, recorded notes, and steps hidden past LEN.
* Each note goes to the nearest note of the scale, from ROOT; halfway between two, it goes down. Two notes of a chord that land on the same note become one.
* With **QNT CHRD** and a CHORD track playing, notes go to the nearest note of the chord playing now.
* The header says how many moved (**SNAPPED 7**) or **ALL IN SCALE**. Hold **EDIT** to undo it.

### 📦 Installation
Open **[rastapopolix.github.io/tessera](https://rastapopolix.github.io/tessera/)** in Chrome or Edge, connect the FM-1 by USB and press **Install**, then keep the tab open and the FM-1 connected until the install window says Done. The installer can also put the official M-VAVE firmware back. Or take `tessera-3.2.2.fwsc` (dual core) or `tessera-3.2.2-1core.fwsc` from this release.

### ⚠️ Good to know
* **Projects from earlier versions load** and play as before, and the project format has not changed.
* On a microtonal scale or your own tuning with QNT on, every note already is a degree of the scale, so Snap to scale moves nothing. A kit or slices has no such row.
* Snap to scale is new and not yet tried on many devices: if something is off, please say so in an [issue](https://github.com/Rastapopolix/tessera/issues).
