#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Theo Dainis (melodee-tess)
"""The 3.3 narrated tour's clips, 16:9, from the firmware's own audio and screens.

  build/host/demo_projects tour build/tour          (tests/demo_projects.c, built with -DMELODEE_LOOK=1)
  python3 tools/tour_video.py build/tour OUTDIR      (needs Pillow and imageio-ffmpeg, or ffmpeg on the PATH)

Each clip's 240 x 240 frames are drawn 4x, nearest, on a 1920 x 1080 card: the screen at the left, at the right the
chapter's name and the clip's label (a page and a knob, as the script's captions), in the device's 5 x 7 font and the
chapter's ink. OUTDIR gets clips/<clip>.mp4 (its own sound, as played), stills/<clip>.png (its first frame, to talk
over), timing.csv (each clip's length and when its label changes) and rough-a.mp4: every clip in order, each after
LEAD s of its still in silence, the room for the narration.
"""
import array
import csv
import math
import shutil
import subprocess
import sys
import wave
from fractions import Fraction
from pathlib import Path

from PIL import Image, ImageDraw

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gen_brand import mark, text   # noqa: E402  (the Mono look's font and mark)

FS, SPF = 44100, 1472                                   # samples a frame (demo_projects.c V_BLOCKS x 32)
FPS = Fraction(FS, SPF)
W, H, S = 1920, 1080, 4
SX, SY = 120, (H - 240 * S) // 2                        # the screen's place
PX = 1200                                               # the labels' column
EDGE = 0.012                                            # s: each clip's fade in and out (no click at a cut)
LEAD = 3.0                                              # s: the still before each clip in the rough cut
LEVEL_RMS, LEVEL_PEAK = -24.0, -3.0                     # dBFS: the scene clips (not a demo) levelled as the 3.0 video's
INK = {"YELLOW": (255, 208, 32), "GREEN": (96, 255, 128), "BLUE": (136, 192, 255), "ORANGE": (255, 144, 40),
       "MOSAIC": (255, 208, 32), "SUNSET": (255, 144, 40), "OCEAN": (136, 192, 255), "NEON": (0, 230, 255)}
CHAPTERS = {"a00": ("COLD OPEN", "OCEAN"), "a01": ("HELLO", "YELLOW"), "a02": ("FOUR TRACKS, REAL SOUNDS", "YELLOW"),
            "a03": ("GENERATORS WRITE THE NOTES", "GREEN"), "a04": ("SHAPING THE LINE", "BLUE"),
            "a05": ("TRACKS THAT LISTEN", "ORANGE"), "a06": ("MODULATE THE MODULATORS", "MOSAIC"),
            "a07": ("FROM A LOOP TO A PIECE", "SUNSET"), "a08": ("PLAYING IT", "NEON"),
            "a09": ("COLOUR, AND A PAGE TO WATCH", "OCEAN"), "a10": ("WHERE TO GET IT", "YELLOW")}
LEVELLED = ("a03e", "a03f")                             # (scenes of their own, not a demo: levelled to the demos)


def mix(a, b, p):
    return tuple(round(x + (y - x) * p / 100) for x, y in zip(a, b))


def wrap(s, width):
    lines, line = [], ""
    for word in s.split():
        t = (line + " " + word).strip()
        if len(t) <= width:
            line = t
        else:
            lines.append(line)
            line = word
    return lines + [line] if line else lines


def panel(chapter, label, ink):
    """the card without the screen: the mark and TESSERA 3.3, the chapter, the label"""
    im = Image.new("RGB", (W, H), (0, 0, 0))
    d = ImageDraw.Draw(im)
    dim = mix((0, 0, 0), ink, 60)
    mark(d, PX + 14, SY + 14, 7)
    text(d, PX + 40, SY + 2, "TESSERA 3.3", 4, dim)
    y = SY + 120
    for line in wrap(chapter, 22):
        text(d, PX, y, line, 4, dim)
        y += 40
    y += 40
    for line in wrap(label, 15)[:5]:
        text(d, PX, y, line, 6, ink)
        y += 60
    d.rectangle([SX - 6, SY - 6, SX + 240 * S + 5, SY + 240 * S + 5], outline=mix((0, 0, 0), ink, 23), width=2)
    return im


def clips(src):
    """(name, dir, wav, [(t0, t1, text)] in the clip's own time), in captions order"""
    rows = [l.split("\t") for l in (src / "captions.txt").read_text(encoding="utf-8").splitlines() if l.strip()]
    out = []
    for name, a, b, t in rows:
        if not out or out[-1][0] != name:
            out.append([name, float(a), []])
        out[-1][2].append((float(a) - out[-1][1], float(b) - out[-1][1], t))
    return [(n, src / n, (src / n).with_suffix(".wav"), caps) for n, _, caps in out]


