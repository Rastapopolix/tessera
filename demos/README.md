# Tessera demo projects

Seven projects that show what Tessera does, each music that keeps changing on its own. Press PLAY and leave it, then open
the pages named below to see what is moving and turn things. The first three came with 3.0; the four after them came
with 3.3 and are built round the long form: the **CHAIN** (one FORM for the whole project, with **KEY B** and **KEY
C**), **ENTRIES** (tracks leaving and coming back on its phrase lines) and the **PIECE** (a length and the shape it
takes). They need Tessera 3.3 or later.

| File | Name on the FM-1 | What it shows |
|---|---|---|
| `still-tide.tsp` | STILL TIDE | Ambient: an ORACLE drone with BREATH, GRAVITY, FALL through the FILTER effect, a bass from STEP as seed, ENERGY |
| `koto-garden.tsp` | KOTO GARDEN | FM koto in HIRA on QUAD, an ORACLE flute in a FORM that develops, a STEP mokugyo with conditions and Accumulate |
| `low-harbour.tsp` | LOW HARBOUR | Ambient dub: ORACLE chord stabs into a drifting echo, a sub bass, a 909 one drop, RESTS |
| `slow-light.tsp` | SLOW LIGHT | Drone: a TRIO fifth struck once and held, a wind, an ORACLE choir and a SOFT SWELL that change with the CHAIN's AABA; 15 minutes in WAVES |
| `cedar-rain.tsp` | CEDAR RAIN | Plucked strings in KUMOI: a harp on TURING, a koto on EUCLID, a MARKOV flute, 808 hand drums that ADORN varies; ABAC through three keys |
| `stone-pier.tsp` | STONE PIER | Ambient dub 2: slow STEP chords into the echo, a sparse sub bass, an 808 one drop; ENTRIES cuts parts in and out |
| `far-shore.tsp` | FAR SHORE | Deep dub, after Rhythm & Sound: one Gm stab drowned in echo and room, a round sub, an 808 rockers beat, air |

They are made by the firmware's own code (`tests/demo_projects.c`), which saves each one to a slot, reads it back
as the backup page does, loads it and plays it; `tests/run_tests.sh` checks that these files still load and play.

## Putting them on your FM-1

**From the web editor** (Tessera 3.0 or later on the FM-1): connect it, open **Projects** > **Demo projects**, choose
a slot for each demo (or **Leave out**) and press **Put on the FM-1**. The editor stops playback, lists what each
chosen slot holds now and asks before it writes; only those slots change. Then SAVE > PROJECT: pick the slot and
load it, and PLAY. Save a backup first (**Complete backup** > **Save backup**) if you want to keep the projects in
those slots.

**With a backup file** (Python 3, from this repository): the demos go into your slots through a backup of your own
FM-1, so everything else on it stays as it is.

1. Connect the FM-1 to the web editor and stop playback. **Complete backup** > **Save backup**: keep that file safe,
   it is your way back.
2. Put the demos into a copy of it (Python 3, from this repository):

   ```
   python3 tools/demo_backup.py my-backup.json my-backup-with-demos.json
   ```

   That puts STILL TIDE, KOTO GARDEN and LOW HARBOUR into slots **B**, **C** and **D** and lists what each slot held.
   `--slots C,D` puts only the first two into C and D; `--slots D demos/low-harbour.tsp` picks one demo and its slot.
3. **Complete backup** > **Restore backup**, and pick `my-backup-with-demos.json`. Then switch the FM-1 off and on.
4. SAVE > PROJECT: pick the slot and load it, then PLAY.

The slots you choose are replaced: save anything you want to keep in them first (or choose other slots). To go back,
restore the backup from step 1.

## STILL TIDE

YO on D, 72 BPM. Made to run for an hour.

- **T1 ORACLE WARM PAD**, CHORD: PROG **DRONE**, BARS 4, HIT HOLD; hold OCT+ on GEN for CHRD DIA3, VOICING OPEN, OCT −1
  and **BREATH 80**: each voice of the chord swells and fades on its own slow cycle.
- **T2 TRIO WARM PAD**, AWAKE at half notes, PROB 55, LENGTH **HIT** (each note rings to the next); EVOLVE every 8
  bars, and **GRAVITY 40** (OCT+ held on EVOLVE) pulls it back home now and then.
- **T3 FM6 TINE EP**, BOUNCE with 5 balls: **FALL**. Leaves land on an electric piano, through the **FILTER** effect
  (FX 1) whose cutoff a TIDE opens and closes.
