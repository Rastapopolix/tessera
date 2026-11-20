# What's new in the beta

Shown on the installer under the Beta choice (web/publish_beta.py --notes, from the beta workflow). Only the
"## " headings and the "- " lines are shown, as plain text. Update it when a change merges into develop; at the
release it becomes the start of docs/RELEASE_X.Y.md, then this file starts again for the next beta.

Found a problem? Open an issue: <https://github.com/Rastapopolix/tessera/issues> (the firmware version, the steps,
what you expected and what happened).

## New since 3.7.3
- CHORD's GEN sheet has Move when and Move by: a BEAT, STEP or OFFBEAT chord moves the strikes you pick (LAST, LAST 2, ODD, CHANGE, RAND) by -2 to +3 semitones, +1 or +2 degrees in key, an inversion, an octave, TOP or LEAD, and is home again on the next strike. A reggae skank pushed up a whole step on the and of 4 is Move when LAST, Move by +2 ST.

## Fixed

## Good to know
- A project with Move when set opens on older firmware, which plays the chords without the moves.

## Please try
- CHORD, HIT OFFBEAT, Move when LAST: try +2 ST, +3 ST and +1 ST, and +2 DEG in a minor key.
- A project from 3.7.3 loads and plays the same.
