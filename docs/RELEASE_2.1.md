**Tessera 2.1** follows [Tessera 2.0.2](https://github.com/Rastapopolix/tessera/releases/tag/v2.0.2): a mixer you can play,
humanized timing with its own knob, easier-to-read screens and steadier MIDI clock sync. Projects, presets and settings
from 2.0, 2.0.1 and 2.0.2 carry over.

### ✨ New
* **MIXER is one 4-channel mixer.** **KNOB 1–4** set the levels of tracks 1–4, and with **OCT+** held they set the four
  pans. The keys work as in the GLO layer without holding anything: the black keys F#3 G#3 A#3 C#4 mute and unmute
  tracks 1–4, the white keys F3 G3 A3 B3 solo a track while held, C4 unmutes all and F4 taps the tempo. On MIXER the
  keys play no notes. The strips show SOLO or MUTE, and a track silenced by another's solo dims.
* **HUMAN TIME on TIMING (KNOB 4, 0–60 ms).** Each step plays up to that many milliseconds late: a new amount each step,
  never early, at most a quarter of a step, and replayed the same from the SEED. It works on every track, STEP tracks
  included, and cords can move it (destination **H TIME**). **TWEAK HUMAN** now shapes velocity and length only, and
  TWEAK opens on STEP tracks for HUMAN.

### 👀 Easier to read
* The small text is a weight heavier and secondary text is brighter, on every screen.
* A setting stays readable when it is zero, OFF or stopped (QUAD, POLY, AWAKE, ANSWER, CHORD, TWEAK); only what is
  really inactive is dimmed. CHORD writes diminished chords as **°** (B°, C°7).
* The page footer is the same everywhere: the engine name, then the sound's name.
* **PANEL GAMMA** now defaults to the panel's own **G2.2**. A unit that was left on the old default (TUNED+VCOM) comes
  up in G2.2; one that picked another curve keeps it. TUNED+VCOM is still in MENU > PANEL GAMMA.

### 🔧 Fixes and improvements (some from Melodee 1.5)
* **MIDI clock sync survives a busy input.** If a burst of MIDI overflows the input queue, the clock pulses lost with it
  are recovered, so the sequencer and the beat stay in phase instead of falling behind. The arpeggiator catches up on
  the steps it missed.
* **FM6 imports the DX7 voice of a 231-byte MainStage-style file** (`F0 43 0n 7D 04 08 …`), over USB and in the web
  editor. It saves and sends back as a standard DX7 voice.
* Smaller audio job slots (about 1 KB more audio memory) and a more compact FM6 pitch table (1.7 KB less flash). The
  sound is unchanged.

### 📦 Installation
Open **[rastapopolix.github.io/tessera](https://rastapopolix.github.io/tessera/)** in Chrome or Edge, connect the
FM-1 by USB and press **Install**. The installer now shows the install in a window with its stages and a progress bar:
keep the tab open and the FM-1 connected until it says Done. It can also put the official M-VAVE firmware back. Or take
`tessera-2.1.fwsc` (dual core) or `tessera-2.1-1core.fwsc` from this release.

### ⚠️ Good to know
* Projects saved with 2.1 still open in 2.0.x, which ignores their HUMAN TIME setting.