- **T4 TRIO SUB BASS**: a line written in STEP (D, A, G and rests), played by **TURING** from it (**SOURCE T4, KEEP
  RHYTHM**): the rhythm stays, the pitches change now and then.
- **ENERGY**: **ARC 60** thins the generators over 3 to 6 minutes and fills them again; **RESTS 70** gives them a rest
  of 8 to 18 s every few minutes while the drone holds.
- **MODS**: T1 TIDE 1 → T1 CUT, T1 SWARM 2 → T1 PAN, T3 TIDE 2 → T3 FX1 RTE (the filter), ENERGY → T2 FX SEND, T3 DRIFT
  → SF2 RTE (the echo's time bends a little).

## KOTO GARDEN

HIRA (Hirajoshi) on C, 92 BPM.

- **T1 FM6 KOTO PLUCK** and **T2 FM6 KOTO RING**, both **QUAD**: two rows of one shared grid, T1 in eighths, T2 in
  eighth triplets (pendulum), so they drift against each other. Hold OCT+ on GEN: column 1 moves **DOWN** every 8
  steps and column 3 at random every 16, so notes travel between the two kotos.
- **T3 ORACLE FLUTE**, **SERIES** (the infinity series) at quarter notes, ringing on, in a **FORM ABAC** of 4-bar
  sections with **DEVELOP 40** (OCT+ held on FORM): each return of A is A varied a little.
- **T4 FM6 MOKUGYO**, STEP: a figure in sixteenths; one pickup plays every other bar (**Condition 1:2**), one on the
  fourth (**4:4**), and one note climbs a degree each pass and back (**Accumulate +1**, LIMIT 4, PING).
- **MODS**: T1 TIDE 3 → T1 Q PROB (the koto thins and fills about once a minute), T3 SWARM 4 → T3 CUT, T2 DRIFT →
  T2 PAN.

## LOW HARBOUR

A dorian, 70 BPM. An ambient dub: slow, with room for the echoes.

- **T1 ORACLE DUB CHORD**, CHORD: PROG **TWO** (Am and D), BARS 2, HIT **OFFBEAT**, with TWEAK **DROP 70** so most
  stabs are left out, and **BREATH 60**. Its amp envelope is short: the echo is the tail. A **FILTER** effect (FX 1)
  whose cutoff a TIDE opens over minutes.
- Send effects: a large REVERB and an **ECHO** of 321 ms (a dotted sixteenth at 70 BPM) with long feedback; T1 sends
  most of its sound into it.
- **T2 TRIO SUB BASS**, MARKOV in quarters, neighbours and rests; EVOLVE every 4 bars with **GRAVITY 50**.
- **T3 909 KIT**, STEP: a one drop (kick and rim on the third beat), hats on some offbeats by **Condition** (2:4, 1:2,
  3:4), an open hat every other bar and a rim pickup on the fourth; TIMING HUMAN 12 ms.
- **T4 FM6 FLUTE**, AWAKE in half notes, PROB 35: an answer now and then, into the echo.
- **ENERGY**: ARC 45, **RESTS 60**. **MODS**: T1 TIDE 2 → T1 FX1 RTE, T1 DRIFT → SF2 RTE (the echo's time bends like a
  tape's), T1 TIDE 3 → T1 SEND 2, T4 SWARM 3 → T4 PAN.

## The 3.3 demos: the long form

Each one sets the three GLO pages of 3.3 (**PIECE**, **CHAIN**, **ENTRIES**). A planned piece begins sparse and layers
in, so give it a minute; the PIECE page shows where it is. In the CHAIN, a generator track that FOLLOWs plays its own A,
B and C sections (how new B and C are is its FORM CONTRAST; how much a return of A varies, its DEVELOP); a melodic STEP
track follows only KEY B and KEY C. Drum tracks never change key. `demo_projects form N SECS` prints how demo N unfolds:
each section, the tracks out, the notes in each.

The editor's Demo projects lists them under the first three, set to **Leave out**: choose a slot for the ones you want.
With the backup file, name them: `python3 tools/demo_backup.py my-backup.json out.json --slots A,B demos/slow-light.tsp
demos/stone-pier.tsp`.

They are made to play clean on the FM-1: no held note is struck again while it sounds, no voice is taken from another
(0 in 15 minutes at the budget of 16), and the voices' CPU cost stays under STILL TIDE's and KOTO GARDEN's
(`demo_projects solo N 0 SECS` prints it).

## SLOW LIGHT

D dorian, 54 BPM. A drone.

- **T1 TRIO FIFTH DRONE**, STEP: one chord (D, A, D') on the first step, the other 63 steps **TIE**, DIV 4 BAR: struck
  once at PLAY and held for the whole piece. ROLE **ANCHOR**, so ENTRIES never takes it out. T1's TIDE 1 moves its CUT.
- **T2 TRIO WIND**, STEP the same way on a high D: air above the drone, its CUT on T2's TIDE 2, its PAN on SWARM 2.
- **T3 ORACLE VOX**, MORPH, a note every 2 bars ringing into the next, LEGATO: a choir's slow line; new in B (CONTRAST
  50), each return of A a little changed (DEVELOP 30). T3's DRIFT moves its vowel (MACRO).
- **T4 FM6 SOFT SWELL**, AWAKE in whole notes an octave up, DROP 45: a chord tone rising out of the drone now and then,
  deep in the reverb; T4's TIDE 3 sends it into the echo and back.
- **CHAIN** AABA, PHRASE 16 bars, WANDER 10. **ENTRIES** 40, DWELL 2, FADE over 4 bars. **PIECE** 15 minutes in three
  **WAVES**, END **LOOP**. ENERGY ARC 30.

## CEDAR RAIN

E kumoi, 80 BPM. Plucked strings and a flute, as KOTO GARDEN, but made another way.

- **T1 FM6 KUGO HARP**, TURING over 12 eighths, PROB 6 (a note now and then new), DROP 30, LENGTH FIXED 1/4. ROLE
  ANCHOR. Each section its own figure (CONTRAST 55). T1's TIDE 3 thins and fills it (DROP).
- **T2 FM6 KOTO WARM**, EUCLID 16 / 5 in sixteenths turned 3, an octave down: the pulse.
- **T3 TRIO BREATH FLUTE**, MARKOV in half notes an octave up, LEGATO, close steps and rests: the song. B and C wander
  further (CONTRAST 70, DEVELOP 35). T3's SWARM 4 moves its breath (CUT).
- **T4 808 KIT**, STEP, two bars of sixteenths: low toms and rims like hand drums, **Conditions** (1:2, 2:4, 4:4), and
  **STEP EVOLVE** HOW **ADORN** (every 4 bars, AMOUNT 20): ornaments in the rests, a little different each round.
- **CHAIN** ABAC (A twice), PHRASE 8, WANDER 15: **KEY B +5** (A), **KEY C −2** (D). **ENTRIES** 45, FADE over 2
  bars. **PIECE** 12 minutes, **ARCH** peaking at 55 %, END **LOOP**.

## STONE PIER

F dorian, 66 BPM. The second ambient dub: slower and emptier than LOW HARBOUR.

- **T1 ORACLE DUB CHORD**, STEP, four bars of eighths: a chord on each bar's second beat held to the "and" (Fm7, Fm7,
  Bb7, Fm7, shells with no root: the bass has it), a Cm7 leading home on the fourth bar every fourth time (2:4). Through
  a **FILTER** that T1's TIDE 2 opens and closes over minutes, and nearly all of it into the **ECHO** (342 ms, a dotted
  sixteenth). ROLE ANCHOR: the chords are always there.
