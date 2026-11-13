#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Theo Dainis (melodee-tess)
"""Tessera Mono, the web pages' pixel face: the Mono look's own 5 x 7 font (firmware/src/mo_font.h MO_FONT) as a
WOFF, so the site, the installer, the editor and the simulator page letter as the device does.

  python3 tools/gen_mono_webfont.py          (needs fonttools)
  python3 tools/gen_mono_webfont.py --check  (tests/run_tests.sh: fails when a page's face is not MO_FONT's)

A font pixel is 100 units, a glyph 6 pixels wide (5 and one clear), 7 above the baseline and 1 below: 800 units an
em, so the face is sharp at 8, 16, 24 ... px. Lower case is drawn as upper (as on the device); a few signs the pages
use and the device has no need of (| { } ~ ` and the middle dot, dashes, ellipsis, times) are added here, and the
accented letters of the pages' other languages (latin(): the whole capital with a 2-row accent above the cap height,
C cedilla, sharp s, inverted ! and ?, guillemets; the device has none of them, so this costs no flash). The line
metrics stay 7 + 1 pixels (typo metrics, USE_TYPO_METRICS); the Windows ascent leaves room for the accents. The WOFF
goes inline, as a data URL, between the TESSERA-MONO markers of every page in PAGES, so each one stays a single file.
"""
import base64
import io
import re
import sys
from pathlib import Path

from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "firmware/src/mo_font.h"
PAGES = ("web/home/index.html", "web/index_pkg.html", "web/editor.html", "web/sim/index.html")
PX, ADV, ASC, DESC = 100, 6, 7, 1
EXTRA = {                                               # code point: 7 rows, bit 4 the left column (as MO_FONT)
    0x60: (8, 4, 0, 0, 0, 0, 0), 0x7B: (2, 4, 4, 8, 4, 4, 2), 0x7C: (4, 4, 4, 4, 4, 4, 4),
    0x7D: (8, 4, 4, 2, 4, 4, 8), 0x7E: (0, 0, 8, 21, 2, 0, 0), 0xB7: (0, 0, 0, 4, 0, 0, 0),
    0xD7: (0, 17, 10, 4, 10, 17, 0), 0x2013: (0, 0, 0, 14, 0, 0, 0), 0x2014: (0, 0, 0, 31, 0, 0, 0),
    0x2026: (0, 0, 0, 0, 0, 0, 21), 0x2019: (12, 4, 8, 0, 0, 0, 0), 0x2018: (4, 8, 12, 0, 0, 0, 0),
    0x201C: (10, 20, 30, 0, 0, 0, 0), 0x201D: (30, 10, 20, 0, 0, 0, 0),
}
ACCENT_ROWS = "accent"                                  # marks a glyph drawn from 2 rows above the cap height
ACCENTS = {"grave": (8, 4), "acute": (2, 4), "circ": (4, 10), "tilde": (13, 22), "uml": (10, 0)}
LATIN = {                                               # code point of the capital: (base letter, accent)
    0xC0: ("A", "grave"), 0xC1: ("A", "acute"), 0xC2: ("A", "circ"), 0xC3: ("A", "tilde"), 0xC4: ("A", "uml"),
    0xC8: ("E", "grave"), 0xC9: ("E", "acute"), 0xCA: ("E", "circ"), 0xCB: ("E", "uml"),
    0xCC: ("I", "grave"), 0xCD: ("I", "acute"), 0xCE: ("I", "circ"), 0xCF: ("I", "uml"), 0xD1: ("N", "tilde"),
    0xD2: ("O", "grave"), 0xD3: ("O", "acute"), 0xD4: ("O", "circ"), 0xD5: ("O", "tilde"), 0xD6: ("O", "uml"),
    0xD9: ("U", "grave"), 0xDA: ("U", "acute"), 0xDB: ("U", "circ"), 0xDC: ("U", "uml"),
}
SIGNS = {                                               # rows as EXTRA; C cedilla has an 8th row, under the baseline
    0xA1: (4, 0, 4, 4, 4, 4, 4), 0xBF: (4, 0, 4, 8, 16, 17, 14), 0xAB: (0, 0, 5, 10, 20, 10, 5),
    0xBB: (0, 0, 20, 10, 5, 10, 20), 0xDF: (12, 18, 18, 22, 17, 17, 22),
}
MARK = re.compile(r"(/\* TESSERA-MONO:[^*]*\*/).*?(/\* /TESSERA-MONO \*/)", re.S)


