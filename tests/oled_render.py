#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Theo Dainis
"""PPM -> PNG for Andamento's screen renders (tests/oled_render.c, Pillow). Under OUTDIR (default build/oled):
  <screen>.png      128 x 64, true size, as the panel shows it
  <screen>_x4.png   four times larger, each pixel a square with a thin dark gap, closer to how the OLED looks
"""
import sys
from pathlib import Path

from PIL import Image

out = Path(sys.argv[1] if len(sys.argv) > 1 else "build/oled")
n = 0
for f in sorted((out / "ppm").glob("*.ppm")):
    img = Image.open(f).convert("RGB")
    img.save(out / f"{f.stem}.png")
    w, h = img.size
    big = Image.new("RGB", (w * 4, h * 4), (0, 0, 0))
    px, bp = img.load(), big.load()
    for y in range(h):
        for x in range(w):
            c = px[x, y]
            for j in range(3):
                for i in range(3):
                    bp[x * 4 + i, y * 4 + j] = c
    big.save(out / f"{f.stem}_x4.png")
    n += 1
print(f"oled_render: {n} screens in {out}/<screen>.png and <screen>_x4.png")
