#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Theo Dainis (melodee-tess)
"""The manual's guided clips, 16:9, from the firmware's own audio and screens, with a caption before every action.

  build/host/demo_projects guide build/guide        (tests/demo_projects.c, built with -DMELODEE_LOOK=1)
  python3 tools/guide_video.py build/guide OUTDIR    (needs Pillow and imageio-ffmpeg, or ffmpeg on the PATH)

OUTDIR gets <clip>.mp4 and <clip>.srt for each clip in build/guide/guide.tsv. A frame: the screen at the left, drawn
3x, nearest; at the right a drawing of the FM-1's panel, quiet but for the control about to be used; across the
bottom the caption, large, white on black. One thing asks for the eye at a time: the caption, then the lit control,
then the screen, whose frame lights for a moment as the action lands. Each caption comes up 3 s (guide.tsv)
before its action: the control is outlined and pulses while it is next, solid while it is pressed or held.
The .srt has the same captions at the same times. The last TAIL s are the end card (the mark, TESSERA and the
version of the newest docs/RELEASE_*.md, the clip's title, the project page) while the music fades out.
"""
import array
import math
import shutil
import subprocess
import sys
import wave
from fractions import Fraction
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gen_brand import INK, centred, mark, text as mono, text_w   # noqa: E402  (the Mono look's font and mark)

ROOT = Path(__file__).resolve().parents[1]
FS, SPF = 44100, 1472                                   # samples a frame (demo_projects.c V_BLOCKS x 32)
FPS = Fraction(FS, SPF)
W, H = 1920, 1080
BG, WHITE, HOT = (0, 0, 0), (255, 255, 255), (255, 208, 32)   # HOT: a control to use, always yellow (Theo, 2026-11-18)
ACC = HOT                                               # the clip's topic accent: the step count, the rule, the flash
ACCENT = {"YELLOW": (255, 208, 32), "WHITE": (240, 240, 240), "GREEN": (96, 255, 128), "BLUE": (136, 192, 255),
          "RED": (255, 112, 100), "ORANGE": (255, 144, 40), "PINK": (255, 176, 216), "HOT PINK": (255, 88, 176),
          "LILAC": (192, 150, 255), "MOSAIC": (240, 240, 240), "SUNSET": (240, 240, 240), "OCEAN": (240, 240, 240),
          "NEON": (240, 240, 240)}                      # (tools/gen_ui_palettes.py's inks; a four-ink set's accent is
                                                        # white)
DIM, DIMTXT = (78, 78, 78), (150, 150, 150)             # the controls not in use
EDGE = 0.012                                            # s: fade in
TAIL = 3.0                                              # s: the end card, the music fading out under it
CUT = 0.4                                               # s: the clip fades into the end card
TAP = 0.5                                               # s: a tap is drawn pressed this long at least
HOLD = 1.5                                              # s: the shortest a caption shows
FLASH = 1.0                                             # s: the screen's frame lights when an action lands
RANK = {"done": 0, "next": 1, "down": 2}
LEVEL_RMS, LEVEL_PEAK = -22.0, -3.0                     # dBFS: the clip's RMS, its highest peak
FONT = ROOT / "assets/fonts/InterTight[wght].ttf"


def font(px, weight):
    f = ImageFont.truetype(str(FONT), px)
    f.set_variation_by_axes([weight])
    return f


F_CAP, F_LABEL, F_SMALL = font(72, 760), font(48, 700), font(32, 600)

# ---------------------------------------------------------------------------------------------- the panel
# The FM-1's controls in a 1000 x 560 drawing (the layout of docs/panel.jpg): (kind, box or centre, printed name)
PANEL_W, PANEL_H = 1000, 560
WHITES = "F3 G3 A3 B3 C4 D4 E4 F4 G4 A4 B4 C5 D5 E5 F5 G5".split()
CTRL = {}
for name, x, y in (("MASTER", 70, 82), ("SELECT", 190, 82), ("PRESETS", 70, 192), ("ALGORITHM", 190, 192),
                   ("KNOB 1", 575, 82), ("KNOB 2", 690, 82), ("KNOB 3", 805, 82), ("KNOB 4", 920, 82)):
    CTRL[name] = ("knob", (x, y), name)
