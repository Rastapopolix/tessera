# Tessera 3.0 demo projects

Three projects that show what Tessera 3.0 does, each a few minutes of music that keeps changing on its own. Press
PLAY and leave it, then open the pages named below to see what is moving and turn things.

| File | Name on the FM-1 | What it shows |
|---|---|---|
| `still-tide.tsp` | STILL TIDE | Ambient: an ORACLE drone with BREATH, GRAVITY, FALL through the FILTER effect, a bass from STEP as seed, ENERGY |
| `koto-garden.tsp` | KOTO GARDEN | FM koto in HIRA on QUAD, an ORACLE flute in a FORM that develops, a STEP mokugyo with conditions and Accumulate |
| `low-harbour.tsp` | LOW HARBOUR | Ambient dub: ORACLE chord stabs into a drifting echo, a sub bass, a 909 one drop, RESTS |

They are made by the firmware's own code (`tests/demo_projects.c`), which saves each one to a slot, reads it back
as the backup page does, loads it and plays it; `tests/run_tests.sh` checks that these files still load and play.

## Putting them on your FM-1

The demos go into your slots through a backup of your own FM-1, so everything else on it stays as it is.

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

The video (about 3 minutes, 1080 x 1080) is made from the firmware's own sound and screens: both demos as `make`
plays them (their first 20 s are the same samples as the WAVs above), then a short tour of the modes the demos leave
out (TURING, EUCLID, MORPH, ATLAS, CONSTELLATION, ANSWER).

```
./build.sh
cc -O2 -w -Ibuild/gen -Ifirmware/src -Itests -DMELODEE_SID=0 -DMELODEE_PHYS=0 -DTRK_START_TESS=1 \
   -o build/dp tests/demo_projects.c -lm
build/dp video build/video                      # each clip's WAV, frames and captions (about 900 MB of frames)
python3 tools/demo_video.py build/video build/tessera-2.0-demo.mp4   # needs Pillow and imageio-ffmpeg (or ffmpeg)
```

The pages shown and the captions are the timelines in `tests/demo_projects.c` (V_STILL, V_GARDEN, the tour).

The project page plays the copy in `assets/video/` (`tessera-2.0-demo.mp4`, and `tessera-2.0-demo.jpg`, its title card
as the poster); `web/make_site.py` puts both in the site's `media/video/`. After making a new video, copy it there
and make the poster again:
`ffmpeg -ss 1 -i assets/video/tessera-2.0-demo.mp4 -frames:v 1 -vf scale=720:720 -q:v 3 assets/video/tessera-2.0-demo.jpg`.
