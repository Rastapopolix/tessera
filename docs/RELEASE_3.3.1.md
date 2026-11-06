**Tessera 3.3.1** follows [Tessera 3.3](https://github.com/Rastapopolix/tessera/releases/tag/v3.3) with a few fixes and one changed default. Everything in the [3.3 notes](https://github.com/Rastapopolix/tessera/releases/tag/v3.3) still applies, and projects, presets and settings carry over as they are.

### 🎹 Changed: the drum track's MIDI channel
* **A drum track now follows ROUT like the other tracks**: with ROUT at CH1-4, track 4 sends and listens on **channel 4**, not 10. **SYSTEM > DRUM** now starts at **OFF**.
* Want General MIDI's drum channel? Set **SYSTEM > DRUM** to **10**, as before. A channel set per track on **GLO > MIDI OUT** still wins over both.

### 🛠️ Fixed
* **ANSWER keeps your timing.** A call played on the keys goes on the nearest step, as recording into STEP does, so a note played a touch early no longer comes back a whole step early.
* **EDIT on NOTES (and a DRUM track's grid) moves on the way you were going.** After a delete, the cursor moves the way KNOB 1 last turned it, so tap after tap clears a run to the left as well as to the right.
* **GEN's MODE is a dial on STEP and ARP tracks too**, as it is for every generator, showing where the mode sits on the knob.
* **The dials' pointer reaches the ring at every angle.** Before, it stopped short when level or upright.

### 📦 Installation
Open **[rastapopolix.github.io/tessera](https://rastapopolix.github.io/tessera/)** in Chrome or Edge, connect the FM-1 by USB and press **Install**, then keep the tab open and the FM-1 connected until the install window says Done. The installer can also put the official M-VAVE firmware back. Or take `tessera-3.3.1.fwsc` (dual core) or `tessera-3.3.1-1core.fwsc` from this release.

### ⚠️ Good to know
* **If a drum machine or a DAW listens to Tessera's drums on channel 10**, set **SYSTEM > DRUM** to **10** after updating. That includes units where DRUM was set to 10 by hand, since 10 was the default. Any other DRUM channel you chose stays as it was.
* **The project format has not changed** since 3.3. What the [3.3 notes](https://github.com/Rastapopolix/tessera/releases/tag/v3.3) say about going back to 3.2.x still applies. Going back to 3.3 from 3.3.1, DRUM reads as 10 again unless you chose another channel.
* The identity stays **FM-1_833**: M-UPGRADE 3.11 and later recognise it, and the web installer offers 3.3.1 as an update.