def mo_font():
    """MO_FONT's first 64 glyphs, ' ' .. '_' (the pages set in capitals; its small letters are a chord name's)"""
    body = re.search(r"MO_FONT\[\d+\]\[7\] = \{(.*?)\n\};", SRC.read_text(), re.S).group(1)
    rows = [tuple(int(v) for v in m.split(",")) for m in re.findall(r"\{([\d,\s]+)\}", body)][:64]
    assert len(rows) == 64 and all(len(r) == 7 for r in rows), "MO_FONT: 64 glyphs of 7 rows"
    return {0x20 + i: r for i, r in enumerate(rows)}


def latin(font):
    """the accented letters (capital and small: the pages draw both as capitals), C cedilla and the signs"""
    out = {}
    for cp, (base, accent) in LATIN.items():
        out[cp] = out[cp + 0x20] = (ACCENT_ROWS,) + ACCENTS[accent] + font[ord(base)]
    out[0xC7] = out[0xE7] = font[ord("C")] + (4,)
    out.update(SIGNS)
    return out


def glyph(rows):
    """the rows' pixels as squares, each row's runs merged (no overlapping contours); rows that start with
    ACCENT_ROWS begin two rows above the cap height"""
    first = 0
    if rows and rows[0] == ACCENT_ROWS:
        rows, first = rows[1:], -2
    pen = TTGlyphPen(None)
    for r, bits in enumerate(rows, first):
        top = (ASC - r) * PX
        c = 0
        while c < 5:
            if bits & (16 >> c):
                e = c
                while e < 5 and bits & (16 >> e):
                    e += 1
                x0, x1 = c * PX, e * PX
                pen.moveTo((x0, top - PX)); pen.lineTo((x0, top)); pen.lineTo((x1, top)); pen.lineTo((x1, top - PX))
                pen.closePath()
                c = e
            else:
                c += 1
    return pen.glyph()


def build():
    cmap_src = mo_font()
    for c in range(0x61, 0x7B):                         # lower case drawn as upper
        cmap_src[c] = cmap_src[c - 32]
    cmap_src.update(EXTRA)
    cmap_src.update(latin(cmap_src))
    names, glyphs, cmap, seen = [".notdef"], {".notdef": TTGlyphPen(None).glyph()}, {}, {}
    for cp in sorted(cmap_src):
        rows = cmap_src[cp]
        if rows not in seen:
            seen[rows] = f"g{cp:04X}"
            names.append(seen[rows])
            glyphs[seen[rows]] = glyph(rows)
        cmap[cp] = seen[rows]
    fb = FontBuilder(unitsPerEm=(ASC + DESC) * PX, isTTF=True)
    fb.setupGlyphOrder(names)
    fb.setupCharacterMap(cmap)
    fb.setupGlyf(glyphs)
    fb.setupHorizontalMetrics({n: (ADV * PX, 0) for n in names})
    fb.setupHorizontalHeader(ascent=ASC * PX, descent=-DESC * PX)
    fb.setupNameTable({"familyName": "Tessera Mono", "styleName": "Regular",
                       "copyright": "Copyright (C) 2026 Theo Dainis; GPL-3.0-only (firmware/src/mo_font.h)"})
    fb.setupOS2(sTypoAscender=ASC * PX, sTypoDescender=-DESC * PX, sTypoLineGap=0, usWinAscent=(ASC + 2) * PX,
                usWinDescent=DESC * PX, fsSelection=0x40 | 0x80, version=4)    # REGULAR, USE_TYPO_METRICS (v4+)
    fb.setupPost()
    fb.font["head"].created = fb.font["head"].modified = 3_844_800_000   # fixed: the same bytes every run
    fb.font.recalcTimestamp = False
    fb.font.flavor = "woff"
    out = io.BytesIO()
    fb.save(out)
    return out.getvalue()


def main(check=False):
    woff = build()
    face = ('@font-face { font-family: "Tessera Mono"; font-display: block; src: url("data:font/woff;base64,'
            + base64.b64encode(woff).decode() + '") format("woff"); }')
    stale = []
    for p in PAGES:
        path = ROOT / p
        s = path.read_text()
        new, n = MARK.subn(lambda m: m.group(1) + "\n" + face + "\n" + m.group(2), s)
        if n != 1:
            raise SystemExit(f"{p}: needs one /* TESSERA-MONO: ... */ ... /* /TESSERA-MONO */ block")
        if new != s:
            stale.append(p)
            if not check:
                path.write_text(new)
    if check and stale:
        raise SystemExit(f"Tessera Mono out of date in {', '.join(stale)}: run python3 tools/gen_mono_webfont.py")
    print(f"Tessera Mono: {len(woff)} B WOFF, {'checked' if check else 'inlined'} in {len(PAGES)} pages")


if __name__ == "__main__":
    main(check="--check" in sys.argv[1:])
