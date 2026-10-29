#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Tessera's brand pictures for GitHub and the project page, in the Mono look: one ink on black, the device's own
5 x 7 font (firmware/src/ui_mono.c MO_FONT) at whole-pixel scales:

  assets/brand/logo.svg            the mark: 3 x 3 tiles outlined in the ink, the top-right one solid (the splash's
                                   tessera that is laid last)
  assets/brand/banner.png          README header, 1280 x 400: the mark, TESSERA, the line, two screens
  assets/brand/social-preview.png  GitHub's social preview (Settings > General), 1280 x 640
  assets/brand/profile.png         a profile picture (the Firmware Hub, forums), 1080 x 1080: the mark, TESSERA, the
                                   line and the four tracks' fills, no screens and no version
  assets/brand/profile-wide.png    the same, 1280 x 640

  python3 tools/gen_brand.py [assets/gallery]   (the screens: tests/gallery_render.c's PNGs, a Mono build's)

The ink is Settings COLOR's default, YELLOW (tools/gen_ui_palettes.py); the four tracks are the Mono look's fills:
solid, hatched, dotted, striped."""
import sys
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gen_mono_webfont import mo_font                    # noqa: E402  (the device's 5 x 7 glyphs)

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "assets/brand"
BG, INK = (0, 0, 0), (255, 208, 32)
JIT = ((0, 1), (1, -1), (-1, 0), (1, 0), (0, 0), (-1, 1), (0, -1), (1, 1), (-1, -1))   # splash.c SPL_JIT
LIT = 2                                                 # the solid tessera: top right (splash.c SPL_GOLD)
TAGLINE = "GENERATIVE FIRMWARE FOR THE M-VAVE FM-1"
GLYPHS = mo_font()


def mix(a, b, p):
    return tuple(round(x + (y - x) * p / 100) for x, y in zip(a, b))


MID, LINE = mix(BG, INK, 78), mix(BG, INK, 23)


def text_w(text, px):
    return (len(text) * 6 - 1) * px


def text(d, x, y, s, px, fill=INK):
    """s in the 5 x 7 font, a font pixel px x px, a glyph 6 wide (lower case drawn as upper)"""
    for k, ch in enumerate(s.upper()):
        rows = GLYPHS.get(ord(ch), GLYPHS[ord("?")])
        for r, bits in enumerate(rows):
            for c in range(5):
                if bits & (16 >> c):
                    gx, gy = x + (k * 6 + c) * px, y + r * px
                    d.rectangle((gx, gy, gx + px - 1, gy + px - 1), fill=fill)


def centred(d, w, y, s, px, fill=INK):
    text(d, (w - text_w(s, px)) // 2, y, s, px, fill)


def mark(d, cx, cy, u):
    """the mosaic centred on cx, cy, tiles u px, grout u/5.5 (splash.c: 22 and 4), each a jitter off the grid; every
    tile an outline u/6 wide, the lit one solid"""
    g, j, t = round(u * 4 / 22), max(1, round(u / 22)), max(2, round(u / 6))
    for i in range(9):
        x = cx - u // 2 + (i % 3 - 1) * (u + g) + JIT[i][0] * j
        y = cy - u // 2 + (i // 3 - 1) * (u + g) + JIT[i][1] * j
        if i == LIT:
            d.rectangle((x, y, x + u - 1, y + u - 1), fill=INK)
        else:
            d.rectangle((x, y, x + u - 1, y + u - 1), outline=INK, width=t)


def logo_svg():
    u, g, t = 22, 4, 4
    rects = []
    for i in range(9):
        x = 2 + (i % 3) * (u + g) + JIT[i][0]
        y = 2 + (i // 3) * (u + g) + JIT[i][1]
        if i == LIT:
            rects.append(f'<rect x="{x}" y="{y}" width="{u}" height="{u}" fill="#ffd020"/>')
        else:
            rects.append(f'<rect x="{x + t // 2}" y="{y + t // 2}" width="{u - t}" height="{u - t}" fill="none" '
                         f'stroke="#ffd020" stroke-width="{t}"/>')
    size = 3 * u + 2 * g + 4
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {size} {size}" width="{size}" height="{size}">'
            f'<title>Tessera</title><rect width="{size}" height="{size}" fill="#000"/>{"".join(rects)}</svg>\n')


def screens(gallery, names, h):
    out = []
    for n in names:
        im = Image.open(gallery / f"{n}.png").convert("RGB")
        out.append(im.resize((h, h), Image.NEAREST if h % 240 == 0 else Image.LANCZOS))
    return out


def tile(img, shot, x, y, frame=6):
    d = ImageDraw.Draw(img)
    d.rectangle((x - frame, y - frame, x + shot.width + frame - 1, y + shot.height + frame - 1), outline=LINE, width=2)
    img.paste(shot, (x, y))


def tracks_bar(d, x, y, w, h):
    """the four tracks' fills, as the Mono look draws them: solid, hatched, dotted, striped"""
    seg = (w - 3 * h) // 4
    for k in range(4):
        x0 = x + k * (seg + h)
        for yy in range(y, y + h):
            for xx in range(x0, x0 + seg):
                on = (k == 0 or (k == 1 and (xx + yy) % 5 < 2) or (k == 2 and (xx - x0) % 4 < 2 and (yy - y) % 4 < 2)
                      or (k == 3 and (xx - x0) % 4 < 2))
                if on:
                    d.point((xx, yy), fill=INK)


