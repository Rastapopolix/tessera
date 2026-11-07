**Tessera 3.4.1** follows [Tessera 3.4](https://github.com/Rastapopolix/tessera/releases/tag/v3.4) with fixes to the screen reader and to how the FM-1 names itself over USB. Everything in the [3.4 notes](https://github.com/Rastapopolix/tessera/releases/tag/v3.4) still applies.

### 🔊 The screen reader says what the screen shows
Every knob on every page, for every engine and every MODE, was checked: what the reader says now matches what the screen shows. Fixed:
* **GEN on an ARP track** says MODE **ARP** (it said STEP).
* **TURING's PROB at 0** says **LOCK**; **ENTRIES**, **STEP EVOLVE's AMOUNT** and **FORM's FLOURISH** at 0 say **OFF** (they said a number).
* **MORPH**: POSITION says its place as shown (64 of 127, not 50 %), and RHYTHM A and B their step as shown (they said one less).
* **ANSWER's FADE** says the number shown.
* **FM6's STORE page**: SEND and INIT are said (they were silent).

The check now runs with every build, so a value said differently from the screen can't slip in again.

### 🔌 The FM-1 shows up as "Tessera FM-1"
* Over USB the FM-1 now names itself **Tessera FM-1** instead of Tessera. Other FM-1 firmwares' web installers look for "FM-1" in the name, so they couldn't find an FM-1 running Tessera, and switching to another firmware didn't work without picking the port by hand. Thanks to keremimo for spotting it.
* If an app remembered the MIDI port as "Tessera", pick it again once. The USB audio devices keep their names, **Tessera In** and **Tessera Out**.

### 📦 Installation
Open **[rastapopolix.github.io/tessera](https://rastapopolix.github.io/tessera/)** in Chrome or Edge, connect the FM-1 by USB and press **Install**, then keep the tab open and the FM-1 connected until the install window says Done. The installer can also put the official M-VAVE firmware back. Or take `tessera-3.4.1.fwsc` (dual core) or `tessera-3.4.1-1core.fwsc` from this release.

### ⚠️ Good to know
* **The project format has not changed** since 3.4: projects, presets and settings carry over, and what the 3.4 notes say about going back still applies.
* The identity stays **FM-1_834**: M-UPGRADE 3.11 and later recognise it, and the web installer offers 3.4.1 as an update.
* The update loader is unchanged.
