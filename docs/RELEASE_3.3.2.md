**Tessera 3.3.2** follows [Tessera 3.3.1](https://github.com/Rastapopolix/tessera/releases/tag/v3.3.1) with one fix. Everything in the [3.3 notes](https://github.com/Rastapopolix/tessera/releases/tag/v3.3) and the [3.3.1 notes](https://github.com/Rastapopolix/tessera/releases/tag/v3.3.1) still applies, and projects, presets and settings carry over as they are.

### 🛠️ Fixed
* **The ENERGY page shows the piece's shape when PIECE has a LENGTH.** With a LENGTH set, ENERGY follows the piece's SHAPE, but the page still drew its own free tide, so the disc showing where the energy is now floated off the curve. Now the page draws the piece's SHAPE, swung by ARC and moved by LEVEL as ENERGY uses it, with the disc on it where the piece is. Without a LENGTH the page is as before.

### 📦 Installation
Open **[rastapopolix.github.io/tessera](https://rastapopolix.github.io/tessera/)** in Chrome or Edge, connect the FM-1 by USB and press **Install**, then keep the tab open and the FM-1 connected until the install window says Done. The installer can also put the official M-VAVE firmware back. Or take `tessera-3.3.2.fwsc` (dual core) or `tessera-3.3.2-1core.fwsc` from this release.

### ⚠️ Good to know
* **Coming from 3.3?** Read the [3.3.1 notes](https://github.com/Rastapopolix/tessera/releases/tag/v3.3.1) too: **SYSTEM > DRUM** starts at **OFF** since 3.3.1, so if something listens to Tessera's drums on channel 10, set DRUM to **10** after updating.
* **The project format has not changed** since 3.3.
* The identity stays **FM-1_833**: M-UPGRADE 3.11 and later recognise it, and the web installer offers 3.3.2 as an update.
