**Tessera 3.7.2** follows [Tessera 3.7.1](https://github.com/Rastapopolix/tessera/releases/tag/v3.7.1): three new chord kinds, ECHO's time in milliseconds, prints that loop the way they played, and fixes to capture and ANSWER. The manual is now also a PDF. Everything in the [3.7](https://github.com/Rastapopolix/tessera/releases/tag/v3.7) and [3.7.1](https://github.com/Rastapopolix/tessera/releases/tag/v3.7.1) notes still applies.

### 🎼 Harmony
* **2NDS, 4THS and 5THS**, three new chord kinds on CHRD (SCL > CHORD): they stack the scale's seconds, fourths or fifths, so they stay in key (quartal and quintal chords). On a microtonal scale they stack its own degrees, which gives close clusters. Thanks to BobFromson (#852).

### 🎛️ Sound and playing
* **ECHO's TIME** (knob 3) now reads in milliseconds, 10 to 367 ms, instead of %, on the FM-1 and in the web editor. Only the display changed: the echo sounds the same.
* **Print** (REC held on a generator or ARP track) now takes whole passes of the pattern's LEN, from where a pass starts, not bars. With a LEN other than 16 the printed loop joins up the way it played: LEN 12 prints 12, 24 or 48 steps.

### 🔧 Fixed
* **Capture and print in a busy project**: the log of notes played now holds 256 notes instead of 128. With all four tracks playing, a four-bar print could come out with its first bar missing.
* **ANSWER while stopped**: a pause of a second and a half now ends the phrase, so the next key starts a new one. Before, every key played while stopped joined the same phrase.

### 📖 The manual
* The manual can now be downloaded as a **PDF** ([tessera-manual.pdf](https://rastapopolix.github.io/tessera/manual/tessera-manual.pdf)), linked from the top of the web manual.

### 📦 Installation
Open **[rastapopolix.github.io/tessera](https://rastapopolix.github.io/tessera/)** in Chrome or Edge, connect the FM-1 by USB and press **Install**, then keep the tab open and the FM-1 connected until the install window says Done. The installer can also put the official M-VAVE firmware back. Or take `tessera-3.7.2.fwsc` (dual core) or `tessera-3.7.2-1core.fwsc` from this release.

### ⚠️ Good to know
* **Projects from 3.7.1 load and play the same.** A project using 2NDS, 4THS or 5THS plays only the chord's root on 3.7.1 or older. As before, a project using DRIVE plays that slot as a FILTER on 3.7 or older, and one using SOURCE KEYS does not load on 3.6.2 or older.
* The identity stays **FM-1_837**: M-UPGRADE 3.11 and later recognise it, and the web installer offers 3.7.2 as an update.
