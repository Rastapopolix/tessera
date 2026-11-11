# Testing the Tessera 3.5 beta

Thank you for trying the beta. This is a checklist for testers: **pick one area, or a few, and run the steps.** Each
area has a short tag in its heading (for example `[KEYS]`, `[BUS]`, `[STEP]`); you do not need to do them all. What is
new in the beta is in [BETA_NOTES.md](BETA_NOTES.md) (the installer shows it too), and the full description is in the
[Manual](../MANUAL.md).

Items marked ✓ were checked by the maintainer on beta7; unmarked ones need testers most.

## Known issues (please don't report these again)

- **Web editor overwrites projects on connect** ([#609](https://github.com/Rastapopolix/tessera/issues/609)): don't connect the web editor to the FM-1 until this is fixed.
- The DIR row flickers while a mod moves it (known issue, being fixed).
- ENTRIES ORDER allows two tracks to have the same rank (known issue, being fixed).
- ARP HOLD does not latch the held chord (known issue, being fixed).
- A mod row with a source but no destination shows no activity; it works once a destination is set (known issue, being fixed).

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

`[SETUP]` `[START]` `[3.4.1]` `[KEYS]` `[GEN]` `[FOLLOW]` `[NOTES]` `[ENTRIES]` `[ARP]` `[CHORD]` `[STEP]` `[BUS]` `[PATCH]` `[SCOPE]` `[SCALE]` `[MIDI]` `[SCREEN]` `[USB]` `[BETA6]` `[BETA7]` `[ANSWER]` `[KNOWN]` `[BACK]`

Steps are written for a **fresh project** (SAVE > PROJECT > new, or power-cycle without loading). "Tap" is a quick
press, "hold" is press and keep pressed. KNOB 1..4 are the four knobs. Where a step says "(Setup X)", do that block
from `[SETUP]` first. Tick the boxes as you go if you copy the list to your own notes. The main checks are in **bold**.

---

## [SETUP] Setup (reused below)

- **S-track**: pick the track with ALGORITHM (or the left knob, if MENU > TRACK KNOB is LEFT). The HOME tiles show T1..T4.
- **S-gen** (a generator on a track): select the track, tap SEQ (lands on GEN), KNOB 1 to the mode name asked for
  (CHORD, TURING, EUCLID, ..., BOUNCE, ANSWER). Press PLAY. For a fresh pad track use T1 (GLASS PAD); for drums, the
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

## [START] Before you start
- [ ] Back up projects and settings (web editor) on your current firmware. ✓ checked on beta7
      1. In the web editor, connect the FM-1 and export projects and settings to files.
      **Expect:** files saved on your computer.
- [ ] Install 3.5-beta7 from the installer's Beta choice; the version screen and the splash (small, bottom right) say 3.5-beta7. ✓ checked on beta7
      1. Open the installer in Chrome or Edge, connect by USB, choose Beta, press Install and wait for Done.
      2. Watch the splash on boot; then hold HOME for the menu and find the version row.
      **Expect:** both say 3.5-beta7.
- [ ] A few 3.4.1, beta4 and beta5 projects load and play the same as before. ✓ checked on beta7
      1. SAVE > PROJECT, pick an old project, load, press PLAY.
      **Expect:** it sounds as you remember it; nothing is refused.
- [ ] Settings (COLOR, MIDI, MUTES, recording) survived the update. ✓ checked on beta7
      1. S-menu: check COLOR, MUTES, NOTES OUT, MIDI settings against what you noted before updating (or the backup you exported).
      **Expect:** same values.

## [3.4.1] 3.4.1 fixes (if you're coming from 3.4)
- [ ] The FM-1 shows up over USB as **Tessera FM-1**; Tessera In / Tessera Out audio still work. ✓ checked on beta7
      1. Connect by USB. Look at the MIDI device list in your DAW or in the web installer's port chooser.
      2. Open the audio device list.
      **Expect:** the MIDI port is "Tessera FM-1" (re-pick it once if an app remembers "Tessera"); audio devices are
      "Tessera In" and "Tessera Out", and sound passes through them.
- [ ] Screen reader: GEN on an ARP track says ARP; TURING PROB 0 says LOCK; ENTRIES 0 says OFF. ✓ checked on beta7
      1. Turn the screen reader on (MENU, accessibility row). Select a track, tap SEQ, GEN KNOB 1 to ARP.
      2. Switch the track to TURING; turn KNOB 3 (PROB) all the way down.
      3. Tap GLO to ENTRIES (GLO pages); turn KNOB 1 to 0.
      **Expect:** it says ARP; says LOCK at 0; says OFF at 0.

## [KEYS] Key grammar *(new in beta5: re-learn these)*
- [ ] **OCT+ held** on any page: the header shows the sheet's name and its line fills, then the sheet opens (~0.7 s). A tap shows nothing. ✓ checked on beta7
      1. Tap SEQ for GEN on a STEP track. Hold OCT+ and keep holding.
      **Expect:** the header names the sheet, a line fills, then the sheet opens (~0.7 s) and stays after you let go.
      2. Tap OCT+ quickly on another page.
      **Expect:** no sheet, no header line.
- [ ] On a page with nothing to open (e.g. FORM): OCT+ held says **NO SHEET**, and letting go doesn't Enter. ✓ checked on beta7
      1. Select a generator track. Tap ARP until FORM. Hold OCT+ for a second, then let go.
      **Expect:** the screen says NO SHEET; you stay on FORM.
- [ ] **OCT- held never goes Back**, on any page; a quick tap only closes what is open (beta6: a tap on a page with nothing open does nothing, it no longer goes HOME). ✓ checked on beta7
      1. On FORM, TIMING, ENERGY, CHAIN (GLO pages), MIXER and a sound page, hold OCT- for 2 seconds, let go.
      **Expect:** you're still on the same page each time.
      2. On a page you entered with OCT+ (or with a sheet or list open), tap OCT- quickly.
      **Expect:** it goes Back / closes the sheet or list.
      3. On a top-level page with nothing open (e.g. SEQ's GEN), tap OCT-.
      **Expect:** nothing happens (HOME has its own button).
- [ ] Turning a knob while an OCT key is held: letting go of the key does nothing else (no Enter, no Back). ✓ checked on beta7
      1. On TWEAK, hold OCT-, turn KNOB 2, let go. Then hold OCT+ on a page with a sheet, turn a knob, let go.
      **Expect:** no page change either time (the sheet stays open if it had opened).
- [ ] The **OCT- light** is on only on pages with a second set of knobs (GEN with a generator, TWEAK, EVOLVE, FORM, TIMING, ENERGY, ✓ checked on beta7
      CHAIN, MIXER, MODS, MIDI OUT, ENTRIES, PIECE USER), and dark elsewhere (e.g. NOTES). On HOME it still shows the octave.
      1. On a generator track visit GEN, EVOLVE, FORM, TIMING, ENERGY, TWEAK, then GLO pages MIXER, MIDI OUT, ENTRIES, CHAIN;
         tap LFO for MODS; GLO > PIECE with SHAPE USER. Watch the OCT- light.
      2. Visit NOTES on a STEP track, and HOME.
      **Expect:** light on in step 1; off on NOTES; on HOME it shows the octave.
- [ ] Now on **OCT- held**: FORM (DEVELOP, CHAIN, SKIP, ROLE), TIMING (SPLIT, HOW), ENERGY (DEPTH), CHAIN (REPEAT), MIXER (PAN), ✓ checked on beta7
      MODS (KNOB 3 CC number, KNOB 4 GATE threshold).
      1. On a generator track: ARP button to FORM, hold OCT-, turn KNOBs 1..4. Repeat on TIMING, ENERGY.
      2. GLO pages to CHAIN: hold OCT-, KNOBs 1..4. MIXER: hold OCT-, KNOBs 1..4.
      3. MODS: make a mod to a track's CC (S-mod, destination CC), hold OCT-, turn KNOB 3. Make a GATE mod (tap OCT+ until ^),
         hold OCT-, turn KNOB 4.
      **Expect:** labels change to DEVELOP/CHAIN/SKIP/ROLE; SPLIT/HOW; DEPTH; REPEAT; pans; CC number; threshold. Letting go
      returns the usual knobs on the same page.
- [ ] **GEN, OCT- held**: KNOB 1 RANDOM; KNOB 2-4 the engine's extra knobs (TURING..MORPH: SOURCE, KEEP, GRAVITY; QUAD: COL, MOVE, ✓ checked on beta7
      EVERY; CONSTL: LOCK, LEN, FILL; ATLAS: KICK, SNR, HAT; ANSWER: SOURCE, ORDER, SHIFT; CHORD: STRUM, DIR, OCT).
      1. S-gen with each generator in turn. On GEN hold OCT- and read the four labels.
      **Expect:** the labels match the list for each engine, KNOB 1 always RANDOM; on release the main knobs return.
- [ ] **GEN sheet** (OCT+ held): QUAD Range, CONSTL Size, ATLAS fourth part, ANSWER Space, CHORD Chord / Voicing / Breath. ✓ checked on beta7
      1. S-gen QUAD, hold OCT+ on GEN. Then CONSTL, ATLAS, ANSWER, CHORD.
      **Expect:** each sheet lists those rows; KNOB 1 picks, KNOB 2 changes. (CHORD with USER progression shows Length where
      Breath was.)
- [ ] ATLAS part keys and CHORD USER's chord entry work with OCT- held. ✓ checked on beta7
      1. S-gen ATLAS on the drum track, hold OCT- on GEN and press a black key.
      2. S-gen CHORD, KNOB 2 PROG to USER, hold OCT- on GEN and press four white keys one after another, release OCT-.
      **Expect:** 1: the fourth part's sound changes (TOM, CLAP, ...), keys silent. 2: keys silent while held; on release
      the progression plays those four roots.
- [ ] **EVOLVE, OCT- held**: KNOB 1 ROLL ALL, KNOB 3 PAT GEN, KNOB 4 GRAVITY / HOW. ✓ checked on beta7
      1. S-gen TURING. ARP button to EVOLVE. Hold OCT-: turn KNOB 1, 3, 4 and read labels.
      2. Switch the track to STEP and look again on EVOLVE.
      **Expect:** ROLL ALL / PAT GEN / GRAVITY on a generator; on a STEP track KNOB 4 is HOW (DRIFT, ADORN, SERIAL).
- [ ] **TWEAK, OCT- held**: RANDOM, then FOLLOW's HOW, RHYTHM, HOLD. ✓ checked on beta7
      1. ARP button to TWEAK on a generator track. Hold OCT-: read labels and turn each knob.
      **Expect:** RANDOM, HOW (SAME..CANON8), RHYTHM (OWN, SOURCE, BOTH), HOLD (OFF, T1..T4).
- [ ] **MIDI OUT**: OCT- held is TRANSPOSE; SOUND and DRUM MAP per track are on the MIDI OUT sheet (OCT+ held). ✓ checked on beta7
      1. Tap GLO to MIDI OUT. Hold OCT-, turn KNOB 1: the display shows TRANSPOSE (e.g. CH 1 ROUT -24 style).
      2. Let go, hold OCT+, KNOB 1 to Sound T1 / Drum map T1, KNOB 2 to change.
      **Expect:** TRANSPOSE on OCT-; Sound and Drum map rows on the sheet. No both-keys layer.
- [ ] **ENTRIES**: OCT- held is ROLE; ORDER per track is on the ENTRIES sheet. ✓ checked on beta7
      1. Tap GLO to ENTRIES. Hold OCT-: KNOB 1..4 change ROLE per track. Let go, hold OCT+: Order T1..T4.
      **Expect:** ROLE on OCT-; ORDER on the sheet.
- [ ] **PIECE**, SHAPE USER, OCT- held: KNOB 1 picks one of 8 points (drawn bigger), KNOB 2 sets its level. ✓ checked on beta7
      1. GLO pages to PIECE. KNOB 1 LENGTH = 10 MIN, KNOB 2 SHAPE = USER. Hold OCT-, turn KNOB 1 then KNOB 2.
      **Expect:** a point is drawn larger as you pick it; KNOB 2 moves its level 0..100 % with the shape in view.
- [ ] Screen reader says the sheets and layers as they open. ✓ checked on beta7
      1. Screen reader on; hold OCT+ on GEN; hold OCT- on GEN.
      **Expect:** it names the sheet / layer as it opens.

## [GEN] Generators and EVOLVE
- [ ] **PAT GEN** (EVOLVE, OCT- held, KNOB 3): different generators on two patterns; switching pattern switches the generator. ✓ checked on beta7
      1. T1 on GEN: MODE TURING, pattern 1 (SEQ pages to PATTERNS shows the 1..8 cells). ARP to EVOLVE, hold OCT-, KNOB 3 = ON.
      2. Go to PATTERNS (hold SEQ), pick pattern 2, tap SEQ for GEN and set MODE EUCLID. Press PLAY.
      3. Switch between pattern 1 and 2 on PATTERNS.
      **Expect:** pattern 1 plays TURING, pattern 2 EUCLID; the GEN page shows the mode of the pattern you switch to.
- [ ] PAT GEN with SONG switching the patterns; survives save, power off and on, and load. ✓ checked on beta7
      1. SEQ pages to SONG: queue pattern 1 then 2 (see SONG page keys). Play it.
      2. SAVE + REC, power off and on, load the project, play.
      **Expect:** the generator changes with the SONG; both setups are intact after reload.
- [ ] **ROLL ALL** (EVOLVE, OCT- held, KNOB 1): every generator track gets a new seed; one step back undoes it. ✓ checked on beta7
      1. Two generator tracks (T1 TURING, T2 MARKOV), PLAY. Note the ROLL number on each EVOLVE page (KNOB 4).
      2. On EVOLVE hold OCT-, turn KNOB 1 right; then left one click.
      **Expect:** both tracks' ROLL numbers change and the tunes change; left puts back the old seeds (once).
- [ ] ROLL and ROLL ALL stopped: NEW LINE. Playing: NEW LINE AT BAR, and the change lands on the bar. ✓ checked on beta7
      1. Stopped: turn KNOB 4 (ROLL) and do ROLL ALL.
      2. Playing: do the same.
      **Expect:** stopped says NEW LINE; playing says NEW LINE AT BAR, the number blinks, and the tune changes at the next bar.
- [ ] **RANDOM** (OCT- held on GEN or TWEAK, KNOB 1 right): new values; turning left puts them back. Never changes MODE, the seed or FOLLOW. ✓ checked on beta7
      1. S-gen EUCLID, hold OCT- on GEN, KNOB 1 right; look at KNOB 2..4 values. Then KNOB 1 left.
      2. Repeat on TWEAK (SHIFT, DROP, HUMAN) stopped and playing.
      **Expect:** screen says RANDOMIZED, then PUT BACK; MODE, seed and FOLLOW stay. A STEP track says NO GENERATOR.
- [ ] GEN on a generator: OCT+ held does **not** open the STEP pattern sheet *(new in beta5)*. ✓ checked on beta7
      1. S-gen TURING. On GEN hold OCT+.
      **Expect:** a generator sheet if the engine has one, otherwise NO SHEET; never the pattern sheet. (PATTERN page still has it.)
- [ ] **ANSWER from a generator** *(new in beta5)*: BOUNCE on T1, ANSWER on T2 with SOURCE T1: the picture and the answer follow what T1 plays (not one note). ✓ checked on beta7
      1. T1 S-gen BOUNCE, BALLS 3. T2 S-gen ANSWER. On T2 GEN hold OCT-, KNOB 2 SOURCE = T1. PLAY.
      **Expect:** T2 shows LISTENING, takes up to 8 of the balls' notes, then answers them.

## [FOLLOW] FOLLOW *(new in beta5)*
- [ ] **Mute the followed track** (MIXER or GLO mute): the following track keeps playing its notes; the muted track is silent. ✓ checked on beta7
      1. T1 S-gen TURING. T2 ARP button to TWEAK, KNOB 4 FOLLOW = T1. PLAY.
      2. Tap GLO to MIXER; press the black key F#3 (mute T1).
      **Expect:** T1 is silent, T2 keeps following T1's notes.
- [ ] Unmute it: no jump or doubled notes. ✓ checked on beta7
      1. Press F#3 again.
      **Expect:** T1 returns in time with no stutter.
- [ ] The same with ANSWER from a muted source and QNT CHRD from a muted CHORD track. ✓ checked on beta7
      1. T3 S-gen ANSWER with SOURCE = T1; mute T1.
      2. T1 S-gen CHORD; T2 SCL page QNT = CHRD; mute T1.
      **Expect:** ANSWER still answers; T2 still snaps to the muted chord track's chords.

## [NOTES] NOTES
- [ ] OCT+ on an empty step says **NOTHING THERE** (no note is inserted) *(new in beta5)*. ✓ checked on beta7
      1. S-step with an empty pattern, NOTES page, move the cursor to an empty step (KNOB 1). Tap OCT+.
      2. On a DRUM track grid, tap OCT+ on an empty cell.
      **Expect:** NOTHING THERE; no note appears. KNOB 2 (or a key) places a note; on the grid KNOB 3 or a white key.
- [ ] OCT+ short press on a note opens the note sheet; a hold opens PATTERN SHEET. ✓ checked on beta7
      1. Cursor on a note, tap OCT+. Back (OCT-). Hold OCT+.
      **Expect:** note sheet; then, on hold, "PATTERN SHEET" appears when the hold counts and the pattern's sheet opens.

## [ENTRIES] ENTRIES
- [ ] **ENTRIES ORDER** (now on the ENTRIES sheet): AUTO and 1ST..4TH; tracks come in in that order and leave in reverse. ✓ checked on beta7
      1. Four tracks playing (any generators). GLO pages to ENTRIES: KNOB 1 to 60 %. Hold OCT+: Order T1 = 4TH, T4 = 1ST,
         others AUTO.
      2. PLAY and listen through a few phrases (CHAIN > PHRASE short helps; GLO pages to CHAIN).
      **Expect:** T4 (1ST) comes in first and leaves last; T1 (4TH) comes in last and leaves first.
      With every track on AUTO it behaves as before.

## [ARP] ARP
- [ ] **PRINT ARP**: hold a chord on an ARP track, hold REC after two bars; the arpeggio goes into STEP and plays on. EDIT held undoes it. ✓ checked on beta7
      1. Select a track, tap SEQ, KNOB 1 = ARP. Tap SEQ for the ARP page. Hold a C chord (3 keys). PLAY.
      2. After two bars, hold REC.
      3. Then hold EDIT.
      **Expect:** the track switches to STEP and plays the same notes without keys held; EDIT held brings the ARP back
      (hold the keys again).
- [ ] ARP ORD plays in the order pressed; **DNUP** down then up; **RND1** a repeating random order that changes when a key is added. ✓ checked on beta7
      1. ARP page KNOB 1 = ORD. Press E, C, G in that order and hold.
      2. KNOB 1 = DNUP with C E G. 3. KNOB 1 = RND1; STOP and PLAY twice; add a key.
      **Expect:** ORD plays E C G; DNUP G E C E G E C ...; RND1 repeats one order, changes when you add a key, and is the same
      after STOP and PLAY.

## [CHORD] Chords
- [ ] **New CHRD kinds** *(new in beta5)*: SUS2, 7SUS4, ADD9, MAJ6 after POW (SCL > CHORD); the CHORD page names them (Csus2, C7sus4, Cadd9, C6). ✓ checked on beta7
      1. S-gen CHORD on a track. Tap SCL until the CHORD page. KNOB 1/2 for CHRD: step past POW.
      **Expect:** SUS2, 7SUS4, ADD9, MAJ6 are listed after POW; the CHORD generator page names the chord (Csus2, C7sus4, Cadd9, C6).
- [ ] The new kinds with OPEN and +OCT voicings, on chord keys, MIDI in and the CHORD generator. ✓ checked on beta7
      1. Set CHRD = ADD9, VOIC = OPEN, press a key on a normal track; then VOIC = +OCT. Repeat with MIDI in, and the CHORD generator.
      **Expect:** chords sound as named, voiced open or with the extra octave.
- [ ] **STRUM** (CHORD generator: GEN, OCT- held, KNOB 2) at 20 ms and up, and up to 250 ms on a slow pad *(range new in beta6)*; DIR (KNOB 3) UP, DOWN, ALT; notes end together; MIDI OUT strums too. ✓ checked on beta7
      1. S-gen CHORD, HIT = BEAT. Hold OCT- on GEN, KNOB 2 to 40 ms, KNOB 3 UP; then DOWN, then ALT. Try HIT = STEP.
      2. KNOB 2 up to 250 ms (by 1 ms to 10, by 5 to 60, then by 10) on a pad: a slow roll.
      3. MENU NOTES OUT = ALL, and watch MIDI notes in a DAW.
      **Expect:** notes start staggered (UP lowest first, DOWN highest first, ALT alternating each strike); short stabs strum
      tighter; all notes end together; MIDI notes arrive staggered.

## [STEP] STEP direction and mods
- [ ] **Direction row** on the pattern's sheet: FWD, BACK, PEND, FOLD, RAND, DRUNK; RAND and DRUNK replay the same after STOP/PLAY; saved. *(FOLD new in beta7, it replaces PONG)* ✓ checked on beta7
      1. S-step with 6 ascending notes. On NOTES hold OCT+ (pattern sheet), Direction row (KNOB 1 then KNOB 2) through each.
      2. Choose RAND, STOP, PLAY twice. 3. SAVE, power cycle, load.
      **Expect:** each direction plays as described (FOLD plays from the ends inwards: 1, 6, 2, 5, 3, 4); RAND and DRUNK play the same line after each PLAY; direction is kept.
      There is no PONG row any more. (A pattern saved with PONG on beta4/5 now plays FOLD.)
- [ ] **DIR as a mod destination**; a GATE mod flips it while its source is high. ✓ checked on beta7
      1. S-step track. S-mod: source T1 LFO, destination DIR, OCT+ tap until ^ (GATE), amount 20 %. S-lfo slow.
      **Expect:** the melody plays BACK while the LFO is high and FWD while it is low.
      2. *(new in beta6)* Hold OCT+ on NOTES for the pattern's sheet while the mod runs.
      **Expect:** the Direction row shows the pattern's own and the one playing, e.g. FWD>BACK, moving with the mod.
- [ ] **HOLD mods** (OCT+ tap on MODS cycles +- / + / GATE / HOLD): an LFO into a filter steps once a bar. ✓ checked on beta7
      1. S-mod: T1 LFO -> T1 CUTOFF, amount 60 %, tap OCT+ until "=" (HOLD). Slow-ish LFO. PLAY.
      **Expect:** the cutoff changes in steps, once per bar, not smoothly.

## [BUS] MIX buses *(new in beta5)*
- [ ] **Two LFOs into BUS A** (SUM), and BUS A as a GATE onto FILL. ✓ checked on beta7
      1. S-mod cord 1: T1 LFO -> BUS A (end of KNOB 3 list), 100 %. Cord 2: T2 LFO -> BUS A. (Use different LFO rates on T1 and T2.)
      2. Cord 3: source BUS A (end of KNOB 1 list), OCT+ until ^, destination FILL (near the end of KNOB 3's list).
      3. Give a STEP track a step with condition FILL (NOTES > note sheet) and PLAY.
      **Expect:** the FILL step plays only when BUS A is above the threshold; the GLO FILL light follows.
- [ ] A track's GATE into BUS B with LAG 200 ms, BUS B onto CUTOFF: an envelope. ✓ checked on beta7
      1. Cord: T1 GATE -> BUS B. Cord: BUS B -> T1 CUTOFF 60 %. On MODS hold OCT+: BUS B Lag = 200 ms.
      **Expect:** the cutoff rises and falls smoothly with each note instead of jumping.
- [ ] **MODS sheet** (OCT+ held): Bus A/B MODE (SUM, MAX, MIN, DIFF) and LAG; KNOB 1 row, KNOB 2 value; OCT+ tap still cycles the mod type. ✓ checked on beta7
      1. On MODS hold OCT+: change each row. Tap OCT- to close. Then tap OCT+ on a cord.
      **Expect:** four rows (Mode A, Lag A, Mode B, Lag B); tapping OCT+ cycles the cord type.
- [ ] MAX / MIN / DIFF with two gates act as OR / AND / XOR. ✓ checked on beta7
      1. Two GATE mods at +100 into BUS A (T1 GATE, T2 GATE). BUS A read as GATE onto FILL (or a LEVEL). Mode MAX, then MIN, then DIFF.
      **Expect:** MAX: either gate opens it; MIN: both; DIFF: one but not both.
- [ ] Marked sources read BUA^, SW3^, TD2^, MC1^ (was BUS^, SWR^ ...). ✓ checked on beta7
      1. On MODS, set GATE type on cords from BUS A, SWARM 3, TIDE 2, MACRO 1.
      **Expect:** the source names show as BUA^, SW3^, TD2^, MC1^.
- [ ] Save, power cycle, load: buses come back. The same project on beta4 or 3.4.1 is refused (expected). ✓ checked on beta7
      1. SAVE + REC, power off and on, load. 2. (Optional) install 3.4.1 and try to load it.
      **Expect:** buses and settings intact; older firmware refuses the file.

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

## [MIDI] MIDI
- [ ] **MIDI OUT DRUM MAP** (now on the MIDI OUT sheet): GM and ROW; ROW drives a drum machine as a row of notes. ✓ checked on beta7
      1. MENU NOTES OUT = ALL. Drum track on ATLAS. GLO to MIDI OUT, hold OCT+, Drum map T4: GM, watch MIDI notes (36, 38, 42);
         then ROW.
      **Expect:** GM sends kick 36, snare 38, closed hat 42; ROW sends one note per drum in a row from 36 (kick, snare, rim, clap, -, low tom...).

## [SCREEN] Screen, colours and the web editor
- [ ] **MENU > GROUND LIGHT** *(new in beta5)*: every palette on paper; readable pages, menus, sheets and the splash; DARK as before; saved. ✓ checked on beta7
      1. S-menu, scroll past COLOR to GROUND, KNOB 2 to LIGHT. Cycle COLOR through all inks and the four MOSAIC sets.
      2. Visit HOME, pages, sheets, and power-cycle for the splash.
      **Expect:** warm paper background; inks are the darker versions; everything readable; DARK is black as before; LIGHT is still set after power-off.
      On paper WHITE is pure black and is named **BLACK** in COLOR *(beta6)*.
- [ ] **On paper, one red and one yellow** *(new in beta6)*: COLOR lists PINK (not also HOT PINK) and YELLOW (not also ORANGE) on LIGHT; MOSAIC, SUNSET, OCEAN, NEON's four track colours still distinct. ✓ checked on beta7
      1. With GROUND LIGHT, open COLOR's list: look for WHITE (now BLACK), RED/HOT PINK and YELLOW/ORANGE. Pick RED, then switch GROUND to DARK.
      **Expect:** WHITE reads BLACK; one red (PINK), one yellow (YELLOW); the four-track sets show four different colours; a chosen RED or ORANGE comes back on DARK.
- [ ] Web editor: user presets in the Sound list; Save to file uses the preset's own name.
      1. In the web editor Sound tab, find an ORACLE or TRIO user preset in the list. Use Save to file on an FM6 user preset.
      **Expect:** presets listed; the file is named after the preset, not TINE EP.

## [USB] USB audio
- [ ] **USB OUTPUT 4 TRACKS**: four stems in a DAW (Mac: works directly; Windows: Live needs an ASIO wrapper for more than 2 channels). ✓ checked on beta7
      1. S-menu: USB OUTPUT = 4 TRACKS (FM-1 reconnects). In the DAW pick Tessera Out as input, enable inputs 1..4, arm four tracks.
      2. Play four different tracks and record.
      **Expect:** each audio track holds one FM-1 track, mono, dry of shared reverb/delay; a muted track is silent in its stem.
- [ ] The volume knob mutes the speaker without changing the stems. ✓ checked on beta7
      1. While recording, turn the FM-1 volume knob down to 0.
      **Expect:** the speaker goes quiet; the recorded stems keep their level.

## [BETA6] Since beta5: beta6 *(new in beta6)*
- [ ] **MODS scroll flicker fixed**: scrolling through the cords on MODS no longer flickers. ✓ checked on beta7
      1. Fill a few cords (S-mod), tap LFO for MODS, play, and turn KNOB 2 through all 16 cords, fast and slow.
      **Expect:** the screen stays steady while the rows change.
- [ ] **FX mod card and MODS AMOUNT flicker fixed**. ✓ checked on beta7
      1. Hold FX and turn a knob on any page to make a cord (the FX mod card). Then on MODS turn KNOB 4 (AMOUNT) back and forth.
      **Expect:** no flicker on the card or on the AMOUNT row.
- [ ] **OCT taps**: a held OCT- is never Back; an OCT- tap only closes what is open; an OCT+ tap only enters. ✓ checked on beta7
      1. On TWEAK, hold OCT- for a second, let go. 2. On a page with nothing open, tap OCT-. 3. On a page with nothing to enter, tap OCT+.
      **Expect:** 1: still on TWEAK. 2: nothing happens (no jump to HOME). 3: nothing happens, no sheet.
- [ ] **FM6 map opens on the last page used**. ✓ checked on beta7
      1. Select an FM6 track, open the EDIT map, go into an operator row and a page (e.g. EG RATE), Back out, move to another operator row, enter.
      **Expect:** each row lands on the page you used last there, not on whichever column the row before had.
- [ ] **MOD SCOPE reads calmly**: a calm line over a dotted source; GATE wedges at the edges with the full line while adjusting; MOSAIC colours by source track. ✓ checked on beta7
      1. Cord T1 LFO fast -> CUTOFF; hold OCT- on MODS. 2. Make it a GATE (OCT+ tap until ^) and turn KNOB 4. 3. MENU > COLOR = MOSAIC, cords from T2 and T3.
      **Expect:** 1: a smooth line, not a smear, over a dotted source. 2: small wedges at each edge, filled while open; the full dashed line appears only while KNOB 4 turns. 3: line in the source track\x27s colour.
      (The same checks are in MOD SCOPE above.)
- [ ] **STEP DIR**: recorded takes follow DIR; a tied note going backwards plays once; a mod on DIR shows on the sheet. ✓ checked on beta7
      1. S-step, record (REC) six notes with one held long enough to tie over steps. Pattern sheet Direction = BACK, then PEND. PLAY.
      2. Put a mod on DIR (S-mod, destination DIR) and hold OCT+ on NOTES.
      **Expect:** 1: every step plays its own recorded notes while the playhead goes backwards; the tied note sounds once, from the tie\x27s last step. 2: Direction reads like FWD>PEND.
- [ ] **STRUM up to 250 ms** (see Chords above) and **Clear mod** on the MODS sheet. ✓ checked on beta7
      1. Cord with a SCALE or a CC set. On MODS hold OCT+: first row **Clear mod** (red); the header names the picked mod (M3 at its right). Tap OCT+ on it.
      **Expect:** the mod is empty (no source, destination, amount); other cords play on; an empty mod says NOTHING THERE. No undo.
- [ ] **GROUND LIGHT**: WHITE becomes BLACK; one red and one yellow on paper (see Screen above). ✓ checked on beta7

## [BETA7] Since beta5: beta7 *(new in beta7)*
- [ ] **BARS knob style keeps a top margin at maximum**. ✓ checked on beta7
      1. MENU > KNOBS style BARS. Turn a knob to its maximum on a page with bars (e.g. GEN, TWEAK, MIXER levels).
      **Expect:** a full bar leaves a one-pixel gap at the top of its outline, same as at the sides and bottom.
- [ ] **FOLD replaces PONG** (see STEP direction above). ✓ checked on beta7
      1. Pattern sheet Direction on a pattern of 8 ascending notes: choose FOLD, PLAY.
      **Expect:** 1 8 2 7 3 6 4 5, and again. No PONG in the list.
- [ ] **Rename while playing**: the name screen opens while playing (keys type, never sound); only the final save needs a stop. ✓ checked on beta7
      1. SAVE > PROJECT, Rename (sheet), with PLAY running. Type a name on the keys.
      2. OCT+ to save.
      **Expect:** 1: the name screen opens and the music keeps going; keys type and are silent. 2: OCT+ says STOP TO SAVE and the screen stays; PLAY stops, OCT+ saves.
- [ ] **Web simulator** no longer lists SID KIT / SID DIGI in DRUM *(simulator only; the device already lacked them)*. ✓ checked on beta7
      1. Open the web simulator, pick a DRUM track\x27s sound list.
      **Expect:** no SID KIT or SID DIGI.
- [ ] **Docs refresh (optional)**: FEATURES, GLOSSARY and RECIPES updated. ✓ checked on beta7
      1. Skim FEATURES and GLOSSARY. Try two recipes tagged (3.5 beta) in RECIPES on the device.
      **Expect:** the recipes work as written; note any step that is wrong.

## [ANSWER] ANSWER chain and ring test *(new)*
- [ ] **T1 a generator, T2 to T4 ANSWER chained, then T2\x27s SOURCE set to T4** (a ring): no hang, no stuck notes, no crash. ✓ checked on beta7
      1. T1 S-gen TURING (or BOUNCE), PLAY.
      2. T2 S-gen ANSWER, GEN hold OCT-, KNOB 2 SOURCE = T1. T3 ANSWER, SOURCE = T2. T4 ANSWER, SOURCE = T3.
      **Expect:** each answers the one before: T2 shows LISTENING then answers T1, T3 answers T2, T4 answers T3.
      3. Set T2\x27s SOURCE = T4 (the ring closes). Play for a minute, STOP, PLAY, mute and unmute T3.
      **Expect:** the music keeps playing; the display stays steady; STOP and PLAY work; nothing hangs or sounds a stuck note. (It may settle to rests: say what you hear.)

## [KNOWN] Known, still being checked
- [ ] USB stems soft clip (#603): needs a check on a device (USB OUTPUT 4 TRACKS with a hot mix; do the stems clip hard or round off?).
- [ ] The web editor finds an FM-1 port by its old name (#605): try it in Chrome on a Mac (port still called "FM-1").

## [BACK] Going back (optional)
- [ ] A beta5 or beta7 project without bus mods opens on 3.4.1: new chord kinds play as power chords, strum together, direction FWD, HOLD as +-. ✓ checked on beta7
      1. Make a project with CHRD = ADD9, STRUM 40, a direction, a HOLD mod; no bus, EOC, CLOCK, PLAY or MOD DEP mods. Save.
      2. Install 3.4.1 and load.
      **Expect:** it loads; chords are POW, strum plays together, direction FWD, HOLD plays as +-.
