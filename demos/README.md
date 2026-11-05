# Tessera demo projects

Eight projects that show what Tessera does, each music that keeps changing on its own. Press PLAY and leave it, then
open the pages named below to see what is moving and turn things. The first three came with 3.0; the five after them
came with 3.3 and are built round the long form: the **CHAIN** (one FORM for the whole project, with **KEY B** and **KEY
C**), **ENTRIES** (tracks leaving and coming back on its phrase lines) and the **PIECE** (a length and the shape it
takes). They need Tessera 3.3 or later.

| File | Name on the FM-1 | What it shows |
|---|---|---|
| `still-tide.tsp` | STILL TIDE | Ambient: an ORACLE drone with BREATH, GRAVITY, FALL through the FILTER effect, a bass from STEP as seed, ENERGY |
| `koto-garden.tsp` | KOTO GARDEN | FM koto in HIRA on QUAD, an ORACLE flute in a FORM that develops, a STEP mokugyo with conditions and Accumulate |
| `low-harbour.tsp` | LOW HARBOUR | Ambient dub: ORACLE chord stabs into a drifting echo, a sub bass, a 909 one drop, RESTS |
| `ember-field.tsp` | EMBER FIELD | Drone: a fifth that never moves (CHAIN: OWN), a tanpura cycle in STEP, a flute whose AABA sections differ; a 20-minute PIECE |
| `night-ferry.tsp` | NIGHT FERRY | Ambient dub 2: an 808 rockers beat, an organ skank, a STEP bass the CHAIN moves to the IV and bVII; ENTRIES cuts |
| `tin-roof.tsp` | TIN ROOF | Drums with variation: ATLAS moved by SWARM and new each section, a 909 part with conditions, ratchets and FILL |
| `cold-orbit.tsp` | COLD ORBIT | Berlin school: a TURING sequence under a filter sweep, ABCA through three keys, a 15-minute ARCH that stops |
| `paper-kites.tsp` | PAPER KITES | Loops of 11 and 13 steps meeting differently each time, a CHAIN that WANDERs through three keys; 30 minutes in WAVES |

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
in, so give it a few minutes; the PIECE page shows where it is. In the CHAIN, a generator track that FOLLOWs plays its own
A, B and C sections (how new B and C are is its FORM CONTRAST; how much a return of A varies, its DEVELOP); a melodic STEP
track follows only KEY B and KEY C. Drum tracks never change key. `demo_projects form N SECS` prints how demo N unfolds:
each section, the tracks out, the notes in each.

The editor's Demo projects lists them under the first three, set to **Leave out**: choose a slot for the ones you want.
With the backup file, name them: `python3 tools/demo_backup.py my-backup.json out.json --slots A,B demos/ember-field.tsp
demos/tin-roof.tsp`.

## EMBER FIELD

E phrygian, 60 BPM. A drone.

- **T1 TRIO FIFTH DRONE**, CHORD: PROG **DRONE**, HIT HOLD, chord **POW**, OCT −1, BREATH 45. Set **OWN** in the CHAIN
  (OCT+ held on FORM), so the drone never moves; ROLE **ANCHOR**, so ENTRIES never takes it out.
- **T2 ORACLE DRONE**, MORPH in whole notes, MONO, ringing on: a slow line inside the drone, new in B (CONTRAST 45).
- **T3 FM6 KOTO WARM**, STEP: a tanpura's cycle in quarter notes (B, E', E', E, a rest, B, E', E).
- **T4 TRIO BREATH FLUTE**, MARKOV in quarter notes, sparse (DENS 35), MONO: the voice of the piece. In A it keeps to
  a few notes; **CONTRAST 70** makes B far from it, and **DEVELOP 30** varies each return of A.
- **CHAIN** AABA, PHRASE 16 bars, WANDER 20. **ENTRIES** 45, DWELL 2, FADE over 4 bars. **PIECE** 20 minutes,
  **PLATEAU**, END **LOOP**. ENERGY ARC 30.
- **MODS**: T2 TIDE 1 → T2 CUT, T1 DRIFT → T1 PAN, T4 SWARM 1 → T4 FX SEND.

## NIGHT FERRY

G minor, 76 BPM. The second ambient dub: its own beat, its own key moves.

- **T1 FM6 DRAWBARS**, EUCLID 8 / 2 turned 2, in eighths, DIA3 chords cut short (LENGTH FIXED 1/16): the skank.
- **T2 FM6 SOLID BASS**, STEP: a two-bar line in eighths. **KEY B +5** takes it (and the skank and lead) to C minor in
  B, **KEY C −2** to F in C: a dub's IV and bVII.
