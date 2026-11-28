#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Put a Tessera MIDI test build (TESSERA_MIDI_ONLY: 8 tracks, no sound; both cores) on an existing site checkout's
installer, a choice of its own beside the beta, or take it off (--off); the release, the beta and the other channels
stay. Its notes (docs/BETA_NOTES_MIDI.md) show under it. A release does not end it: --off does.

  web/publish_midi.py docs --dual tessera-3.7.7-tm1.fwsc --single tessera-3.7.7-tm1-1core.fwsc \\
      --version 3.7.7-tm1 --commit SHA --built-at 2026-11-28T08:00:00Z [--notes docs/BETA_NOTES_MIDI.md]
  web/publish_midi.py docs --off"""
import argparse
from pathlib import Path
from make_site import main, release_packages
from publish_beta import read_notes


def publish(out, dual=None, single=None, version=None, commit=None, built_at=None, off=False, notes=None):
    release, packages = release_packages(out)
    midi = False if off else {"pkg": {"dual": dual, "single": single}, "version": version, "commit": commit,
                              "builtAt": built_at}
    if midi and notes and Path(notes).exists():
        midi["notes"] = read_notes(notes)
    main(packages["dual"], packages["single"], release, out, midi=midi, installer_only=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("out")
    parser.add_argument("--off", action="store_true", help="take Tessera MIDI's test build off the installer")
    parser.add_argument("--dual")
    parser.add_argument("--single")
    parser.add_argument("--version")
    parser.add_argument("--commit")
    parser.add_argument("--built-at")
    parser.add_argument("--notes", help="what is in the test build (docs/BETA_NOTES_MIDI.md), shown on the installer")
    args = parser.parse_args()
    if not args.off and not all((args.dual, args.single, args.version, args.commit, args.built_at)):
        parser.error("--dual, --single, --version, --commit and --built-at are needed (or --off)")
    publish(args.out, args.dual, args.single, args.version, args.commit, args.built_at, args.off, args.notes)
