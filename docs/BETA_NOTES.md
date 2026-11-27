# What's new in the beta

Shown on the installer under the Beta choice (web/publish_beta.py --notes, from the beta workflow). Only the
"## " headings and the "- " lines are shown, as plain text. Update it when a change merges into develop; at the
release it becomes the start of docs/RELEASE_X.Y.md, then this file starts again for the next beta.

Found a problem? Open an issue: <https://github.com/Rastapopolix/tessera/issues> (the firmware version, the steps,
what you expected and what happened).

## New since 3.7.6
- GLO > SYSTEM shows CPU 1 CORE when the FM-1's second core has stopped (it then renders on one core until power-off, with about half the room); the screen reader says it too.
- FM6's STORE page now comes last, after VOICE.
- Thanks in ABOUT > CREDITS to u/dreaminganimal and @jfworld.

## Fixed
- Fewer clicks when the voices run out: a voice BUDGET has to take now fades over about 3 ms instead of 0.7 ms.
- While the voice pool is nearly full, quiet release tails (under -20 dB) fade faster, so BUDGET frees a quiet voice before it has to take a loud one (the ADSR engines; FM6 and ORACLE keep their own envelopes).
- ORACLE: a fast sub bass no longer clicks between notes (the amp envelope's floors).
- Less CPU at idle: a silent moment skips the output stage (the sound is unchanged).
- LIGHTS OFF: the keys on MIXER and PERF no longer blink to show what can be pressed; the mute, solo, FILL and HOLD states still light (hold GLO for the blinking map).

## Good to know
- Don't lower BUDGET to cure clicks: a small pool is full most of the time, and every new note then has to take a voice. Lower it for CPU.

## Please try
- A project from 3.7.6 loads and plays the same.
- A busy project at a low BUDGET (8 to 12) with long releases: fewer clicks than on 3.7.6?
- ORACLE with FM6 tracks: watch FADED and LATE on GLO > SYSTEM, and tell us if CPU 1 CORE ever shows.
