**Tessera 3.5.2** follows [Tessera 3.5.1](https://github.com/Rastapopolix/tessera/releases/tag/v3.5.1): a USB audio fix, a steadier CPU meter, and SELECT now pages consistently everywhere, including on MIXER. Everything in the [3.5](https://github.com/Rastapopolix/tessera/releases/tag/v3.5) and [3.5.1](https://github.com/Rastapopolix/tessera/releases/tag/v3.5.1) notes still applies.

### 🔌 USB audio: no more resets on a short or full ring
* When the USB audio ring ran empty or full, it used to reset and wait 512 frames before sending audio again — an extra silence on top of whatever caused it. Now an underrun pads only the missing frames (fading out fast), an overrun drops the oldest frames back to the target, and either way the stream crossfades back in over 32 frames instead of cutting. The ring never resets mid-stream any more.
* The voices sounding are now also capped by how much CPU time is actually being spent, not only by BUDGET's fixed weights. A busy project (FX, USB, a costly patch) that used to overload, shed a voice, and overload again now settles on a steady ceiling instead of clicking on every shed.

### 🎚️ SELECT always pages, MIXER included
* **SELECT now pages consistently everywhere**, including on MIXER: 3.5.1 made SELECT turn the tempo on MIXER instead of paging (the same thing HOME does), which meant SELECT couldn't reach or leave MIXER, and scrolling past PERF skipped it. SELECT on MIXER now changes pages like everywhere else.
* **Tempo on MIXER is still one knob-turn away**: hold GLO and turn SELECT, the same combo that already adjusts tempo on every other page.
* The GLO order also changes: **LANDSCAPE moves to the end of the shaping pages** — MIXER, PERF, PIECE, CHAIN, ENTRIES, LANDSCAPE, SYSTEM, MIDI IN, MIDI OUT.

### 📊 A steadier CPU meter
* The CPU% bar and number on the SYSTEM page no longer jitter from one reading to the next; a brief spike still shows (nothing about the real load reporting changed), but the displayed reading settles rather than flickering.
* The CPU column also no longer looks like something you could turn — it's a reading, not a setting.

### 📦 Installation
Open **[rastapopolix.github.io/tessera](https://rastapopolix.github.io/tessera/)** in Chrome or Edge, connect the FM-1 by USB and press **Install**, then keep the tab open and the FM-1 connected until the install window says Done. The installer can also put the official M-VAVE firmware back. Or take `tessera-3.5.2.fwsc` (dual core) or `tessera-3.5.2-1core.fwsc` from this release.

### ⚠️ Good to know
* **The project format has not changed** since 3.5: projects, presets, settings and learned MIDI CCs carry over both ways, and what the 3.5 notes say about going back to 3.4.1 still applies.
* The identity stays **FM-1_835**: M-UPGRADE 3.11 and later recognise it, and the web installer offers 3.5.2 as an update.

### 🙏 Thanks
To Dominic, for the USB audio report and the Health readings that pinned down the cause.
