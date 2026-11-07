**Tessera 3.4** follows [Tessera 3.3.2](https://github.com/Rastapopolix/tessera/releases/tag/v3.3.2): Tessera can now say what is on its screen, so the FM-1 can be played without seeing it. Around it: new ways for a track to follow another, a PIECE that keeps real time and comes home at the end, and USB audio named the right way round. Projects, presets and settings from 3.3 carry over.

### 🔊 Screen reader
* **Tessera speaks its screen** through the [FM-1 Screen Reader](https://charlesvestal.github.io/fm1-reader/): plug the FM-1 in by USB, open the page in Chrome or Edge and click Connect. It speaks with its own voice or hands the words to VoiceOver, NVDA, JAWS or TalkBack. Thanks to João Durães for asking (issue #430), and to Charles Vestal for the reader and its protocol.
* It says each page as you open it, a knob's name and value once you stop turning it, list and menu rows with their place, the header's messages, PLAY and STOP, yes/no questions and their keys, the GLO and SCL layers, and a step when you press its key while stopped. PATTERNS, SONG, the sound browser, the PROJECT, USER and STORE slots and SCALES say what is selected.
* Tessera's abbreviations are said as words ("T1" is "track 1"), from a words file on the Tessera site. Tessera is silent until the reader connects.
* **The help cards are gone** to make room for it (ARP held + a knob). Every knob is described in the [manual](https://github.com/Rastapopolix/tessera/blob/main/MANUAL.md) under its page and in the [parameter reference](https://github.com/Rastapopolix/tessera/blob/main/docs/PARAMETERS.md). Holding ARP and turning a knob now just turns it.

### 🎼 FOLLOW: HOW and RHYTHM
* **TWEAK, OCT+ held**: KNOB 2 **HOW** and KNOB 3 **RHYTHM**, for a track that FOLLOWs another.
* **HOW**: **SAME** (as before), **CHORD** (the third, fifth or seventh of the chord on the source note, whichever is nearest), **CONTRA** (moves the other way, by scale degrees), **BASS** (an octave or two down, on beats 1 and 3) and **CANON1 to CANON8** (the source's note that many notes back).
* **RHYTHM**: **OWN** (as before), **SOURCE** (plays only when the followed track has played) or **BOTH**.
* A new recipe: a harmony on your own melody.

### 🧭 PIECE
* **A piece keeps real time.** It counts the time played, so a BPM change, a TEMPO mod or clock drift never makes the shape jump, and a piece lasts its LENGTH in minutes, whatever the tempo.
* **It comes home.** In the last 5 % of a piece, generator notes lean towards the root and the fifth, more so near the end, the same each PLAY.
* **Fewer RESTS near the top** of the shape, so the loudest part stays full.
* **NEW SONG** roles now also set each track's ENTRIES ROLE (DRUMS → RHYTHM, BASS → BASS, CHORDS and PAD → HARMONY, LEAD → MELODY), so the right parts leave and come back first.

### 🎹 USB audio
* **MENU > USB AUDIO** is named from the FM-1's side: **IN** is the computer playing into the FM-1, **OUT** is the computer recording the FM-1. The computer's devices are named the same way: **Tessera In** plays into the FM-1, **Tessera Out** records it. **USB INPUT** is now **USB OUTPUT**. Thanks to pavig-coder (issue #494).

### 📦 Installation
Open **[rastapopolix.github.io/tessera](https://rastapopolix.github.io/tessera/)** in Chrome or Edge, connect the FM-1 by USB and press **Install**, then keep the tab open and the FM-1 connected until the install window says Done. The installer can also put the official M-VAVE firmware back. Or take `tessera-3.4.fwsc` (dual core) or `tessera-3.4-1core.fwsc` from this release.

### ⚠️ Good to know
* **USB audio devices swap names.** A DAW or system setting that picked "Tessera In" or "Tessera Out" by name now points at the other one: pick it again once. Your menu choice keeps doing what it did, and only its label changes (if you had chosen IN to record the FM-1, it now shows OUT).
* **No help cards**: see above.
* **Projects from 3.3 load** and play as before. A project saved on 3.4 with a FOLLOW HOW or RHYTHM other than SAME and OWN still opens on 3.3.x, which plays it as SAME and OWN.
* The identity is **FM-1_834**: M-UPGRADE 3.11 and later recognise it, and the web installer offers 3.4 as an update.

### 🙏 Thanks
To João Durães, Charles Vestal and pavig-coder, and to the people on Reddit who said a screen reader mattered more than the help cards.
