# Generative patching with Tessera

Generative music is music you make by building a system and letting it play: you decide how the parts behave and
how they listen to each other, not every note. Modular synthesists have worked this way for decades, with patch
cables, random voltages and slow modulation. Tessera does the same with four tracks, its generators and its mods
(the cords on **LFO > MODS**).

This guide has two parts. **Six ideas** explains the principles behind generative patches, each with recipes you
can try in a minute or two and the classic modular patch it comes from. **Where the generators come from** tells the
story behind each generator: the module, script, composer or mathematician it is adapted from, and how it works.

You don't need a modular to follow either part. Where a recipe says **MODS**, see
[MANUAL: MODS](MANUAL.md#mods-the-modulation-cords); for a quick start, see [RECIPES.md](RECIPES.md). The recipes
need Tessera 3.5 (the MIX buses, EOC, CLOCK, PLAY, MOD DEPTH, scaling and HOLD mods and the mod scope are new in it).

**Contents**

- [How the recipes are written](#how-the-recipes-are-written)
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

## How the recipes are written

Each recipe starts from a new project: track 1 **GLASS PAD**, track 2 **STRING MACH**, track 3 **BELLS**, track 4
the **808 KIT**, every track on STEP with nothing in it, no mods. Then:

- **What you hear**, and why it's worth trying.
- **Cords**: each mod as source, destination and amount. `T1 S&H → T1 LFO RTE, +40` is a mod from track 1's S&H
  to track 1's LFO rate, at +40 %. A mark after the source is the mod's kind: **+** one way only, **^** GATE (on or off
  at a threshold), **=** HOLD (sampled and held). No mark: **+-**, both ways, a new mod's default.
- **Keys**: the steps on the FM-1. To make a mod, tap **LFO** for **MODS**, then **KNOB 2** picks the mod (1 to 16),
  **KNOB 1** its source, **KNOB 3** its destination and **KNOB 4** its amount (a new destination starts at 50 %).
  **OCT+** (tapped) steps the picked mod through **+-**, **+**, **^** and **=** (a source that is already 0 to 1,
  such as ENV, GATE or EOC, starts at +). The keys follow one rule on every page: **hold OCT+** for the page's sheet
  (KNOB 1 the row, KNOB 2 its value, OCT- closes it); **hold OCT-** for the page's second knobs, only while held.
- **Watch it**: what to look for on the **mod scope**. Hold **OCT-** on MODS and the picked mod's move is drawn over
  time (the solid line) over its source (the dashed one); a GATE mod draws its threshold across the source. Let go and
  the rows come back. The same hold gives MODS' second knobs, so you can turn a threshold and watch it move.
- **Variation**: one change to try next.

Most recipes take three cords or fewer, and every cord is one you'd want to turn while it plays. Pages are named as
the buttons reach them: **SEQ** for GEN and PATTERN; **ARP** for EVOLVE, TWEAK, TWEAK 2, LENGTH and ENERGY; **LFO**
for MODS, LFO, LFO 2 and ORGANIC; **GLO** for MIXER and PERF.

---

## Six ideas

### 1. Randomness you can lock

Pure randomness is rarely musical: a melody where every note is new each time has nothing to remember. What makes
a random patch sing is a hand on the dial between **new** and **again**: a phrase that repeats long enough to be
recognised, then changes a little, then settles. Most generative tools are built around that one control.

#### A loop you can lock and loosen

*On a modular: a Turing Machine into a quantizer into an oscillator, its knob just off Lock.*

**What you hear:** a melody that is yours for a while, then drifts, and that you can freeze the moment you like it.
It's the quickest way to hear what "controlled randomness" means.

**Cords:** none.

**Keys:**
1. Select track 3 (BELLS). **SEQ** for GEN: **KNOB 1** MODE **TURING**, **KNOB 2** LENGTH **8**, **KNOB 3** PROB
   **0** (the knob reads Lock). Press **PLAY**: eight steps loop exactly.
2. Turn PROB to **10**: now and then a step changes as it comes round, and stays changed. At 50 it changes all the
   time; back at 0 it locks wherever it is.
3. A second voice from the same idea: select track 2, **ARP** for TWEAK, **KNOB 4** FOLLOW **T3**; hold **OCT-** and
   turn **KNOB 2** to HOW **CHORD**. Track 2 harmonises every change track 3 makes.

**Watch it:** a generator's register is a mod source. Make `T3 CV → T3 PAN, +30` and hold OCT- on it: the dashed
line is the register as a stepped shape, the same every pass at PROB 0, a step at a time new at 10.

**Variation:** HOW **CANON2** in place of CHORD: track 2 plays track 3's line two notes late, a round.

There's no quantizer to patch: every generator plays in the track's scale. The changed step that becomes part of
the tune is a shift register remembering (see [TURING](#turing-shift-registers)).

#### Randomness on a knob

*On a modular: Marbles' DEJA VU, which slides between repeating the last random choices and making new ones; its
jitter, which loosens the clock.*

**What you hear:** one knob decides whether the piece repeats or wanders, so you can hold a good passage and let go
when you're ready; now and then a step breaks into two quicker notes.

**Cords:**
- `MACRO 1 → T3 T PROB, +60`
- `T1 SWARM 3 → T3 SPLIT, +40`

**Keys:**
1. Track 3 TURING at PROB 0, as above.
2. **MODS**: mod 1 from **MACRO 1** (after the tracks' sources in KNOB 1's list) to **T3 T PROB**, **+60**. On
   **GLO > PERF**, **KNOB 1** is now the "again or new" dial: down, the loop holds; up, it changes almost every pass.
3. Mod 2 from **T1 SWARM 3** to **T3 SPLIT**, **+40**: SPLIT comes and goes with SWARM 3.
4. A pull back home: on track 3, **ARP** for EVOLVE, **KNOB 2** EVERY **2**; hold **OCT-** and turn **KNOB 4** to
   GRAVITY **30**. Each round the drift steps back toward the loop you set.

**Watch it:** pick mod 2 and hold OCT-: SWARM 3 (dashed) rising and falling over 16 bars, and the chance of a split
(solid) following it.

**Variation:** turn MACRO 1 up for eight bars, then down: the loop locks on whatever it became. That's deja vu.

PROB, SPLIT and GRAVITY each act on a different thing: which notes, when they play, and how far from home they go.

### 2. Many speeds at once

A patch feels alive when things move at different speeds: notes every fraction of a second, a filter over a few
bars, the density over minutes, the whole piece over half an hour. Each slower layer frames the faster ones, as
weather frames a day. Modular players use LFOs at wildly different rates for this; Tessera gives you the slow ones
ready-made.

#### A tide over the whole piece

*On a modular: a very slow LFO or a long function generator, minutes per cycle, into the VCAs and filters; often a
second, slower one opening and closing the first one's depth.*

**What you hear:** a piece that can play for an hour and still go somewhere: busier, quieter, resting, coming back,
the drone's filter following the tide more in some quarter-hours than in others.

**Cords:**
- `ENERGY → T1 CUTOFF, +40`
- `T1 TIDE 1+ → M1 AMT, +100`, scaling

**Keys:**
1. Tracks 3 and 4 play generators (**SEQ** for GEN: TURING on track 3, ATLAS on track 4). Track 1 plays a slow
   **CHORD** (MODE CHORD, HIT HOLD) as a drone.
2. Track 3, **ARP** for ENERGY: **KNOB 1** ARC **60**, **KNOB 2** RESTS **20**. The generators thin out and fill up
   over 3 to 6 minutes, and rest now and then. On track 1, hold **OCT-** on ENERGY and turn **KNOB 1** to DEPTH **0**,
   so the drone stays steady through it all.
3. **MODS**: mod 1 from **ENERGY** (after MACRO 4 in KNOB 1's list) to **T1 CUTOFF**, **+40**. The drone opens as
   the piece gets busy.
4. Mod 2 from **T1 TIDE 1** to **M1 AMT** (near the end of KNOB 3's list), **+100**; tap **OCT+** once for **+**.
   Hold **OCT-** and turn **KNOB 3** right: **M1*** on the row, mod 2 now scales mod 1 between nothing and all of it,
   over TIDE 1's 17 minutes a turn.

**Watch it:** pick mod 1 and hold OCT-: 64 bars of ENERGY dashed, and the filter's move solid inside it, a wide copy
when TIDE 1 is high and almost flat when it is low.

**Variation:** for a planned shape (intro, build, peak, ending) instead of an endless one, add **GLO > PIECE** with a
LENGTH: see [A piece that plays itself](RECIPES.md#a-piece-that-plays-itself-piece-chain-and-entries).

Three speeds: the notes, the tide (minutes), and the slow change in how much the tide matters.

### 3. Feedback: a system that plays itself

Patch a module's output back into its own control and it starts to steer itself: an LFO that speeds up when it's
high and slows down when it's low, a random voltage that decides how soon the next random voltage comes. Feedback
is what makes a patch feel less like a machine running a program and more like something with a will of its own.
The art is keeping it from running away or freezing; the last recipe here is the brake.

#### An envelope that plays itself

*On a modular: the Krell patch, which Todd Barton built on a Buchla 200e as a homage to Louis and Bebe Barron's
soundtrack for* Forbidden Planet *(1956). A looping envelope whose end of cycle fires the next one; at each cycle,
random voltages set its own rise and fall, and the sound's pitch and colour.*

**What you hear:** a pad that breathes in and out, a new note on each breath, every breath its own length. Nothing
here runs on the clock: the patch decides when the next note comes.

**Cords:**
- `T1 EOC → T1 CLOCK, +100`
- `T1 S&H → T1 LFO RTE, +20`
- `T1 LFO → T1 GAIN, +100`

**Keys:**
1. Track 1 (GLASS PAD), **SEQ** for GEN: MODE **MARKOV**, **KNOB 3** DENS **100** (a note every step). **ARP** for
   LENGTH: **KNOB 1** LENGTH **HIT**, so each note rings until the next.
2. **LFO** for LFO: **KNOB 1** RATE **35** (about three seconds a cycle), **KNOB 2** WAVE **TRI**.
3. **MODS**: mod 1 from **T1 EOC** (the last of track 1's sources) to **T1 CLOCK**, **+100**. EOC is a short gate
   at the start of each LFO cycle; CLOCK steps the generator on each one instead of on its DIV. Press **PLAY**: a note
   at the top of every cycle.
4. Mod 2 from **T1 S&H** to **T1 LFO RTE**, **+20**. S&H draws a new value as each cycle starts, and sets that
   cycle's speed: the notes come at irregular, self-made times, from about one second apart to ten.
5. Mod 3 from **T1 LFO** to **T1 GAIN**, **+100**. GAIN at +100 is a VCA: silent at the bottom of the LFO, full at
   the top. The triangle starts each cycle at the bottom, so each note swells up from silence and fades away just as
   the next one comes.

**Watch it:** pick mod 3 and hold OCT-: the swells drawn one after another, each a different width.

**Variation:** mod 3 at **−100** and LFO **WAVE SAW**: each note starts at full and dies away, a plucked Krell. Or
add `T1 S&H → T1 CUTOFF, +30` for a colour of its own on each note.

#### Stepped chaos

*On a modular: Rob Hordijk's Rungler (in his Benjolin and Blippoo Box): a shift register clocked by one oscillator,
fed by another, its stepped output patched back into the oscillators' rates. A melody that half-repeats and keeps
pulling on its own clock.*

**What you hear:** a stepped bell line that almost repeats, speeding up and slowing down as it goes, now settling
into a figure, now breaking out of it.

**Cords:**
- `T1 EOC → T3 CLOCK, +100`
- `T3 CV → T1 LFO RTE, +25`
- `T1 S&H → T3 T PROB, +40`

**Keys:**
1. Track 3 (BELLS), **SEQ** for GEN: MODE **TURING**, LENGTH **8**, PROB **10**. Track 1's LFO is the clock
   (track 1 plays nothing): **LFO** for LFO, **KNOB 1** RATE **40**, a cycle every two and a half seconds.
2. **MODS**: mod 1 from **T1 EOC** to **T3 CLOCK**, **+100**. The register steps once each cycle of track 1's LFO.
3. Mod 2 from **T3 CV** (the register) to **T1 LFO RTE**, **+25**. The register sets the speed of its own clock: high
   values step faster, up to about two steps a second.
4. Mod 3 from **T1 S&H** to **T3 T PROB**, **+40**. The clock's own random value decides how often the register
   flips a bit.

**Watch it:** pick mod 2 and hold OCT-: a bar of the register, stepped (dashed), and the clock rate it sets (solid);
the high steps are the short ones.

**Variation:** mod 2 at **−25**: high values slow the clock, so the line lingers on its top notes.

#### Two LFOs pulling on each other

*On a modular: two LFOs, each one's output into the other's rate input.*

**What you hear:** two simple, regular motions become one that's hard to predict but never jumpy: good for pads and
anything that should drift.

**Cords:**
- `T1 LFO → T2 LFO RTE, +40`
- `T2 LFO → T1 LFO RTE, +40`
- `T1 LFO → T1 CUTOFF, +30`
- `T2 LFO → T2 PAN, +50`

**Keys:**
1. Tracks 1 and 2 play long notes (a held chord on each, or **CHORD** with HIT HOLD).
2. **MODS**: mods 1 and 2, each LFO onto the other's **LFO RTE**, **+40**. Mods 3 and 4 let you hear them: track 1's
   LFO on its **CUTOFF**, track 2's on its **PAN**.
3. Push it: **+70** on both rate mods and they lurch; **−40** on one makes them chase instead of push.

**Watch it:** pick mod 3 and hold OCT-: the filter's sweep stretches and squeezes, never quite the same twice.

**Variation:** a ring of four: mod 2 becomes `T2 LFO → T3 LFO RTE`, and add `T3 LFO → T4 LFO RTE` and
`T4 LFO → T1 LFO RTE`. Each LFO is pushed by the one before it. It takes every track's LFO.

#### Two generators steering each other

*On a modular: two sequencers or random sources cross-patched: one's CV moves the other's probability, the other's
moves the first one's.*

**What you hear:** a system that never quite settles: stretches where the two lock into a pattern, and moments where
one change starts a cascade.

**Cords:**
- `T3 CV → T2 M LEAP, +40`
- `T2 CV → T3 T PROB, +30`

**Keys:**
1. Track 3 **TURING**, PROB **10**; track 2 **MARKOV**.
2. **MODS**: mod 1 from **T3 CV** to **T2 M LEAP**, **+40** (Turing's register widens Markov's leaps); mod 2 from
   **T2 CV** to **T3 T PROB**, **+30** (Markov's note unlocks Turing's loop).

**Watch it:** pick mod 2 and hold OCT-: Markov's note (dashed) and the push on Turing's PROB (solid); a calm
stretch is a short, flat line.

**Variation:** brake it, below.

A generator reads its settings only at its own step, so the loop has a built-in delay: it wanders, but it doesn't
explode.

#### A brake for any feedback patch

*On a modular: a VCA on every feedback path, all on one control voltage, so one knob takes the patch from dry to
wild.*

**What you hear:** the same patch from calm to tangled on one control: each generator on its own, then a little
influence, then the whole web.

**Cords:** none (or `MACRO 1 → MOD DEP, −100` to play it from a knob).

**Keys:**
1. Any feedback patch above, playing.
2. **GLO** for PERF (the page after MIXER); hold **OCT+** for PERF's sheet: **Mod depth**, **KNOB 2** from 100 % down. MOD DEPTH scales the
   moving part of every mod at once (never an OFFSET): at 0 the patch is dry, every generator as you set it.
3. To ride it while you play: on MODS, a mod from **MACRO 1** to **MOD DEP** (the end of KNOB 3's list), **−100**.
   On PERF, **KNOB 1** up brakes the whole patch.

**Watch it:** set Mod depth to 30 %, go back to MODS and hold OCT- on a feedback mod: the solid line is a third of
the height it was, the dashed source as busy as ever.

**Variation:** brake the Krell: its notes keep coming (a CLOCK mod keeps its division), but the swells flatten and
every breath takes the same time.

### 4. From a curve to an event

Much of a modular patch is continuous: LFOs, envelopes, random voltages gliding. Music also needs events: a note,
a fill, a part coming in. The bridge is the **comparator**: a gate that is on while a voltage is above a threshold
and off below it. A slow curve becomes a long, irregular rhythm of on and off. A **quantizer** does the same for
pitch, turning a gliding voltage into notes of a scale.

#### A voice that comes and goes with the tide

*On a modular: a slow random voltage into a comparator; its gate opens a VCA on a voice.*

**What you hear:** a part that comes in and out without an arrangement: a few minutes in, a few minutes out, never
on a schedule and never random either.

**Cords:**
- `T2 TIDE 2^ → T2 PLAY, +100`

**Keys:**
1. Track 2 plays a generator (MARKOV, say).
2. **MODS**: mod 1 from **T2 TIDE 2** to **T2 PLAY** (near the end of track 2's destinations), **+100**. Tap **OCT+**
   twice: **^**, a GATE. PLAY lets track 2 be heard while its mods reach half: now only while TIDE 2 is above the
   middle. (A shut track plays on unheard, so a track following it keeps its place.)

**Watch it:** hold OCT- on mod 1: 64 bars of TIDE 2 (dashed), the threshold across it, and the gate (solid) high
while track 2 is heard. Turn **KNOB 4** while holding to move the threshold: higher, shorter visits.

**Variation:** a window. Set the first mod's threshold to **−20** (hold OCT-, **KNOB 4**) and add `T2 TIDE 2^ → T2 PLAY, −100` at threshold
**+20**: track 2 plays only while TIDE 2 sits between them.

#### Long gates

*On a modular: two LFOs at unrelated rates mixed, the sum into a comparator: the gate pattern repeats only after the
LFOs' common period, minutes away. A staple of Serge and Buchla generative patches.*

**What you hear:** a voice that comes in for a few seconds, rests, comes back for longer, in a rhythm that takes
minutes to repeat, from two plain LFOs.

**Cords:**
- `T1 LFO → BUS A, +50`
- `T2 LFO → BUS A, +50`
- `BUS A^ → T3 PLAY, +100`

**Keys:**
1. Track 3 plays a generator (TURING on BELLS). Tracks 1 and 2 are the LFOs: track 1 **LFO** RATE **20** (about 7 s a
   cycle), track 2 RATE **25** (about 5 s).
2. **MODS**: mods 1 and 2 from **T1 LFO** and **T2 LFO** to **BUS A** (the end of KNOB 3's list), **+50** each. BUS A
   adds them (its MODE is SUM).
3. Mod 3 from **BUS A** (the end of KNOB 1's list) to **T3 PLAY**, **+100**; tap **OCT+** twice for **^**. Track 3
   plays while the sum is above the middle.

**Watch it:** hold OCT- on mod 3: four bars of the sum (dashed) against the threshold, swelling high when the two
LFOs agree and staying near the middle when they don't; the gate (solid) is long, then short, then long again. Keep
holding for a few sweeps: no two are alike.

**Variation:** hold OCT- on mod 3 and raise the threshold (**KNOB 4**) to **+40**: track 3 comes in only when both LFOs are
high together, rarely and briefly.

**Try next:**
- **Fills that come and go.** `ENERGY^ → FILL, +100`, threshold **+40**: steps set to FILL play while the piece is at
  its busiest.
- **Logic of two gates.** Two GATE mods into a bus at **+100**, and on MODS' sheet (hold **OCT+**) the bus's **Mode**:
  MAX is OR (either open), MIN is AND (both), DIFF is XOR (one or the other). Read the bus as a GATE mod.
- **A gate on a mod.** `T1 SLEW^ → M3 AMT, +100`, scaling (hold OCT-, **KNOB 3** right): mod 3 keeps its own amount
  and works only while SLEW is up.

### 5. Parts that listen to each other

Four tracks running side by side are four loops. Cross-patch them, so one's playing changes another's sound,
pitch or rhythm, and they start to sound like a band. On a modular this is the reason for a big case: every module
can hear every other.

#### One part makes room for another

*On a modular: the kick's envelope into a VCA on the pad (sidechain ducking).*

**What you hear:** the pad breathing with the drums. It's the simplest cross-patch, and you hear it at once.

**Cords:**
- `T4 ENV → T1 LEVEL, −60`

**Keys:**
1. Track 4 (808 KIT): **SEQ** for GEN, MODE **ATLAS**, **KNOB 4** DENS **40**. Track 1 plays a long pad (LENGTH HIT,
   or CHORD with HIT HOLD).
2. **MODS**: mod 1 from **T4 ENV** to **T1 LEVEL**, **−60**. Each hit pushes the pad down for the length of its
   envelope.

**Watch it:** hold OCT- on mod 1: a bar of the kit's envelopes (dashed) and the dips they cut in the pad (solid).

**Variation:** `T4 ENV → T1 CUTOFF, −40` in place of LEVEL, a filter duck you feel more than hear.

#### An envelope from another track's notes

*On a modular: a gate into a slew limiter (Maths' rise and fall): every gate becomes a smooth envelope, sent to as
many places as you like.*

**What you hear:** track 3's notes opening track 1's filter, each with a soft rise and fall; notes in a row hold it
open.

**Cords:**
- `T3 GATE → BUS B, +100`
- `BUS B → T1 CUTOFF, +60`

**Keys:**
1. Track 3 plays a sparse generator (MARKOV, DENS **40**); **SEQ** for PATTERN, **KNOB 2** DIV **1/8**. Track 1 plays
   a long pad.
2. **MODS**: mod 1 from **T3 GATE** to **BUS B**, **+100**. Hold **OCT+** for MODS' sheet: **Bus B lag 100 ms**
   (KNOB 1 the row, KNOB 2 the value), then **OCT-** to close it. The bus now rises and falls over 100 ms with each
   gate: an attack and a release.
3. Mod 2 from **BUS B** to **T1 CUTOFF**, **+60**.

**Watch it:** hold OCT- on mod 2: the bus's ramps (dashed), each one a gate rounded off, and the filter following.

**Variation:** send the same envelope somewhere else, with no amounts to keep in step: `BUS B → T2 LEVEL, +40`. Or a
longer Lag (**500 ms**) for a slow swell that only the busiest phrases open fully.

#### A sample and hold clocked by another voice

*On a modular: a random or slow voltage into a sample and hold whose clock is another sequencer's gate (the
Wogglebug's stepped output, clocked from outside).*

**What you hear:** track 3's filter taking a new place on each of track 2's notes, and staying there: the two parts
move together without playing together.

**Cords:**
- `T1 SWARM 5= → T3 CUTOFF, +50`, held on track 2's notes

**Keys:**
1. Track 3 plays a generator; track 2 a sparser one (EUCLID, STEPS **16**, PULSE **3**).
2. **MODS**: mod 1 from **T1 SWARM 5** to **T3 CUTOFF**, **+50**. Tap **OCT+** three times: **=**, HOLD.
3. Hold **OCT-** and turn **KNOB 1** to **HOLD ON T2 NOTES**: SWARM 5 is taken on each of track 2's notes and held
   until the next.

**Watch it:** hold OCT- on mod 1: SWARM 5's smooth wave (dashed) and the steps cut from it (solid), one at each of
track 2's notes.

**Variation:** a second mod from the same source to **T3 SHIFT** (+30, HOLD on T2's notes): track 3 is transposed
in its scale at the same moments.

**Try next: one track's notes open another's mod.** `T3 ENV → M2 AMT, +100`, scaling (hold OCT-, KNOB 3 right),
where mod 2 is `T1 LFO → T1 CUTOFF, +40`: the pad's filter moves only while track 3's notes ring.

### 6. How much, not which

A lot of generative music is shaped by density rather than notes: how often something plays, how full a pattern is,
whether two rhythms land together. Clock dividers, Euclidean patterns and gate logic are a modular's tools for this,
and changing a density by hand (or by a slow source) changes the feel more than changing any single note.

#### A rhythm web

*On a modular: one clock into dividers (/2, /3, /5) and Euclidean modules, their gates combined with logic (AND,
XOR), the web patched back so one rhythm moves another's fill or rotation.*

**What you hear:** an interlocking groove from three simple parts that keeps finding new shapes without ever losing
the beat.

**Cords:**
- `T2 GATE → T3 E ROT, +25`
- `T1 SWARM 1 → T2 E PULSE, +30`

**Keys:**
1. Track 2: **SEQ** for GEN, MODE **EUCLID**, **KNOB 2** STEPS **16**, **KNOB 3** PULSE **5**. Track 3: EUCLID,
   STEPS **12**, PULSE **4** (12 against 16: the two meet every three bars).
2. Track 3, **ARP** for TWEAK 2: **KNOB 3** LOGIC **XOR T2**. Track 3 plays only where the two rhythms *don't* land
   together: they interlock.
3. **MODS**: mod 1 from **T2 GATE** to **T3 E ROT**, **+25**: each of track 2's hits turns track 3's pattern a little.
   Mod 2 from **T1 SWARM 1** to **T2 E PULSE**, **+30**: over a minute, track 2 thins out and fills up again.

**Watch it:** hold OCT- on mod 2: SWARM 1's slow wave over 16 bars, and the pulses it adds and takes away.

**Variation:** a clock divider with nothing to line up: set track 3's LOGIC back to OFF and add
`T2 GATE → T3 CLOCK, +33`. Track 3 now steps on every third of track 2's hits instead of on its DIV (50 % every
second, 25 % every fourth). AND, OR and NOT on LOGIC do the other
classic things: play only together, either, or only when the other rests.

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
GRAVITY (a pull back home) and seeds that replay. Recipes: [A loop you can lock and loosen](#a-loop-you-can-lock-and-loosen),
[Stepped chaos](#stepped-chaos).

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

### The utility modules: buses, EOC, CLOCK, HOLD

The mods' own building blocks restate a modular's utility modules, ideas only:

- **BUS A and BUS B** are a mixer and a slew limiter in one: mods summed, or combined as MAX, MIN and DIFF (the OR,
  AND and XOR of gates), with a LAG that turns a gate into a rise and fall, as Make Noise's Maths does.
- **EOC** is the end-of-cycle pulse of a cycling envelope (Maths again, and Buchla's function generators), and
  **CLOCK** is the clock input it is patched into: a generator stepped by any gate, divided by its amount.
- **HOLD** is a sample and hold, clocked by the bar or by a track's notes; **S&H** and **SLEW** are one clocked by
  the track's LFO, stepped and smoothed, as on Make Noise's Wogglebug (after Ken Stone's design).
- **GATE** is a comparator, with a little hysteresis, as Ken Stone's Gated Comparator has.

### And Tessera's own

BOUNCE (balls in a box, a note at each wall), POLY (four notes on four rhythms), CHORD (progressions in the
track's scale) and the long-form layers (FORM, CHAIN, ENTRIES, the piece planner) are Tessera's own, built on the
same principles: a system you set up, with an off position for everything automatic.

---

## Tessera's limits, and why they help

Coming from a modular, a few differences matter:

- **Four tracks, four LFOs.** Each track has one LFO, so a ring of four cross-modulated LFOs uses them all. The slow
  sources (DRIFT, TIDE, SWARM, ENERGY), each generator's GATE, NOTE and CV, the two buses and each LFO's EOC are extra
  sources that cost no LFO.
- **Sixteen cords.** Most recipes here use three or fewer. A limit like this keeps a patch readable; **MATRIX**
  shows who moves whom, and the mod scope shows how.
- **Control rate.** Mods move about 1,400 times a second, plenty for anything but audio-rate modulation, which the
  engines (FM6, ORACLE's POLY MOD) do inside the voice.
- **Events.** A mod can open, close or thin a part (PLAY, DROP, FILL, density), and **CLOCK** steps a generator on
  any gate, so notes can come off the grid, at the patch's own times; a track's DIV is then the fastest it steps.
- **Intervals.** A mod onto PITCH moves in small, unquantized steps: right for vibrato and drift, wrong for a fifth.
  For an interval, use **SHIFT** (a mod onto it moves in whole scale steps) or FOLLOW.
- **One brake.** MOD DEPTH (GLO > PERF's sheet) scales every mod at once, so a feedback patch always has a way back
  to dry.
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
- Make Noise (Maths, Wogglebug): <https://www.makenoisemusic.com/>
- Todd Barton building the Krell patch on a Buchla: <https://vimeo.com/48466272>
