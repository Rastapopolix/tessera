# What's new in the beta

Shown on the installer under the Beta choice (web/publish_beta.py --notes, from the beta workflow). Only the
"## " headings and the "- " lines are shown, as plain text. Update it when a change merges into develop; at the
release it becomes the start of docs/RELEASE_X.Y.md, then this file starts again for the next beta.

Found a problem? Open an issue: <https://github.com/Rastapopolix/tessera/issues> (the firmware version, the steps,
what you expected and what happened).

## New since 3.6.2
- LANDSCAPE places: a named project is islands, land cut by narrow channels (with an arched bridge) or all land, and each CHAIN section's shore has one thing of its own: a torii on A (all land: a seated Buddha), on B and C a pagoda, a lit stone lantern or a pine (now and then a cherry, its petals falling slowly), some shores bare; each reflected in the water.

- Capture-to-loop: TURING, EUCLID, MARKOV and MORPH have a new SOURCE, KEYS. Play a phrase on the track's keys (or MIDI IN), up to 16 notes; a bar without a key ends it, and the generator then plays and varies what you played, with KEEP and GRAVITY as for a STEP source. The next phrase you play replaces it. REC held prints the take into STEP so you can fix a note by hand, and the generator plays on from that pattern.

## Fixed

## Good to know
- A project with a track set to SOURCE KEYS doesn't load on 3.6.2 or older firmware.
- Named projects whose place is channels or all land look different from 3.6.2 (their hills are new); islands places keep their hills and the torii where it stood (B and C gain a little ground under their thing).

## Please try
- A project from 3.6.2 loads and plays the same.
- Capture-to-loop: on a TURING track, OCT- held, KNOB 2 to KEYS. Play two bars, stop playing for a bar, and listen. Then hold REC and check STEP holds what you played.
- Name a project (try UNDERTOW, KOTO GARDEN, SALT MARSH) with a CHAIN FORM such as ABCA and watch LANDSCAPE as the sections change: one thing on each shore, its reflection rippling, the lantern's glow with the notes.
- On LANDSCAPE, turn a knob with a macro driving a mod: the music moves and the value shows across the foot.
