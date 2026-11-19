# What's new in the beta

Shown on the installer under the Beta choice (web/publish_beta.py --notes, from the beta workflow). Only the
"## " headings and the "- " lines are shown, as plain text. Update it when a change merges into develop; at the
release it becomes the start of docs/RELEASE_X.Y.md, then this file starts again for the next beta.

Found a problem? Open an issue: <https://github.com/Rastapopolix/tessera/issues> (the firmware version, the steps,
what you expected and what happened).

## New since 3.7.1
- ECHO: TIME (knob 3) now reads in milliseconds, 10 to 367 ms, instead of %. The web editor shows it the same way. Only the display changed: the echo sounds the same and projects are unchanged.

## Fixed
- ANSWER: with the transport stopped, a pause of a second and a half now ends the phrase, so the next key starts a new one. Before, every key played while stopped joined the same phrase, and once it held 16 notes the picture kept the old ones.

## Good to know

## Please try
- A project from 3.7.1 loads and plays the same.
