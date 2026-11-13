**Tessera 3.5.3** follows [Tessera 3.5.2](https://github.com/Rastapopolix/tessera/releases/tag/v3.5.2): just the CPU meter, fixed properly this time. Everything in the [3.5](https://github.com/Rastapopolix/tessera/releases/tag/v3.5), [3.5.1](https://github.com/Rastapopolix/tessera/releases/tag/v3.5.1) and [3.5.2](https://github.com/Rastapopolix/tessera/releases/tag/v3.5.2) notes still applies.

### 📊 The CPU meter, actually steady this time
* 3.5.2's attempt at steadying the CPU% bar and number on the SYSTEM page still jittered. Two bugs: the running average was biased to drift low, and — the real cause — the displayed number had no buffer against normal noise nudging the average back and forth across a whole-percent line. Both are fixed; the number now settles and stays put.

### 📦 Installation
Open **[rastapopolix.github.io/tessera](https://rastapopolix.github.io/tessera/)** in Chrome or Edge, connect the FM-1 by USB and press **Install**, then keep the tab open and the FM-1 connected until the install window says Done. The installer can also put the official M-VAVE firmware back. Or take `tessera-3.5.3.fwsc` (dual core) or `tessera-3.5.3-1core.fwsc` from this release.

### ⚠️ Good to know
* **The project format has not changed** since 3.5: projects, presets, settings and learned MIDI CCs carry over both ways, and what the 3.5 notes say about going back to 3.4.1 still applies.
* The identity stays **FM-1_835**: M-UPGRADE 3.11 and later recognise it, and the web installer offers 3.5.3 as an update.
