**Tessera 2.1.1** is a small fix to [Tessera 2.1](https://github.com/Rastapopolix/tessera/releases/tag/v2.1): chord names fit
their boxes on CHORD's CIRCLE progression. Projects, presets and settings carry over from 2.1 as they are.

### 🔧 Fixes
* **CHORD on CIRCLE:** with 7th chords (CHRD DIA7), names like "Fmaj7" and "Bm7b5" ran out of their boxes into the
  neighbouring chords, because CIRCLE's seven chords leave narrow boxes. The chord's quality under its root now takes
  the largest size the box holds, so every name fits. In an even narrower box, "maj7" is written "M7".

### 📦 Installation
Open **[rastapopolix.github.io/tessera](https://rastapopolix.github.io/tessera/)** in Chrome or Edge, connect the
FM-1 by USB and press **Install**, then keep the tab open and the FM-1 connected until the install window says Done.
The installer can also put the official M-VAVE firmware back. Or take `tessera-2.1.1.fwsc` (dual core) or
`tessera-2.1.1-1core.fwsc` from this release.
