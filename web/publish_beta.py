#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Put a beta (the next update, both cores) on an existing site checkout's installer, or take it off (--off); keep its
release, editor, project page and the other channels. The beta's own editor (this checkout's web/, with the demo and CHK
test projects) goes to webapp/editor-beta/, linked from the installer's Beta choice. The release that the beta leads to drops it by itself
(make_site.py), so --off is only for a beta withdrawn before then.

  web/publish_beta.py docs --dual tessera-3.3-beta1.fwsc --single tessera-3.3-beta1-1core.fwsc \\
      --version 3.3-beta1 --commit SHA --built-at 2026-11-04T19:00:00Z
  web/publish_beta.py docs --off"""
import argparse
from pathlib import Path
from make_site import main, release_packages


def read_notes(path):
    """docs/BETA_NOTES.md: its "## " headings and the "- " lines under them, as [[heading, [line, ..]], ..], plain text
    (** and ` dropped); the rest of the file (its own notes for the maintainers) is left out"""
    notes = []
    for line in Path(path).read_text(encoding="utf-8").splitlines():
        text = line.replace("**", "").replace("`", "").strip()
        if text.startswith("## "):
            notes.append([text[3:].strip(), []])
        elif text.startswith("- ") and notes:
            notes[-1][1].append(text[2:].strip())
    return [n for n in notes if n[1]]


def publish(out, dual=None, single=None, version=None, commit=None, built_at=None, off=False, notes=None):
    release, packages = release_packages(out)
    beta = False if off else {"pkg": {"dual": dual, "single": single}, "version": version, "commit": commit,
                              "builtAt": built_at}
    if beta and notes and Path(notes).exists():
        beta["notes"] = read_notes(notes)
    main(packages["dual"], packages["single"], release, out, beta=beta, installer_only=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("out")
    parser.add_argument("--off", action="store_true", help="take the beta off the installer")
    parser.add_argument("--dual")
    parser.add_argument("--single")
    parser.add_argument("--version")
    parser.add_argument("--commit")
    parser.add_argument("--built-at")
    parser.add_argument("--notes", help="what is new in the beta (docs/BETA_NOTES.md), shown on the installer")
    args = parser.parse_args()
    if not args.off and not all((args.dual, args.single, args.version, args.commit, args.built_at)):
        parser.error("--dual, --single, --version, --commit and --built-at are needed (or --off)")
    publish(args.out, args.dual, args.single, args.version, args.commit, args.built_at, args.off, args.notes)