CTRL["OCT-"] = ("btn", (40, 258, 110, 290), "OCT-")
CTRL["OCT+"] = ("btn", (150, 258, 220, 290), "OCT+")
for row, y, names in ((0, 152, "FX SEL ENV LFO EDIT GLO"), (1, 222, "HOME SAVE ARP SEQ PLAY REC")):
    for i, n in enumerate(names.split()):
        x = 552 + i * 70
        CTRL[n] = ("btn", (x, y, x + 58, y + 52), n)
SLOT = 950 / len(WHITES)
for i, n in enumerate(WHITES):
    cx = 25 + SLOT * (i + 0.5)
    CTRL[n] = ("white", (cx - 22, 432, cx + 22, 538), n)
    if n[0] in "FGACD" and i + 1 < len(WHITES):
        bx = 25 + SLOT * (i + 1)
        CTRL[n[0] + "#" + n[1:]] = ("black", (bx - 20, 326, bx + 20, 418), n[0] + "#" + n[1:])


def draw_panel(im, ox, oy, states, screen, t):
    """the panel at (ox, oy); states: control -> 'next' or 'down'; screen: the frame, shown small in the panel"""
    d = ImageDraw.Draw(im)
    d.rounded_rectangle((ox, oy, ox + PANEL_W, oy + PANEL_H), 28, outline=DIM, width=3)
    d.rounded_rectangle((ox + 535, oy + 136, ox + 977, oy + 290), 12, outline=DIM, width=2)
    d.rounded_rectangle((ox + 22, oy + 312, ox + 978, oy + 548), 16, outline=DIM, width=2)
    d.rounded_rectangle((ox + 262, oy + 22, ox + 488, oy + 248), 14, outline=DIM, width=3)
    mono(d, ox + 375 - text_w("SCREEN", 3) // 2, oy + 125, "SCREEN", 3, DIMTXT)   # (not a second copy of it: the eye
                                                    # would have two moving screens to choose between)
    pulse = 0.5 + 0.5 * math.sin(t * 2 * math.pi * 1.6)
    marks = []
    for name, (kind, g, label) in CTRL.items():
        st = states.get(name)
        line = HOT if st else DIM
        width = (4 + round(4 * pulse)) if st == "next" else (3 if st else 2)
        fill = HOT if st == "down" else None
        width = 4 if st == "done" else width
        if kind == "knob":
            x, y = ox + g[0], oy + g[1]
            r = 26
            d.ellipse((x - r, y - r, x + r, y + r), outline=line, width=width, fill=fill)
            d.line((x, y, x + 12, y - 14), fill=BG if st == "down" else line, width=3)
            mono(d, x - text_w(label, 2) // 2, y - r - 22, label, 2, HOT if st else DIMTXT)
            top = (x, y - r - 26)
        else:
            x0, y0, x1, y1 = ox + g[0], oy + g[1], ox + g[2], oy + g[3]
            rad = 20 if kind in ("white", "black") else 10
            if st == "next":                            # (a glow ring outside the control)
                d.rounded_rectangle((x0 - 8, y0 - 8, x1 + 8, y1 + 8), rad + 8, outline=HOT, width=max(2, width - 3))
            d.rounded_rectangle((x0, y0, x1, y1), rad, outline=line, width=width, fill=fill)
            if kind == "btn":
                px = 2
                mono(d, (x0 + x1) // 2 - text_w(label, px) // 2, (y0 + y1) // 2 - 7, label,
                     px, BG if st == "down" else (HOT if st else DIMTXT))
            elif st:                                    # (a key in use: its name on it)
                s = label.replace("#", "#")
                px = 2 if len(s) > 2 else 3
                mono(d, (x0 + x1) // 2 - text_w(s, px) // 2 + 1, y1 - 34, s, px, BG if st == "down" else HOT)
            top = ((x0 + x1) // 2, y0 - 6)
        if st in ("next", "down"):
            marks.append(top)
    for x, y in marks:                                  # (an arrow down onto each control in use)
        dy = -round(6 * pulse)
        d.polygon([(x - 15, y - 20 + dy), (x + 15, y - 20 + dy), (x, y + dy)], fill=HOT)


# ---------------------------------------------------------------------------------------------- captions
def wrap_px(d, words, f, width):
    lines, line = [], []
    for w in words:
        if line and d.textlength(" ".join(line + [w]), font=f) > width:
            lines.append(line)
            line = [w]
        else:
            line.append(w)
    return lines + [line] if line else lines


def draw_caption(im, text, controls, step, steps):
    d = ImageDraw.Draw(im)
    y0 = 808
    d.rectangle((0, y0, W, H), fill=BG)
    d.line((60, y0, W - 60, y0), fill=ACC, width=4)
    if step:
        d.text((60, y0 + 22), f"STEP {step} OF {steps}", font=F_SMALL, fill=ACC)
    y = y0 + (64 if step else 44)
    for line in wrap_px(d, text.split(), F_CAP, W - 220)[:2]:
        x = 60
        spans = {}
        for k in range(len(line)):                      # (a control's name, one word or two: KNOB 2)
            for n in (2, 1):
                if k not in spans and " ".join(line[k:k + n]).rstrip(",.:;") in controls:
                    spans[k] = n
                    break
        trk = {}                                        # (yellow means a control: tracks stay white in captions;
                                                        # the screen shows their colours)
        k = 0
        while k < len(line):
            if k in spans:                              # (a control: a keycap, its punctuation after it)
                word = " ".join(line[k:k + spans[k]])
                name = word.rstrip(",.:;")
                w = d.textlength(name, font=F_CAP)
                d.rounded_rectangle((x - 2, y + 2, x + w + 22, y + 84), 12, outline=HOT, width=4)
                d.text((x + 10, y), name, font=F_CAP, fill=HOT)
                x += w + 22
                d.text((x, y), word[len(name):], font=F_CAP, fill=WHITE)
                x += d.textlength(word[len(name):] + " ", font=F_CAP) + 4
                k += spans[k]
                continue
            d.text((x, y), line[k], font=F_CAP, fill=trk.get(k, WHITE))
            x += d.textlength(line[k] + " ", font=F_CAP)
            k += 1
        y += 90


# ---------------------------------------------------------------------------------------------- the end card
def version():
    """the newest docs/RELEASE_X.Y[.Z].md: the version on the end card"""
    vs = [tuple(int(n) for n in f.stem[8:].split(".")) for f in (ROOT / "docs").glob("RELEASE_*.md")
          if all(n.isdigit() for n in f.stem[8:].split("."))]
    return ".".join(map(str, max(vs))) if vs else ""


def end_card(title):
    """the mark large, TESSERA and the version in the brand's ink, the clip's title, where to get it"""
    im = Image.new("RGB", (W, H), BG)
    d = ImageDraw.Draw(im)
    mark(d, W // 2, 330, 56)
    centred(d, W, 500, f"TESSERA {version()}".strip(), 12, INK)
    d.text((W // 2, 660), title, font=F_LABEL, fill=WHITE, anchor="mm")
    d.text((W // 2, 760), "rastapopolix.github.io/tessera", font=F_LABEL, fill=(200, 200, 200), anchor="mm")
    d.text((W // 2, 810), "Free, GPL-3.0. Try it in the browser.", font=F_SMALL, fill=(200, 200, 200),
           anchor="mm")
    return im


# ---------------------------------------------------------------------------------------------- the clip
def state(r, t, tap):
    """a control's state at t: next (its caption is up), down (pressed, at least tap s), done (until the caption goes)"""
    end = max(r["t1"], r["t0"] + tap)
    if r["end"] - end < tap:                            # (no "done" too short to read: down until the caption goes)
        end = max(end, r["end"])
    return ("down" if r["t0"] <= t < end else
            "next" if r["cap"] <= t < r["t0"] else "done" if r["t0"] <= t < r["end"] else None)


def check(clip, rows, caps, secs):
    """warn of a caption on screen shorter than HOLD"""
    bad = [f"caption {c[2][:30]!r} {c[1] - c[0]:.1f} s" for c in caps if c[1] - c[0] < HOLD]
    for b in bad:
        print(f"guide_video: {clip}: shown too briefly: {b}")

def rows_of(src):
    """{clip: (title, rows)}"""
    out, titles = {}, {}
    for line in (src / "guide.tsv").read_text(encoding="utf-8").splitlines():
        if not line.strip():
            continue
        clip, cap, a, b, ctl, label, caption = line.split("\t")[:7]
        if float(cap) == -2:
            titles[clip] = (caption, label)
            continue
        out.setdefault(clip, []).append({"cap": float(cap), "t0": float(a), "t1": float(b),
                                         "ctl": None if ctl == "-" else ctl, "label": label,
                                         "caption": None if caption == "-" else caption})
    for rows in out.values():                           # (a row with no caption of its own goes on with the one before)
        for i, r in enumerate(rows):
            if r["caption"] is None:
                r["cap"], r["caption"] = rows[i - 1]["cap"], rows[i - 1]["caption"]
    return {clip: (titles.get(clip, (clip.upper(), "YELLOW")), rows) for clip, rows in out.items()}


def captions(rows, secs):
    """[(t0, t1, text, step)]: one per caption, step counting the captions that lead to an action"""
    caps, step = [], 0
    for r in rows:
        if caps and caps[-1][2] == r["caption"] and caps[-1][0] == r["cap"]:
            continue
        if r["ctl"]:
            step += 1
        caps.append([r["cap"], None, r["caption"], step if r["ctl"] else 0])
    for i, c in enumerate(caps):
        c[1] = caps[i + 1][0] if i + 1 < len(caps) else secs
    return caps, step


def srt_time(s):
    ms = round(s * 1000)
    return f"{ms // 3600000:02d}:{ms // 60000 % 60:02d}:{ms // 1000 % 60:02d},{ms % 1000:03d}"


def level(a):
    peak = max(abs(v) for v in a) or 1
    rms = math.sqrt(sum(v * v for v in a) / len(a)) or 1
    gain = min(10 ** (LEVEL_RMS / 20) * 32768 / rms, 10 ** (LEVEL_PEAK / 20) * 32768 / peak)
    return array.array("h", (max(-32768, min(32767, round(v * gain))) for v in a)), 20 * math.log10(gain)


def fades(a):
    n = len(a) // 2
    k_in, k_out = int(EDGE * FS), int(TAIL * FS)
    for i in range(n):
        g = min(1.0, i / k_in, (n - 1 - i) / k_out)
        if g < 1.0:
            a[2 * i] = int(a[2 * i] * g)
            a[2 * i + 1] = int(a[2 * i + 1] * g)
    return a


def ffmpeg():
    exe = shutil.which("ffmpeg")
    if exe:
        return exe
    import imageio_ffmpeg
    return imageio_ffmpeg.get_ffmpeg_exe()


def make(src, clip, title, rows, dst):
    frames_dir = src / clip
    frames = len(list(frames_dir.glob("f*.ppm")))
    secs = frames * SPF / FS
    caps, steps = captions(rows, secs - TAIL)
    controls = {r["ctl"] for r in rows if r["ctl"]}
    for r in rows:                                      # (a control stays lit, done, until its caption goes)
        r["end"] = next(c[1] for c in caps if c[0] == r["cap"])
    check(clip, rows, caps, secs - TAIL)

    with wave.open(str(src / f"{clip}.wav")) as r:
        a = array.array("h", r.readframes(r.getnframes()))
    a, gain = level(a)
    need = frames * SPF * 2
    a = a[:need] + array.array("h", [0] * max(0, need - len(a)))
    wav = dst / f"{clip}.work.wav"
    with wave.open(str(wav), "wb") as w:
        w.setnchannels(2); w.setsampwidth(2); w.setframerate(FS)
        w.writeframes(fades(a).tobytes())

    srt = dst / f"{clip}.srt"
    srt.write_text("".join(f"{i + 1}\n{srt_time(c[0])} --> {srt_time(c[1] - 0.04)}\n{c[2]}\n\n"
                           for i, c in enumerate(caps)), encoding="utf-8")

    base = Image.new("RGB", (W, H), BG)
    bd = ImageDraw.Draw(base)
    bd.rectangle((56, 46, 56 + 728, 46 + 728), outline=DIM, width=2)   # (the screen's frame)
    mark(bd, 878, 66, 12)
    mono(bd, 914, 52, "TESSERA", 4)
    mono(bd, 1100, 52, f"GUIDE: {title}", 4, WHITE)

    mp4 = dst / f"{clip}.mp4"
    cmd = [ffmpeg(), "-y", "-loglevel", "error", "-f", "rawvideo", "-pix_fmt", "rgb24", "-s", f"{W}x{H}",
           "-framerate", f"{FPS.numerator}/{FPS.denominator}", "-i", "-", "-i", str(wav),
           "-c:v", "libx264", "-preset", "medium", "-crf", "18", "-pix_fmt", "yuv420p", "-tune", "animation",
           "-c:a", "aac", "-b:a", "192k", "-movflags", "+faststart", str(mp4)]
    stills = [int(s) for s in __import__("os").environ.get("GUIDE_STILLS", "").split(",") if s]
    p = None if stills else subprocess.Popen(cmd, stdin=subprocess.PIPE)
    card = end_card(title)
    for k in ([k for k in stills if k < frames] if stills else range(frames)):
        t = k * SPF / FS
        if t >= secs - TAIL + CUT:                     # (the end card, then nothing else)
            if stills:
                card.save(dst / f"{clip}-{k:05d}.png")
            else:
                p.stdin.write(card.tobytes())
            continue
        scr = Image.open(frames_dir / f"f{k:05d}.ppm").convert("RGB")
        im = base.copy()
        im.paste(scr.resize((720, 720), Image.NEAREST), (60, 50))
        states = {}
        for r in rows:
            if not r["ctl"]:
                continue
            st = state(r, t, TAP)
            st = None if st == "done" else st             # (pressed and let go: the panel goes quiet)
            if st and (RANK[st] > RANK.get(states.get(r["ctl"]), -1)):
                states[r["ctl"]] = st
        flash = min((t - r["t0"] for r in rows if r["ctl"] and 0 <= t - r["t0"] < FLASH), default=None)
        draw_panel(im, 860, 120, states, scr, t)
        if flash is not None:                           # (the action lands: the screen's frame lights, then fades)
            a = 1 - flash / FLASH
            ImageDraw.Draw(im).rectangle((52, 42, 788, 778), outline=tuple(round(c * a) for c in ACC), width=6)
        c = next((c for c in caps if c[0] <= t < c[1]), caps[-1])
        draw_caption(im, c[2], controls, c[3], steps)
        if t >= secs - TAIL:                            # (the clip fades into the end card)
            im = Image.blend(im, card, (t - (secs - TAIL)) / CUT)
        if stills:                                      # (GUIDE_STILLS=k,..: those frames as PNGs, no video)
            im.save(dst / f"{clip}-{k:05d}.png")
            continue
        p.stdin.write(im.tobytes())
    if stills:
        wav.unlink()
        return
    p.stdin.close()
    if p.wait():
        raise SystemExit("ffmpeg failed")
    wav.unlink()
    print(f"guide_video: {mp4} ({frames} frames, {secs:.1f} s, gain {gain:+.1f} dB), {srt} ({len(caps)} captions)")


def main(src, dst):
    src, dst = Path(src), Path(dst)
    dst.mkdir(parents=True, exist_ok=True)
    global ACC
    for clip, ((title, ink), rows) in rows_of(src).items():
        ACC = ACCENT.get(ink, ACCENT["YELLOW"])
        make(src, clip, title, rows, dst)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2])