- **T2 TRIO SUB BASS**, STEP: F held for three eighths, then most of each bar empty; Bb under the third bar, a pickup
  every other time.
- **T3 808 KIT**, STEP: a one drop, the kick alone on three; a rim on four every other bar, hats by **Condition**, an open
  hat every fourth bar, a rim pickup on **FILL**.
- **T4 TRIO SOFT LEAD**, AWAKE in half notes, DROP 50, LEGATO, into the echo: a call now and then.
- **CHAIN** AB, PHRASE 8 (A twice): **KEY B +5**, the fourth, as a dub's verse goes. **ENTRIES 55, CUT**, DWELL 1: bass,
  drums and lead drop out and come back on the phrase lines, as a dub's mix does. **PIECE** 10 minutes in three
  **WAVES**, END **LOOP**. ENERGY ARC 30.
- **MODS**: T1 TIDE 2 → T1 FX1 RTE (the filter), T1 DRIFT → SF2 RTE (the echo's time bends like a tape's), T1 TIDE 1 →
  SF2 CHR (its feedback swells and settles), T4 SWARM 3 → T4 PAN.

## FAR SHORE

G minor, 76 BPM. A deep dub after Rhythm & Sound: almost nothing moves, and the space is the music.

- **T1 FM6 DX TINE**, STEP, two bars of sixteenths: one chord, Gm (B flat, D, F over the bass's G), a short stab just
  after two in each bar, a softer one on the "and" of four every other time (1:2). Through a **FILTER** that T1's TIDE 2
  opens and closes over minutes, nearly all of it into the **ECHO** (296 ms, a dotted sixteenth) and a big **REVERB**;
  T1's SWARM 3 throws some stabs deeper into the echo.
- **T2 TRIO SUB BASS**, STEP: a round line of long notes (TIEs): G, G again, B flat, C; D held, C, B flat, an F pickup
  every other time. POLY, so a note never restarts the one still ringing out.
- **T3 808 KIT**, STEP: NIGHT FERRY's beat (rockers: kick on one and three, rims on two and four, hats by **Condition**,
  a tom pickup on the fourth bar, a clap on **FILL** that T3's SWARM 5 holds now and then).
- **T4 TRIO WIND**, one note struck at PLAY and held by TIEs at 4 bars a step: air under everything; T4's TIDE 1 darkens
  and brightens it. OWN in the CHAIN.
- **CHAIN** AABA, PHRASE 16: **KEY B +5** takes the chord and the bass to C minor for B. **ENTRIES 50, CUT**, DWELL 1:
  the chord and the bass drop out and come back on the phrase lines; the drums and the air are ANCHORs. **PIECE** 12
  minutes in three **WAVES**, END **LOOP**.
- Made not to crackle: the echo's feedback is fixed (no cord moves it), the stabs are FM6 (a third of ORACLE's CPU), no
  sounding voice is ever struck again, no voice is taken.
