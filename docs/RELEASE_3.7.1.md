**Tessera 3.7.1** follows [Tessera 3.7](https://github.com/Rastapopolix/tessera/releases/tag/v3.7): a new effect, DRIVE, and a manual that is easier to read. Everything in the [3.7 notes](https://github.com/Rastapopolix/tessera/releases/tag/v3.7) still applies.

### 🎛️ Sound
* **DRIVE**, a new effect: a soft clip that rounds the peaks off for warmth or grit (good on FM sounds). **RATE** is the drive, **CHAR** folds the peaks back as a wavefolder does, **MIX** blends it with the dry sound. Put it in FX1 to drive the track before its other effects; it works as a send effect too. It is light on the CPU. Thanks to pavig-coder / u/paulvigo for the idea (#824).

### 📖 The manual
* The [manual](https://rastapopolix.github.io/tessera/manual/) has had a readability pass: chapters grouped, plain names for the MODS destinations, the fine print folded away, links on first use and a "Try this" example in each chapter.

### 📦 Installation
Open **[rastapopolix.github.io/tessera](https://rastapopolix.github.io/tessera/)** in Chrome or Edge, connect the FM-1 by USB and press **Install**, then keep the tab open and the FM-1 connected until the install window says Done. The installer can also put the official M-VAVE firmware back. Or take `tessera-3.7.1.fwsc` (dual core) or `tessera-3.7.1-1core.fwsc` from this release.

### ⚠️ Good to know
* **Projects from 3.7 load and play the same.** A project that uses DRIVE loads on 3.7 and older, but plays that slot as a FILTER. As with 3.7, a project using SOURCE KEYS does not load on 3.6.2 or older.
* The identity stays **FM-1_837**: M-UPGRADE 3.11 and later recognise it, and the web installer offers 3.7.1 as an update.
