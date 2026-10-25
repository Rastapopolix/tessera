**Tessera 2.1** brings four engines back from [Felucca](https://github.com/hugelton/Felucca) and
[GHOULBOX](https://github.com/jasonpersinger/ghoulbox-fm1-dungeon-synth): ANALOG, PHASE, LOFI and GURDY. Projects, presets
and settings carry over from 2.0.x as they are.

### 🎹 New engines
* **ANALOG** (16 sounds): two band-limited oscillators (saw, square, triangle, sine, PWM, hard sync, sub), noise, drive and
  a low-pass filter.
* **PHASE** (7 sounds): phase distortion, from the author's CrispyZebra oscillator.
* **LOFI** (8 sounds): a chip voice with pulse, triangle, saw, noise and a 4-bit wave RAM, stepped volume and a sweep.
* **GURDY** (6 sounds): a hurdy-gurdy. One wheel bows the melody string and the drones; the trompette's buzz is kicked by
  the wrist strokes (COUP). Its sounds load with a reverb on effect slot 1 (and a chorus on slot 2 where GHOULBOX had
  one); other engines still leave your effects alone.

### Not in this port
The presets' effect sends and suggested patterns (Tessera has its own effect chain), ANALOG's filter type and envelope
sync, and GURDY's crank, RING and drone temperament. They need parameters and controls Tessera does not have.

### 🔁 Good to know
* LOFI takes engine id 8, the retired GRAIN's (LOFI's own id 3 is SID's in Tessera). ANALOG (0) and PHASE (2) are no
  longer retired ids.
* The app is 574,460 B of the 581,564 B slot; user presets (64) and projects are unaffected.

### 📦 Installation
Open **[rastapopolix.github.io/tessera](https://rastapopolix.github.io/tessera/)** in Chrome or Edge, connect the
FM-1 by USB and press **Install**. The installer can also put the official M-VAVE firmware back. Or take
`tessera-2.1.fwsc` (dual core) or `tessera-2.1-1core.fwsc` from this release.
