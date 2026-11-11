# What's new in the beta

Shown on the installer under the Beta choice (web/publish_beta.py --notes, from the beta workflow). Only the
"## " headings and the "- " lines are shown, as plain text. Update it when a change merges into develop; at the
release it becomes the start of docs/RELEASE_X.Y.md, then this file starts again for the next beta.

Want to help test? The steps, by area, and how to report a problem: [BETA_TESTING.md](BETA_TESTING.md).

## New since 3.4.1
- PAT GEN: each of a track's 8 patterns can keep its own generator setup, so switching pattern switches the generator. Off by default: EVOLVE, OCT- held, KNOB 3.
- ROLL ALL: every generator track gets a new seed at once, and one step back undoes it. EVOLVE, OCT- held, KNOB 1.
- ENTRIES ORDER: choose the order tracks come in (AUTO, 1ST to 4TH); they leave in reverse. ENTRIES' sheet (OCT+ held).
- PRINT ARP: hold REC on an ARP track to print what the arpeggio played into STEP, as on a generator track; EDIT held brings the ARP back (#530).
- USER scales: four 12-note scales of your own, USER1 to USER4, in the USER family on SCL > SCALES. With one picked, a key adds its note or takes it out. Every generator, QNT and FOLLOW play it; it is kept with the settings and saved with a project that plays it.
- RANDOM: hold OCT- on GEN or TWEAK and turn KNOB 1 right for new values on the page's knobs, each inside its range; turn left to put them back. MODE, the seed and FOLLOW never change (#529).
- ARP modes: DNUP (down, then up) and RND1 (one random order, repeated until the held notes change; from the track's SEED, so PLAY replays it). SEQ > ARP, KNOB 1.
- MIDI OUT DRUM MAP: GM or ROW, for drum machines that take their tracks as a row of notes (such as the Analog Rytm). MIDI OUT's sheet (OCT+ held).
- CHORD STRUM: a CHORD track's notes start a few ms apart, like a played chord. STRUM (OFF, 1 to 250 ms: by 1 ms to 10, by 5 to 60, then by 10) and DIR (UP, DOWN, ALT) on GEN, OCT- held, KNOB 2 and 3. Short strikes strum tighter; all notes end together.
- STEP direction: each pattern plays FWD, BACK, PEND, FOLD, RAND or DRUNK. FOLD goes from the ends inwards: 1 8 2 7 3 6 4 5. Direction row on the pattern's sheet (OCT+ held on PATTERN, NOTES, or GEN with STEP or ARP). RAND and DRUNK follow the SEED, so PLAY replays them.
- DIR as a mod destination: a cord can move a pattern's direction at each step; a GATE mod switches direction while its source is high.
- NOTES: OCT+ on an empty step no longer places a note (on the drum grid, no longer sets a hit); it says NOTHING THERE. KNOB 2 or a key places a note (the grid: KNOB 3 or a white key). OCT+ on a note still opens its sheet; held, OCT+ opens the pattern's sheet and the screen says PATTERN SHEET once the hold counts.
- GROUND: MENU > GROUND, below COLOR, puts the screen on warm paper (LIGHT) instead of black (DARK, the default). Each ink becomes a darker version that reads well on paper; the MOSAIC sets keep four different inks. COLOR's sheet shows them on paper while LIGHT is on (#557). On paper WHITE is pure black and named BLACK, and COLOR shows one red, PINK, and one yellow, YELLOW (RED and HOT PINK, and ORANGE, look the same there; a chosen RED or ORANGE comes back on DARK).
- HOLD mods: OCT+ on MODS cycles +- / + / GATE / HOLD. HOLD takes the source at the start of each bar and keeps it until the next.
- MOD SCOPE: hold OCT- on MODS to see the picked mod move over time (a line) over its source (faint dots), a GATE's threshold as a wedge at each edge (filled while open; a dashed line across while KNOB 4 turns it). With a MOSAIC set the line takes its source's track colour. The window follows the source: a bar for the notes' own, 2 to 16 bars for an LFO, up to 64 for TIDE and ENERGY. Let go and the rows come back. The screen reader says its shape once as it opens.
- Chord kinds SUS2, 7SUS4, ADD9 and MAJ6 on CHRD (SCL > CHORD, or the CHORD track's layer), after POW: fixed shapes on any note, for the chord keys, MIDI in, step entry, the arp and the CHORD generator.
- Keys, one rule on every page: OCT+ goes in, OCT- goes back. Tap OCT+ = Enter: the row, slot, note or mod under the cursor; where there is nothing to enter, nothing (a tap no longer opens the sound's or the page's sheet). Hold OCT+ = the page's sheet (it stays open; the header shows its name and a line filling until it opens; a page without one says NO SHEET). Tap OCT- = Back: closes a sheet, list, map or selection, and a page opened from the EDIT map or from MATRIX goes back there; on a page with nothing open it does nothing (it no longer goes to HOME: HOME does). Hold OCT- = the page's second set of knobs, only while held (never Back, never a sheet). The OCT- LED is lit on pages that have such a set. OCT+ held no longer gives knobs anywhere.
- Moved from OCT+ held to OCT- held (same knobs, other key): FORM (DEVELOP, CHAIN, SKIP, ROLE), TIMING (SPLIT, HOW), ENERGY (DEPTH), CHAIN (REPEAT), MIXER (PAN), MODS (a GATE's threshold, a CC mod's number), GEN (KNOB 2 to 4: the generator's second layer, e.g. TURING's SOURCE KEEP GRAVITY, CHORD's STRUM DIR OCT; KNOB 1 stays RANDOM), EVOLVE (KNOB 4 GRAVITY or HOW, beside ROLL ALL and PAT GEN), TWEAK (FOLLOW's HOW, RHYTHM, HOLD), MIDI OUT (TRANSPOSE), ENTRIES (ROLE). ATLAS' fourth-part keys and CHORD / PROG USER's chord entry: OCT- + keys.
- Moved onto a sheet (OCT+ held): GEN's new sheet has QUAD's RANGE, CONSTL's SIZE, ATLAS' fourth part, ANSWER's SPACE and CHORD's CHRD, VOIC and BREATH / LEN. MIDI OUT's sheet has each track's SOUND and DRUM MAP (the both-keys layer is gone). ENTRIES' sheet has ORDER. MODS' sheet has the MIX buses.
- PIECE, SHAPE USER: hold OCT-, KNOB 1 picks one of the eight points, KNOB 2 sets its level, with the shape in view (was OCT+ / OCT- held for points 1 to 4 / 5 to 8).
- Project files: save a slot's project as a .tsp file, or load one into a slot, in the web editor (Projects tab) and in the web simulator (under the FM-1). Move songs between the simulator and the FM-1 this way.
- Manual: recording the four tracks as separate stems over USB (USB OUTPUT 4 TRACKS), for macOS and Windows; on Windows, Live needs an ASIO wrapper such as FlexASIO for more than two channels.
- Clear mod: hold OCT+ on MODS; the sheet's header names the picked mod (M3 at its right) and its first row, Clear mod, empties it with OCT+ (no undo).
- MIX buses: BUS A and BUS B, two mixers for mods. Send any mods into a bus (the end of KNOB 3's list on MODS) and use the bus as a source (the end of KNOB 1's list). Hold OCT+ on MODS for its sheet: each bus's Mode (SUM, MAX, MIN, DIFF: with gates OR, AND, XOR) and Lag (OFF, 10 ms to 20 s: a gate into a lagged bus is an envelope). A bus can feed itself, one block late, safely.
- EOC: a new source on every track, the LFO's cycle start as a gate (high from each wrap until an eighth of the cycle has passed). Last in a track's source list on MODS. Use it as a GATE mod: onto FILL, a bus, a mod's amount. A project that uses it does not load on older firmware.
- Mods of mods can scale: hold OCT- on a mod of mods (M1 AMT .. M16 AMT) and turn KNOB 3 right. It then multiplies the other mod's move by 0 to 1 instead of adding to it (M3* on the row), so a GATE or an envelope opens and closes a mod that keeps its own amount; a negative amount closes it instead. KNOB 3 left adds again.
- MOD DEPTH: one control for how much of the whole patch plays, 0 to 100 % (GLO > PERF, hold OCT+ for its sheet). It scales every mod's movement, FILL, TEMPO and CC included, but never an OFFSET: at 0 the patch is dry. MOD DEP is also a mod destination, so a MACRO can ride it. A GATE mod onto FILL at 100 % stops holding FILL once MOD DEPTH is under 50 %.
- PLAY: a mod destination on each track that decides whether it is heard. While its mods reach half its notes sound, under half they don't; a GATE mod onto PLAY makes a gated voice in one cord. A shut track plays on unheard (it still leads, as MUTE).
- CLOCK: a new destination. A generator track with a mod onto CLOCK steps on the source's edges and not on its DIV: with EOC as the source, a note each LFO cycle (a Krell: add S&H onto the LFO's rate). AMOUNT is the division (50 % every second edge), negative the gate closing; a GATE source counts hits. First step at PLAY; DIV is the step's length and the fastest it steps. A project that uses it does not load on older firmware.
- HOLD on notes: hold OCT- on a HOLD mod and turn KNOB 1 to clock it by a track's notes (HOLD ON T1 NOTES .. T4) instead of the bar: the source is taken on each note that track plays.
- MIXER's sheet: hold OCT+ on MIXER for Mutes (NOW, BAR, PHRASE), the same setting as MENU > MUTES, at hand while you play the mute keys.

## Fixed
- ARP HOLD: with CLK set to an external clock (USB, TRS) and the transport stopped, a chord latched with HOLD stopped when the keys were let go, though it played while they were held. It now keeps playing at the local tempo, as the keys do. A chord stopped by the external Stop (or a lost clock) still waits for Start or Continue; a new key plays again.
- USB OUTPUT 4 TRACKS: a loud chord crackled in its stem (cut hard at full scale, #586). A stem now has the stereo recording's level, 6 dB under before, and the outputs' soft clip.
- Web editor on macOS: "Tessera not found" when the FM-1's MIDI port is still named "FM-1" (CoreMIDI keeps the first name it saw). The editor now also tries ports named FM-1 / FM1, takes one only if it answers the editor handshake as Tessera, and says so if an FM-1 does not (stock firmware: install Tessera with the installer). The not-found text hints at the cached name (#527).
- Rename and SAVE while playing: the name screen now opens (the keys type, they never sound); only writing the name needs the transport stopped. OCT+ says STOP TO SAVE and the screen stays; PLAY stops, OCT+ saves. Before, Rename closed the sheet and nothing opened (#602).
- KNOBS BARS: a bar at its maximum no longer touches the top of its outline; it keeps the same one-pixel gap there as at the sides and bottom (13 pixels of travel, not 15).
- STEP direction: a pattern of recorded notes (REC) played them forwards while BACK or PEND moved the playhead backwards; each step now plays its own recorded notes. A tied note going backwards plays once, from the tie's last step, as long as the tie (it played again on its own step). The pattern sheet's Direction row shows a mod moving it as FWD>PEND: the pattern's own, then the one playing.
- SCL > SCALES with a USER scale picked: the keys light and play by their own note whatever QNT is; with QNT on they were snapped and lit wrong (#559).
- OCT+ on GEN with a generator no longer opens the STEP pattern's sheet (GEN keeps it for STEP and ARP; PATTERN for all).
- OCT- held for a short look at a page's second knobs (TWEAK, GEN, MIXER, EVOLVE ..) went to HOME when let go under 0.7 s; now only a quick tap is a tap.
- FM6's EDIT map: moving to an operator's row lands on the page last used there (its tiles have no names; OCT+ opened whichever column the row before had, such as EG RATE).
- A long OCT- press no longer goes back on any page (it did on FORM, TIMING, ENERGY, CHAIN, MIXER and the sound pages), and a knob turned while an OCT key is held no longer counts as a tap of it.
- ARP MODE ORD plays the notes in the order the keys were pressed; it played them low to high (as UP) unless ARP 2's ORDER was PLAY.
- Holding OCT- on EVOLVE or ENTRIES and letting go no longer takes you to HOME (#519).
- EVOLVE's ROLL and ROLL ALL say NEW LINE on the screen (NEW LINE AT BAR while playing), so a turn is no longer silent when stopped (#528).
- Web editor: ORACLE, TRIO and other user presets now show in the Sound tab's preset list, and Save to file names an FM6 user preset after itself, not after TINE EP (#526).
- ANSWER with a SOURCE track that runs a generator (BOUNCE, TURING ...) now answers the notes that track plays; it read that track's hidden STEP pattern instead, often one note or none.
- Muting a generator track (MIXER or GLO's mute keys) no longer stops the tracks that follow it: a muted track plays on unheard, so FOLLOW, an ANSWER's SOURCE, QNT CHRD, QUAD's LINK and the cords still hear its notes, and unmuted it comes back in time.
- Web simulator: DRUM no longer lists SID KIT and SID DIGI, which the FM-1 hasn't had since 3.0 (taken out for flash room; a project saved with them plays the 909). The simulator now has the same sounds as the device (#593).

## Good to know
- Projects saved with these new settings lose them on 3.4.x.
- A project with ARP MODE DNUP or RND1 plays REPEAT on 3.4.x and earlier betas (the last note held).
- STEP direction PONG (there and back, the ends twice: too close to PEND to hear) is now FOLD. A pattern saved with PONG on beta4 or beta5 plays FOLD; earlier betas play FOLD patterns as PONG.
- GROUND is kept with the settings, not in the palette: older firmware keeps your ink and draws it on black.
- On older firmware, a strummed project plays its chords together (3.5-beta4 and beta5 do not load a project with STRUM over 60 ms), a pattern with a direction plays FWD, and a HOLD mod plays as +-.
- A project with CHRD SUS2, 7SUS4, ADD9 or MAJ6 plays POW (power chords) on 3.4.x and earlier betas. A cord or MIDI CC on CHRD now spreads over 14 kinds, not 10, so it lands on slightly different ones.
- A project with a mod into or out of a MIX bus does not load on 3.4.x or earlier betas; the bus settings alone are ignored there.
- On older firmware, a mod of mods that scales plays as one that adds (mod 3's amount plus its move).
- On older firmware, MOD DEPTH plays at 100 %; a project with a mod onto MOD DEP does not load there.
- A project with a mod onto PLAY does not load on 3.4.x or earlier betas.
- On older firmware, a HOLD mod clocked by a track's notes holds by the bar.
- A project with EOC as a source or a mod onto CLOCK does not load on 3.4.x or earlier betas.

## Please try
- PAT GEN with different generators on two patterns: switch by hand and with SONG, save, power off and on, load.
- ENTRIES ORDER with the bass set to come in later.
- PRINT ARP: an ARP track with a chord held, hold REC after two bars; then EDIT held.
- ROLL and ROLL ALL on EVOLVE, stopped and playing: the screen says NEW LINE (NEW LINE AT BAR while playing).
- Web editor: an ORACLE or TRIO user preset shows in the Sound list; Save to file on an FM6 user preset uses its own name.
- Clear mod: pick a mod with a SCALE or a CC, OCT+ held, Clear mod; the mod is empty and the others play on.
- MIX buses: two LFOs into BUS A at SUM, BUS A as a GATE onto FILL; a track's GATE into BUS B with Lag 200 ms onto CUTOFF; save, load.
- RANDOM on GEN with a few generators, stopped and playing; then turn back.
- GROUND LIGHT with a few inks and a MOSAIC set: HOME, the pages, LANDSCAPE, the boot screen after a power cycle; is everything readable on the device's panel?
- USER scales: make one on the keys, play a generator in it, save, power off and on, load; load that project on 3.4.1 (it plays the scale picked under it).
- ANSWER on T2 with SOURCE T1, BOUNCE on T1: it shows LISTENING, takes up to 8 of the balls' notes, then answers them.
- ARP DNUP and RND1 with a chord held: RND1 keeps its order, changes when a key is added, and is the same after STOP and PLAY.
- CHORD STRUM on a CHORD track: STRUM 20 ms and up, and 100 to 250 ms on a slow pad, DIR UP, DOWN and ALT, with BEAT or STEP stabs; MIDI OUT gets the notes spread too.
- STEP direction: FOLD and DRUNK on a melody, with SONG and a pattern change; save, load, and STOP and PLAY replays RAND.
- STEP direction backwards: a REC-recorded pattern on BACK and PEND (each step its own notes on the way back), a tied note on BACK (once, as long as the tie), and a mod onto DIR with the pattern's sheet open (Direction reads FWD>PEND).
- CHRD SUS2, 7SUS4, ADD9, MAJ6 on the chord keys and on a CHORD track (the name on the CHORD page: Csus2, C7sus4, Cadd9, C6), with OPEN and +OCT.
- HOLD mod: an LFO into a filter or pitch held per bar; a cord to DIR.
- MOD SCOPE: hold OCT- on MODS with an LFO mod, a GATE mod (turn its threshold) and a mod of mods; does the picture keep up, and is it readable on the panel (one ink, a MOSAIC set, a light GROUND)? Fast mods should draw a calm line, not a smear.
- The keys on every page: OCT+ held opens a sheet or says NO SHEET; OCT- held gives the second knobs and goes back to the page when let go; the OCT- LED is lit where it does something.
- FOLLOW: T2 FOLLOW T1, mute T1 on MIXER: T2 keeps playing.
- USB stems: USB OUTPUT 4 TRACKS, record the four channels in a DAW (Windows with FlexASIO or ASIO4ALL).
- Scaling mods of mods: T2 GATE scaling M3 (mod 3 an LFO onto CUTOFF at 60 %); then T4 ENV at -100 scaling it; watch it with MOD SCOPE.
- MOD DEPTH: a feedback patch (two generators onto each other) braked with MOD DEPTH; a MACRO onto MOD DEP at -100 %; a GATE onto FILL as MOD DEPTH passes 50 %.
- PLAY: a TIDE or an LFO GATE onto a pad's PLAY; a FOLLOW track behind a shut leader keeps playing.
- HOLD on notes: T1 RAND as HOLD onto T3 CUTOFF, clocked by T2's notes; save, load.
- EOC: an LFO's EOC as a GATE mod onto FILL or a bus; on a slow LFO the gate is a short pulse each cycle.
- CLOCK: EOC onto a generator's CLOCK, then S&H onto that LFO's rate; AMOUNT 50 % for every second edge; a GATE source counting hits; STOP and PLAY start it again.
- A project from 3.4.1 loads and plays the same.
