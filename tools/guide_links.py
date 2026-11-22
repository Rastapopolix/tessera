#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Theo Dainis (melodee-tess)
"""The guided clips' YouTube links in MANUAL.md, from tools/guide_videos.json (each clip's video, its playlist and the
manual section it belongs to; the clips themselves are rendered by tests/demo_projects.c guide).

  python3 tools/guide_links.py          rewrite the links in MANUAL.md
  python3 tools/guide_links.py --check  fail when MANUAL.md's links are not the JSON's

Each section that has clips gets one line under its heading: the clips' thumbnails (assets/guides/<clip>.png), each
a link to its video, in the JSON's order. The site's manual turns that line into click-to-load players and the PDF
into text links (web/make_manual.py videos()). The playlists go in one paragraph after "New to Tessera?". Every line
it writes ends in MARK, so a rerun replaces them and a clip moved to another section moves with it.
"""
import html
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANUAL = ROOT / "MANUAL.md"
DATA = ROOT / "tools/guide_videos.json"
MARK = "<!-- guide videos -->"
PLAYLIST = "https://www.youtube.com/playlist?list="
VIDEO = "https://youtu.be/"
THUMBS = "assets/guides/"                                   # each clip's thumbnail, 640x360
AFTER = "**New to Tessera?**"                               # the playlists' paragraph follows this one


def short(title):
    """a playlist's name in the paragraph: 'Tessera reference: mods' -> 'mods'"""
    return re.sub(r"^Tessera( reference)?: ", "", title)


def build(text, data):
    lines = []
    for l in text.split("\n"):                          # (each removed line takes the blank line before it)
        if l.endswith(MARK):
            if lines and lines[-1] == "":
                lines.pop()
        else:
            lines.append(l)
    by = {}
    for c in data["clips"]:
        by.setdefault(c["section"], []).append(c)
    out, done = [], set()
    k = 0
    while k < len(lines):
        l = lines[k]
        out.append(l)
        m = re.match(r"^#+ (.*)$", l)
        if m and m.group(1).strip() in by and m.group(1).strip() not in done:
            h = m.group(1).strip()
            done.add(h)
            cs = by[h]
            thumbs = " ".join(f'<a href="{VIDEO}{c["video"]}"><img src="{THUMBS}{c["clip"]}.png" width="240" '
                              f'alt="Video: {html.escape(c["title"])}"></a>' for c in cs)
            out += ["", f"<p>{thumbs}</p> {MARK}"]
        if l.startswith(AFTER):
            while k + 1 < len(lines) and lines[k + 1] != "":
                k += 1
                out.append(lines[k])
            p = data["playlists"]
            first, rest = p[0], p[1:]
            out += ["", f"**Guide videos.** Short captioned clips show each part on the screen. Start with "
                        f"[{short(first['title'])}]({PLAYLIST}{first['id']}), {len([c for c in data['clips'] if c['playlist'] == first['id']])} "
                        f"clips in order, then the reference playlists: "
                        + ", ".join(f"[{short(x['title'])}]({PLAYLIST}{x['id']})" for x in rest)
                        + f". Each section below links its own clips. {MARK}"]
        k += 1
    missing = set(by) - done
    if missing:
        raise SystemExit(f"guide_links: no heading in MANUAL.md for {', '.join(sorted(missing))}")
    return "\n".join(out)


def main(check=False):
    data = json.loads(DATA.read_text(encoding="utf-8"))
    text = MANUAL.read_text(encoding="utf-8")
    new = build(text, data)
    if check:
        if new != text:
            raise SystemExit("guide_links: MANUAL.md's video links are out of date: run python3 tools/guide_links.py")
        print("guide_links: MANUAL.md matches tools/guide_videos.json")
        return
    MANUAL.write_text(new, encoding="utf-8")
    print(f"guide_links: {len(data['clips'])} clips in {len({c['section'] for c in data['clips']})} sections")


if __name__ == "__main__":
    main("--check" in sys.argv[1:])
