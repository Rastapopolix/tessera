# Testing the Tessera 3.5 beta

Thank you for trying the beta. This is a checklist for testers: **pick one area, or a few, and run the steps.** Each
area has a short tag in its heading (for example `[PATCH]`, `[SCOPE]`, `[SCALE]`); you do not need to do them all. What is
new in the beta is in [BETA_NOTES.md](BETA_NOTES.md) (the installer shows it too), and the full description is in the
[Manual](../MANUAL.md).

The maintainer has already checked the other items on a device with beta7, so this list holds only what still needs testers.

## Known issues (please don't report these again)

- **Web editor overwrites projects on connect** ([#609](https://github.com/Rastapopolix/tessera/issues/609)): don't connect the web editor to the FM-1 until this is fixed.
- "MEMORY FULL" with audio crackles after long, heavily evolving projects with REC armed ([#612](https://github.com/Rastapopolix/tessera/issues/612)): the crackle on each captured note is fixed in 3.5-rc1; the message itself is most likely the recording store filling up (turn REC off when you are done overdubbing).

## How to report

Open an issue on GitHub: <https://github.com/Rastapopolix/tessera/issues/new>. Please include:

- **Firmware version**: MENU > ABOUT (hold HOME for the menu).
- **What you did**: the steps, numbered as far as you can (the numbered steps below are a good model).
- **What you expected** (the **Expect:** line, or what you thought would happen).
- **What happened** instead.
- **A project file or a photo of the screen** if it helps (SAVE > PROJECT, or export from the web editor).

One issue per problem, please. Check the [existing issues](https://github.com/Rastapopolix/tessera/issues) first: if
yours is there, add what you saw to it. A step that works as written is worth a line too (a comment is enough).

## What the beta is

A test build of the next Tessera, 3.5, to be tried and pushed before the release. Tessera is experimental firmware: use
it at your own risk, and do not unplug the FM-1 while it writes.

## Install, and the way back

1. **Back up first.** In the [web editor](https://rastapopolix.github.io/tessera/webapp/editor/), connect the FM-1 and
   export your projects and settings to files.
2. **Install the beta** with the [web installer](https://rastapopolix.github.io/tessera/webapp/installer/) (Chrome or
   Edge, FM-1 connected directly by USB): choose **Beta**, press Install and wait for Done.
3. **No FM-1 to hand?** The [web simulator](https://rastapopolix.github.io/tessera/webapp/sim/) has a **Beta** choice
   too. Most steps below work there; the audio, USB and MIDI ones need the device.
4. **To go back to 3.4.1,** open the installer again and choose the 3.4.1 release (the installer page also has a way
   back to the official firmware, with a backup first). Projects saved with beta-only features may not load on older
   firmware; the steps say where. Keep your backup files.

## Areas

`[SETUP]` `[PATCH]` `[SCOPE]` `[SCALE]` `[SCREEN]` `[KNOWN]`

Steps are written for a **fresh project** (SAVE > PROJECT > new, or power-cycle without loading). "Tap" is a quick
press, "hold" is press and keep pressed. KNOB 1..4 are the four knobs. Where a step says "(Setup X)", do that block
from `[SETUP]` first. Tick the boxes as you go if you copy the list to your own notes. The main checks are in **bold**.

---

## [SETUP] Setup (reused below)

- **S-track**: pick the track with ALGORITHM (or the left knob, if MENU > TRACK KNOB is LEFT). The HOME tiles show T1..T4.
- **S-gen** (a generator on a track): select the track, tap SEQ (lands on GEN), KNOB 1 to the mode name asked for
  (CHORD, TURING, EUCLID, ..., BOUNCE, ANSWER). Press PLAY. For a fresh pad track load GLASS PAD on T1 with PRESETS (T1 starts on KUGO HARP); for drums, the
  DRUM track (808 KIT; check HOME).
- **S-step** (a STEP track with notes): select a track, tap SEQ, GEN KNOB 1 = STEP, tap SEQ again for NOTES. Press a few
  white keys with REC armed (tap REC, PLAY, play keys, tap REC), or use KNOB 1 to move the cursor and KNOB 2 to place
  notes. Four to eight notes are enough.
- **S-mod** (make a cord): tap LFO (lands on MODS). KNOB 2 picks the cord (1 to 16), KNOB 1 the source, KNOB 3 the
  destination (list for the *selected* track, then project things at the end), KNOB 4 the amount.
  Quicker: hold FX and turn a knob on any page.
- **S-lfo** (an LFO that moves): select a track, tap LFO until the LFO page, turn KNOB 1/2 for rate and depth so the
  mark on MODS visibly moves. Set a slow rate (a few seconds) when a test says "slow".
- **S-menu**: hold HOME to open the menu; KNOB 1 picks a row, KNOB 2 changes its value, tap OCT- to leave.
- **S-sheet**: hold OCT+; KNOB 1 picks the row, KNOB 2 changes it; OCT- (tap) closes.

## [PATCH] Patching primitives *(new in beta5)*
- [ ] **EOC** (last in a track's source list on MODS): an LFO's EOC as a GATE mod onto FILL or a bus; on a slow LFO a short pulse each cycle.
      1. S-lfo: T1 LFO slow (~4 s a cycle). S-mod: source T1 EOC (last in T1's list), OCT+ until ^, destination FILL (or BUS A).
      **Expect:** the mod's light blinks briefly once per LFO cycle (high for an eighth of the cycle).
- [ ] **CLOCK**: EOC onto a generator's CLOCK gives a note each LFO cycle; add S&H onto that LFO's rate (a Krell). AMOUNT 50 % = every second edge; a GATE source counts hits; STOP and PLAY start it again.
      1. T1 S-gen TURING. S-mod: T1 EOC -> T1 CLOCK (destination CLOCK), amount 100 %.
      2. Add: T1 S&H -> T1 LFO RTE (the LFO rate), amount 40 %.
      3. Set the CLOCK cord's amount to 50 %.
      4. Change the source to T2 GATE (T2 on some generator): the cord counts T2's hits.
      5. STOP, PLAY.
      **Expect:** 1: one note per LFO cycle. 2: the note spacing wanders. 3: every second cycle. 4: steps on every hit of T2's gate
      (or every Nth by the amount). 5: first step plays at PLAY.
- [ ] **Scaling mods of mods**: hold OCT- on a mod of mods (M1 AMT .. M16 AMT), KNOB 3 right: the row shows M3*; a T2 GATE opens and closes mod 3 (an LFO onto CUTOFF at 60 %); KNOB 3 left adds again.
      1. Cord 3: T1 LFO -> T1 CUTOFF, 60 %. Cord 5: source T2 GATE (T2 plays a generator), destination M3 AMT, amount 100 %.
      2. Select cord 5, hold OCT-, KNOB 3 right.
      3. KNOB 3 left.
      **Expect:** the row shows M3*; cord 3 moves the cutoff only while T2 plays; left gives the adding behavior again.
      Optional: replace cord 5's source with T4 ENV at -100 %: the cutoff sweep is shaped by T4's notes.
- [ ] **MOD DEPTH** (GLO > PERF, OCT+ held, its sheet): a feedback patch braked to dry at 0 %; offsets stay; a MACRO onto MOD DEP at -100 %; a GATE onto FILL lets go as MOD DEPTH passes 50 %.
      1. Make a feedback patch: T1 GATE -> T2 PITCH-ish setting (e.g. T2 E PULSE), T2 GATE -> T1 setting. Tap GLO to PERF; hold OCT+:
         MOD DEPTH 100 %, then 0.
      2. Cord: source MACRO 1 -> destination MOD DEP (end of the KNOB 3 list), amount -100 %. Turn PERF KNOB 1 up.
      3. Add a GATE mod onto FILL at 100 %; sweep MOD DEPTH across 50 %.
      **Expect:** at 0 the patch is dry (fixed offsets stay); MACRO 1 up fades the patch out; the FILL light drops below 50 %.
- [ ] **PLAY**: a TIDE or LFO GATE onto a pad's PLAY gates the voice in one cord; a track FOLLOWing a shut leader keeps playing.
      1. Pad track with a generator. S-mod: source T1 LFO (slow), OCT+ until ^, destination T1 PLAY (near the end of the list), +100 %.
      2. T2 TWEAK FOLLOW = T1.
      **Expect:** T1 sounds only while the LFO is high; T2 keeps playing T1's notes when T1 is silent.
- [ ] **HOLD on notes**: hold OCT- on a HOLD mod, KNOB 1 to HOLD ON T2 NOTES: T1 RAND as HOLD onto T3 CUTOFF steps on T2's notes; save, load.
      1. T2 on a generator. Cord: source T1 RAND, type HOLD (=), destination T3 CUTOFF, 60 %.
      2. Select it, hold OCT-, KNOB 1 to HOLD ON T2 NOTES.
      3. Save, power off and on, load.
      **Expect:** T3's cutoff jumps to a new value on each T2 note; setting survives the reload.
- [ ] **MIXER sheet** (OCT+ held on MIXER): Mutes NOW, BAR, PHRASE; matches MENU > MUTES; mute keys land on the bar or phrase.
      1. Tap GLO to MIXER; hold OCT+; KNOB 2 to BAR. Open S-menu and check MUTES says the same. Press a black key while playing.
      **Expect:** the same value in both places; the mute lands on the next bar (or phrase).
- [ ] A project using EOC, CLOCK, PLAY or a mod onto MOD DEP is refused on beta4 / 3.4.1 (expected).
      1. (Optional, on older firmware.) Load the saved project.
      **Expect:** refused.

## [SCOPE] MOD SCOPE *(new in beta5)*
- [ ] **Hold OCT- on MODS**: the picked mod's row stays, a scope shows its move (solid) over its source (dashed); let go and the rows return.
      1. A cord T1 LFO -> T1 CUTOFF at 60 %. Playing. On MODS hold OCT-.
      **Expect:** the cord's row at top; below it a graph: a line for the move over a dotted source (calm, no smear even on a fast LFO) *(beta6)*; release brings back the four rows.
- [ ] Turn KNOB 4 (GATE threshold) while holding: the dashed threshold line moves against the source.
      1. Change the cord to GATE (OCT+ tap until ^). Hold OCT-, turn KNOB 4.
      **Expect:** a GATE shows a wedge at each edge (filled while the gate is open) *(beta6)*; while KNOB 4 turns, the full dashed line crosses the source and moves.
- [ ] A HOLD mod steps once a bar; a slow mod (TIDE, DRIFT) widens the window; a BUS A cord shows the bus.
      1. HOLD cord: hold OCT-. 2. Cord from TIDE 1, then DRIFT. 3. A cord from BUS A.
      **Expect:** HOLD draws a staircase per bar; TIDE shows up to 64 bars, DRIFT 16; the bus is drawn as the source.
- [ ] **MOSAIC colours by source track** *(new in beta6)*: with a MOSAIC set the scope line takes its source's track colour (a bus, macro or project source in the main ink).
      1. MENU > COLOR = MOSAIC. Cords from T2 LFO and from T3 LFO; hold OCT- on each.
      **Expect:** each line in its track's colour; a cord from BUS A in the main ink.
- [ ] Smooth while playing (frame rate), readable on DARK and LIGHT ground.
      1. While playing, hold OCT- for a while. Then MENU > GROUND = LIGHT and repeat.
      **Expect:** smooth movement; readable in both.
- [ ] **Pinned** *(new in 3.5-rc2)*: hold OCT-, tap OCT+, let go of OCT-: the scope stays and says SCOPE PINNED.
      1. On MODS with an LFO cord, hold OCT-, tap OCT+, let go. Turn KNOB 4 (the amount); KNOB 2 to another mod.
      **Expect:** the scope stays while the knobs work as without OCT- (the amount changes the line); another mod gets its scope. Tap OCT-: the rows come back, SCOPE FREE, and you stay on MODS. Pin it again and leave MODS: coming back shows the rows.
- [ ] Screen reader says it once as it opens (e.g. SCOPE 2 BARS, CYCLING 2 A BAR, LOW -60 HIGH 60 %), not again while held.
      1. Screen reader on; hold OCT- on MODS for several seconds.
      **Expect:** it speaks once on opening, not repeatedly.

## [SCALE] Scales
- [ ] **USER scales** (SCL > SCALES, USER): a key adds or removes its note; generators, QNT and FOLLOW play in it; saved.
      1. Tap SCL (SCALES). KNOB 1 to USER family, KNOB 2 to USER1. Press F#, G, and others: the screen says e.g. USER1 F# IN; press a key again for OUT.
      2. S-gen TURING on a track, PLAY. Set QNT on another STEP track; FOLLOW.
      3. SAVE, power off and on; load.
      **Expect:** generators play only the picked notes; the root can't be removed; scale is restored after reload.
- [ ] **With QNT on** (SNAP, WHITE, ALL...), the keys on SCALES still light by the USER scale and play their own note *(new in beta5, #559)*.
      1. QNT = SNAP (TEMPO page QNT or SCL). On SCALES with USER1 picked, look at lit keys and play a key.
      **Expect:** lit keys match the scale; each key plays its own note.

## [SCREEN] Screen, colours and the web editor
- [ ] Web editor: user presets in the Sound list; Save to file uses the preset's own name.
      1. In the web editor Sound tab, find an ORACLE or TRIO user preset in the list. Use Save to file on an FM6 user preset.
      **Expect:** presets listed; the file is named after the preset, not TINE EP.

## [KNOWN] Known, still being checked
- [ ] **CPU meter** *(new in 3.5-rc2)*: GLO > SYSTEM's CPU shows two numbers, average/peak (e.g. 55/92 %).
      1. Load a busy demo (two FM6 or ORACLE tracks), PLAY, open GLO > SYSTEM.
      **Expect:** the second number is higher and jumps with busy moments; at 99 you may hear crackles. Mute the busiest track: both numbers drop within a second.
- [ ] **A track you can't hear rests** *(new in 3.5-rc2)*: mute a track with a long REVERB or ECHO tail, wait, unmute.
      1. A pad with REVERB on T1, playing. Mute T1 (GLO, black key) for a few bars, then unmute.
      **Expect:** it fades back in on time, no click or old echo burst. T2 FOLLOW T1 keeps following while T1 is muted.
- [ ] USB stems soft clip (#603): needs a check on a device (USB OUTPUT 4 TRACKS with a hot mix; do the stems clip hard or round off?).
- [ ] The web editor finds an FM-1 port by its old name (#605): try it in Chrome on a Mac (port still called "FM-1").

