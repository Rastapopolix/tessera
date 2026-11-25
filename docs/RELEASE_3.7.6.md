**Tessera 3.7.6** follows [Tessera 3.7.5](https://github.com/Rastapopolix/tessera/releases/tag/v3.7.5): a calmer LANDSCAPE, synced LFOs that land on the bar after a DAW starts mid-song, and tidier pages. Everything in the [3.7](https://github.com/Rastapopolix/tessera/releases/tag/v3.7), [3.7.1](https://github.com/Rastapopolix/tessera/releases/tag/v3.7.1), [3.7.2](https://github.com/Rastapopolix/tessera/releases/tag/v3.7.2), [3.7.3](https://github.com/Rastapopolix/tessera/releases/tag/v3.7.3), [3.7.4](https://github.com/Rastapopolix/tessera/releases/tag/v3.7.4) and [3.7.5](https://github.com/Rastapopolix/tessera/releases/tag/v3.7.5) notes still applies.

### 💡 Lights
* **LANDSCAPE (and IDLE's LANDSCAPE) no longer flashes the keys.** They follow **MENU > LIGHTS** (dark at OFF); only a solo, FILL or HOLD stays bright. MIXER and PERF are unchanged.

### 🌊 LFOs
* **A SYNC + FREE LFO stays on the bar when the clock jumps.** With CLK on USB or TRS, a Song Position from a DAW (PLAY from mid-song, a loop, a locate) puts the LFO where it would be had it played from the top. Before, it started from its PHASE at the jump, off the bar. Cords and the matrix that use the LFO (S&H, SLEW, EOC) follow it.
* **LFO WAVE has no list picker any more:** turn the knob and the shape shows under it.

### 🖥️ Screens
* **Slot and list pages** (PROJECT, SAVE > USER, FM6's STORE, VOICE, SCL and the other lists) **get the ordinary page header and their family's tabs**, so FM6's EDIT pages keep one shape through STORE. Slot pages still show the track's sound under the rows.
* In MOSAIC, the sound browser's picked row and card are white, as in every other list.

### 📦 Installation
Open **[rastapopolix.github.io/tessera](https://rastapopolix.github.io/tessera/)** in Chrome or Edge, connect the FM-1 by USB and press **Install**, then keep the tab open and the FM-1 connected until the install window says Done. The installer can also put the official M-VAVE firmware back. Or take `tessera-3.7.6.fwsc` (dual core) or `tessera-3.7.6-1core.fwsc` from this release.

### ⚠️ Good to know
* **Projects from 3.7.5 load and play the same**, and a project saved on 3.7.6 opens on 3.7.5. No new settings or project parts.
* The identity stays **FM-1_837**: M-UPGRADE 3.11 and later recognise it, and the web installer offers 3.7.6 as an update.
