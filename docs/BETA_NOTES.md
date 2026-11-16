# What's new in the beta

Shown on the installer under the Beta choice (web/publish_beta.py --notes, from the beta workflow). Only the
"## " headings and the "- " lines are shown, as plain text. Update it when a change merges into develop; at the
release it becomes the start of docs/RELEASE_X.Y.md, then this file starts again for the next beta.

Found a problem? Open an issue: <https://github.com/Rastapopolix/tessera/issues> (the firmware version, the steps,
what you expected and what happened).

## New in 3.6-beta9
- MARKOV and MORPH: turn LOCK up from 0 while playing and the pass you just heard loops at once (in beta8 they played on fresh for two passes, then looped the second).
- ENTRIES' ROLE AUTO now guesses each track's part before it plays, from its generator, sound and register, judging the four tracks together: CHORD and chord keys are HARMONY; GRIDS, a kit or NOISE RHYTHM; the lowest track clearly below the others BASS. The ROLE knob shows the guess. A project on AUTO may get different roles than in beta8.
- ENTRIES with STYLE FADE now fades MIDI OUT too: while a track fades out or in, its notes go out with their velocity scaled by its level (a synth that ignores velocity still hears a cut). A track at SOUND OFF now fades over MIDI as well.
- HOME: hold ENV and tap OCT- or OCT+ to move the selected track an octave down or up (its TRN, up to two octaves either way); ENV + both OCTs puts it back to 0. Plain OCT- and OCT+ still move the keys' octave.
- PRESETS: LIST now has the engines after the categories (FM6, ORACLE, TRIO, PHYS, DRUM), each listing only that engine's sounds, its user presets included: an ORACLE pad is easy to find. HOME's PRESETS knob and GLO + PRESETS follow the list you pick, as before.
- EDIT > VOICE starts with the track's Sound (SOUND KUGO HARP >): KNOB 2 steps through the sounds, OCT+ opens PRESETS right there; OK keeps the sound, BACK puts the old one back, and both bring you back to VOICE instead of HOME.
- LANDSCAPE: during a PIECE the moon waxes lit on the left and wanes lit on the right, so it faces into the scene as it rises and sets.
- Fewer crackles under heavy load: the FM-1 now holds back new voices as soon as the CPU passes 70 % (a new note takes over a fading one, the bass kept), before it has to cut a sound. A dense chord may lose a released tail a little earlier.
- TRIO sounds (STRING MACH, WARM PAD ...) now share the work with the FM-1's second core, as FM6, ORACLE and DRUM already did: SYSTEM's CPU should sit lower with TRIO tracks. The sound is exactly the same.

## New in 3.6-beta8
- Microtonal scales and your own Scala tunings now play with QNT OFF too. A scale of up to 12 degrees an octave (the maqamat RAST, BAYATI, HIJAZ, SABA, HUSAYN, SIKAH, the just and meantone scales, the Japanese pentatonics) keeps the keyboard a piano's: every key plays its own note, and the keys nearest the scale's degrees take its tuning. RAST on C: E and B play half flat, C# is an ordinary C#, so you play a maqam as you would on a piano and can still reach every note off it. A bigger tuning (24EDO, 31EDO, a large Scala file) plays every degree in a row, one a key and one a MIDI note. They used to fall back to ordinary semitones: a project saved with a microtonal scale and QNT OFF now plays in its tuning.

