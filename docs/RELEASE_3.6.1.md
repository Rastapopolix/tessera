**Tessera 3.6.1** follows [Tessera 3.6](https://github.com/Rastapopolix/tessera/releases/tag/v3.6): a new mod source that knows whether a track is heard, slower synced LFOs, live mod marks in Mono, project names in the editor, the harmony button called SEL as printed, and a few fixes. Everything in the [3.6 notes](https://github.com/Rastapopolix/tessera/releases/tag/v3.6) still applies.

### 🔌 MODS
* **IN, a new track source** (KNOB 1's list, after EOC): high while the track is heard, low while MUTE, another track's SOLO or ENTRIES has it out, and in between while ENTRIES fades it (STYLE FADE). A track's GATE stays on while ENTRIES has it out (its notes still play, unheard), so IN is the one to use when a mod should act only while a track is in, or, at a negative amount, only while it is out. MATRIX lists it with the other track sources. A new recipe uses it: [dub throws while the drums are out](https://github.com/Rastapopolix/tessera/blob/main/RECIPES.md#dub-throws-while-the-drums-are-out-in-361).
* **Live marks in Mono**: a knob the mods move now shows where they move it: a bead riding the dial's ring (a bar: a notch beside it), redrawn as it moves. The effects' and the send effects' knobs show marks too.
* **Effect destinations named as the effect names them**: on a FILTER, **FX1 CUT** and **FX1 RES**; on an ECHO, **FX1 TIM** and **FX1 RPT** (Mono: CUT1, RES1; a send effect's S1CT ..), instead of RTE and CHR for every effect. The screen reader says them.

### 🎛️ Sound and playing
* **LFO SYNC: 8, 16, 32 and 64 bars** (LFO 2), shown first after OFF, slowest first: one cycle can now last a whole long-form piece.

### 🖥️ On the screen and the editor
* **SEL, as printed on the FM-1**: wherever the physical harmony button is meant, it is now called SEL: [SEL] SET, [SEL] LOCK, [SEL] DONE, the button test (PRESS SEL), the manual, cheatsheet and recipes, and the web simulator's button and help. SCL stays the scale page and the scale parameter.
* **LANDSCAPE**: during a PIECE the moon fills evenly as it climbs: a thin crescent as it rises, full at the top half way through the piece, then the other thin crescent as it sets. Its phase no longer follows the PIECE's SHAPE (which filled it close to the horizon on most shapes).
* **The editor's PROJECTS** (A to D) show each slot's project name, as do the slot pickers of the demos and project files. With older firmware they show used / empty as before.

### 🔧 Fixed
* **ENTRIES ORDER**: turning a track's ORDER past a rank another track holds no longer takes it from that track; the knob swaps only where a turn stops. (T1 at 1ST, T2 turned from AUTO to 2ND no longer leaves T1 on AUTO.) The editor, MIDI and loading a project swap as before.

### 📦 Installation
Open **[rastapopolix.github.io/tessera](https://rastapopolix.github.io/tessera/)** in Chrome or Edge, connect the FM-1 by USB and press **Install**, then keep the tab open and the FM-1 connected until the install window says Done. The installer can also put the official M-VAVE firmware back. Or take `tessera-3.6.1.fwsc` (dual core) or `tessera-3.6.1-1core.fwsc` from this release.

### ⚠️ Good to know
* **Projects from 3.6 load on 3.6.1 and play the same.** A project saved on 3.6.1 still loads on 3.6, with two exceptions: a mod using **IN** is dropped there, and an LFO SYNC of **8 to 64 bars** becomes 1/32. As with 3.6, a project saved on 3.6 or 3.6.1 does not load on 3.5.3 or older.
* The identity stays **FM-1_836**: M-UPGRADE 3.11 and later recognise it, and the web installer offers 3.6.1 as an update.
