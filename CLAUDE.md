# CLAUDE.md

Tessera: alternative firmware for the M-Vave FM-1 (GPL-3.0), a hard fork of Melodee with its own generators
(`firmware/src/gen.c`, `tessera.c`), long-form layers (FORM chaining, ENTRIES, the piece planner, ENERGY) and
its own Mono look. The repo was called `melodee-tess` until 2.0; build flags and some file names still say
`MELODEE_` / `melodee.bin` and stay that way. Plan, phases and tasks: `ROADMAP.md` in the private notes repo
`~/src/tessera-notes` (github.com/Rastapopolix/tessera-notes), with `HANDOVER.md`, `docs/analysis/` and the design
notes in `docs/design/`. This repo is public: keep plans, reviews and decisions there, not here (they are in
`.gitignore`).

## Start and end of each session

Read `~/src/tessera-notes/ROADMAP.md` first (`git -C ~/src/tessera-notes pull`) and claim the task under
"In progress" (branch, session), along with any new save-format part, parameter ID, cord code or test-page number.
At the end, update it (tick tasks, record decisions) and commit and push it there; commit code here as usual.

Branches: feature branches from `develop`, PRs into `develop`. `main` holds releases and the site (`docs/`):
release and site PRs only. Betas: push `beta/X.Y-betaN` from develop (`.github/workflows/beta.yml`), then a site PR
into `main`. Never delete the branch `detailed-look` (the retired detailed look's last state).

## Build

```
./build.sh                       # build/melodee.bin, build/loader/ota.bin, build/tessera.fwsc
./build.sh --release X.Y[.Z]     # release package and build/release-X.Y/
MELODEE_DUAL_CORE=0 ./build.sh   # serial (one-core) variant
```

WSL Ubuntu, Linux x86-64: the JieLi toolchain runs natively (`~/.jieli/toolchain`), no Docker.
The AC79 SDK is at `~/fw-AC79_AIoT_SDK`. Build options are listed in `BUILDING.md`.

## Test

```
tests/run_tests.sh                       # host tests; run after ./build.sh
GOLDEN_UPDATE=1 sh tests/run_tests.sh    # only after an intended change of the sound
BUDGET_UPDATE=1 sh tests/run_tests.sh    # only after an intended change of the CPU cost
```

The suite renders every engine against `tests/golden.txt`, checks CPU cost, and renders the UI.
Generators can be developed and tested on the PC; flash the device only to check how it plays.

## Hardware constraints

- JieLi AC79 at 360 MHz. CPU1 is optional and retires itself on units that misread SRAM:
  everything must work single-core.
- Audio at 44.1 kHz. Modulation runs at control rate (once per audio block).
- Flash is the tight limit: the app image must fit the XIP window of 573,440 B, and the build prints
  `image N B`. Measure every change and put the cost in the PR.
- The build checks the audio working arena against the linker's `_resource_minimum`; it reports the
  free arena and fails below it. Watch RAM (.data+.bss of 98,304 B) as well.
- Any function placed in `.audio_text` needs `aligned(16)` (the build fails otherwise).
- No 64-bit `/` or `%` in firmware code: the JieLi libgcc lacks `__udivdi3` / `__umoddi3`.

## Look and build flags

- The look is **Mono only** (`MELODEE_LOOK=1`, the default in tools/build.py): one 5x7 face, ink on black,
  nine inks (Settings > COLOR). Code for both looks goes through `LOOK(d, m)` (core.h); new UI work is Mono
  only. The detailed look no longer fits the flash; it is kept on the branch `detailed-look`.
- `MELODEE_ACCESS=1` (the device default; host tests build with 0): the screen reader in place of the help
  cards. New pages, knobs and messages must speak: run `tools/speak_audit.sh` and add words to `firmware/words.tsv`.
- Every option is listed in `BUILDING.md`.

## Rules

- Follow the existing code conventions: the unity build (`melodee.c` and `tests/hostsim.c`
  `#include` the `.c` files), C style, naming, comment density. New sources go in both.
- Prefer new files over heavy rewrites of shared upstream files, to keep upstream syncing easy.
- All hardware access stays behind the platform layer (`firmware/hal/`), so generator,
  sequencer, mod and UI code compiles for host tests and the web simulator.
- Every automatic layer (Energy, Entries, Wander, Evolve, Gravity, the piece planner) has an
  off/zero position with no effect. Automation is opt-in.
- New parameter IDs, cord codes or project (TSP1) parts can make projects unreadable on older
  firmware: say so in the PR and the release notes.
- On-screen names avoid other makers' product names (ATLAS, TIDE, SWARM); credits name the originals.
- Every parameter has a central ID, range and help text (the online reference is generated
  from it). Fixed maximums for cords, sources, scopes, voices.
- Flag risky areas before changing them: bootloader, flash layout, update/install, dual-core start.
- Run `tests/run_tests.sh` after every change. Report failures; do not hide them.
- Small commits, one change each, on a feature branch. Leave `main` untouched unless asked.
- Never release a `MELODEE_CORE1_TEST` build (fault-injection test firmware).
- Keep GPL-3.0 headers and the upstream attribution (Melodee, Felucca, fm1-base), and a LICENSING.md row for each port.
- Do not commit build output, `__pycache__/` or backup files (`*.bak*`, `*.pre*`).
