# What's new in the beta

Shown on the installer under the Beta choice (web/publish_beta.py --notes, from the beta workflow). Only the
"## " headings and the "- " lines are shown, as plain text. Update it when a change merges into develop; at the
release it becomes the start of docs/RELEASE_X.Y.md, then this file starts again for the next beta.

Found a problem? Open an issue: <https://github.com/Rastapopolix/tessera/issues> (the firmware version, the steps,
what you expected and what happened).

## New since 3.7
- A new effect, DRIVE: a soft clip that rounds the peaks off for warmth or grit (good on FM sounds). RATE is the drive, CHAR folds the peaks back as a wavefolder does, MIX blends it with the dry sound. Put it in FX1 to drive the track before its other effects. Thanks to pavig-coder / u/paulvigo (issue #824).

## Fixed

## Good to know
- A project that uses DRIVE loads on older firmware, but plays that slot as a FILTER.

## Please try
- A project from 3.7 loads and plays the same.
- DRIVE on an FM6 track: turn RATE up slowly, then CHAR. Listen for clicks while turning, and for harshness at the top.
