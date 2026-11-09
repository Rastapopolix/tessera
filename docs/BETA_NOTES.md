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
- RANDOM: hold OCT- on GEN or TWEAK and turn KNOB 1 right for new values on the page's knobs, each inside its range; turn left to put them back. MODE, the seed and FOLLOW never change (#529).
- ARP modes: DNUP (down, then up) and RND1 (one random order, repeated until the held notes change; from the track's SEED, so PLAY replays it). SEQ > ARP, KNOB 1.
- MIDI OUT DRUM MAP: GM or ROW, for drum machines that take their tracks as a row of notes (such as the Analog Rytm). MIDI OUT, OCT+ and OCT- held.
- CHORD STRUM: a CHORD track's notes start a few ms apart, like a played chord. STRUM (OFF, 5 to 60 ms) and DIR (UP, DOWN, ALT) on GEN, OCT- held, KNOB 2 and 3. Short strikes strum tighter; all notes end together.
- STEP direction: each pattern plays FWD, BACK, PEND, PONG, RAND or DRUNK. Direction row on the pattern's sheet (OCT+ held on PATTERN, NOTES, or GEN with STEP or ARP). RAND and DRUNK follow the SEED, so PLAY replays them.
- DIR as a mod destination: a cord can move a pattern's direction at each step; a GATE mod switches direction while its source is high.
- HOLD mods: OCT+ on MODS cycles +- / + / GATE / HOLD. HOLD takes the source at the start of each bar and keeps it until the next.

## Fixed
- ARP MODE ORD plays the notes in the order the keys were pressed; it played them low to high (as UP) unless ARP 2's ORDER was PLAY.
- Holding OCT- on EVOLVE or ENTRIES and letting go no longer takes you to HOME (#519).
- EVOLVE's ROLL and ROLL ALL say NEW LINE on the screen (NEW LINE AT BAR while playing), so a turn is no longer silent when stopped (#528).
- Web editor: ORACLE, TRIO and other user presets now show in the Sound tab's preset list, and Save to file names an FM6 user preset after itself, not after TINE EP (#526).
- ANSWER with a SOURCE track that runs a generator (BOUNCE, TURING ...) now answers the notes that track plays; it read that track's hidden STEP pattern instead, often one note or none.

## Good to know
- Projects saved with these new settings lose them on 3.4.x.
- A project with ARP MODE DNUP or RND1 plays REPEAT on 3.4.x and earlier betas (the last note held).
- On older firmware, a strummed project plays its chords together, a pattern with a direction plays FWD, and a HOLD mod plays as +-.

## Please try
- PAT GEN with different generators on two patterns: switch by hand and with SONG, save, power off and on, load.
- ENTRIES ORDER with the bass set to come in later.
- PRINT ARP: an ARP track with a chord held, hold REC after two bars; then EDIT held.
- ROLL and ROLL ALL on EVOLVE, stopped and playing: the screen says NEW LINE (NEW LINE AT BAR while playing).
- Web editor: an ORACLE or TRIO user preset shows in the Sound list; Save to file on an FM6 user preset uses its own name.
- RANDOM on GEN with a few generators, stopped and playing; then turn back.
- USER scales: make one on the keys, play a generator in it, save, power off and on, load; load that project on 3.4.1 (it plays the scale picked under it).
- ANSWER on T2 with SOURCE T1, BOUNCE on T1: it shows LISTENING, takes up to 8 of the balls' notes, then answers them.
- ARP DNUP and RND1 with a chord held: RND1 keeps its order, changes when a key is added, and is the same after STOP and PLAY.
- CHORD STRUM on a CHORD track: STRUM 20 ms and up, DIR UP, DOWN and ALT, with BEAT or STEP stabs; MIDI OUT gets the notes spread too.
- STEP direction: PONG and DRUNK on a melody, with SONG and a pattern change; save, load, and STOP and PLAY replays RAND.
- HOLD mod: an LFO into a filter or pitch held per bar; a cord to DIR.
- A project from 3.4.1 loads and plays the same.
