# What's new in the beta

Shown on the installer under the Beta choice (web/publish_beta.py --notes, from the beta workflow). Only the
"## " headings and the "- " lines are shown, as plain text. Update it when a change merges into develop; at the
release it becomes the start of docs/RELEASE_X.Y.md, then this file starts again for the next beta.

## New since 3.4.1
- PAT GEN: each of a track's 8 patterns can keep its own generator setup, so switching pattern switches the generator. Off by default: EVOLVE, OCT- held, KNOB 3.
- ROLL ALL: every generator track gets a new seed at once, and one step back undoes it. EVOLVE, OCT- held, KNOB 1.
- ENTRIES ORDER: choose the order tracks come in (AUTO, 1ST to 4TH); they leave in reverse. ENTRIES' sheet (OCT+ held).
- PRINT ARP: hold REC on an ARP track to print what the arpeggio played into STEP, as on a generator track; EDIT held brings the ARP back (#530).
- USER scales: four 12-note scales of your own, USER1 to USER4, in the USER family on SCL > SCALES. With one picked, a key adds its note or takes it out. Every generator, QNT and FOLLOW play it; it is kept with the settings and saved with a project that plays it.
- RANDOM: hold OCT- on GEN or TWEAK and turn KNOB 1 right for new values on the page's knobs, each inside its range; turn left to put them back. MODE, the seed and FOLLOW never change (#529).
- ARP modes: DNUP (down, then up) and RND1 (one random order, repeated until the held notes change; from the track's SEED, so PLAY replays it). SEQ > ARP, KNOB 1.
- MIDI OUT DRUM MAP: GM or ROW, for drum machines that take their tracks as a row of notes (such as the Analog Rytm). MIDI OUT's sheet (OCT+ held).
- CHORD STRUM: a CHORD track's notes start a few ms apart, like a played chord. STRUM (OFF, 5 to 60 ms) and DIR (UP, DOWN, ALT) on GEN, OCT- held, KNOB 2 and 3. Short strikes strum tighter; all notes end together.
- STEP direction: each pattern plays FWD, BACK, PEND, PONG, RAND or DRUNK. Direction row on the pattern's sheet (OCT+ held on PATTERN, NOTES, or GEN with STEP or ARP). RAND and DRUNK follow the SEED, so PLAY replays them.
- DIR as a mod destination: a cord can move a pattern's direction at each step; a GATE mod switches direction while its source is high.
- NOTES: OCT+ on an empty step no longer places a note (on the drum grid, no longer sets a hit); it says NOTHING THERE. KNOB 2 or a key places a note (the grid: KNOB 3 or a white key). OCT+ on a note still opens its sheet; held, OCT+ opens the pattern's sheet and the screen says PATTERN SHEET once the hold counts.
- GROUND: MENU > GROUND, below COLOR, puts the screen on warm paper (LIGHT) instead of black (DARK, the default). Each ink becomes a darker version that reads well on paper; the MOSAIC sets keep four different inks. COLOR's sheet shows them on paper while LIGHT is on (#557).
- HOLD mods: OCT+ on MODS cycles +- / + / GATE / HOLD. HOLD takes the source at the start of each bar and keeps it until the next.
- MOD SCOPE: hold OCT- on MODS to see the picked mod move over time (solid) over its source (dashed), a GATE's threshold as a line across it. The window follows the source: a bar for the notes' own, 2 to 16 bars for an LFO, up to 64 for TIDE and ENERGY. Let go and the rows come back. The screen reader says its shape once as it opens.
- Chord kinds SUS2, 7SUS4, ADD9 and MAJ6 on CHRD (SCL > CHORD, or the CHORD track's layer), after POW: fixed shapes on any note, for the chord keys, MIDI in, step entry, the arp and the CHORD generator.
- Keys: OCT+ goes in, OCT- goes back. Held, OCT+ opens the page's sheet (the header shows its name and a line filling until it opens; a page without one says NO SHEET) and OCT- gives the page's second set of knobs while held. Moved from OCT+ held to OCT- held: FORM (DEVELOP, CHAIN, SKIP, ROLE), TIMING (SPLIT, HOW), ENERGY (DEPTH), CHAIN (REPEAT), MIXER (PAN) and MODS (a GATE's threshold, a CC mod's number). The OCT- LED is lit on pages that have such a layer. GEN, EVOLVE, TWEAK, MIDI OUT, ENTRIES and PIECE move in a later beta. Since then the rest moved too, so OCT+ held has no knob layer anywhere: GEN (KNOB 1 RANDOM, KNOB 2 to 4 the generator's layer; QUAD's RANGE, CONSTL's SIZE, ATLAS' fourth part, ANSWER's SPACE and CHORD's CHRD, VOIC and BREATH / LEN on a new GEN sheet), EVOLVE (KNOB 1 ROLL ALL, KNOB 3 PAT GEN, KNOB 4 GRAVITY or HOW), TWEAK (FOLLOW's HOW, RHYTHM, HOLD), MIDI OUT (TRANSPOSE; SOUND and DRUM MAP on its sheet), ENTRIES (ROLE; ORDER on its sheet) and PIECE (SHAPE USER: KNOB 1 picks a point, KNOB 2 its level, the shape in view). ATLAS' part keys and CHORD USER's entry are on OCT- held too.
- MIX buses: BUS A and BUS B, two mixers for mods. Send any mods into a bus (the end of KNOB 3's list on MODS) and use the bus as a source (the end of KNOB 1's list). Hold OCT+ on MODS for its sheet: each bus's Mode (SUM, MAX, MIN, DIFF: with gates OR, AND, XOR) and Lag (OFF, 10 ms to 20 s: a gate into a lagged bus is an envelope). A bus can feed itself, one block late, safely.

## Fixed
- A long OCT- press no longer goes back on any page (it did on FORM, TIMING, ENERGY, CHAIN, MIXER and the sound pages), and a knob turned while an OCT key is held no longer counts as a tap of it.
- ARP MODE ORD plays the notes in the order the keys were pressed; it played them low to high (as UP) unless ARP 2's ORDER was PLAY.
- Holding OCT- on EVOLVE or ENTRIES and letting go no longer takes you to HOME (#519).
- EVOLVE's ROLL and ROLL ALL say NEW LINE on the screen (NEW LINE AT BAR while playing), so a turn is no longer silent when stopped (#528).
- Web editor: ORACLE, TRIO and other user presets now show in the Sound tab's preset list, and Save to file names an FM6 user preset after itself, not after TINE EP (#526).
- ANSWER with a SOURCE track that runs a generator (BOUNCE, TURING ...) now answers the notes that track plays; it read that track's hidden STEP pattern instead, often one note or none.
- Muting a generator track (MIXER or GLO's mute keys) no longer stops the tracks that follow it: a muted track plays on unheard, so FOLLOW, an ANSWER's SOURCE, QNT CHRD, QUAD's LINK and the cords still hear its notes, and unmuted it comes back in time.

## Good to know
- Projects saved with these new settings lose them on 3.4.x.
- A project with ARP MODE DNUP or RND1 plays REPEAT on 3.4.x and earlier betas (the last note held).
- GROUND is kept with the settings, not in the palette: older firmware keeps your ink and draws it on black.
- On older firmware, a strummed project plays its chords together, a pattern with a direction plays FWD, and a HOLD mod plays as +-.
- A project with CHRD SUS2, 7SUS4, ADD9 or MAJ6 plays POW (power chords) on 3.4.x and earlier betas. A cord or MIDI CC on CHRD now spreads over 14 kinds, not 10, so it lands on slightly different ones.
- A project with a mod into or out of a MIX bus does not load on 3.4.x or earlier betas; the bus settings alone are ignored there.

## Please try
- PAT GEN with different generators on two patterns: switch by hand and with SONG, save, power off and on, load.
- ENTRIES ORDER with the bass set to come in later.
- PRINT ARP: an ARP track with a chord held, hold REC after two bars; then EDIT held.
- ROLL and ROLL ALL on EVOLVE, stopped and playing: the screen says NEW LINE (NEW LINE AT BAR while playing).
- Web editor: an ORACLE or TRIO user preset shows in the Sound list; Save to file on an FM6 user preset uses its own name.
- MIX buses: two LFOs into BUS A at SUM, BUS A as a GATE onto FILL; a track's GATE into BUS B with Lag 200 ms onto CUTOFF; save, load.
- RANDOM on GEN with a few generators, stopped and playing; then turn back.
- GROUND LIGHT with a few inks and a MOSAIC set: HOME, the pages, LANDSCAPE, the boot screen after a power cycle; is everything readable on the device's panel?
- USER scales: make one on the keys, play a generator in it, save, power off and on, load; load that project on 3.4.1 (it plays the scale picked under it).
- ANSWER on T2 with SOURCE T1, BOUNCE on T1: it shows LISTENING, takes up to 8 of the balls' notes, then answers them.
- ARP DNUP and RND1 with a chord held: RND1 keeps its order, changes when a key is added, and is the same after STOP and PLAY.
- CHORD STRUM on a CHORD track: STRUM 20 ms and up, DIR UP, DOWN and ALT, with BEAT or STEP stabs; MIDI OUT gets the notes spread too.
- STEP direction: PONG and DRUNK on a melody, with SONG and a pattern change; save, load, and STOP and PLAY replays RAND.
- CHRD SUS2, 7SUS4, ADD9, MAJ6 on the chord keys and on a CHORD track (the name on the CHORD page: Csus2, C7sus4, Cadd9, C6), with OPEN and +OCT.
- HOLD mod: an LFO into a filter or pitch held per bar; a cord to DIR.
- MOD SCOPE: hold OCT- on MODS with an LFO mod, a GATE mod (turn its threshold) and a mod of mods; does the picture keep up, and is it readable on the panel (a light GROUND too)?
- A project from 3.4.1 loads and plays the same.
