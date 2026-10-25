# Contributing to Tessera

Thanks for your interest in Tessera. Contributions are welcome, and this page should save you time before you
start.

## Where Tessera is heading

Tessera is a **generative** firmware for the M-Vave FM-1. Its focus is the generators and the pages around them
(EVOLVE, FORM, modulation), not a large collection of synth engines.

The FM-1 has very little room. The app area in flash is about 573 KB and is nearly full, and static RAM and CPU
are tight too. In 2.0 several engines (SID as a melodic engine, PHYS, CZ-1 and Prophet) were removed on purpose,
to free flash, RAM and CPU for the generators and for Tessera's own voices.

This is not the final engine line-up. Once the generative side is done, I will review the balance between the
generators and the synth voices, prune what is not pulling its weight, and then look at which new voices fit the
firmware best. Engine suggestions are welcome as issues in the meantime; they will be weighed in that review.

## What is likely to be merged

- **Bug fixes**, especially crashes, timing, audio glitches and save/load problems.
- **Generator ideas and improvements**, ideally discussed in an issue first.
- **Docs**: the manual, recipes, the cheatsheet, corrections and clearer wording.
- **The web tools**: installer, simulator and editor fixes.
- **Size and performance work** that gives flash, RAM or CPU back.

## What is unlikely to be merged

- **New or returning synth engines, for now**, even behind build flags. Until the review above, every kilobyte is
  planned for the generators, and engines that are not shipped still need tests and maintenance. Open an issue
  instead, so the idea is on the list for the review. Variants with more engines are a good fit for your own fork,
  and you are welcome to keep one.
- **Changes to the flash layout, bootloader or update path** without discussion first. These can brick a device.
- Changes to the project links, credits or branding in this repository.

## Before a large change

Please **open an issue first** for anything bigger than a small fix. A short description of what you want to do is
enough. It saves you from writing code that cannot be merged, and I can often point you at the right place to start.

## Working on the code

- [BUILDING.md](BUILDING.md) covers the toolchain, the build and flashing.
- Run the test suite before opening a pull request. It renders every engine against `tests/golden.txt`, checks CPU
  cost and renders the UI. Generators can be developed and tested on a PC; flash the device only to check how it
  plays.
- Say in the pull request how the **device image size** changed (the build's size check prints it), and keep each
  pull request to one change.

## Licence

Tessera is GPL-3.0-only. By contributing you agree that your contribution is licensed the same way. Credit for code
or ideas from other projects goes in [LICENSING.md](LICENSING.md).