- **T3 808 KIT**, STEP: rockers, the kick on one and three, rims on two and four, hats by **Condition** (1:2, 2:4), a
  tom pickup on the fourth bar (4:4), a clap on **FILL** that T3's SWARM 5 holds now and then.
- **T4 TRIO SOFT LEAD**, TURING over 8 steps, DROP 55, most of it into the echo; **SKIP 25** holds its section a phrase
  now and then.
- **CHAIN** ABAC (A twice), PHRASE 8, WANDER 15. **ENTRIES 60, CUT**, DWELL 1: tracks drop out and slam back on the
  phrase lines, as a dub's mix does. **PIECE** 8 minutes in three **WAVES**, END **NEW** (a new piece each time).
- **MODS**: T1 DRIFT → SF2 RTE (the echo's time bends), T1 TIDE 2 → SF2 CHR (its feedback), T3 SWARM 5 → FILL (a gate),
  T4 TIDE 3 → T4 SEND 2.

## TIN ROOF

A mixolydian, 108 BPM. Drums that keep changing.

- **T1 808 KIT** on **ATLAS**: X 150, Y 110; T1's SWARM 2 and 3 move X and Y, ENERGY its DENS, and each CHAIN section
  is a pattern of its own (CONTRAST 65), a return of A a little changed (DEVELOP 35). ROLE ANCHOR.
- **T2 909 KIT**, STEP, two bars of rims, cowbell and toms: **Conditions** (1:2, 2:4, 3:4, 2:2, 4:4), a tom fill on
  **FILL** (T2's SWARM 6 holds it now and then), **RATCH x2** at CHANCE 18 for rolls, HUMAN 10 ms.
- **T3 TRIO FAT BASS**, EUCLID 16 / 5 in sixteenths, an octave down, cut short; **T4 FM6
  CLAVINET**, POLY (3, 4 and 6 against each other), SKIP 30. Both move with **KEY B +5** and **KEY C −2**.
- **CHAIN** ABAC (A twice), PHRASE 8, WANDER 30. **ENTRIES** 55, CUT. **PIECE** 10 minutes, **BUILDS** 3, END **NEW**.
- **MODS**: T1 SWARM 2 → T1 X, T1 SWARM 3 → T1 Y, T2 SWARM 6 → FILL, ENERGY → T1 DENS, T4 DRIFT → T4 CUT.

## COLD ORBIT

D minor, 120 BPM. A sequencer piece in the Berlin way.

- **T1 ORACLE SEQ BLIP**, TURING, 16 steps of sixteenths, PROB 4 (all but locked), MONO; T1's TIDE 2 sweeps its CUT
  over minutes and SWARM 4 its RES. Each section a new sequence (CONTRAST 55).
- **T2 FM6 SAW BASS**, EUCLID 16 / 7 in eighths, two octaves down, MONO.
- **T3 TRIO STRING MACH**, CHORD: the minor **CIRCLE**, 4 bars a chord, OPEN, held.
- **T4 FM6 SYNC LEAD**, AWAKE in quarter notes, sparse, MONO.
- **CHAIN** **ABCA** (A twice), PHRASE 8: **KEY B −2** (C minor), **KEY C +3** (F minor). **ENTRIES** 70, FADE over
  2 bars. **PIECE** 15 minutes, **ARCH** peaking at 60 %, END **STOP**: it layers in, peaks, thins and stops.
- **MODS**: T1 TIDE 2 → T1 CUT, T1 SWARM 4 → T1 RES, ENERGY → T1 SEND 2, T3 DRIFT → T3 PAN.

## PAPER KITES

F major pentatonic, 66 BPM. Loops of different lengths, after Eno's airport music.

- **T1 FM6 HARP**, STEP: F, A and C' in a loop of **11** eighths. **T2 ORACLE KEYS**, STEP: G and D' in a loop of
  **13**. They meet differently each time round, and both follow KEY B and KEY C.
- **T3 TRIO SOFT PAD**, MORPH in whole notes, an octave down, ringing; **T4 ORACLE GLASS PAD**, CONSTELLATION in
  quarter notes catching a few stars. Both FOLLOW the CHAIN.
- **CHAIN** **ABCA**, PHRASE 16, **WANDER 40** (the way through A, B and C is drawn): **KEY B +5** (B flat), **KEY C −3**
  (D). **ENTRIES** 50, DWELL 3, FADE over 4 bars. **PIECE** 30 minutes in four **WAVES**, END **LOOP**.
- **MODS**: T3 TIDE 1 → T3 CUT, T2 DRIFT → T2 PAN, T1 SWARM 2 → T1 FX SEND.

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
