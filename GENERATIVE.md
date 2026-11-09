# Generative patching with Tessera

Generative music is music you make by building a system and letting it play: you decide how the parts behave and
how they listen to each other, not every note. Modular synthesists have worked this way for decades, with patch
cables, random voltages and slow modulation. Tessera does the same with four tracks, its generators and its mods
(the cords on **LFO > MODS**).

This guide has two parts. **Six ideas** explains the principles behind generative patches, each with recipes you
can try in a minute or two and the classic modular patch it comes from. **Where the generators come from** tells the
story behind each generator: the module, script, composer or mathematician it is adapted from, and how it works.

You don't need a modular to follow either part. Where a recipe says **MODS**, see
[MANUAL: MODS](MANUAL.md#mods-the-modulation-cords); for a quick start, see [RECIPES.md](RECIPES.md). Every recipe
here also works in the [browser simulator](https://rastapopolix.github.io/tessera/webapp/sim/).

**Contents**

- [Six ideas](#six-ideas)
  - [1. Randomness you can lock](#1-randomness-you-can-lock)
  - [2. Many speeds at once](#2-many-speeds-at-once)
  - [3. Feedback: a system that plays itself](#3-feedback-a-system-that-plays-itself)
  - [4. From a curve to an event](#4-from-a-curve-to-an-event)
  - [5. Parts that listen to each other](#5-parts-that-listen-to-each-other)
  - [6. How much, not which](#6-how-much-not-which)
- [Where the generators come from](#where-the-generators-come-from)
- [Tessera's limits, and why they help](#tesseras-limits-and-why-they-help)
- [Further reading](#further-reading)

**How the recipes are written.** A mod is written as source, destination and amount: **T1 S&H** to **T1 LFO RTE**,
**+40** means a cord from track 1's S&H to track 1's LFO rate, at +40 %. Pick the source with **KNOB 1**, the mod
with **KNOB 2**, the destination with **KNOB 3** and the amount with **KNOB 4**. **OCT+** on MODS cycles a mod
between **+-** (both ways), **+** (one way) and **GATE** (on or off at a threshold). Each recipe starts from a new
project: track 1 **GLASS PAD**, track 2 **STRING MACH**, track 3 **BELLS**, track 4 the **808 KIT**.

---

## Six ideas

### 1. Randomness you can lock

Pure randomness is rarely musical: a melody where every note is new each time has nothing to remember. What makes
a random patch sing is a hand on the dial between **new** and **again**: a phrase that repeats long enough to be
recognised, then changes a little, then settles. Most generative tools are built around that one control.

#### A loop you can lock and loosen

*On a modular: a Turing Machine into a quantizer into an oscillator, its knob just off Lock.*

**Why try it:** it's the quickest way to hear what "controlled randomness" means. A melody that is yours for a
while, then drifts, and that you can freeze the moment you like it.

1. Track 3 (BELLS): **SEQ** for GEN, **MODE TURING**, **LENGTH 8**, **PROB 0** (the knob reads Lock). Press **PLAY**.
   Eight steps loop exactly.
2. Turn **PROB** to **10**: now and then one step changes as it comes round, and stays changed. The loop drifts
   slowly. At 50 it changes all the time; back at 0 it locks wherever it is.
3. A second voice from the same idea: track 2, any generator, **TWEAK**: **FOLLOW T3**, and hold **OCT+** for
   **HOW CHORD**. Track 2 harmonises every change track 3 makes.

**Listen for:** the moment a changed step becomes part of the tune. That's a shift register remembering (see
[TURING](#turing-shift-registers)). There's no quantizer to patch: every generator plays in the track's scale.

#### Randomness on a knob

*On a modular: Marbles' DEJA VU, which slides between repeating the last random choices and making new ones; its
jitter, which loosens the clock.*

**Why try it:** to perform with randomness. One knob decides whether the piece repeats or wanders, so you can hold a
good passage and let go when you're ready.

1. Start from the recipe above, track 3 TURING at **PROB 0**.
2. **MODS**: **MACRO 1** to **T3 T PROB**, **+60**. On **GLO > PERF**, **KNOB 1** is now the "again or new" dial:
   down, the loop holds; up, it changes almost every pass.
3. Loosen the timing too: **T1 SWARM 3** to **T3 SPLIT**, **+40**. Every so often a step breaks into two quicker
   notes, more often as SWARM 3 rises.
4. **EVOLVE**, hold **OCT+**: **GRAVITY 30**. Now the drift is pulled back towards home.

**Listen for:** how PROB, SPLIT and GRAVITY each act on a different thing: which notes, when they play, and how far
from home they go.

### 2. Many speeds at once

A patch feels alive when things move at different speeds: notes every fraction of a second, a filter over a few
bars, the density over minutes, the whole piece over half an hour. Each slower layer frames the faster ones, as
weather frames a day. Modular players use LFOs at wildly different rates for this; Tessera gives you the slow ones
ready-made.

#### A tide over the whole piece

*On a modular: a very slow LFO or a long function generator, minutes per cycle, into the VCAs and filters; often a
second, slower one moving the first one's depth.*

**Why try it:** to make a piece that can play for an hour and still go somewhere: busier, quieter, resting, coming
back.

1. Tracks 3 and 4 play generators (TURING and ATLAS, say). Track 1 plays a drone or a slow CHORD.
2. **ARP > ENERGY**: **ARC 60**, **RESTS 20**. The generators now thin out and fill up over 3 to 6 minutes, and rest
   now and then. On track 1, hold **OCT+** on ENERGY: **DEPTH 0**, so the drone stays steady through it all.
3. The sound follows: **ENERGY** to **T1 CUTOFF**, **+40**.
4. A slower hand on top: **LFO > ORGANIC** on track 1, **SPEED 50**. **MODS**: **T1 TIDE 1** (about 17 minutes a turn)
   to **M1 AMT**, **+50**, where mod 1 is the ENERGY cord from step 3. The tide's pull on the filter now grows and fades over
   a quarter of an hour.

**Listen for:** three speeds: the notes, the tide (minutes), and the slow change in how much the tide matters.
For a planned shape (intro, build, peak, ending), see
[A piece that plays itself](RECIPES.md#a-piece-that-plays-itself-piece-chain-and-entries).

### 3. Feedback: a system that plays itself

Patch a module's output back into its own control and it starts to steer itself: an LFO that speeds up when it's
high and slows down when it's low, a random voltage that decides how soon the next random voltage comes. Feedback
is what makes a patch feel less like a machine running a program and more like something with a will of its own.
The art is keeping it from running away or freezing.

#### An envelope that plays itself

*On a modular: the Krell patch, which Todd Barton built on a Buchla 200e as a homage to Louis and Bebe Barron's
soundtrack for* Forbidden Planet *(1956). One LFO is the whole patch: each cycle it triggers sample & holds whose
random voltages set the LFO's own speed and shape, and the sound's pitch and colour.*

**Why try it:** it's the classic self-playing patch, and it shows feedback in one cord: the random value decides
how long the next swell lasts, so the music sets its own rhythm.

1. Track 1 (GLASS PAD): **MODE MARKOV**, **DENS 30**; **LENGTH**: **HIT**, so each note rings until the next.
2. **LFO**: **WAVE TRI**, **RATE** low (a cycle of a few seconds). **LFO 2**: **TRIG FREE**, so notes don't restart it.
3. **MODS**: **T1 LFO** to **T1 GAIN**, **+100**, and press **OCT+** once to make it **+**. The LFO is now the
   pad's envelope: it swells from silence and fades back, over and over.
4. **T1 S&H** to **T1 LFO RTE**, **+40**. S&H draws a new random value at each LFO cycle: every swell now has its
   own length.
5. **T1 S&H** to **T1 CUTOFF**, **+30**: each swell its own colour.

**Listen for:** irregular breathing that never quite repeats. The swells' lengths come from the patch, not from the
clock.

#### Two LFOs pulling on each other

*On a modular: two LFOs, each one's output into the other's rate input.*

**Why try it:** two simple, regular motions become one that's hard to predict but never jumpy: good for pads and
anything that should drift.

1. **MODS**: **T1 LFO** to **T2 LFO RTE**, **+40**, and **T2 LFO** to **T1 LFO RTE**, **+40**.
2. Hear them: **T1 LFO** to **T1 CUTOFF**, **+30**; **T2 LFO** to **T2 PAN**, **+50**.
3. Push it: **+70** on both rate cords and they lurch; **−40** on one makes them chase instead of push.
4. A ring of four: **T2 LFO** to **T3 LFO RTE**, **T3 LFO** to **T4 LFO RTE**, **T4 LFO** to **T1 LFO RTE**
   (track 2's to track 1's replaced). Each LFO is pushed by the one before it.

**Listen for:** the two motions catching each other and drifting apart. Each LFO alone is plain; together they
are not.

#### Two generators steering each other

*On a modular: two sequencers or random sources cross-patched: one's CV moves the other's probability, the other's
moves the first one's.*

**Why try it:** to hear a system that never quite settles, and to learn the most useful habit with feedback: a
brake.

1. Track 3 **TURING** (**PROB 10**), track 2 **MARKOV**.
2. **MODS**: **T3 CV** to **T2 M LEAP**, **+40** (Turing's register widens Markov's leaps) and **T2 CV** to
   **T3 T PROB**, **+30** (Markov's note unlocks Turing's loop).
3. The brake: **MACRO 1** to **M1 AMT**, **−100**, and **MACRO 1** to **M2 AMT**, **−100** (with the first two cords
   as mods 1 and 2). On **GLO > PERF**, turning **KNOB 1** up turns both cords down: the system calms, and each
   generator plays on its own.

**Listen for:** stretches where the two lock into a pattern, and moments where one change starts a cascade. A
generator reads its settings only at its own step, so the loop has a built-in delay: it wanders, but it doesn't
explode.

### 4. From a curve to an event

Much of a modular patch is continuous: LFOs, envelopes, random voltages gliding. Music also needs events: a note,
a fill, a part coming in. The bridge is the **comparator**: a gate that is on while a voltage is above a threshold
and off below it. A slow curve becomes a long, irregular rhythm of on and off. A **quantizer** does the same for
pitch, turning a gliding voltage into notes of a scale.

#### A voice that comes and goes with the tide

*On a modular: a slow random voltage into a comparator; its gate opens a VCA on a voice.*

**Why try it:** to bring parts in and out without writing an arrangement. The slow source decides, so the entries
are never on a schedule, and never random either.

1. Track 2 plays a generator, and **TWEAK > DROP 100**: it's silent.
2. **LFO > ORGANIC** on track 2: **SPEED 50**.
3. **MODS**: **T2 TIDE 2** to **T2 DROP**, **−100**. Press **OCT+** twice: **GATE** (a **^** after the source).
   Hold **OCT+** and turn **KNOB 4** for the threshold: **0**.
4. Track 2 plays only while TIDE 2 is above the middle: a few minutes in, a few minutes out, never the same twice.

**Listen for:** the rhythm of the arrangement: long, uneven stretches that still feel planned.

**Try next:**
- **A window.** Two GATE mods from one source, one at threshold **−20** with **+50** and one at **+20** with
  **−50**, move the destination only while the source sits between them.
- **Fills that come and go.** **ENERGY** to **FILL**, **+100**, as a **GATE** at **40**: steps set to FILL play
  while the piece is at its busiest.
- **A gate on a mod.** **T1 SLEW** to **M3 AMT**, **+100**, as a GATE, with mod 3's own amount at 0: mod 3 works only
  while SLEW is up.

### 5. Parts that listen to each other

Four tracks running side by side are four loops. Cross-patch them, so one's playing changes another's sound,
pitch or rhythm, and they start to sound like a band. On a modular this is the reason for a big case: every module
can hear every other.

#### One part makes room for another

*On a modular: the kick's envelope into a VCA on the pad (sidechain ducking).*

**Why try it:** it's the simplest cross-patch, and you hear it at once: the pad breathes with the drums.

1. Track 4 (808 KIT): **MODE ATLAS**, **DENS 40**. Track 1 plays a long pad (LENGTH HIT, or CHORD with HIT HOLD).
2. **MODS**: **T4 ENV** to **T1 LEVEL**, **−60**. Each hit pushes the pad down for the length of its envelope.
3. A subtler version: **T4 ENV** to **T1 CUTOFF**, **−40**.

**Listen for:** the pad pumping with the groove; turn the amount down until you feel it more than hear it.

**Try next: one track's notes deepen another's mod.** **T3 ENV** to **M2 AMT**, **+60**, where mod 2 is step 3's cord to
track 1's CUTOFF: each note of track 3 makes the pad's filter move further for a moment. That's a VCA on a modulation, done
with a mod of a mod.

### 6. How much, not which

A lot of generative music is shaped by density rather than notes: how often something plays, how full a pattern is,
whether two rhythms land together. Clock dividers, Euclidean patterns and gate logic are a modular's tools for this,
and changing a density by hand (or by a slow source) changes the feel more than changing any single note.

#### A rhythm web

*On a modular: one clock into dividers (/2, /3, /5) and Euclidean modules, their gates combined with logic (AND,
XOR), the web patched back so one rhythm moves another's fill or rotation.*

**Why try it:** to build an interlocking groove from three simple parts, and hear how logic and a little
cross-modulation make it shift without ever losing the beat.

1. Track 2: **MODE EUCLID**, **STEPS 16**, **PULSE 5**. Track 3: **EUCLID**, **STEPS 12**, **PULSE 4** (12 against 16:
   the two meet every three bars).
2. Track 3, **TWEAK 2**: **LOGIC XOR T2**. Track 3 now plays only where the two rhythms *don't* land together: they
   interlock.
3. Cross-patch: **T2 GATE** to **T3 E ROT**, **+25**. Each of track 2's hits turns track 3's pattern a little.
4. A slow hand: **T1 SWARM 1** to **T2 E PULSE**, **+30**. Over a minute, track 2 thins out and fills up again.
5. Different speeds: **PATTERN**: **DIV 1/8** on track 2 and **1/16** on track 3. Or try **MODE POLY** on one track:
   four rhythms at their own divisions (/4 against /3) in a single track.

**Listen for:** the beat never breaks, but the groove keeps finding new shapes. AND, OR and NOT on LOGIC do the
other classic things: play only together, either, or only when the other rests.

---

## Where the generators come from

Tessera's generators are ideas from modular synthesis, from monome's norns scripts, from composers and from
mathematics, restated for four tracks. Almost none of the code is copied: the idea is borrowed and written anew
(ATLAS is the exception: it uses Grids' drum map; [LICENSING.md](LICENSING.md) lists every borrowed part). This is
where they come from, how they work, and what Tessera changed.

### TURING: shift registers

**The original:** the *Turing Machine*, a Eurorack module by Tom Whitwell of Music Thing Modular, released as open
hardware. Whitwell has named Ken Stone's Gated Comparator and Grant Richter's Noise Ring among its ancestors;
shift-register music machines go back to the Triadex Muse and Buchla's Source of Uncertainty.

**How it works:** a *shift register* is a row of bits (ones and zeros) that move one place along at each clock. In
a Turing Machine the bit that falls off the end is fed back in at the start, so the row goes round in a loop. On its
way back, the bit may be flipped, with a chance set by the big knob. Read some of the bits as a number and you have
a voltage, a note; the loop of bits is a loop of notes.

```
 clock →   [1][0][1][1][0][0][1][0]  →  read as a number: a note
            ↑                    │
            └── flipped? (PROB) ─┘
```

At 0 % nothing flips and the loop repeats forever: *locked*. At 100 % every returning bit is flipped, so the loop
is locked again but twice as long, its second half the first one turned inside out. In between, the loop slowly
mutates, and a changed bit stays changed: the machine remembers its mistakes.

**In Tessera:** TURING keeps a 16-bit register, loops its first LENGTH bits (2 to 16), flips at PROB and plays a note
from the low eight bits, in the track's scale over SPAN octaves; its lowest bit decides whether the step plays.
The register is also the generator's CV, a mod source. Tessera adds SOURCE (your own STEP pattern as the loop),
GRAVITY (a pull back home) and seeds that replay. Recipe: [A loop you can lock and loosen](#a-loop-you-can-lock-and-loosen).

### MARKOV: chains of chance

**The original:** the *Markov chain*, after the Russian mathematician Andrey Markov (early 1900s). One of the first
pieces of computer-composed music, Lejaren Hiller and Leonard Isaacson's *Illiac Suite* for string quartet (1957),
used Markov chains in its last movement.

**How it works:** the next note is chosen by chance, but the chances depend on the note playing now. From C, say, a
D is likely, a G less so, a leap of an octave rare. Each note only "remembers" the one before, yet the line has a
character: it moves the way its table of chances says.

**In Tessera:** MARKOV walks over eight scale degrees. LEAP moves the chances from neighbours (a stepwise line) to
wide leaps; DENS is the chance a step plays at all. With a SOURCE, MARKOV *learns* its chances from your pattern:
it plays what followed each note somewhere in your melody, so it sounds like you, rearranged.

### EUCLID: evenly spread hits

**The original:** Godfried Toussaint's paper *The Euclidean Algorithm Generates Traditional Musical Rhythms* (2005)
showed that spreading k hits as evenly as possible over n steps, using the same steps as Euclid's algorithm for the
greatest common divisor (E. Bjorklund had used it for timing in particle accelerators), produces many of the world's
traditional rhythms. Three hits over eight steps is the Cuban tresillo; five over eight the cinquillo.

**How it works:** hits are spread as evenly as the steps allow; rotating the pattern moves where it starts. Two
Euclidean patterns of different lengths drift against each other and meet again after a while.

**In Tessera:** EUCLID has STEPS, PULSE and ROT, each a mod destination, with pitches picked by the seed. The same
fill is on the STEP pattern's sheet. Recipe: [A rhythm web](#a-rhythm-web).

### ATLAS: a map of grooves

**The original:** *Grids*, the topographic drum sequencer by Émilie Gillet of Mutable Instruments, open source.
Gillet arranged a collection of drum patterns on a map; any point between them is a blend.

**How it works:** X and Y pick a point on a 5 x 5 map of 25 patterns; the patterns near it are blended, each step
a probability for kick, snare and hat. Density decides how many of the probable hits play: low, only the most
certain; high, everything. Move X and Y slowly and the groove morphs smoothly from one style to another.

**In Tessera:** ATLAS uses Grids' map and its way of reading it (credited in [LICENSING.md](LICENSING.md)), with a
fourth part, TOMS, made from the hits the kick and snare only just miss. X, Y, density and each part's share are mod
destinations: an LFO on X is a groove that travels.

### SERIES: a melody alike at every scale

**The original:** the Danish composer Per Nørgård found his *infinity series* around 1959-60 and built works on it
from the 1960s on (*Voyage into the Golden Screen*, 1968; his Symphony No. 2, 1970). The norns script *nørgård*
brought it to the sound computer.

**How it works:** the series is self-similar: take every second note and you get the series again (upside down,
or one step up, depending on where you start); every fourth note, again. Its simplest form begins 0, 1, −1, 2, 1, 0, −2, 3. Played slowly it is a melody; inside it,
the same melody is playing at half speed, and at a quarter, at once.

**In Tessera:** SERIES works the series out from the bits of the step's number. START picks where it begins, INTV
how many scale degrees a step of the series is, LOOP repeats a stretch of it.

### AWAKE, QUAD, CONSTELLATION, FALL: from norns

**norns** is monome's sound computer (2018): a small open box that runs *scripts*, short programs in Lua with a
sound engine in SuperCollider, which its community writes and shares at norns.community. Many are small generative
instruments, each one idea done well, and several of Tessera's generators restate one of them for the FM-1 (ideas
only, no code copied):

- **AWAKE**, after *Awake* by tehn (monome), the script that greets a new norns: two loops of different lengths, one
  the melody, one lifting it up the scale. Because the lengths differ, the combination takes many passes to come
  round again.
- **QUAD**, after *initenere*: a 4 x 4 grid of notes shared by the four tracks, a row each; each track walks its row
  at its own speed, and one track's step can move another's (time routing).
- **CONSTELLATION**, after *Constellation* by timothy-taylor: a ship flies through a sky of stars, and the stars that
  cross its crosshair play. Where the crosshair sits sets the melody's register; how full the sky is, its density.
- **FALL** (BOUNCE past four balls), after *Fall* by ambalek: leaves fall at speeds of their own and play where
  they land, left low, right high, then fall again.

### ANSWER and ENERGY: from Zen-1

**ANSWER** (call and response: your phrase, played back as answers that change a little each time) and **ENERGY**
(a conductor that thins and fills the whole piece over minutes) take their ideas from ZEN-1, another custom
firmware for the FM-1, by aceyprime; again ideas only, written anew. (Some of ZEN-1's FM6 sounds are among the FM6
presets, credited in [LICENSING.md](LICENSING.md).)

### TIDE and SWARM: slow chaos and free-running LFOs

The two ORGANIC mod sources come from two modules:

- **TIDE**, after *Triple Sloth* by Nonlinear Circuits (Andrew Fitch): three chaotic circuits that wander around
  two "strange attractors", circling one for a while and then the other, never repeating. Tessera uses J. C.
  Sprott's piecewise-linear *jerk* system, one of the simplest equations that is chaotic. TIDE's three speeds are
  Tessera's own: about 17 minutes, 3 minutes and 1 minute a turn.
- **SWARM**, after *ochd* by Instruō and DivKid: eight free-running LFOs, each faster than the one before, chosen so
  that no two ever line up. Eight related motions, from a minute to a couple of seconds a turn.

### And Tessera's own

BOUNCE (balls in a box, a note at each wall), POLY (four notes on four rhythms), CHORD (progressions in the
track's scale) and the long-form layers (FORM, CHAIN, ENTRIES, the piece planner) are Tessera's own, built on the
same principles: a system you set up, with an off position for everything automatic.

---

## Tessera's limits, and why they help

Coming from a modular, a few differences matter:

- **Four tracks, four LFOs.** Each track has one LFO, so a ring of four cross-modulated LFOs uses them all. The slow
  sources (DRIFT, TIDE, SWARM, ENERGY) and each generator's GATE, NOTE and CV are extra sources that cost no LFO.
- **Sixteen cords.** The biggest recipe here uses six. A limit like this keeps a patch readable; **MATRIX** shows
  who moves whom.
- **Control rate.** Mods move about 1,400 times a second, plenty for anything but audio-rate modulation, which the
  engines (FM6, ORACLE's POLY MOD) do inside the voice.
- **Events on the grid.** A mod can open, close or thin a part (DROP, FILL, density), but notes still come on the
  track's steps. TIMING's SPLIT and HUMAN loosen that grid when you want it looser.
- **Everything replays.** Every random choice comes from a seed, so PLAY plays a piece the same way again and ROLL
  finds a new one. A modular patch is gone when you pull the cables; a Tessera patch can be saved, replayed and
  shared.

## Further reading

- Music Thing Modular, the Turing Machine: <https://musicthing.co.uk/>
- Mutable Instruments' documentation (Grids, Marbles), open source: <https://pichenettes.github.io/mutable-instruments-documentation/>
- Godfried Toussaint, *The Euclidean Algorithm Generates Traditional Musical Rhythms* (2005):
  <https://cgm.cs.mcgill.ca/~godfried/publications/banff.pdf>
- monome norns and its scripts: <https://monome.org/docs/norns/>, <https://norns.community/>
- Nonlinear Circuits: <https://www.nonlinearcircuits.com/>
- Todd Barton building the Krell patch on a Buchla: <https://vimeo.com/48466272>
