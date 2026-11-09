# What's new in the beta

Shown on the installer under the Beta choice (web/publish_beta.py --notes, from the beta workflow). Only the
"## " headings and the "- " lines are shown, as plain text. Update it when a change merges into develop; at the
release it becomes the start of docs/RELEASE_X.Y.md, then this file starts again for the next beta.

## New since 3.4.1
- PAT GEN: each of a track's 8 patterns can keep its own generator setup, so switching pattern switches the generator. Off by default: EVOLVE, OCT+ held, KNOB 4.
- ROLL ALL: every generator track gets a new seed at once, and one step back undoes it. EVOLVE, OCT- held, KNOB 4.
- ENTRIES ORDER: choose the order tracks come in (AUTO, 1ST to 4TH); they leave in reverse. ENTRIES, OCT- held.
- PRINT ARP: hold REC on an ARP track to print what the arpeggio played into STEP, as on a generator track; EDIT held brings the ARP back (#530).
- USER scales: four 12-note scales of your own, USER1 to USER4, in the USER family on SCL > SCALES. With one picked, a key adds its note or takes it out. Every generator, QNT and FOLLOW play it; it is kept with the settings and saved with a project that plays it.
- MIDI OUT DRUM MAP: GM or ROW, for drum machines that take their tracks as a row of notes (such as the Analog Rytm). MIDI OUT, OCT+ and OCT- held.

## Fixed
- Holding OCT- on EVOLVE or ENTRIES and letting go no longer takes you to HOME (#519).
- EVOLVE's ROLL and ROLL ALL say NEW LINE on the screen (NEW LINE AT BAR while playing), so a turn is no longer silent when stopped (#528).
- Web editor: ORACLE, TRIO and other user presets now show in the Sound tab's preset list, and Save to file names an FM6 user preset after itself, not after TINE EP (#526).

## Good to know
- Projects saved with these new settings lose them on 3.4.x.

## Please try
- PAT GEN with different generators on two patterns: switch by hand and with SONG, save, power off and on, load.
- ENTRIES ORDER with the bass set to come in later.
- PRINT ARP: an ARP track with a chord held, hold REC after two bars; then EDIT held.
- USER scales: make one on the keys, play a generator in it, save, power off and on, load; load that project on 3.4.1 (it plays the scale picked under it).
- A project from 3.4.1 loads and plays the same.
