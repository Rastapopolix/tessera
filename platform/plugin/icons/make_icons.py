#!/usr/bin/env python3
"""The standalone app's icons from the site's mark (assets/brand/logo.svg): tessera.ico (Windows: 16 to 256 px) and
tessera.icns (macOS: 16 to 1024 px). 64 px and up are drawn from the SVG; 16 to 48 px are drawn on the pixel grid
(PIXEL: the mark's 3 x 3 squares, the top right one filled, whole pixels), where the SVG would blur.
Needs rsvg-convert (librsvg) and Pillow. Run from anywhere: python3 platform/plugin/icons/make_icons.py"""
import io
import subprocess
from pathlib import Path

from PIL import Image

HERE = Path(__file__).resolve().parent
LOGO = HERE.parents[2] / "assets" / "brand" / "logo.svg"


# size: (margin, gap, square, stroke), with 2 margin + 2 gap + 3 square = size
PIXEL = {16: (1, 1, 4, 1), 24: (1, 2, 6, 1), 32: (2, 2, 8, 2), 48: (3, 3, 12, 3)}
INK = (0xFF, 0xD0, 0x20, 255)


def draw_pixel(px):
    m, g, c, t = PIXEL[px]
    im = Image.new("RGBA", (px, px), (0, 0, 0, 255))
    for row in range(3):
        for col in range(3):
            x, y = m + col * (c + g), m + row * (c + g)
            for yy in range(y, y + c):
                for xx in range(x, x + c):
                    edge = min(xx - x, yy - y, x + c - 1 - xx, y + c - 1 - yy) < t
                    if edge or (row == 0 and col == 2):
                        im.putpixel((xx, yy), INK)
    return im


def draw(px):
    if px in PIXEL:
        return draw_pixel(px)
    png = subprocess.run(["rsvg-convert", "-w", str(px), "-h", str(px), str(LOGO)], check=True,
                         capture_output=True).stdout
    return Image.open(io.BytesIO(png)).convert("RGBA")


def main():
    ico = [draw(px) for px in (256, 128, 64, 48, 32, 24, 16)]
    ico[0].save(HERE / "tessera.ico", format="ICO", sizes=[im.size for im in ico], append_images=ico[1:])
    big = draw(1024)
    big.save(HERE / "tessera.icns", format="ICNS",
             append_images=[draw(px) for px in (512, 256, 128, 64, 32, 16)])
    print("icons:", HERE / "tessera.ico", HERE / "tessera.icns")


if __name__ == "__main__":
    main()