def level(a):
    peak = max(abs(v) for v in a) or 1
    rms = math.sqrt(sum(v * v for v in a) / len(a)) or 1
    gain = min(10 ** (LEVEL_RMS / 20) * 32768 / rms, 10 ** (LEVEL_PEAK / 20) * 32768 / peak)
    return array.array("h", (max(-32768, min(32767, round(v * gain))) for v in a))


def edges(a):
    n, k = len(a) // 2, int(EDGE * FS)
    for i in range(min(k, n)):
        g = i / k
        for j in (2 * i, 2 * i + 1, 2 * (n - 1 - i), 2 * (n - 1 - i) + 1):
            a[j] = int(a[j] * g)
    return a


def ffmpeg():
    exe = shutil.which("ffmpeg")
    if exe:
        return exe
    import imageio_ffmpeg
    return imageio_ffmpeg.get_ffmpeg_exe()


def encode(frames, wav, out):
    """frames: an iterable of W x H images at FPS; wav: its sound"""
    cmd = [ffmpeg(), "-y", "-loglevel", "error", "-f", "rawvideo", "-pix_fmt", "rgb24", "-s", f"{W}x{H}",
           "-framerate", f"{FPS.numerator}/{FPS.denominator}", "-i", "-", "-i", str(wav),
           "-c:v", "libx264", "-preset", "medium", "-crf", "18", "-pix_fmt", "yuv420p", "-r", "30",
           "-c:a", "aac", "-b:a", "192k", "-ar", "48000", "-movflags", "+faststart", str(out)]
    p = subprocess.Popen(cmd, stdin=subprocess.PIPE)
    for im in frames:
        p.stdin.write(im.tobytes())
    p.stdin.close()
    if p.wait():
        raise SystemExit(f"ffmpeg failed on {out}")


def write_wav(path, a):
    with wave.open(str(path), "wb") as w:
        w.setnchannels(2); w.setsampwidth(2); w.setframerate(FS)
        w.writeframes(a.tobytes())


def main(src, dst):
    src, dst = Path(src), Path(dst)
    for sub in ("clips", "stills", "work"):
        (dst / sub).mkdir(parents=True, exist_ok=True)
    rows, parts = [], []
    for name, d, w, caps in clips(src):
        chapter, ink_name = CHAPTERS[name[:3]]
        ink = INK[ink_name]
        ppm = sorted(d.glob("f*.ppm"))
        with wave.open(str(w)) as r:
            a = array.array("h", r.readframes(r.getnframes()))
        if name.startswith(LEVELLED):
            a = level(a)
        a.extend([0] * max(0, len(ppm) * SPF * 2 - len(a)))
        a = edges(a)
        cards = {t: panel(chapter, t, ink) for _, _, t in caps}

        def frames():
            for i, f in enumerate(ppm):
                t = i / float(FPS)
                cap = next((c for t0, t1, c in caps if t0 <= t < t1), caps[-1][2])
                im = cards[cap].copy()
                im.paste(Image.open(f).convert("RGB").resize((240 * S, 240 * S), Image.NEAREST), (SX, SY))
                yield im

        wav = dst / "work" / f"{name}.wav"
        write_wav(wav, a)
        encode(frames(), wav, dst / "clips" / f"{name}.mp4")
        still = next(frames())
        still.save(dst / "stills" / f"{name}.png")
        # the still in silence, LEAD s: the rough cut's room for the voice
        n = round(LEAD * float(FPS))
        quiet = dst / "work" / f"{name}-lead.wav"
        write_wav(quiet, array.array("h", [0] * (n * SPF * 2)))
        encode((still for _ in range(n)), quiet, dst / "work" / f"{name}-lead.mp4")
        parts += [dst / "work" / f"{name}-lead.mp4", dst / "clips" / f"{name}.mp4"]
        secs = len(ppm) / float(FPS)
        rows.append([name, chapter, f"{secs:.1f}", " | ".join(f"{t0:.1f}s {t}" for t0, _, t in caps)])
        print(f"{name:24s} {secs:5.1f} s  {ink_name}")
    with open(dst / "timing.csv", "w", newline="") as f:
        c = csv.writer(f)
        c.writerow(["clip", "chapter", "seconds", "labels (from the clip's start)"])
        c.writerows(rows)
    lst = dst / "work" / "rough.txt"
    lst.write_text("".join(f"file '{p.resolve()}'\n" for p in parts))
    subprocess.run([ffmpeg(), "-y", "-loglevel", "error", "-f", "concat", "-safe", "0", "-i", str(lst), "-c", "copy",
                    str(dst / "rough-a.mp4")], check=True)
    total = sum(float(r[2]) for r in rows) + LEAD * len(rows)
    print(f"{dst / 'rough-a.mp4'}: {len(rows)} clips, {total / 60:.1f} min with the leads")


if __name__ == "__main__":
    main(*sys.argv[1:3])