def wordmark(d, x, y, px, sub):
    text(d, x, y, "TESSERA", px)
    text(d, x + px // 2, y + 7 * px + 3 * sub + 4, TAGLINE, sub, MID)


def banner(gallery):
    w, h = 1280, 400
    img = Image.new("RGB", (w, h), BG)
    d = ImageDraw.Draw(img)
    mark(d, 124, 200, 44)
    wordmark(d, 222, 130, 12, 2)
    tracks_bar(d, 224, 282, 466, 8)
    shots = screens(gallery, ("gen_constel", "gen_chord"), 240)
    for k, s in enumerate(shots):
        tile(img, s, 766 + k * 258, 80)
    return img


def social(gallery):
    w, h = 1280, 640
    img = Image.new("RGB", (w, h), BG)
    d = ImageDraw.Draw(img)
    mark(d, 152, 168, 50)
    text(d, 262, 104, "TESSERA", 14)
    text(d, 266, 218, TAGLINE, 4, MID)
    shots = screens(gallery, ("gen_euclid", "gen_grids", "gen_chord", "gen_turing"), 240)
    x0 = (w - (4 * 240 + 3 * 28)) // 2
    for k, s in enumerate(shots):
        tile(img, s, x0 + k * 268, 320)
    tracks_bar(d, x0, 596, 4 * 240 + 3 * 28, 8)
    return img


def profile(w, h):
    """the mark over TESSERA, the line and the tracks' fills, centred"""
    img = Image.new("RGB", (w, h), BG)
    d = ImageDraw.Draw(img)
    k = min(w, h) / 1080                              # (1080 square: the sizes below)
    u = round(92 * k)
    mark(d, w // 2, round(h * 0.30), u)
    name, line = max(4, round(24 * k)), max(2, round(4 * k))
    y = round(h * 0.30 + 1.9 * u)
    centred(d, w, y, "TESSERA", name)
    y += 7 * name + round(40 * k)
    centred(d, w, y, TAGLINE, line, MID)
    bw = text_w(TAGLINE, line)
    tracks_bar(d, (w - bw) // 2, y + 7 * line + round(48 * k), bw, max(6, round(10 * k)))
    box = ImageChops.difference(img, Image.new("RGB", (w, h), BG)).getbbox()   # then the whole group centred
    out = Image.new("RGB", (w, h), BG)
    out.paste(img.crop((0, box[1], w, box[3])), (0, (h - (box[3] - box[1])) // 2))
    return out


def main():
    gallery = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "assets/gallery"
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / "logo.svg").write_text(logo_svg(), encoding="utf-8")
    banner(gallery).save(OUT / "banner.png", optimize=True)
    social(gallery).save(OUT / "social-preview.png", optimize=True)
    profile(1080, 1080).save(OUT / "profile.png", optimize=True)
    profile(1280, 640).save(OUT / "profile-wide.png", optimize=True)
    print(f"brand: {OUT}: logo.svg, banner.png, social-preview.png, profile.png, profile-wide.png")


if __name__ == "__main__":
    main()
