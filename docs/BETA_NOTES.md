# What's new in the beta

Shown on the installer under the Beta choice (web/publish_beta.py --notes, from the beta workflow). Only the
"## " headings and the "- " lines are shown, as plain text. Update it when a change merges into develop; at the
release it becomes the start of docs/RELEASE_X.Y.md, then this file starts again for the next beta.

Found a problem? Open an issue: <https://github.com/Rastapopolix/tessera/issues> (the firmware version, the steps,
what you expected and what happened).

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
- After an overload (a crackle), the voices come back within a few seconds while you keep playing, and one overload no longer holds every track to one or two voices.
- Help on the device for ATLAS DENSITY, ATLAS PART4, CHORD PROG, FOLLOW RHYTHM and CONSTELLATION SPEED now says to hold OCT-, as the keys work.
- SYSTEM: the CPU dial no longer shows its own jittering number. The CPU bar further down is the one readout.

## Good to know
- A project that uses one of the new MODS destinations does not load on 3.6-beta1 or older.
- The project format has changed: projects saved on this beta do not load on 3.5.3 or older. Keep a backup before you try it.
- Patterns (PAT GEN) keep ANSWER's first 8 notes; CHORD, held lengths and notes 9-16 belong to the track, not the pattern.

## Please try
- A mod onto GATE, SWING or ARP RTE on a playing track, RATCH on a generator track, VELO on a STEP track.
- On MODS, a mod from T3's LFO: turn KNOB 3. T3 comes first and the title follows the groups; with T1 on Turing, T1 offers T PROB but not E PULSE.
- The 909 on ATLAS at 85 BPM with DENSITY at 64 % or more: the snares keep playing. A busy four-track project: after a crackle, the voices come back within seconds.
- A project from 3.5.3 loads and plays the same.
- On an ANSWER track, hold some keys longer than others: the answer holds them too.
- Set CHORD to TRUE and play a chord: the answer plays it as a chord.
- Play a call of more than 8 notes: the answer plays all of them.
