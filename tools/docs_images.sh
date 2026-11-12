#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# The pictures in README.md, MANUAL.md, RECIPES.md and the project page (assets/gallery/<name>.png): the real drawing
# code, rendered on the PC by tests/gallery_render.c in the device build's engine set and screen reader.
#
#   tools/docs_images.sh            render, and write the PNGs into assets/gallery
#   tools/docs_images.sh --check    render, and list the PNGs that no longer match (exit 1 if any); writes nothing
#
# A PNG is kept in assets/gallery when it is there already or a page links it, so a new picture is: a SHOTS line in
# tests/gallery_render.c, a link in the page, then this. ABOUT (about.png) shows the version and the build date, so
# the check skips it. Needs a C compiler (cc, or CC=...) and Python 3 with Pillow.
set -e
cd "$(dirname "$0")/.."
CHECK=0
[ "$1" = "--check" ] && CHECK=1
CC=${CC:-cc}
OUT=build/gallery
VER=$(ls docs/RELEASE_*.md | sed 's/.*RELEASE_//; s/\.md$//' | sort -V | tail -1)   # (the newest release's notes)

python3 -c "import sys; sys.path.insert(0, 'tools'); import build; build.generate()" >/dev/null
[ -f build/gen/x0x_drum_tables.h ] || python3 tools/gen_909_tables.py build/gen/x0x_drum_tables.h
rm -rf "$OUT" && mkdir -p "$OUT/ppm" "$OUT/png"
$CC -O1 -w -DMELODEE_LOOK=1 -DMELODEE_ACCESS=1 -DMELODEE_USB_AUDIO=1 "-DMELODEE_VERSION=\"v$VER\"" \
    -Ibuild/gen -Ifirmware/src -Itests -o build/gallery_render tests/gallery_render.c -lm
build/gallery_render "$OUT" >/dev/null
python3 tools/gallery_png.py "$OUT/ppm" "$OUT/png" >/dev/null

CHECK=$CHECK python3 - <<'EOF'
import os, re, shutil, sys
from pathlib import Path
from PIL import Image, ImageChops
new, gal = Path("build/gallery/png"), Path("assets/gallery")
linked = set()
for page in list(Path(".").glob("*.md")) + list(Path("web/home").glob("*.html")):
    linked |= set(re.findall(r"assets/gallery/([\w-]+)\.png", page.read_text(encoding="utf-8")))
keep = sorted(linked | {p.stem for p in gal.glob("*.png")})
missing = [n for n in keep if not (new / f"{n}.png").exists()]
stale, wrote = [], 0
for n in keep:
    src, dst = new / f"{n}.png", gal / f"{n}.png"
    if not src.exists():
        continue
    if dst.exists():
        a, b = Image.open(src).convert("RGB"), Image.open(dst).convert("RGB")
        if a.size == b.size and not ImageChops.difference(a, b).getbbox():
            continue
    if os.environ["CHECK"] == "1":
        if n != "about":
            stale.append(n + ("" if dst.exists() else " (new)"))
    else:
        shutil.copyfile(src, dst)
        wrote += 1
if missing:
    print("docs_images: linked or kept, but no SHOTS line in tests/gallery_render.c:", " ".join(missing))
if os.environ["CHECK"] == "1":
    if stale:
        print("docs_images: these pictures no longer match the screens:", " ".join(stale))
        print("Run tools/docs_images.sh and commit assets/gallery.")
    else:
        print(f"docs_images: all {len(keep)} pictures match the screens")
    sys.exit(1 if stale or missing else 0)
print(f"docs_images: {wrote} of {len(keep)} pictures updated in assets/gallery")
sys.exit(1 if missing else 0)
EOF
