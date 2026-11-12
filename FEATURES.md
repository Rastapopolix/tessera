# Tessera: what it does

Tessera is free firmware for the M-Vave FM-1 that turns it into a four-track instrument that can write music for
you: pick a sound, pick a generator, press play.

For the experienced: a generative toolbox with a modular's habits. Thirteen generators and a step sequencer, a
sixteen-cord modulation matrix where every generator is also a source, and layers for form, arrangement and
whole-piece planning, all optional, all off until you switch them on, and every random choice comes from a seed, so
a piece replays exactly.

This page lists what Tessera has, by category, so you can see how much is there before you open the manual. It does
not teach anything: each category ends with a **More:** link to the part of the [MANUAL](MANUAL.md) that does, and
[GLOSSARY.md](GLOSSARY.md) explains the terms. For step-by-step patches, see [RECIPES.md](RECIPES.md).

Features not yet in a stable release (3.4.1 is the latest) are tagged **(3.5 beta)**. Everything else is in the
current release.

## Sound engines

- **FM6**: six-operator FM with 32 algorithms, per-operator envelopes and scaling, a pitch envelope, an LFO, and the
  usual bend, portamento and controller settings; bells, electric pianos, glass and metal.
- **FM6 engine models**: MODERN, MARK I (the classic DX7 tables) or OPL, three flavours of one voice.
- **FM6 shortcuts**: one knob stretches all the amp envelopes at once, and one sets brightness, since FM6 has no
  filter of its own.
- **ORACLE**: a polyphonic analogue-style voice with two oscillators, noise, POLY MOD and a four-pole resonant
  filter; warm pads, brass, sync leads, basses and plucks.
- **ORACLE character**: early or late filter models, per-voice vintage drift, glide, filter and amp envelopes, and
  its own LFO with routing.
- **TRIO**: three oscillators with set intervals and detune; simple basses, leads and string-machine pads.
- **PHYS**: physical models of bars and bells, plucked or bowed strings, drum heads, and strings with sympathetic
  ringing; mallets, chimes, bowed pads and drones.
- **DRUM**: 808 and 909 kits on eight lanes (kick, snare, clap, closed and open hat, toms, rim, cowbell), edited on
  their own grid.
- **One engine per track**: mix them freely, such as a DRUM kit, an ORACLE bass, an FM6 bell and a PHYS pad.
- **Presets and favourites**: browse sounds by category, favourite or recent, from the home page or while any page is
  open.
- **Sound design pages**: a map of each engine's pages, with undo and redo while you edit.
- **User sounds**: store an edited sound in a user slot; sounds live in the project.
- **Voice budget**: one adjustable pool of voices shared by all tracks, so you can trade polyphony for headroom.
- **Per-track envelope and LFO**: classic vibrato, wobble and tremolo, with their own destinations.
- **Ambient default set**: a new project starts with pads, bells, a string machine and an 808 kit.

