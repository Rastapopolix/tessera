# Tessera glossary

The words, labels and ideas you meet in Tessera, A to Z: the abbreviations on the screen, the names of Tessera's own
features, and the synthesis and modular terms a newcomer may not know. Where a term is a general one, the entry says
what it means in plain words first, then what it means *here*.

Entries marked **(3.5 beta)** are not in a stable release yet. For the big picture see [FEATURES.md](FEATURES.md); for
how everything works, the [MANUAL](MANUAL.md); for patches to try, [RECIPES.md](RECIPES.md); for every setting with its
range, [docs/PARAMETERS.md](docs/PARAMETERS.md).

**Jump to:** [A](#a) · [B](#b) · [C](#c) · [D](#d) · [E](#e) · [F](#f) · [G](#g) · [H](#h) · [I](#i) · [K](#k) ·
[L](#l) · [M](#m) · [N](#n) · [O](#o) · [P](#p) · [Q](#q) · [R](#r) · [S](#s) · [T](#t) · [U](#u) · [V](#v)

## Labels that mean different things in different places

Tessera has about a hundred pages and only so many short words, so a few labels are used twice. Each is explained
under its own letter; here is the list to be careful with.

- **DIR**: a pattern's playing direction (STEP), the order a chord is strummed (CHORD), how QUAD walks its grid, and
  a mod destination that changes the first.
- **GATE**: a way a cord can work, a source a generator offers, and how long a note is held (a track's gate).
- **HOLD**: a cord mode, an arpeggiator setting, a way a CHORD track plays its chords, a QUAD direction, and the FILL
  hold on the GLO keys.
- **LENGTH**: the LENGTH page (how long notes ring), TURING's LENGTH (steps in its loop) and PIECE's LENGTH (how long
  the whole piece is).
- **PLAY**: the PLAY button (start and stop), and PLAY, a mod destination that decides whether a track is heard.
- **SHAPE**: a sound's waveform shape, TIMING's roll shape, and PIECE's shape over time.
- **SOURCE**: where a cord's movement comes from, and the track or pattern a generator (TURING, EUCLID, MARKOV,
  MORPH) or ANSWER listens to.
- **DEPTH**: ENERGY's per-track DEPTH and MOD DEPTH, the whole patch's amount, are different controls.
- **ALGORITHM**: FM6's operator layout, and also the name of the knob that picks the track.
- **CLOCK**: the project's clock source and CLOCK OUT (MIDI), and CLOCK, a mod destination that steps a generator.
- **FILL**: a step condition, a key you hold, and a mod destination.
- **MODE**: what a track plays (GEN's MODE), the arpeggio's order (ARP MODE), and how a MIX bus combines its cords.
- **LAG** and **SLEW**: both smooth a jump; LAG is a MIX bus setting, SLEW a mod source. See each entry.

---

## A

**ACCUM** (accumulate). A step setting that moves the pitch up (or down) by a set amount each time the pattern comes
round, so a line climbs. In Tessera: a step's *Accumulate*, plus a pattern-wide Accumulate on the pattern's sheet.
[MANUAL](MANUAL.md#a-steps-ratchet-nudge-and-accumulate)

**ADSR** (attack, decay, sustain, release). The four stages of an envelope: how fast the sound rises, falls to a held
level, stays there while the key is down, and dies away after. In Tessera: ORACLE, TRIO and others show ATK, DEC,
SUS, REL. FM6 has operator envelopes instead (EG RATE and EG LVL). See *Envelope*.

**AEG** (amp envelope). On FM6, one knob that stretches all the carriers' envelopes together: right is a slower
attack and longer release. [MANUAL](MANUAL.md#sounds)

**ALGORITHM**. In FM, the wiring of the operators: which ones modulate which, and which are heard. FM6 has 32. In
Tessera the **ALGORITHM** knob on the panel is also the one that picks the track (swap it with PRESETS in MENU > TRACK
KNOB). [MANUAL](MANUAL.md#the-panel-at-a-glance)

**ANCHOR**. An ENTRIES role: a track that never leaves, such as a drone or bass. See *ROLE*.

**ANSWER**. A generator for call and response: you play a phrase, it plays it back, changed a little more each time.
Its SOURCE can be another track. [MANUAL](MANUAL.md#answer-source-order-shift-space)

**ARC**. ENERGY's slow tide: how far the music swings from sparse to full.

**ARP** (arpeggiator). Plays the notes you hold one after another instead of together. In Tessera: a track's MODE can
be ARP, and the **ARP** button is also the section for the pages that shape any track (EVOLVE, FORM, TWEAK and so on).
[MANUAL](MANUAL.md#seq-the-mode-and-its-pages)

**ATLAS**. A drum generator: a map of drum patterns where X and Y pick the groove, with a fourth drum part. Best on a
DRUM track. [MANUAL](MANUAL.md#atlas-kick-snr-hat-the-fourth-part)

**AWAKE**. A generator of two loops of different lengths; the second lifts the first up the scale.
[MANUAL](MANUAL.md#the-generators)

## B

**BI / UNI** (bipolar, unipolar). Bipolar swings both sides of a middle (-1 to +1), unipolar only from 0 up (0 to 1).
In Tessera: the LFO page's **POL** is BI or UNI. A cord's own **+-** (both ways) or **+** (one way) is separate, and
decides how that cord moves. See *+-*.

**BOUNCE** (and **FALL**). A generator: balls in a box, a note where one hits a wall. With more than four balls it
becomes FALL, leaves in the wind. [MANUAL](MANUAL.md#the-generators)

**BRANCH**. A TWEAK 2 setting: the chance that a hit plays on another track's sound, so two tracks answer each other.
[MANUAL](MANUAL.md#tweak-2)

**BREATH**. On a CHORD track, how often a voice of a held chord sits out a slow cycle, so the chord breathes.
(Also an FM6 controller name; see the sound pages.)

**BUDGET**. The size of the pool of voices shared by all four tracks (GLO > GLOBAL > BUDGET, 16 by default). Lower it
if notes crackle. [MANUAL](MANUAL.md#how-many-voices-at-once)

**BUS A / BUS B** **(3.5 beta)**. See *MIX buses*.

## C

**CC** (control change). A MIDI message that carries a knob position, 0 to 127. In Tessera: a controller's CC can set
a Tessera parameter (MIDI CC IN), and a mod onto a track's **CC** destination sends a CC out to another synth.
[MANUAL](MANUAL.md#midi-cc-control)

**CHAIN**. One song form for the whole project (GLO > CHAIN), so every track that follows it changes section (A, B, C)
and key at the same time. Also the track setting on FORM, FOLLOW or OWN, that decides whether it does.
[MANUAL](MANUAL.md#chain-one-form-for-the-project)

**CHORD**. A generator that plays a progression of chords in your scale. Also the **CHORD** page on SCL for the chord
kind and voicing. [MANUAL](MANUAL.md#chord-strum-dir-oct-chrd-voic-breath-or-len)

**CHRD** (chord kind). Which chord shape a note makes: triads, sevenths in the scale (DIA3, DIA7), or fixed shapes
(MAJ, MIN, DOM7, MAJ7, MIN7, SUS4, POW; SUS2, 7SUS4, ADD9, MAJ6 **(3.5 beta)**). Used by the chord keys, MIDI in, the
arpeggiator and the CHORD generator. See also *QNT* and *VOIC*.

**CLK / CLOCK**. The timing signal that keeps music in step. In Tessera: **CLK** (GLO > GLOBAL) picks the clock
source: internal, USB or TRS. **CLOCK OUT** sends MIDI clock. **CLOCK**, a mod destination **(3.5 beta)**, makes a
generator step on a cord's edges instead of its own DIV. [MANUAL](MANUAL.md#mods-the-modulation-cords)

**COLOR**. MENU > COLOR: the screen's ink. Nine single inks (YELLOW, WHITE, GREEN, BLUE, RED, ORANGE, PINK, HOT PINK,
LILAC) and four sets with a colour per track (MOSAIC, SUNSET, OCEAN, NEON). See *GROUND*.
[MANUAL](MANUAL.md#the-panel-at-a-glance)

**COND** (condition). A step setting that makes it play only some of the time: every 1st of 2 passes to every 8th of
8, or only on a FILL, or only the first pass. [MANUAL](MANUAL.md#a-steps-condition-and-fill)

**CONSTL** (CONSTELLATION). A generator: a ship flies through a sky of stars, and the stars that cross its crosshair
play. [MANUAL](MANUAL.md#constellation-lock-len-fill-size)

**CONTR** (contrast). On FORM and CHAIN: how far a new section moves from the one before it.

**Cord**. A mod: one connection from a source to a destination, as a patch cable would make. Tessera has 16.
See *MODS*.

**CV** (control voltage). On a modular, a signal that controls something. In Tessera: each generator offers a **CV**
source (its note value as a smooth movement) beside its **GATE** and **NOTE**.

**Cycle**. One full turn of a repeating thing: one wave of an LFO, one time round a loop, one tide of ENERGY. See
*EOC*.

## D

**DEVELOP**. A FORM setting: each time a section returns it varies a little. [MANUAL](MANUAL.md#form)

**DENS** (density). How many steps or hits play, or the chance that a step plays.

**DIV** (division). A track's step length as a fraction of the beat (1/4, 1/8, 1/16, triplets). Smaller is faster.
Each track and pattern has its own, which is how you get polymeter.

**DIR** (direction). Four meanings: the way a pattern plays (**STEP direction**: FWD, BACK, PEND, FOLD, RAND, DRUNK
**(3.5 beta)**); the order a CHORD strum goes (UP, DOWN, ALT **(3.5 beta)**); how a QUAD track walks its grid; and
**DIR**, a mod destination that changes a pattern's direction **(3.5 beta)**.

**Drawbars**. On a tonewheel organ, sliders that set how much of each harmonic (16', 8', 5 1/3' and so on) is in
the sound. In Tessera there is an FM6 preset named DRAWBARS that builds the same thing from six carriers; there are
no drawbar controls.

**DRIFT**. A slow wander each track has, a new point about every 12 seconds, eased between. Also a mod source. On
ORACLE, VINTAGE gives each voice its own slight drift.

**DRUM**. The engine for drum kits (808 and 909), eight lanes: BD, SD, CP, CH, OH, TM, RS, CB.
[MANUAL](MANUAL.md#sounds)

**DRUNK**. A direction **(3.5 beta)**: a random walk, a step forward or back at a time. Follows the SEED.

## E

**EDIT**. The button for the sound's own pages. Tap it again for the map of them.

**ENERGY**. A project-wide slow tide from sparse and quiet to full and loud, with occasional rests. DEPTH sets how
much it moves each track. Off at 0. [MANUAL](MANUAL.md#energy)

**ENTRIES**. Tracks drop out and come back in on phrase lines, like a band arranging itself, in an order set by each
track's ROLE. An ANCHOR never leaves. Off at 0. [MANUAL](MANUAL.md#entries-tracks-out-and-back-in)

**Envelope**. A shape that plays out over time from a trigger, usually each note: it rises, falls, holds and dies
away. It makes a sound pluck, swell or sustain, and is a standard thing to patch to a filter or volume. In Tessera:
each track has an ENV, a mod source that follows its notes. See *ADSR*.

**EOC** (end of cycle) **(3.5 beta)**. A source on every track: the LFO's cycle start as a gate, high from each wrap
of the LFO for an eighth of the cycle. Useful as a GATE mod, or onto CLOCK for a note per LFO cycle.

**Euclidean**. A rhythm that spreads a number of hits as evenly as possible over a number of steps, which gives many
traditional rhythms (3 over 8 is the tresillo). In Tessera: the **EUCLID** generator (STEPS, PULSE, ROT), and
*Euclid* on a pattern's sheet to write one into steps.

**EVOLVE**. A phrase mutates every few bars; you set how much (AMOUNT), how often (EVERY) and what is LOCKed. Also
where ROLL, ROLL ALL, PAT GEN and GRAVITY live. [MANUAL](MANUAL.md#evolve)

**EXPR** (expression). The MIDI expression controller, a mod source.

## F

**FALL**. See *BOUNCE*.

**FILL**. Three things. A step *condition* that plays only when a fill is called; a key you hold (on the GLO layer or
MIXER) to call one; and **FILL**, a mod destination, so an LFO or ENERGY can call fills by itself.
[MANUAL](MANUAL.md#a-steps-condition-and-fill)

**Filter**. Removes part of a sound's frequencies. A low-pass filter keeps the lows and removes the highs; its
*cutoff* is where, its *resonance* the emphasis there. In Tessera: TRIO, NOISE and ORACLE have their own; FM6
has none (use MLVL, or the **FILTER** effect, on any track). A mod onto **CUTOFF** moves each engine's own (MANUAL:
"CUTOFF and SHAPE per engine").

**FM** (frequency modulation). Making a sound by having one oscillator change the pitch of another at audio speed,
which adds overtones: bells, electric pianos, metal. In Tessera: the **FM6** engine.

**FOLLOW**. Two meanings. In TWEAK, a track plays a harmony line off another track's pitch, in key, even when the
leader is muted. In FORM, a track whose CHAIN is FOLLOW takes its section from the project's CHAIN (OWN keeps its
own). [MANUAL](MANUAL.md#tweak)

**FORM**. Sections for a track: A, B, C built from its own phrase, in an order like AABA or ABAC.
[MANUAL](MANUAL.md#form)

**FM6**. Tessera's six-operator FM engine, with three engine models (MODERN, MARK I, OPL). See *FM* and *Operator*.

**FX**. The effects button, and the **mod shortcut**: hold it and turn a knob to make a cord for that parameter.

## G

**GATE**. On a modular, a signal that is high while a key is held and low when released. In Tessera: a *GATE mod* is
a cord that moves its destination by its whole amount while the source is over a threshold, and not at all under it
(**^**); a generator offers a **GATE** source (high on its notes); and a track's *gate* is how long its steps hold.

**GEN**. The first SEQ page: pick the track's MODE (STEP, ARP, or one of thirteen generators) and its main knobs.
The **GEN layer** is the second set of knobs under OCT- held, and the **GEN sheet** (OCT+ held) has the rest.
[MANUAL](MANUAL.md#the-gen-layer)

**Generator**. Something that writes notes for you, in your scale and at the track's DIV. Tessera has thirteen.
Every generator is also a mod source.

**GLO** (global). The button for the whole project: MIXER, PERF, GLOBAL (tuning, voice BUDGET, clock source), SYSTEM,
MIDI OUT, PIECE, CHAIN, ENTRIES, LANDSCAPE. Held, it gives the performance keys (mutes, solos, FILL, levels, tempo).

**GRAVITY**. A pull toward a track's home notes, so a drifting line comes back. On EVOLVE (STEP EVOLVE's HOW) and on
the generators that can use a STEP seed.

**GROUND** **(3.5 beta)**. MENU > GROUND: the screen on **DARK** (black, the default) or **LIGHT** (warm paper, each
ink darkened). [MANUAL](MANUAL.md#the-panel-at-a-glance)

## H

**HOLD**. Five meanings. A *HOLD mod* takes its source once a bar (or on a chosen track's notes **(3.5 beta)**) and
keeps it, like a sample and hold (**=**). The arpeggiator's **HOLD** keeps the notes after you let go. A CHORD
track's **HIT HOLD** strikes a chord once and lets it ring. QUAD's **DIR** has a HOLD. And holding **FILL** on the GLO
keys. Say which you mean.

**HOME**. The home page: all four tracks, their sounds and four knobs each. Hold the button for the menu.

**HUMAN**. Plays each step a little late, as a player behind the beat. On TIMING (generators and STEP) and TWEAK.

## I

**Inversion**. A chord with a note other than the root at the bottom: first inversion has the third lowest. In
Tessera: INV1, INV2 on VOIC.

## K

**KEY** (source). A mod source that follows the note's pitch.

**Key link**. SCL > Key link: when LINKED (the default), changing the ROOT on one melodic track changes it on all.
[MANUAL](MANUAL.md#harmony-one-key-chord-and-qnt-chrd)

**Krell**. A classic self-playing modular patch (named after the aliens in the film *Forbidden Planet*): a slow
random source picks the notes while another random source changes how fast they come, so the music seems to play
itself. In Tessera **(3.5 beta)**: an LFO's EOC onto a generator's CLOCK plays a note each LFO cycle, and S&H onto that
LFO's rate gives every cycle its own length.

## L

**LAG**. On a MIX bus **(3.5 beta)**, a slew on the result: how long a full move takes, 10 ms to 20 s. A GATE
through a lagged bus is an envelope.

**LANDSCAPE**. A night lake drawn by the music (GLO). It changes nothing in the sound.

**Layer**. In the manual, either one of Tessera's optional parts (sounds, generators, shaping, form, mods, performing)
or the *second set of knobs* a page shows with OCT- held ("the GEN layer"). See *OCT+ / OCT-*.

**LEN**. LENGTH, in a narrow label. See *LENGTH*.

**LENGTH**. Three things. The **LENGTH** page: how long a generator's notes ring, from clipped to pads. TURING's
LENGTH: the steps in its loop (2 to 16). PIECE's LENGTH: how long the whole piece is, 1 minute to 4 hours or FREE.

**LFO** (low-frequency oscillator). A wave too slow to hear as a pitch, used to move something else: a vibrato, a
filter sweep, a tremolo. In Tessera: each track has one (LFO, LFO 2, LFO DEST), also a source on MODS.

**LINK**. A QUAD setting: how the other QUAD tracks follow this one. Not Key link.

## M

**MACRO**. One of PERF's four hand knobs. A macro does nothing alone; send it to as many cords as you like.

**MARKOV**. A generator: a random walk over scale degrees that mostly steps to neighbours with the odd leap.

**MATRIX**. The whole patch as a 4 x 4 grid of the tracks (who modulates whom), lit as cords move.
[MANUAL](MANUAL.md#matrix)

**MIDI LEARN**. MENU: put any Tessera knob on a CC of your controller; remembered.

**MIX buses** **(3.5 beta)**. **BUS A** and **BUS B**: two mixers for cords. Every cord sent into a bus is combined
there (Mode: SUM, MAX, MIN, DIFF; with gates that is OR, AND, XOR), optionally slowed by LAG, and the bus is a source
for other cords.

**MLVL** (modulator level). FM6's brightness: the level of the operators that are not carriers.

**MOD DEPTH / MOD DEP** **(3.5 beta)**. One control for how much of the whole patch plays, 0 to 100 % (GLO > PERF's
sheet). At 0 the patch is dry. **MOD DEP** is also a destination so a MACRO can ride it.

**MODE**. What a track plays: STEP, ARP, or a generator (GEN's KNOB 1). Also the ARP's order, and a MIX bus's
combining mode.

**MODS**. The modulation cords (LFO button): up to 16, each from a source on any track to a destination on any
track. A *mod of mods* (M1 AMT to M16 AMT) moves another cord's amount, or scales it **(3.5 beta)**.
[MANUAL](MANUAL.md#mods-the-modulation-cords)

**Modulation**. Using one signal to change a setting of another: an LFO moving a filter, one track's notes opening
another's. On a modular, a patch cable does this.

**MORPH**. A generator: slides from one rhythm preset to another with a single knob.

**MOSAIC**. A colour set with a colour per track (also SUNSET, OCEAN, NEON). See *COLOR*.

**MOTION**. Knob moves recorded onto a pattern while you record live.

**MUTES**. MENU > MUTES: when the mute keys take effect while playing: NOW, BAR or PHRASE. A muted track plays on
unheard, so tracks that follow it still hear it.

## N

**NO SHEET**. What the screen says when you hold OCT+ on a page that has no sheet.

**NOTES**. The page for writing steps by hand.

**NRPN**. A MIDI message with a longer number than a CC; in Tessera every parameter has an ID and can be set this
way. [MANUAL](MANUAL.md#midi-cc-control)

## O

**OCT+ / OCT-**. On the home page, octave up and down. Everywhere else the two keys go **in** and **back**: tap OCT+
to Enter the row, slot, note or cord under the cursor; tap OCT- to go Back. Hold OCT+ for the page's **sheet**; hold
OCT- for the page's **second knobs** (the layer) while held. **(3.5 beta)** [MANUAL](MANUAL.md#the-panel-at-a-glance)

**ORACLE**. The engine for a polyphonic analogue-style voice: two oscillators, noise, POLY MOD and a four-pole
low-pass filter.

**ORGANIC**. The page for TIDE and SWARM speeds.

**Operator**. In FM, one oscillator with its own envelope. One operator bends the pitch of another; FM6 has six, wired
by the ALGORITHM.

**Oscillator**. The part that makes a repeating wave, and so the pitch. Tessera's voices use oscillators for ORACLE
and TRIO, operators for FM6.

## P

**PAT GEN** **(3.5 beta)**. Each of a track's eight patterns keeps its own generator setup, so changing pattern
changes the generator. Off by default. On EVOLVE's second layer.

**PATTERN**. The sequencer page for a track's length, speed and gate. Each track has eight patterns of its own
steps; **PATTERNS** picks which plays, and **SONG** lines them up.

**PERF**. GLO > PERF: four macro knobs, and MOD DEPTH on its sheet. [MANUAL](MANUAL.md#perf-four-macros)

**PHRASE**. A group of bars (4, 8 or 16, the CHAIN's PHRASE). CHAIN, ENTRIES and PIECE change on phrase lines.

**PHYS**. The engine for physical models: bars and bells, plucked and bowed strings, drum heads.

**PIECE**. A whole-piece plan: length, shape (ARCH, RISE, FALL, PLATEAU, VALLEY, WAVES, BUILDS, STEPS, FLAT or USER)
and an end (STOP, LOOP, NEW or FREE). It steers ENERGY, CHAIN and ENTRIES along the shape.
[MANUAL](MANUAL.md#piece-a-length-and-a-shape)

**PLAY**. The button that starts the music; every generator starts its random choices from the top of its seed, so a
piece replays the same. And **PLAY**, a mod destination **(3.5 beta)** that decides whether a track is heard.

**POLY**. A generator: four notes stepped on by four rhythms, for polyrhythms. **POLY MOD** (ORACLE) lets the filter
envelope and oscillator B bend oscillator A's pitch, pulse width or the cutoff.

**PRESETS**. Browse the sounds by category, favourite or recent; the knob on the panel. **USER** slots store sounds
you edit.

**PRINT**. Hold REC: what a generator or arpeggio just played becomes ordinary steps you can edit.
[MANUAL](MANUAL.md#print-and-keep)

**PROB / PROBABILITY**. A chance, 0 to 100 %, such as a step changing or a note being skipped.

## Q

**QNT** (quantise). Snap notes to the scale, so wrong notes become right ones. **QNT CHRD** snaps to the notes of the
CHORD track's current chord, so every part stays in key with it. Also *Quantize* on the pattern's sheet, and record
quantise on TEMPO. [MANUAL](MANUAL.md#harmony-one-key-chord-and-qnt-chrd)

**QUAD**. A generator: a 4 x 4 grid of notes the four tracks share, a row each.

## R

**RANDOM**. OCT- held on GEN or TWEAK, KNOB 1: new values for the page's knobs, inside their ranges, with a way back
**(3.5 beta)**. See also the RAND source, a stream of random values.

**Ratchet**. Repeating a note several times within one step: a drum roll or stutter. In Tessera: RATCH x1 to x8 on
TIMING, and a step's own Ratchet.

**REC**. Record from the keys; held, print.

**RESTS**. ENERGY's occasional silences, 8 to 18 s.

**ROLE**. A track's part for ENTRIES: BASS, RHYTHM, HARMONY, MELODY or ANCHOR (AUTO guesses).

**ROLL / ROLL ALL**. EVOLVE's ROLL picks a new SEED, so a new line; turn back for the last one. ROLL ALL does it for
every generator track at once, with one step back **(3.5 beta)**.

**ROOT**. The key's first note.

## S

**S&H** (sample and hold). Take a value from a changing source at a moment and keep it until the next moment, so a
smooth signal becomes steps. In Tessera: **S&H** is a source on each track (random values for an LFO cycle), and a
**HOLD** cord does the same to any source, per bar.

**Scale**. A set of notes the music stays within. 77 are built in; four **USER** scales **(3.5 beta)** are yours, and
**Scala tunings** are microtonal ones from the web editor.

**SCL**. The harmony button: SCALES, the scale and key, QNT, CHORD.

**Seed**. The starting number for a generator's random choices. The same seed plays the same line.

**SELECT**. The knob that turns the pages of the section shown (on the home page, the tempo).

**SEQ**. The button for what the selected track plays.

**Sheet**. A page's extra settings, out of sight until you hold OCT+. Where a page has none, the screen says NO SHEET.

**SLEW**. Smoothing that limits how fast a value may change. In Tessera: **SLEW** is a mod source; **LAG** is the
same smoothing on a MIX bus.

**SONG**. Your written patterns lined up in order with repeats, per track. It runs beside CHAIN and does not follow
it.

**SPLIT**. A TIMING setting: the chance a step plays at half its division, or a note is pushed late.

**SPREAD**. A track's stereo width, also a destination.

**STEP**. A track that plays notes you wrote. Also one position in a pattern.

**STRUM** **(3.5 beta)**. A CHORD track's notes start a few ms apart (1 to 250), like a played chord.

**Suspended chord (sus)**. A chord with its third replaced by the second or fourth, so it sounds open and
unresolved. In Tessera: SUS2, SUS4, 7SUS4 **(3.5 beta)** on CHRD.

**SWARM**. Eight related slow wanderers per track, from 64 s to 2.4 s a turn at the default speed, set with RATE on
ORGANIC. A mod source.

**Swing**. Delaying every second step so a straight rhythm shuffles. In Tessera: TEMPO's swing and each pattern's
swing.

## T

**TEMPO**. The project's BPM, swing and record quantise. With an external CLK, the clock's tempo wins. Also a mod
destination that speeds the whole project up or slows it.

**TIDE**. A very slow source: three chaotic movements per track, minutes a turn, set with SPEED on ORGANIC.

**TIMING**. A page of rolls and feel: RATCH, CHANCE, SHAPE, HUMAN, and under OCT- held, SPLIT.

**TRIO**. The engine for three oscillators with set intervals and detune.

**Trigger**. A short pulse that says "now", where a gate says "while". In Tessera a generator's GATE source goes high
with each of its notes; set a cord to GATE to use any source like a trigger or a switch.

**Turing machine**. A looping random shift register: it plays a loop of a few notes and, with some chance, changes
one as it comes round. Locked, it repeats exactly. In Tessera: **TURING** (LENGTH, PROB, SPAN).

**TWEAK / TWEAK 2**. Shape a line after it is written: shift, drop, humanise, follow another track (TWEAK); pass hits
to another track, combine rhythms with logic, play only on some bars (TWEAK 2).

## U

**USER**. A slot that stores a sound you edited (SAVE > USER). Also **USER1 to USER4**, four 12-note scales of your
own **(3.5 beta)**, and **CHORD USER**, your own progression.

## V

**VCA** (voltage-controlled amplifier). The part that sets the volume under the control of a signal, such as an
envelope. In Tessera, a cord that *scales* another cord is a VCA on a mod **(3.5 beta)**; a gated PLAY cord opens
and shuts a whole track.

**VEL**. Velocity, how hard a note was played; a mod source.

**VOIC** (voicing). How a chord's notes are laid out: close together, spread open, or turned over. In Tessera: CLOSE,
OPEN, INV1, INV2 or +OCT. See *Inversion*.

**Voice**. One note being sounded. Tessera has a shared pool (BUDGET).

---

**+-, +, ^, =**. The mark after a cord's source: both ways, one way, GATE, HOLD. OCT+ on MODS cycles them.

*Credits for the ideas behind several generators and sources are in the [Credits](MANUAL.md#credits) and
[LICENSING.md](LICENSING.md).*
