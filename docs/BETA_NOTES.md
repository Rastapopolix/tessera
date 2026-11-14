# What's new in the beta

Shown on the installer under the Beta choice (web/publish_beta.py --notes, from the beta workflow). Only the
"## " headings and the "- " lines are shown, as plain text. Update it when a change merges into develop; at the
release it becomes the start of docs/RELEASE_X.Y.md, then this file starts again for the next beta.

Found a problem? Open an issue: <https://github.com/Rastapopolix/tessera/issues> (the firmware version, the steps,
what you expected and what happened).

## New since 3.5.3
- ANSWER holds each note as long as you held its key in the call (when the track's LENGTH is on STEP, the default; FIXED and HIT still set the length).
- ANSWER captures up to 16 notes in a call (was 8).
- ANSWER CHORD (on the GEN sheet, hold OCT+): ARP spreads notes played together one step apart, as before; TRUE answers them as a real chord.

## Fixed
- SYSTEM: the CPU dial no longer shows its own jittering number. The CPU bar further down is the one readout.

## Good to know
- The project format has changed: projects saved on this beta do not load on 3.5.3 or older. Keep a backup before you try it.
- Patterns (PAT GEN) keep ANSWER's first 8 notes; CHORD, held lengths and notes 9-16 belong to the track, not the pattern.

## Please try
- A project from 3.5.3 loads and plays the same.
- On an ANSWER track, hold some keys longer than others: the answer holds them too.
- Set CHORD to TRUE and play a chord: the answer plays it as a chord.
- Play a call of more than 8 notes: the answer plays all of them.