More: [Sounds](MANUAL.md#sounds)

## Effects and mixing

- **Three insert effects per track**: each can be PHASER, WIDTH, FORMANT, TAPE, SPIT, ECHO, ROOM, REVERB, CHORUS or
  FILTER.
- **Three shared send effects**: space and echo that every track feeds, with a send level per track.
- **A resonant FILTER effect**: a low-pass for any track, including FM6.
- **Effect parameters are modulation targets**: mix, rate and character move with cords; echo time glides instead of
  clicking.
- **MIXER**: levels, pans, mutes and solos for all four tracks, plus FILL and tap tempo on the keys.
- **Pan and spread** per track, and a master level.

More: [Every page, and why you would use it](MANUAL.md#every-page-and-why-you-would-use-it) (FX and GLO), and
[The panel at a glance](MANUAL.md#the-panel-at-a-glance) (MIXER)

## Sequencing and patterns

- **Step sequencer**: write notes by hand, with velocity and length, and a hit grid for DRUM tracks.
- **Step extras**: ratchets, nudges, conditions (1:2 to 8:8, FILL, FIRST) and a pitch accumulator that climbs each
  pass.
- **Pattern tools**: snap to scale, double, halve, or write a Euclidean rhythm.
- **Eight patterns per track**, each with its own length, speed (DIV), gate and swing.
- **SONG**: line patterns up in order with repeats, per track.
- **Polymeter**: tracks run at different divisions and lengths against each other.
- **Pattern direction**: forward, back, pendulum, ping-pong, random or drunk walk **(3.5 beta)**.
- **PAT GEN**: each pattern keeps its own generator setup, so switching pattern switches the generator
  **(3.5 beta)**.
- **Arpeggiator**: up, down, up-down, random, as-played and repeat modes, with octaves, gate, swing, probability and
  hold; down-up and a random-once order **(3.5 beta)**.
- **Tempo, swing and record quantise**; tap tempo; external clock follow.
- **Live recording**: play in from the keys, layer by layer, with MOTION keeping knob moves.
- **Print**: what a generator just played becomes ordinary steps you can edit; an arpeggio too **(3.5 beta)**.

More: [SEQ](MANUAL.md#seq-the-mode-and-its-pages), [PATTERNS and SONG](MANUAL.md#patterns-and-song),
[Print and keep](MANUAL.md#print-and-keep)

## Generators

- **TURING**: a looping shift register; lock it or let it drift.
- **EUCLID**: hits spread as evenly as possible, giving many world rhythms.
- **MARKOV**: a wandering line that mostly steps to neighbours with the odd leap.
- **MORPH**: slides one rhythm into another with a single knob.
- **ATLAS**: a map of drum patterns; move X and Y for a different groove, with a fourth drum part.
- **BOUNCE and FALL**: balls in a box, or leaves in the wind, play where they land.
- **AWAKE**: two loops of different lengths, one lifting the other.
- **POLY**: four notes on four rhythms, for polyrhythms.
- **QUAD**: a 4 x 4 grid of notes the tracks share, a row each, linked across tracks.
- **CONSTELLATION**: a ship flies through a sky of stars; the stars it passes play.
- **SERIES**: a self-similar melody from Nørgård's infinity series that unfolds for a long time.
- **CHORD**: a progression in your scale (pop, 50s, 2-5-1, circle, drone, walk, or your own), as held chords, beats,
  stabs or offbeats.
- **ANSWER**: play a phrase and it answers, changing a little more each time; it can listen to another track.
- **Your steps as a seed**: TURING, EUCLID, MARKOV and MORPH can vary a STEP pattern you wrote, keeping its rhythm or
  its pitches.
- **Seeds**: every random choice comes from a seed, so PLAY replays exactly and ROLL finds a new line.
- **ROLL ALL**: new seeds on every generator track at once, with one step back **(3.5 beta)**.
- **RANDOM**: new values for a page's knobs, each inside its range, with a way back **(3.5 beta)**.
- **Generators join in time** when picked while others play.
- **Generators are mod sources**: each offers its gate, note and CV to the cords.

More: [The generators](MANUAL.md#the-generators), [The GEN layer](MANUAL.md#the-gen-layer)

## Shaping a line

- **EVOLVE**: a phrase mutates every few bars, and you choose how much, how often and what stays locked.
- **STEP EVOLVE**: the same for steps you wrote, so a pattern varies as it plays.
- **GRAVITY**: a drifting line comes back home.
- **TWEAK**: shift, drop notes, humanise, or follow another track's pitch.
- **FOLLOW**: a track plays a harmony line off another track, in key, even when the leader is muted.
- **TWEAK 2**: pass hits to another track (BRANCH), combine tracks' rhythms with logic, play only on certain bars.
- **LENGTH**: how long notes ring, from clipped to pads made of sparse notes.
- **TIMING**: ratchets x1 to x8, even, accelerating or decelerating, split timing and a behind-the-beat feel.
- **CHORD STRUM**: chord notes start a few milliseconds apart, up, down or alternating, like a played chord
  **(3.5 beta)**.

More: [EVOLVE](MANUAL.md#evolve), [TWEAK](MANUAL.md#tweak), [TWEAK 2](MANUAL.md#tweak-2),
[LENGTH](MANUAL.md#length), [TIMING](MANUAL.md#timing)

## Long-form and arrangement

- **FORM**: give a track sections such as AABA or ABAC, built from its own phrase.
- **DEVELOP**: each return of a section varies a little.
- **CHAIN**: one shared form for the project, so every track changes section (and key) together.
- **ENTRIES**: tracks drop out and come back on phrase lines like a band arranging itself, in a musical order by role
  (bass, rhythm, harmony, melody), with an ANCHOR that never leaves.
- **ENTRIES order**: choose the order tracks come in; they leave in reverse **(3.5 beta)**.
- **ENERGY**: a slow tide from sparse to busy over minutes, with occasional rests and a depth per track.
- **PIECE**: plan a whole piece, 1 minute to 4 hours, with a shape (arch, rise, fall, plateau, valley, waves,
  builds, steps or one you draw) and an end (stop, loop, new seeds or free).
- **PIECE resolves home**: in its last stretch, notes are drawn toward the key's root and fifth.
- **PERF**: four macro knobs, each driving as many cords as you like, for a build or breakdown.
- **LANDSCAPE**: a night-lake screen drawn by the music, something to watch while it plays.
- **Everything starts off**: use any layer alone, or all together; the same seed replays the same piece.

More: [FORM](MANUAL.md#form), [The long-form layers together](MANUAL.md#the-long-form-layers-together),
[PIECE](MANUAL.md#piece-a-length-and-a-shape), [CHAIN](MANUAL.md#chain-one-form-for-the-project),
[ENTRIES](MANUAL.md#entries-tracks-out-and-back-in), [ENERGY](MANUAL.md#energy), [PERF](MANUAL.md#perf-four-macros)

## Harmony

- **77 built-in scales** and one key for the whole project.
- **Quantise (QNT)**: snaps notes to the scale, with keyboard layouts (white keys, all keys, MPC).
- **QNT CHRD**: a CHORD track keeps every other part in key with the current chord.
- **Chord kinds and voicings**: triads, sevenths, sus, add9, power chords, open voicings and octave doubling; SUS2,
  7SUS4, ADD9 and MAJ6 **(3.5 beta)**.
- **Your own progressions** on the CHORD generator.
- **USER scales**: four 12-note scales of your own, made on the keys and saved with the project **(3.5 beta)**.
- **Scala tunings**: load microtonal `.scl` files from the web editor and play every melodic track in them.
- **Transpose** per track and a quick-set for the key.

More: [Harmony](MANUAL.md#harmony-one-key-chord-and-qnt-chrd), [USER scales](MANUAL.md#your-own-scales-user1-to-user4),
[Scala tunings](MANUAL.md#your-own-tunings-scala-files)

## Modulation and patching

- **MODS**: 16 cords from any source on any track to almost any destination on any track, generators and effects
  included.
- **Sources**: LFO, envelope, velocity, key, random, sample and hold, slew, mod wheel, aftertouch, expression,
  generator gate, note and CV, PERF macros and more.
- **TIDE and SWARM**: very slow organic sources, minutes a turn, or eight related wanderers.
- **Destinations**: pitch, cutoff, shape, level, pan, effect sends and parameters, envelope and LFO settings,
  generator knobs, tempo, FILL, and MIDI CC out to other synths.
- **Cord modes**: both ways, one way, GATE (all or nothing past a threshold) and HOLD (taken once a bar).
- **Mods of mods**: a cord can move another cord's amount.
- **Scaling mods of mods**: a cord can multiply another instead of adding, so a gate or an envelope opens and closes
  a mod **(3.5 beta)**.
- **MIX buses**: two mixers for mods, with sum, max, min or difference modes and lag; a gate through a lagged bus is
  an envelope **(3.5 beta)**.
- **EOC**: the LFO's cycle start as a gate source **(3.5 beta)**.
- **CLOCK destination**: a generator steps on a source's edges, so an LFO cycle can trigger a note
  **(3.5 beta)**.
- **PLAY destination**: a mod decides whether a track is heard, giving a gated voice in one cord **(3.5 beta)**.
- **DIR destination**: a cord changes a pattern's direction **(3.5 beta)**.
- **HOLD on notes**: clock a HOLD cord by a track's notes instead of the bar **(3.5 beta)**.
- **MOD DEPTH**: one control for how much of the whole patch plays, 0 to 100 %; a brake for feedback patches
  **(3.5 beta)**.
- **MOD SCOPE**: watch a cord's movement over time against its source, and pin it while you turn the knobs
  **(3.5 beta)**.
- **MATRIX**: the whole patch as a 4 x 4 grid, lit as cords move.
- **Mod shortcut**: make a cord on the spot from the knob you are turning.
- **Live marks**: every knob a cord moves shows it.

More: [MODS](MANUAL.md#mods-the-modulation-cords), [MATRIX](MANUAL.md#matrix),
[ORGANIC: TIDE and SWARM](MANUAL.md#organic-tide-and-swarm), [Mod shortcut](MANUAL.md#mod-shortcut)

## Performance

- **Mute and solo keys** on any page, landing now, on the bar or on the phrase; the setting is also on the MIXER's sheet
  **(3.5 beta)**.
- **Muting never stops the music**: a muted track plays on unheard, so followers and cords still hear it.
- **FILL**: steps with a FILL condition play a fill on demand.
- **PERF macros**: one knob drives many cords at once.
- **ROLL**: a new seed, a new line, in one gesture, and back again.
- **MIDI LEARN**: put any Tessera knob on your own controller's knob, remembered.
- **Play the keys**: the keys play the selected track, with MIDI input and chord keys.
- **Tap tempo and tempo from SELECT** while playing.

More: [The panel at a glance](MANUAL.md#the-panel-at-a-glance), [Shortcuts](MANUAL.md#shortcuts),
[Menu and settings](MANUAL.md#menu-and-settings)

## MIDI and USB

- **MIDI out**: every track and generator on its own channel, with transpose.
- **Silence a track's own sound** and use it only to play outside gear or to modulate.
- **Drum maps** for MIDI out: General MIDI or one note per row **(3.5 beta)**.
- **MIDI clock out** with start and stop, or follow an external clock.
- **MIDI CC and NRPN in**: every parameter has an ID and can be controlled from a controller, DAW or sequencer.
- **Mods out as CC**: LFOs and generators modulate other synths' knobs.
- **USB audio**: stereo in and out to a computer, with separate volume.
- **Four stems over USB**: each track recorded on its own channel in a DAW **(3.5 beta)**.
- **USB host**: plug a USB-MIDI keyboard, synth or MIDI-to-CV module straight into the FM-1.
- **norns and other hosts**: connect over USB MIDI.

More: [MIDI OUT channels](MANUAL.md#midi-out-channels), [MIDI CC control](MANUAL.md#midi-cc-control),
[USB host](MANUAL.md#usb-host), [Recording the four tracks as separate stems](MANUAL.md#recording-the-four-tracks-as-separate-stems)

## Interface and accessibility

- **A minimal surface**: four knobs, a few buttons, one page at a time.
- **One key rule on every page**: OCT+ goes in, OCT- goes back, and holding either reaches the page's extra layers
  **(3.5 beta)**.
- **Sheets**: deeper settings stay out of sight until you ask for them.
- **Mono look**: a 5 x 7 pixel face, one ink on black.
- **Thirteen inks**: nine single inks (yellow, white, green, blue, red, orange, pink, hot pink, lilac) and four
  multi-colour sets with a colour per track.
- **GROUND LIGHT**: put the screen on warm paper, with each ink darkened to read well **(3.5 beta)**.
- **Dials or bars** for knob values, a large-value readout and panel contrast options.
- **Battery display** and a menu for MIDI, USB and device settings.
- **Screen reader**: speaks every page, knob and message through a web page and your computer's screen reader.
- **Boot screen**: the logo is laid tile by tile, then swept away.

More: [The panel at a glance](MANUAL.md#the-panel-at-a-glance), [Screen reader](MANUAL.md#screen-reader),
[Menu and settings](MANUAL.md#menu-and-settings)

## Tools

- **Web installer**: flash from Chrome or Edge, with a way back to the official firmware and a backup first.
- **Web editor**: edit sounds, sequences and scales from the computer, load Scala files, make complete backups.
- **Web simulator**: try Tessera in the browser without the hardware.
- **Projects**: one record holds patterns, sounds, generators and seeds, forms, cords and layers; four slots plus
  autosave and a template.
- **New song screen**: pick a key, tempo and a role per track.
- **Demo projects** to load and learn from.
- **Health monitor**: watch CPU load, late audio and shed voices live, in the web editor; its USB rows tell a click on
  the USB side from an overloaded FM-1 **(3.5 beta)**.
- **CPU meter** on GLO > SYSTEM: the average and the worst moment of the last second **(3.5 beta)**.
- **Free time from silent tracks**: a muted or faded-out track stops making sound, leaving its time to the tracks you
  hear **(3.5 beta)**.
- **Manual, recipes and parameter reference**: step-by-step patches, and a reference generated from the firmware.
- **Free and open**: GPL-3.0, with every line readable.

More: [Saving and older projects](MANUAL.md#saving-and-older-projects), [If something goes wrong](MANUAL.md#if-something-goes-wrong)

---

*Tessera is a fork of Melodee by keremimo, itself built on Felucca by Leo Kuroshita / Hügelton Instruments. Its
generators draw on Mutable Instruments' Grids and on several norns scripts (ideas only), and ORACLE descends from
Melodee's analogue voice. See [LICENSING.md](LICENSING.md) and the [Credits](MANUAL.md#credits).*
