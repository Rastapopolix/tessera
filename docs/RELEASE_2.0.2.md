**Tessera 2.0.2** is a small fix to [Tessera 2.0.1](https://github.com/Rastapopolix/tessera/releases/tag/v2.0.1): starting a
new project while something plays now works, on the FM-1 and in the simulator. Projects, presets and settings carry over
from 2.0 and 2.0.1 as they are.

### 🔧 Fixes
* **New song stops playback.** SAVE > PROJECT > **New song**, then **OCT+**, used to show only a small "STOP FIRST" while
  a project played, and did nothing else: in the simulator the demo just kept playing. It now stops playback itself, as
  SAVE + REC already does, and goes on to "Start a new song?" (when there are unsaved changes) and the NEW SONG screen.

### 📖 Manual and simulator
* The **manual** describes how to start a new project (it was missing): SAVE > PROJECT, **KNOB 1** to **New song**,
  **OCT+**, then **OCT+** for Yes to "Start a new song?", the key, a role per track, and **OCT+** to create it.
* The **simulator** page gives the same steps with the computer keys (SAVE `8`, OCT+ `]`, OCT- `[`) under the demo
  button.

### 📦 Installation
Open **[rastapopolix.github.io/tessera](https://rastapopolix.github.io/tessera/)** in Chrome or Edge, connect the
FM-1 by USB and press **Install**. The installer can also put the official M-VAVE firmware back. Or take
`tessera-2.0.2.fwsc` (dual core) or `tessera-2.0.2-1core.fwsc` from this release.