- LANDSCAPE: a named project gets a place of its own, made from its name (its hills or islands, its stars, where the moon rests, a torii gate on A's shore). The layers you switch on show in the scene: a PIECE's shape is the moon's phase (waxing, lit on the left, as the shape rises; waning, lit on the right, as it falls), with dusk (or the stars coming out) over its last tenth; each CHAIN section has its own shore, the new one coming in from the right, and a shooting star when the form comes round; ENERGY blows the clouds right as it builds and left as it thins, and a rest lays mist on the water.
- MENU > IDLE LANDSCAPE: after two minutes without a touch while playing, the screen moves to LANDSCAPE; the first touch only takes you back to the page you were on. MENU > TEXT BOLD draws the small text heavier on every screen. OFF and NORMAL (the defaults) as before. LARGE VALUE ON now keeps the large value about a second after the knob stops.
- While a PIECE plays, the line under every header is solid as far as the piece has got and dotted after. The PLAY triangle (HOME and the list headers) is solid on the 1, hollow on the other beats, and blinks on the 1 of a phrase line (CHAIN's 4, 8 or 16 bars).
- MIXER: a waiting mute (MENU MUTES BAR or PHRASE) gets a thin line under its MUTE box that counts down to the line where it lands.
- ROLL: while playing, a turned seed waiting for the bar line shows dotted under the one playing until it lands. ROLL ALL goes back two levels: turn it left once for the seeds before the last ROLL ALL, twice for the ones before that.
- QUAD's DIR and RANGE are drawn as pictures (arrows, two dots for RAND, a bar for HOLD; one to three ticks for the range) instead of single signs.
- HOLD, GLO + D4: while held, every generator keeps the line it is playing (EVOLVE skips its rounds, TURING plays as PROB 0, MARKOV, MORPH and CONSTELLATION repeat their loop, ATLAS keeps its bar); let go and they carry on. Tap twice to latch it, once more to let go. On MIXER, PERF and LANDSCAPE, D4 alone does it. MARKOV and MORPH at LOCK 0 repeat the last pass they played.
- MIDI OUT's sheet, SECTION PC (one row a track, OFF by default): when the CHAIN goes to section A, B or C, the track sends Program Change 0, 1 or 2 on its MIDI OUT channel, before that bar's notes. A project with SECTION PC on loads on older firmware with it off.
- MODS: a STEP track's GATE and NOTE sources now follow its pattern, as a generator track's do.
- MENU > NOTE LIGHTS: the keys light up while a sequence plays their notes, for as long as each note lasts. OFF (the default) as before; STEP lights a STEP pattern's notes; ALL the generators' too.

## New in 3.6-beta7
- QNT link (SCL, the row after QNT): LINKED keeps one QNT for every track, as before; OWN gives each track its own QNT and MPC degree, so one track can take MPC pads while others follow the chords (CHRD) or HARM (#755). A project saved with OWN loads on older firmware with each track's QNT as saved.

## New in 3.6-beta6
- HARM, a harmonizer (after Dreamsequence's): with a CHORD track playing, each note of the scale up plays the chord's next note up, so a run up the keys is an arpeggio that follows the chords. SCL > QNT HARM does this for the keys, MIDI IN and the generators; FOLLOW's new HOW HARM does it to another track's line, SHIFT moving by chord notes. A project using HOW HARM does not load on older firmware.
- MENU > ABOUT shows **SPACE**: how much flash and RAM this firmware leaves free for new features.
- ECHO now rests while nothing reaches it and its tail has died away, which frees CPU when tracks are quiet. The sound is exactly the same.
- PPQN, the MIDI clock's pulses per beat: on GLO > MIDI IN, hold OCT- and KNOB 4 sets 24 (the default, the MIDI standard), 48, 4, 8 or 12, for the clock the FM-1 follows and for CLOCK OUT. If the FM-1 plays twice as fast as the gear it follows (HOME shows double the tempo), try 48. Like CLK, it is kept through power-off and project loads (#747).
- After a project loads, a track playing one of your own FM6 voices finds the user slot holding that voice again: HOME shows its name (not TINE EP) and PRESETS starts from it. If no slot holds it any more, HOME shows the name stored in the voice (#744).
- The installer waits a few seconds for the FM-1 instead of giving up after one quick look, and when another app or tab holds the FM-1's MIDI port it says so and names the port, instead of FM-1 not found (#743).
- The FM-1's update mode is now called Tessera Update on the computer (it was Melodee Update). A computer that has seen the old name may keep showing it.

## New in 3.6-beta5
- MODS: KNOB 1's source list now starts at the selected track too, as KNOB 3's does: on track 3, the first turn lands on T3 LFO.
- When the FM-1 runs short of CPU and has to cut a sound early, it now cuts the quietest fading drum first (a hat's tail), not the kick or a snare in the middle of a roll.

## New in 3.6-beta4
- MODS: KNOB 3's list now starts at the selected track (it started at the mod's source's track, and at T1 for a new mod); a new mod's first turn of KNOB 3 lands on the selected track's PITCH. Thanks for the report (#724).

## New in 3.6-beta3
- SIM CONTROL (in the menu: hold HOME): with the web simulator open and the FM-1 connected by USB, the FM-1's knobs, buttons and keys play only the simulator, and the FM-1 itself stays silent. Hold FX + GLO for a second to get the panel back; closing the simulator or unplugging ends it too. It is off every time the FM-1 starts.

## New in 3.6-beta2
- MODS: 22 more destinations: STRUM, STR DIR, RATCH, R CHANC, GATE, N LEN, LOCK, VELO, SWING, GLIDE, GRAVITY, DEVELOP, AN ECHO, AN SHFT, AN SPCE, BREATH, ACC LIM, TIDE SP, SWRM RT, ARP RTE, ARP GTE and ARP OCT.
- MODS: KNOB 3's destinations come in groups (SOUND, ENV, LFO, MIX, FX, GEN, FEEL, EVOLVE, ARP, MODS), and the list's title names the group under the cursor, for example T3 LFO.
- MODS: a track lists only the destinations that act on what it plays (Turing's settings on a Turing track, STRUM on a CHORD track). A mod already set to one keeps it.
- MODS: the list starts at the mod's own source track; a new mod's first turn of KNOB 3 lands on that track's PITCH.
- The sound designer has been removed, to make room for new features. Saving and loading user sounds from the SOUND sheet works as before.

## New since 3.5.3
- ANSWER holds each note as long as you held its key in the call (when the track's LENGTH is on STEP, the default; FIXED and HIT still set the length).
- ANSWER captures up to 16 notes in a call (was 8).
- ANSWER CHORD (on the GEN sheet, hold OCT+): ARP spreads notes played together one step apart, as before; TRUE answers them as a real chord.

## Fixed
- FM6 AEG (EDIT 2, KNOB 4): the slow side is usable all the way up. It slows the envelopes of the operators you hear by up to 24 rate steps (it was 40) and never to a release longer than about 15 s, unless the patch's own is longer; near the top, notes rang on for minutes and held voices. A project with AEG turned right sounds a little shorter in its tails than before; AEG 0 and the left side are as they were.
- Web simulator, An FM-1 on USB: a click of the FM-1's knobs is one step again where Windows listed the FM-1 twice (lists such as the menu and MODS skipped every other row).
- After an overload (a crackle), the voices come back within a few seconds while you keep playing, and one overload no longer holds every track to one or two voices.
- Help on the device for ATLAS DENSITY, ATLAS PART4, CHORD PROG, FOLLOW RHYTHM and CONSTELLATION SPEED now says to hold OCT-, as the keys work.
- SYSTEM: the CPU dial no longer shows its own jittering number. The CPU bar further down is the one readout.

## Good to know
- A project that uses one of the new MODS destinations does not load on 3.6-beta1 or older.
- The project format has changed: projects saved on this beta do not load on 3.5.3 or older. Keep a backup before you try it.
- Patterns (PAT GEN) keep ANSWER's first 8 notes; CHORD, held lengths and notes 9-16 belong to the track, not the pattern.

## Please try
- ENTRIES with STYLE FADE and a track on an outside synth that responds to velocity (MENU > NOTES OUT ALL): its notes fade out and back in with the FM-1's level.
- A project that crackled in beta8 (two dense FM6 tracks, or TRIO pads): fewer clicks, and SYSTEM's CPU peak lower.
- ENTRIES with every ROLE on AUTO: the bass comes in first and the melody leaves first, from the first section.
- MIDI clock from your gear (CLK USB or TRS): the FM-1 plays at the gear's tempo. If it plays twice as fast, set PPQN to 48 (GLO > MIDI IN, hold OCT-, KNOB 4). CLOCK OUT at 8 or 4 into gear that expects fewer pulses.
- Save a project with a track on one of your own FM6 voices (F001 ..), switch off and on, load it: HOME names the voice, and PRESETS starts from its slot.
- The installer with the editor or a DAW open: it names the MIDI port that is in use.
- SIM CONTROL: open the web simulator with the FM-1 on USB, turn SIM CONTROL on in the menu, play the FM-1: only the simulator answers. Hold FX + GLO for a second: the FM-1 plays again. Hold GLO alone while it is on: GLO's settings open in the simulator.
- A mod onto GATE, SWING or ARP RTE on a playing track, RATCH on a generator track, VELO on a STEP track.
- On MODS, a mod from T3's LFO: turn KNOB 3. T3 comes first and the title follows the groups; with T1 on Turing, T1 offers T PROB but not E PULSE.
- The 909 on ATLAS at 85 BPM with DENSITY at 64 % or more: the snares keep playing. A busy four-track project: after a crackle, the voices come back within seconds.
- A project from 3.5.3 loads and plays the same.
- On an ANSWER track, hold some keys longer than others: the answer holds them too.
- Set CHORD to TRUE and play a chord: the answer plays it as a chord.
- Play a call of more than 8 notes: the answer plays all of them.