- **MODS**: T1 TIDE 2 → T1 FX1 RTE, T1 DRIFT → SF2 RTE (the echo's time bends a little, as tape), T1 SWARM 3 → T1 SEND
  2, T3 SWARM 5 → FILL (a gate), T4 TIDE 1 → T4 CUT.

## Making them again

```
cc -O2 -w -Ibuild/gen -Ifirmware/src -DMELODEE_SID=0 -DMELODEE_PHYS=0 -DTRK_START_TESS=1 \
    -o build/host/demo_projects tests/demo_projects.c -lm      # (after ./build.sh: build/gen)
build/host/demo_projects make build/demos 180                  # the records, and 3 minutes of each as WAV
cp build/demos/*.tsp demos/
```

A change of what the generators play, or of the project format, can change the records: make them again and check
they still sound right (the WAVs) before committing.

## The demo video

The video (6 minutes, 1080 x 1080, in the Mono look) is made from the firmware's own sound and screens: the three
demos as `make` plays them (their first 20 s are the same samples as the WAVs above), then every generator with its
knobs turned through the real UI (`ui_input`, as a hand turns them), then what is new in 3.0, TWEAK and TIMING, and
LOW HARBOUR again under the end card, fading out.

```
cc -O2 -w -DMELODEE_LOOK=1 -Ibuild/gen -Ifirmware/src -Itests -DMELODEE_SID=0 -DMELODEE_PHYS=0 -DTRK_START_TESS=1 \
   -o build/dp tests/demo_projects.c -lm                     # (after ./build.sh or tools/build.py's generate(): build/gen)
build/dp video build/video                      # each clip's WAV, frames and captions (about 1.8 GB of frames)
python3 tools/demo_video.py build/video build/tessera-3.0-demo.mp4   # needs Pillow and imageio-ffmpeg (or ffmpeg)
```

The pages shown, the knob turns and the captions are the timelines in `tests/demo_projects.c` (V_STILL, V_GARDEN,
V_HARBOUR, v_generators, v_features); `DM_DEBUG=1` prints the knobs each clip ends on.

The project page plays the copy in `assets/video/` (`tessera-3.0-demo.mp4`, and `tessera-3.0-demo.jpg`, its title card
as the poster); `web/make_site.py` puts both in the site's `media/video/`. After making a new video, copy it there
and make the poster again:
`ffmpeg -ss 1 -i assets/video/tessera-3.0-demo.mp4 -frames:v 1 -vf scale=720:720 -q:v 3 assets/video/tessera-3.0-demo.jpg`.
