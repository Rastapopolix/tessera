**Tessera 3.7.5** follows [Tessera 3.7.4](https://github.com/Rastapopolix/tessera/releases/tag/v3.7.4): the FM-1 now shows you when it runs out of time, and a new echo that keeps to the beat. Everything in the [3.7](https://github.com/Rastapopolix/tessera/releases/tag/v3.7), [3.7.1](https://github.com/Rastapopolix/tessera/releases/tag/v3.7.1), [3.7.2](https://github.com/Rastapopolix/tessera/releases/tag/v3.7.2), [3.7.3](https://github.com/Rastapopolix/tessera/releases/tag/v3.7.3) and [3.7.4](https://github.com/Rastapopolix/tessera/releases/tag/v3.7.4) notes still applies.

### 🚦 Overload blink
* **The header's four track dots blink when the FM-1 runs out of time**: once when it faded a voice to catch up, twice when the sound came out late (the crackle you hear). At most one blink every 5 seconds. On HOME, which has no dots, they show for a moment where the engine and the battery are.
* **GLO > SYSTEM** counts both since power-on: **FADED** and **LATE**.
* **MENU > OVERLOAD** turns the blink **OFF** (it's **BLINK** by default). FADED and LATE still count with it off.

### 🔁 Effects
* **SYNC ECHO**, a new FX type for the insert and send slots: ECHO with its TIME set as a note value at the tempo playing (1/32 up to 1/1, with triplets and dotted values, one knob step each). It follows BPM, a MIDI clock or a TEMPO mod, and glides when the tempo changes. The FM-1's echo holds up to 371 ms, so a longer note value halves until it fits (a 1/4 at 120 BPM repeats on the 1/8s).

### 💾 Projects
* **A project from another Tessera target keeps its extra parts** through a save on the FM-1 (for example tracks 5 to 8 of a project made in the Tessera plugin), so it goes back to that target whole. The FM-1 says **EXTRAS KEPT** when it loads one, and **EXTRAS LOST** if a save no longer has room for them beside the music.

### 📦 Installation
Open **[rastapopolix.github.io/tessera](https://rastapopolix.github.io/tessera/)** in Chrome or Edge, connect the FM-1 by USB and press **Install**, then keep the tab open and the FM-1 connected until the install window says Done. The installer can also put the official M-VAVE firmware back. Or take `tessera-3.7.5.fwsc` (dual core) or `tessera-3.7.5-1core.fwsc` from this release.

### ⚠️ Good to know
* **Projects from 3.7.4 load and play the same.** A project with a SYNC ECHO opens on 3.7.4 or older, which plays that effect as DRIVE.
* The OVERLOAD setting is saved with the other settings; older firmware ignores it.
* The identity stays **FM-1_837**: M-UPGRADE 3.11 and later recognise it, and the web installer offers 3.7.5 as an update.
