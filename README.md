<p align="center"><img src="assets/brand/banner.png" alt="Tessera: generative firmware for the M-Vave FM-1" width="100%"></p>

<p align="center">
<b><a href="https://rastapopolix.github.io/tessera/">Project page</a></b> ·
<b><a href="https://rastapopolix.github.io/tessera/webapp/installer/">Install</a></b> ·
<b><a href="https://rastapopolix.github.io/tessera/webapp/sim/">Try it in your browser</a></b> ·
<b><a href="MANUAL.md">Manual</a></b> ·
<b><a href="RECIPES.md">Recipes</a></b> ·
<b><a href="https://github.com/Rastapopolix/tessera/releases/latest">Releases</a></b>
</p>

**Tessera** is a generative firmware for the M-Vave FM-1. Each of the four tracks can play its own steps, an
arpeggio, or one of thirteen generators, and the pages around them evolve the phrases, build song forms, pass hits
between tracks and modulate anything from anything. GPL-3.0-only. **Status: Experimental.**
By **Theo Dainis** ([Rastapopolix](https://github.com/Rastapopolix)).

Tessera grew out of Melodee (Tess), a fork of [Melodee](https://github.com/keremimo/melodee) by **keremimo**,
itself built on *Felucca* by **Leo Kuroshita / Hügelton Instruments**. Releases up to 1.3.x were published as
Melodee (Tess).

*Tessera* (plural *tesserae*) is the Latin word for the small tile of stone or glass in a mosaic. Each generator
makes a short phrase; EVOLVE and FORM set mutated phrases side by side into sections, so a piece is built from small
tiles arranged into a larger pattern. The word also meant a die, which suits the chance-driven generators.

---

## Install

Flash it from your browser (Chrome or Edge, FM-1 connected directly by USB):
**[Web installer](https://rastapopolix.github.io/tessera/webapp/installer/)**. Afterwards you can edit sounds
and sequences from the computer with the [web editor](https://rastapopolix.github.io/tessera/webapp/editor/).

Each release has two builds: `tessera-X.Y.fwsc` (standard, dual core: start here) and
`tessera-X.Y-1core.fwsc` (single core, for FM-1s that crash with the standard build).

Do not unplug the FM-1 while it is writing. This is experimental firmware: use it at your own risk. The installer
page also has a way back to the official V15 firmware, with a backup first.

---

## What's inside

* **Thirteen generators** on SEQ > GEN: TURING, EUCLID, MARKOV, MORPH, ATLAS (with a fourth drum part), BOUNCE (and
  FALL), AWAKE, POLY, QUAD, CONSTELLATION, SERIES, CHORD and ANSWER, besides the step sequencer and the arpeggiator.
* **Shaping**: EVOLVE mutates a phrase every few bars, and **STEP EVOLVE** does the same to steps you wrote; FORM
  builds sections such as AABA, and with **DEVELOP** each return varies a little; TWEAK shifts, drops, humanises or follows another track; TWEAK 2 branches hits
  to another track's sound, combines tracks with logic, and plays only on certain bars; LENGTH sets how long notes
  hold. **GRAVITY** lets a drifting line come back home.
* **Your steps as a seed**: TURING, EUCLID, MARKOV and MORPH can play from a STEP pattern you wrote (SOURCE) and vary
  it, keeping its rhythm or its pitches as you choose.
* **Steps**: per step ratchets, nudges, conditions (1:2 to 8:8, FILL, FIRST) and a pitch accumulator that climbs each
  pass; the pattern's sheet doubles, halves or writes a Euclidean rhythm.
* **Harmony**: one key and scale for the whole project (**Key link**); a CHORD track plays a progression (or your
  own), and **QNT CHRD** makes the other tracks follow it.
* **Modulation**: 16 cords from any source to any destination on any track, generators and effects included; slow organic
  sources (TIDE, SWARM); live marks on every knob a cord moves, and **MATRIX** to see the whole patch.
* **Print**: hold REC to turn what a generator played into ordinary steps.
* **Sounds**: FM6 (six-operator FM, three engine models), **ORACLE** (two oscillators, POLY MOD and a four-pole
  filter: a classic polyphonic analogue voice), TRIO and DRUM (808, 909), with three insert effects a track and three
  send effects, a resonant FILTER among them.
* **MIDI OUT** of every track and the generators, each on a channel of its own (GLO > MIDI OUT), and **CLOCK OUT**,
  over USB.
* **Its own look**: one ink on black and a 5 x 7 pixel font, as on a monochrome OLED; YELLOW by default, or
  WHITE, GREEN, BLUE, RED, or ORANGE, PINK, HOT PINK or LILAC as the FM-1's shells, or MOSAIC: a colour per track
  (MENU > COLOR).

The **[manual](MANUAL.md)** covers every page, knob and shortcut; **[RECIPES.md](RECIPES.md)** has step-by-step
patches and tips.

---

## Gallery

*The pages as the firmware draws them, rendered on the host from the firmware's own drawing code
(`tests/gallery_render.c`) in the default YELLOW ink, shown at 2x.*

<table>
<tr>
<td align="center" valign="top" width="33%"><img src="assets/gallery/home.png" alt="HOME" width="240"><br><b>HOME</b><br><sub>The four tracks, their sounds and knobs.</sub></td>
<td align="center" valign="top" width="33%"><img src="assets/gallery/gen_constel.png" alt="CONSTELLATION" width="240"><br><b>CONSTELLATION</b><br><sub>Stars play as they cross the ship's crosshair (after the norns script Constellation).</sub></td>
<td align="center" valign="top" width="33%"><img src="assets/gallery/gen_grids.png" alt="ATLAS" width="240"><br><b>ATLAS</b><br><sub>Inspired by Émilie Gillet's Grids (Mutable Instruments): kick, snare, hat and a fourth part.</sub></td>
</tr>
<tr>
<td align="center" valign="top"><img src="assets/gallery/gen_quad.png" alt="QUAD" width="240"><br><b>QUAD</b><br><sub>A 4 x 4 grid of notes the tracks share, a row each (after the norns script initenere).</sub></td>
<td align="center" valign="top"><img src="assets/gallery/gen_chord.png" alt="CHORD" width="240"><br><b>CHORD</b><br><sub>Progressions in the track's scale; QNT CHRD makes the others follow.</sub></td>
<td align="center" valign="top"><img src="assets/gallery/gen_series.png" alt="SERIES" width="240"><br><b>SERIES</b><br><sub>Per Nørgård's infinity series.</sub></td>
</tr>
<tr>
<td align="center" valign="top"><img src="assets/gallery/gen_turing.png" alt="TURING" width="240"><br><b>TURING</b><br><sub>A looping shift register.</sub></td>
<td align="center" valign="top"><img src="assets/gallery/gen_poly.png" alt="POLY" width="240"><br><b>POLY</b><br><sub>Four notes on four rhythms.</sub></td>
<td align="center" valign="top"><img src="assets/gallery/gen_fall.png" alt="FALL" width="240"><br><b>FALL</b><br><sub>Leaves in the wind, playing where they land (after the norns script Fall).</sub></td>
</tr>
<tr>
<td align="center" valign="top"><img src="assets/gallery/gen_evolve.png" alt="EVOLVE" width="240"><br><b>EVOLVE</b><br><sub>LOCK, EVERY, AMOUNT; ROLL picks the seed.</sub></td>
<td align="center" valign="top"><img src="assets/gallery/gen_tweak2.png" alt="TWEAK 2" width="240"><br><b>TWEAK 2</b><br><sub>BRANCH, LOGIC and COND.</sub></td>
<td align="center" valign="top"><img src="assets/gallery/step_evolve.png" alt="STEP EVOLVE" width="240"><br><b>STEP EVOLVE</b><br><sub>Your steps, varied as they play.</sub></td>
</tr>
<tr>
<td align="center" valign="top"><img src="assets/gallery/cords.png" alt="MODS" width="240"><br><b>MODS</b><br><sub>16 cords, any source to any destination.</sub></td>
<td align="center" valign="top"><img src="assets/gallery/matrix.png" alt="MATRIX" width="240"><br><b>MATRIX</b><br><sub>The cords track by track, lit as they move.</sub></td>
<td align="center" valign="top"><img src="assets/gallery/help.png" alt="Help card" width="240"><br><b>Help card</b><br><sub>Hold ARP and turn a knob.</sub></td>
</tr>
<tr>
<td align="center" valign="top"><img src="assets/gallery/gen_answer.png" alt="ANSWER" width="240"><br><b>ANSWER</b><br><sub>Play a phrase; it answers, changing a little each time.</sub></td>
<td align="center" valign="top"><img src="assets/gallery/gen_timing.png" alt="TIMING" width="240"><br><b>TIMING</b><br><sub>Ratchets x1 to x8, EVEN, ACCEL or DECEL.</sub></td>
<td align="center" valign="top"><img src="assets/gallery/gen_energy.png" alt="ENERGY" width="240"><br><b>ENERGY</b><br><sub>A slow tide over minutes, and rests.</sub></td>
</tr>
<tr>
<td align="center" valign="top"><img src="assets/gallery/edit_fm6.png" alt="FM6" width="240"><br><b>FM6</b><br><sub>Six-operator FM.</sub></td>
<td align="center" valign="top"><img src="assets/gallery/oracle_osc.png" alt="ORACLE" width="240"><br><b>ORACLE</b><br><sub>Two oscillators, POLY MOD, a four-pole filter.</sub></td>
<td align="center" valign="top"><img src="assets/gallery/edit_trio.png" alt="TRIO" width="240"><br><b>TRIO</b><br><sub>Three oscillators.</sub></td>
</tr>
<tr>
<td align="center" valign="top"><img src="assets/gallery/drum.png" alt="DRUM" width="240"><br><b>DRUM</b><br><sub>808 and 909 kits.</sub></td>
<td align="center" valign="top"><img src="assets/gallery/oracle_filter.png" alt="ORACLE FILTER" width="240"><br><b>ORACLE FILTER</b><br><sub>CUTOFF, RES, its envelope and key tracking.</sub></td>
<td align="center" valign="top"><img src="assets/gallery/fx_filter.png" alt="FILTER effect" width="240"><br><b>FILTER FX</b><br><sub>A resonant low-pass for any track.</sub></td>
</tr>
</table>

**The inks** (MENU > COLOR), the same HOME in each: YELLOW is the default; ORANGE, PINK, HOT PINK and LILAC match the
FM-1's shells; MOSAIC gives each track its own colour and shows the selection in white.

<table>
<tr>
<td align="center"><img src="assets/gallery/ink_yellow.png" alt="YELLOW" width="180"><br><b>YELLOW</b></td>
<td align="center"><img src="assets/gallery/ink_white.png" alt="WHITE" width="180"><br><b>WHITE</b></td>
<td align="center"><img src="assets/gallery/ink_green.png" alt="GREEN" width="180"><br><b>GREEN</b></td>
<td align="center"><img src="assets/gallery/ink_blue.png" alt="BLUE" width="180"><br><b>BLUE</b></td>
</tr>
<tr>
<td align="center"><img src="assets/gallery/ink_red.png" alt="RED" width="180"><br><b>RED</b></td>
<td align="center"><img src="assets/gallery/ink_orange.png" alt="ORANGE" width="180"><br><b>ORANGE</b></td>
<td align="center"><img src="assets/gallery/ink_pink.png" alt="PINK" width="180"><br><b>PINK</b></td>
<td align="center"><img src="assets/gallery/ink_hot_pink.png" alt="HOT PINK" width="180"><br><b>HOT PINK</b></td>
<td align="center"><img src="assets/gallery/ink_lilac.png" alt="LILAC" width="180"><br><b>LILAC</b></td>
<td align="center"><img src="assets/gallery/ink_mosaic.png" alt="MOSAIC" width="180"><br><b>MOSAIC</b></td>
</tr>
</table>

More pages are in the [manual](MANUAL.md).

---

## Building

See [BUILDING.md](BUILDING.md). The host tests run with `tests/run_tests.sh`; the gallery renders with
`tests/gallery_render.c` (build line at its top) and the banner and social preview with `tools/gen_brand.py`.

---

## About me

I'm Theo Dainis, from New Zealand. I've been using and building synthesisers for the past 15 years, and I'm passionate about modular and open source. Elektron and Mutable Instruments are big influences on me. I love the DIY side: building things, and learning how music hardware works on the inside. Tessera is the sequencer I wanted on the FM-1: modular in a box, a small instrument that writes with you.

### Built with Claude Code

Tessera's code is written with [Claude Code](https://claude.com/claude-code), Anthropic's AI coding tool. I designed it, made the decisions, and test every release on my own FM-1 before it goes out. The source is open, so you can read every line and every change. Like any firmware, it comes with no warranty: save a **Complete backup** from the web editor before you flash.

---

## Credits and licence

Tessera is by **Theo Dainis (Rastapopolix)**. GPL-3.0-only, inherited from Melodee. Original copyright and SPDX headers are preserved. Melodee is by keremimo,
Felucca by Leo Kuroshita / Hügelton Instruments.

The ATLAS generator uses the drum map of **Grids** by Émilie Gillet / Mutable Instruments. Several generators are
inspired by **norns** scripts (ideas only, no code copied): **QUAD** by initenere, **FALL** by ambalek's Fall,
**CONSTELLATION** by timothy-taylor's Constellation, **AWAKE** by tehn's Awake and **SERIES** by nørgård.
**ORACLE** is Melodee's Prophet engine (after the Prophet-5) under its own name, with its own sounds. The TIDE and SWARM mod sources are inspired by **Triple Sloth** by Nonlinear Circuits (Andrew Fitch) and **ochd** by
Instruō and DivKid. See
[LICENSING.md](LICENSING.md) for every third-party part and its licence.
