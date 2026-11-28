# What's new in the Tessera MIDI test build

Shown on the installer under the Beta choice when the beta is a Tessera MIDI build (beta/X.Y-tmN: web/publish_beta.py
--notes, from the beta workflow). Only the "## " headings and the "- " lines are shown, as plain text.

Found a problem? Open an issue: <https://github.com/Rastapopolix/tessera/issues> (say "Tessera MIDI", the version,
the steps, what you expected and what happened).

## Tessera MIDI: the FM-1 as a sequencer for your other gear
- No sound of its own: everything goes out over MIDI (USB, or USB host).
- 8 tracks instead of 4, four at a time: ALGORITHM picks the track (past T4: tracks 5 to 8), PRESETS on HOME picks the bank (right 5-8, left 1-4). The header shows the bank.
- 32 mods instead of 16, and no voice limit: any track can send full chords.
- The 8-track features: the ATLAS genre banks, GEN LANE and RANGE, CHORD USER qualities, chord chops, FOLLOW KICK, ENTRIES across 8 tracks, DROP.
- SLOPE (a rise and fall LFO) and Program Change in.
- The buttons: FX opens ECHO and FEEL (the MIDI FX), MIDI OUT (each track's channel and transpose), then MIDI IN. EDIT opens VOICE (POLY or MONO, PRIO, ALLOC, MUTE). SAVE opens PROJECT; hold SAVE to quick-save. The sound and effect pages are gone.
- Where a sound's name was, a track shows its MIDI channel (CH 3, or OFF); HOME shows what plays each track (STEP, TURING ..).
- NOTES OUT is ALL: the sequencer's notes go out, not only the keys.

## New in tm3
- MIDI FX, on FX: the ECHO page repeats each note (up to 8, TIME 1/32 to 1/4), each FADE % softer and PITCH semitones apart.
- The FEEL page: STRUM spreads a chord's notes (up to 100 ms, DIR up, down or random); VEL sends every note at one velocity (0: as played); HUMAN plays notes a little late and softer or harder, at random.
- ENV: each track has an envelope again (ATK DEC SUS REL), started by every note. ENV CC sends it as a CC (CC number, DEPTH, BASE; DEPTH 0 sends nothing).
- LFO CC (in place of LFO DEST): the track's LFO as a CC, swinging around BASE. ENV and LFO now also work as mod sources.
- MIXER: LEVEL goes out as CC 7 and PAN as CC 10 when you move them.
- MODS lists only destinations that do something over MIDI (no filter, shape, pitch ..).

## Good to know
- Back up with the web editor first. Tessera MIDI keeps bigger projects in the flash the user presets and FM6 voice banks used: once a big project is saved there, those presets are gone when you go back to the standard build.
- A project of more than about 38 KB opens only in Tessera MIDI. A very full one may say PROJECT TOO BIG.
- Back to the synths: choose the standard build in the installer.

## Please try
- Tracks 5 to 8: select them, record into them, and check their notes go out on their channels.
- Save and load an 8-track project, and a 3.7.6 project.
- Drive your gear for a while: notes, clock, CCs from the mods.
- MIDI FX: echoes and strums on a synth, and that every note still ends when you stop.
- ENV CC and LFO CC on a filter cutoff (CC 74 is common).
