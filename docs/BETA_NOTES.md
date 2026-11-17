# What's new in the beta

Shown on the installer under the Beta choice (web/publish_beta.py --notes, from the beta workflow). Only the
"## " headings and the "- " lines are shown, as plain text. Update it when a change merges into develop; at the
release it becomes the start of docs/RELEASE_X.Y.md, then this file starts again for the next beta.

Found a problem? Open an issue: <https://github.com/Rastapopolix/tessera/issues> (the firmware version, the steps,
what you expected and what happened).

## New since 3.6.1
- LANDSCAPE: KNOB 1 to 4 turn PERF's MACRO 1 to 4, shown large while turned, so you can make a big move and watch the lake answer. IDLE LANDSCAPE's first touch still only takes you back.

- Capture-to-loop: TURING, EUCLID, MARKOV and MORPH have a new SOURCE, KEYS. Play a phrase on the track's keys (or MIDI IN), up to 16 notes; a bar without a key ends it, and the generator then plays and varies what you played, with KEEP and GRAVITY as for a STEP source. The next phrase you play replaces it. REC held prints the take into STEP so you can fix a note by hand, and the generator plays on from that pattern.

## Fixed

## Good to know
- A project with a track set to SOURCE KEYS doesn't load on 3.6.1 or older firmware.

## Please try
- Capture-to-loop: on a TURING track, OCT- held, KNOB 2 to KEYS. Play two bars, stop playing for a bar, and listen. Then hold REC and check STEP holds what you played.
- A project from 3.6.1 loads and plays the same.
- On LANDSCAPE, turn a knob with a macro driving a mod: the music moves and the value shows across the foot.
