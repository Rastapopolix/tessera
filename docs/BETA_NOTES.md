# What's new in the beta

Shown on the installer under the Beta choice (web/publish_beta.py --notes, from the beta workflow). Only the
"## " headings and the "- " lines are shown, as plain text. Update it when a change merges into develop; at the
release it becomes the start of docs/RELEASE_X.Y.md, then this file starts again for the next beta.

Found a problem? Open an issue: <https://github.com/Rastapopolix/tessera/issues> (the firmware version, the steps,
what you expected and what happened).

## New since 3.7.4
- Overload blink: when the FM-1 runs out of time, the header's four track dots fill for a moment (once: a voice faded to catch up; twice: the sound was late, a crackle). GLO > SYSTEM counts both since power-on (FADED, LATE). MENU > OVERLOAD turns the blink off.

## Fixed

## Good to know

## Please try
- A project from 3.7.4 loads and plays the same.
- Push a busy project until it crackles: do the dots blink when you hear it, and stay still when you don't?
