# What's new in the beta

Shown on the installer under the Beta choice (web/publish_beta.py --notes, from the beta workflow). Only the
"## " headings and the "- " lines are shown, as plain text. Update it when a change merges into develop; at the
release it becomes the start of docs/RELEASE_X.Y.md, then this file starts again for the next beta.

Found a problem? Open an issue: <https://github.com/Rastapopolix/tessera/issues> (the firmware version, the steps,
what you expected and what happened).

## New since 3.7.1
- Three new chord kinds on CHRD: 2NDS, 4THS and 5THS stack the scale's seconds, fourths or fifths, so they stay in key (quartal and quintal chords). On a microtonal scale they stack its own degrees, which gives close clusters (issue #852).

## Fixed
- ANSWER: with the transport stopped, a pause of a second and a half now ends the phrase, so the next key starts a new one. Before, every key played while stopped joined the same phrase, and once it held 16 notes the picture kept the old ones.

## Good to know
- A project using 2NDS, 4THS or 5THS plays only the chord's root on 3.7.1 or older.

## Please try
- SCL > CHORD: CHRD 4THS and 5THS on a few keys, with each VOIC, and on a CHORD generator track.
- A project from 3.7.1 loads and plays the same.
