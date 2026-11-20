**Tessera 3.7.4** follows [Tessera 3.7.3](https://github.com/Rastapopolix/tessera/releases/tag/v3.7.3): chords that move on the strikes you pick, and a web editor, simulator and manual that are easier to read and work with a screen reader. Everything in the [3.7](https://github.com/Rastapopolix/tessera/releases/tag/v3.7), [3.7.1](https://github.com/Rastapopolix/tessera/releases/tag/v3.7.1), [3.7.2](https://github.com/Rastapopolix/tessera/releases/tag/v3.7.2) and [3.7.3](https://github.com/Rastapopolix/tessera/releases/tag/v3.7.3) notes still applies.

### 🎼 Harmony
* **Move when and Move by**, two new rows on CHORD's GEN sheet (OCT+ held), for BEAT, STEP and OFFBEAT hits. **Move when** picks which strikes move: LAST (the bar's last, the "and" of 4 with OFFBEAT), LAST 2, ODD, CHANGE (the last strike before the chord changes) or RAND (one a bar, from the SEED, so PLAY replays it). **Move by** picks how far: -2 to +3 semitones, +1 or +2 degrees in key, an inversion up or down, an octave, TOP (the top note a degree up) or LEAD (a semitone below the next chord). The next strike is home again. A reggae skank pushed up a whole step on the and of 4 is Move when LAST, Move by +2 ST. Move when is OFF by default, and HIT HOLD never moves.

### 🌐 Web editor, simulator and manual
* **The web editor is easier to read**: bigger text, softer body colour and roomier rows, larger touch targets, and the 6-OP FM table fits the window. The editor and the web simulator now have a **LIGHT / DARK** button.
* **Screen readers**: the editor's controls are named in words with their page (for example "ENV: attack (ATK)"), and sliders say the value shown. The **web simulator** can speak what its screen shows, in the firmware's own words, as the FM-1's screen reader does.
* **The editor shows its grouped pages again** with 3.7 firmware. Since 3.7 it fell back to a plain list of parameters.
* **The manual describes each picture** for screen readers, rather than just naming the screen.

### 📦 Installation
Open **[rastapopolix.github.io/tessera](https://rastapopolix.github.io/tessera/)** in Chrome or Edge, connect the FM-1 by USB and press **Install**, then keep the tab open and the FM-1 connected until the install window says Done. The installer can also put the official M-VAVE firmware back. Or take `tessera-3.7.4.fwsc` (dual core) or `tessera-3.7.4-1core.fwsc` from this release.

### ⚠️ Good to know
* **Projects from 3.7.3 load and play the same.** A project with Move when set opens on 3.7.3 or older, which plays its chords without the moves.
* The identity stays **FM-1_837**: M-UPGRADE 3.11 and later recognise it, and the web installer offers 3.7.4 as an update.
