#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Theo Dainis (melodee-tess)
"""Tessera Mono, the web pages' pixel face: the Mono look's own 5 x 7 font (firmware/src/gfx.c MO_FONT) as a
WOFF, so the site, the installer, the editor and the simulator page letter as the device does.

  python3 tools/gen_mono_webfont.py          (needs fonttools)
  python3 tools/gen_mono_webfont.py --check  (tests/run_tests.sh: fails when a page's face is not MO_FONT's)

A font pixel is 100 units, a glyph 6 pixels wide (5 and one clear), 7 above the baseline and 1 below: 800 units an
em, so the face is sharp at 8, 16, 24 ... px. Lower case is drawn as upper (as on the device); a few signs the pages
use and the device has no need of (| { } ~ ` and the middle dot, dashes, ellipsis, times) are added here. The WOFF
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
SRC = ROOT / "firmware/src/gfx.c"
PAGES = ("web/home/index.html", "web/index_pkg.html", "web/editor.html", "web/sim/index.html")
PX, ADV, ASC, DESC = 100, 6, 7, 1
EXTRA = {                                               # code point: 7 rows, bit 4 the left column (as MO_FONT)
    0x60: (8, 4, 0, 0, 0, 0, 0), 0x7B: (2, 4, 4, 8, 4, 4, 2), 0x7C: (4, 4, 4, 4, 4, 4, 4),
    0x7D: (8, 4, 4, 2, 4, 4, 8), 0x7E: (0, 0, 8, 21, 2, 0, 0), 0xB7: (0, 0, 0, 4, 0, 0, 0),
    0xD7: (0, 17, 10, 4, 10, 17, 0), 0x2013: (0, 0, 0, 14, 0, 0, 0), 0x2014: (0, 0, 0, 31, 0, 0, 0),
    0x2026: (0, 0, 0, 0, 0, 0, 21), 0x2019: (12, 4, 8, 0, 0, 0, 0), 0x2018: (4, 8, 12, 0, 0, 0, 0),
    0x201C: (10, 20, 30, 0, 0, 0, 0), 0x201D: (30, 10, 20, 0, 0, 0, 0),
}
MARK = re.compile(r"(/\* TESSERA-MONO:[^*]*\*/).*?(/\* /TESSERA-MONO \*/)", re.S)


def mo_font():
    """MO_FONT's first 64 glyphs, ' ' .. '_' (the pages set in capitals; its small letters are a chord name's)"""
    body = re.search(r"MO_FONT\[\d+\]\[7\] = \{(.*?)\n\};", SRC.read_text(), re.S).group(1)
    rows = [tuple(int(v) for v in m.split(",")) for m in re.findall(r"\{([\d,\s]+)\}", body)][:64]
    assert len(rows) == 64 and all(len(r) == 7 for r in rows), "MO_FONT: 64 glyphs of 7 rows"
    return {0x20 + i: r for i, r in enumerate(rows)}


def glyph(rows):
    """the rows' pixels as squares, each row's runs merged (no overlapping contours)"""
    pen = TTGlyphPen(None)
    for r, bits in enumerate(rows):
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
                       "copyright": "Copyright (C) 2026 Theo Dainis; GPL-3.0-only (firmware/src/ui_mono.c)"})
    fb.setupOS2(sTypoAscender=ASC * PX, sTypoDescender=-DESC * PX, usWinAscent=ASC * PX, usWinDescent=DESC * PX)
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
