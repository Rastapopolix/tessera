# Test projects for the device checks

Short projects, one per feature still to be checked on the FM-1, each set so its feature shows within a minute or two.
They are plain music on purpose (A minor, 110 BPM, the same four tracks), so only the feature under test changes.
Put them on the FM-1 from the web editor (**Projects** > **Demo projects** > **Test projects**: pick a slot for each,
four at a time) and play them while you go down the checklist.

All four tracks start the same: **T1** a TRIO pad on CHORD, **T2** an FM6 piano on TURING (eighths), **T3** a TRIO sub
bass on EUCLID (3 in 8), **T4** the 909 on ATLAS.

| File | Name on the FM-1 | What to check |
|---|---|---|
| `chk-chain.tsp` | CHK CHAIN | GLO > CHAIN: FORM ABAC, PHRASE 4 bars (about 9 s), WANDER 0. Every track changes section on the same line; PLAY again replays the same. Turn WANDER up and try OWN / SKIP on a FORM page. |
| `chk-entries.tsp` | CHK ENTRIES | The same CHAIN with ENTRIES 50, FADE over 1 bar, T3 as BASS. Tracks fade out on section lines and come back one a phrase; the mute key blinks the bar before. Try STYLE CUT and a hand mute. |
| `chk-piece.tsp` | CHK PIECE | GLO > PIECE: 2 minutes, ARCH, END STOP, on ENTRIES 75. It layers in, is full at the top, thins out, fades and stops. Watch the PIECE page. |
| `chk-mods.tsp` | CHK MODS | Mod 1 (T1 SWARM 2 to T1 CUT) is at 0 %, and mod 2 (T2 SWARM 1 to M1 AMT) moves its amount: T1's filter mod breathes in and out. Mod 3 is a GATE (T3 SWARM 3 past its middle opens T2's filter by 60). Mod 4 holds FILL (the GLO FILL cell lights). |
| `chk-tempo.tsp` | CHK TEMPO | SWARM 1 onto TEMPO at +30: everything speeds up and slows down together over about a minute. With MENU CLOCK OUT on, an outside synth follows. |
| `chk-midi.tsp` | CHK MIDI | T2 is SOUND OFF on channel 1 and two octaves down on the way out (TRANSPOSE -24); T3 is SOUND OFF with its channel OFF, a lane whose CV opens T1's filter. Set MENU MIDI OUT to ALL and connect a synth. |
| `chk-cpu.tsp` | CHK CPU | Four ORACLE tracks, a FILTER insert on T2, the room and echo high. Watch the editor's **Settings** > **Health** while it plays, on two cores and on one. |

MIDI OUT and CLOCK OUT are device settings, not saved in a project: set them in the MENU.

They are made by `tests/demo_projects.c` (`demo_projects checks make DIR`), as the demos are, and
`tests/run_tests.sh` checks that these files still load and play. After a change to the project format or a default,
make them again: build `demo_projects` as run_tests.sh does, run `demo_projects checks make demos/checks 20`.
