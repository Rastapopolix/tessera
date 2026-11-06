#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Theo Dainis (melodee-tess)
"""The 3.3 narrated tour's clips, 16:9, from the firmware's own audio and screens.

  build/host/demo_projects tour build/tour          (tests/demo_projects.c, built with -DMELODEE_LOOK=1)
  python3 tools/tour_video.py build/tour OUTDIR      (needs Pillow and imageio-ffmpeg, or ffmpeg on the PATH)
  python3 tools/tour_video.py demo build/tour INTRO OUT.mp4
                                                     (the captioned demo for the project page, 1080 x 1080, music only;
                                                      INTRO: the intro's frames, tessera-notes docs/video/3.3/intro_render.c)

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
LEVELLED = ("a03e", "a03f", "b03f", "b06a", "b06b")                           # (scenes of their own, not a demo: levelled to the demos)


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
        chapter, ink_name = CHAPTERS["a" + name[1:3]]
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


# The demo (no voice): 1080 x 1080 as the 3.0 video, music only, a short caption at a time under the screen, a card
# between chapters. Each entry: a card (title, line) or a part of a clip (clip, from s, for s, [(at s, caption)])
CUT = [
    ("b00-undertow-open", 0, 14, [(0, "Play it by hand, or let it play")]),
    ("Generators", "Thirteen ways to write a part"),
    ("a03b-turing-lock", 2, 12, [(0, "TURING: a loop that slowly drifts")]),
    ("a03c-euclid-pulses", 2, 9, [(0, "EUCLID: hits spread evenly")]),
    ("a03d-markov-then-all", 0, 16, [(0, "MARKOV: a wandering flute"), (8, "Together, one piece")]),
    ("b03f-constellation", 0, 10, [(0, "CONSTELLATION: stars play as the ship passes")]),
    ("Shaping", "How the notes are played"),
    ("a04c-split", 3, 12, [(0, "New: SPLIT breaks a step in two")]),
    ("a04d-adorn", 0, 8, [(0, "ADORN: drum ornaments, new each round")]),
    ("Tracks that listen", "Parts that answer and follow"),
    ("a05a-call-answer", 8, 18, [(0, "A melody you write: the call"), (6, "ANSWER replies, changed each time")]),
    ("a05c-follow", 4, 10, [(0, "A bass on its own note"), (3, "Now it follows the melody")]),
    ("Modulate the modulators", "Movement, and hands on"),
    ("b06a-lfo-on-lfo", 0, 16, [(0, "An LFO sweeps the filter"), (7, "New: a second mod swells the first")]),
    ("b06b-perf", 1, 10, [(0, "New: PERF macros, one knob, many moves")]),
    ("b06c-stone-pier", 0, 10, [(0, "STONE PIER: tides move the dub echo")]),
    ("From a loop to a piece", "New in 3.3: the long form"),
    ("b07a-chain-key", 2, 14, [(0, "CHAIN: sections change together"), (6, "B, a fourth up")]),
    ("b07b-kites-entries", 0, 14, [(0, "ENTRIES: parts come and go")]),
    ("b07c-kites-piece-0", 1, 2.5, [(0, "PIECE: a shape for the whole piece")]),
    ("b07c-kites-piece-1", 1, 2.5, [(0, "PIECE: a shape for the whole piece")]),
    ("b07c-kites-piece-2", 1, 2.5, [(0, "PIECE: a shape for the whole piece")]),
    ("b07c-kites-piece-3", 1, 2.5, [(0, "PIECE: a shape for the whole piece")]),
    ("b07c-kites-piece-4", 1, 2.5, [(0, "PIECE: a shape for the whole piece")]),
    ("b09b-undertow-close", 0, 6, [(0, "Nine inks"), (3, "Four-colour palettes, three new")], "grids"),
    ("b09b-undertow-close", 6, 14, [(0, "LANDSCAPE: rain on water, drawn by the music")]),
]
END_CLIP = ("b09b-undertow-close", 20)                  # the end card's music: UNDERTOW, on from the last clip
GRID9 = ["YELLOW", "WHITE", "GREEN", "BLUE", "RED", "ORANGE", "PINK", "HOT PINK", "LILAC"]
GRID4 = ["MOSAIC", "SUNSET", "OCEAN", "NEON"]
DW = 1080                                               # the demo's square
DS = 3                                                  # its screen, 3x: 720
DX, DY = (DW - 240 * DS) // 2, 150
CARD_S, TITLE_S, END_S, END_HOLD = 2.0, 3.0, 10.0, 2.0
CUT_FADE, CUT_FADE_CARD, CUT_FADE_FIRST = 0.3, 1.0, 2.0   # s: the music's fade between clips; out of and into a
                                                        # card; in after the intro and title
CHAPTER_INK = {"00": "OCEAN", "03": "GREEN", "04": "BLUE", "05": "ORANGE", "06": "MOSAIC", "07": "SUNSET",
               "09": "SUNSET", "10": "YELLOW"}      # (by a clip's chapter number, name[1:3])


def grid(src, names, cols, tile, gap, caption, ink):
    """the same screen in each ink (demo_projects tour: OUTDIR/inks/NN-NAME/), cols a row, the caption under them"""
    from gen_brand import centred
    im = Image.new("RGB", (DW, DW), (0, 0, 0))
    d = ImageDraw.Draw(im)
    found = {p.name.split("-", 1)[1]: p for p in (src / "inks").iterdir()}
    rows = (len(names) + cols - 1) // cols
    w = cols * tile + (cols - 1) * gap
    x0, y0 = (DW - w) // 2, (DW - 120 - (rows * tile + (rows - 1) * gap)) // 2
    for k, n in enumerate(names):
        sc = Image.open(found[n] / "f00000.ppm").convert("RGB").resize((480, 480), Image.NEAREST)
        im.paste(sc.resize((tile, tile), Image.LANCZOS), (x0 + (k % cols) * (tile + gap), y0 + (k // cols) * (tile + gap)))
    centred(d, DW, DW - 100, caption, 5, ink)
    return im


def demo_card(lines, ink):
    im = Image.new("RGB", (DW, DW), (0, 0, 0))
    d = ImageDraw.Draw(im)
    from gen_brand import centred
    mark(d, DW // 2, 360, 56)
    y = 540
    for s, px, dimmed in lines:
        for line in wrap(s, (DW - 120) // (6 * px)):
            centred(d, DW, y, line, px, mix((0, 0, 0), ink, 70) if dimmed else ink)
            y += 7 * px + max(10, 3 * px)
        y += 2 * px
    return im


def demo_frame(caption, ink):
    im = Image.new("RGB", (DW, DW), (0, 0, 0))
    d = ImageDraw.Draw(im)
    from gen_brand import centred
    mark(d, 76, 76, 14)
    text(d, 118, 62, "TESSERA 3.3", 4, mix((0, 0, 0), ink, 70))
    d.rectangle([DX - 4, DY - 4, DX + 240 * DS + 3, DY + 240 * DS + 3], outline=mix((0, 0, 0), ink, 23), width=2)
    y = DY + 240 * DS + 30
    for line in wrap(caption, (DW - 80) // 30)[:2]:
        centred(d, DW, y, line, 5, ink)
        y += 52
    return im


def demo(src, intro, out):
    src, intro, out = Path(src), Path(intro), Path(out)
    work = out.with_suffix(".work")
    work.mkdir(parents=True, exist_ok=True)
    audio = array.array("h")
    plan = []                                           # (kind, payload, frames)
    nf = lambda s: round(s * float(FPS))

    def silence(n):
        audio.extend([0] * (n * SPF * 2))

    # the intro (splash.c's own frames, 4x on black), in silence
    im_t = 0.0
    for line in (intro / "frames.txt").read_text().split("\n"):
        if not line.strip():
            continue
        name, ms = line.split()
        n = nf(im_t + int(ms) / 1000) - nf(im_t)
        im_t += int(ms) / 1000
        big = Image.new("RGB", (DW, DW), (0, 0, 0))
        big.paste(Image.open(intro / name).convert("RGB").resize((240 * 4, 240 * 4), Image.NEAREST), ((DW - 960) // 2,) * 2)
        plan.append(("still", big, n))
        silence(n)
    title = demo_card([("TESSERA 3.3", 12, False), ("Composing through systems, on the M-Vave FM-1", 4, True)], INK["YELLOW"])
    plan.append(("still", title, nf(TITLE_S)))
    silence(nf(TITLE_S))
    ink = INK["YELLOW"]
    for e in CUT:
        idx = CUT.index(e)
        if len(e) == 2:                                 # a card, in the next chapter's ink
            nxt = CUT[idx + 1][0][1:3]
            c = demo_card([(e[0], 9, False), (e[1], 4, True)], INK[CHAPTER_INK.get(nxt, "YELLOW")])
            plan.append(("still", c, nf(CARD_S)))
            silence(nf(CARD_S))
            continue
        name, start, dur, caps = e[:4]
        ink = INK[CHAPTER_INK.get(name[1:3], "YELLOW")]
        f0, n = nf(start), nf(dur)
        ppm = sorted((src / name).glob("f*.ppm"))[f0:f0 + n]
        if len(e) > 4:                                  # the ink grids over this part of the clip's sound
            g9 = grid(src, GRID9, 3, 300, 15, caps[0][1], ink)
            g4 = grid(src, GRID4, 2, 430, 24, caps[1][1], ink)
            ppm = [g9 if i / float(FPS) < caps[1][0] else g4 for i in range(n)]
        with wave.open(str(src / f"{name}.wav")) as r:
            a = array.array("h", r.readframes(r.getnframes()))
        if name.startswith(LEVELLED):
            a = level(a)
        a = a[f0 * SPF * 2:(f0 + n) * SPF * 2]
        a.extend([0] * max(0, n * SPF * 2 - len(a)))
        # (fades: long out of and into silence, short between clips, none where the same clip carries on)
        def goes_on(a, b):
            return len(a) > 2 and len(b) > 2 and a[0] == b[0] and abs(a[1] + a[2] - b[1]) < 1e-6
        k_in = 0 if idx and goes_on(CUT[idx - 1], e) else int(
            (CUT_FADE_FIRST if idx == 0 else CUT_FADE_CARD if len(CUT[idx - 1]) == 2 else CUT_FADE) * FS)
        last = idx + 1 == len(CUT)
        k_out = 0 if (last and (name, start + dur) == END_CLIP) or (not last and goes_on(e, CUT[idx + 1])) else int(
            (CUT_FADE_CARD if last or len(CUT[idx + 1]) == 2 else CUT_FADE) * FS)
        m = len(a) // 2
        for i in range(max(k_in, k_out)):
            for kk, frm in ((k_in, i), (k_out, m - 1 - i)):
                if i < kk:
                    g = math.sin(0.5 * math.pi * i / kk) ** 2
                    a[2 * frm] = int(a[2 * frm] * g)
                    a[2 * frm + 1] = int(a[2 * frm + 1] * g)
        audio.extend(a)
        plan.append(("clip", (ppm, caps, ink), n))
    # the end: the last clip's music carries on under the end card, fading out
    with wave.open(str(src / f"{END_CLIP[0]}.wav")) as r:
        a = array.array("h", r.readframes(r.getnframes()))
    n = nf(END_S)
    s0 = nf(END_CLIP[1]) * SPF * 2
    a = a[s0:s0 + n * SPF * 2]
    a.extend([0] * max(0, n * SPF * 2 - len(a)))
    m = len(a) // 2
    h = int(END_HOLD * FS)
    for i in range(m):
        g = 1.0 if i < h else max(0.0, (m - 1 - i) / (m - h)) ** 1.6
        a[2 * i] = int(a[2 * i] * g)
        a[2 * i + 1] = int(a[2 * i + 1] * g)
    audio.extend(a)
    end = demo_card([("TESSERA 3.3", 12, False), ("rastapopolix.github.io/tessera", 4, True),
                     ("Free, GPL-3.0. Installs from the browser.", 3, True)], INK["YELLOW"])
    plan.append(("still", end, n))
    total = sum(p[2] for p in plan)
    assert len(audio) == total * SPF * 2, (len(audio), total * SPF * 2)
    wav = work / "audio.wav"
    write_wav(wav, audio)

    def frames():
        for kind, payload, n in plan:
            if kind == "still":
                for _ in range(n):
                    yield payload
                continue
            ppm, caps, ink = payload
            cards = {}
            for i, f in enumerate(ppm):
                if isinstance(f, Image.Image):          # (an ink grid: the whole frame)
                    yield f
                    continue
                t = i / float(FPS)
                cap = [c for at, c in caps if at <= t][-1]
                if cap not in cards:
                    cards[cap] = demo_frame(cap, ink)
                im = cards[cap].copy()
                im.paste(Image.open(f).convert("RGB").resize((240 * DS, 240 * DS), Image.NEAREST), (DX, DY))
                yield im

    cmd = [ffmpeg(), "-y", "-loglevel", "error", "-f", "rawvideo", "-pix_fmt", "rgb24", "-s", f"{DW}x{DW}",
           "-framerate", f"{FPS.numerator}/{FPS.denominator}", "-i", "-", "-i", str(wav),
           "-c:v", "libx264", "-preset", "medium", "-crf", "18", "-pix_fmt", "yuv420p", "-r", "30",
           "-c:a", "aac", "-b:a", "192k", "-ar", "48000", "-movflags", "+faststart", str(out)]
    p = subprocess.Popen(cmd, stdin=subprocess.PIPE)
    for im in frames():
        p.stdin.write(im.tobytes())
    p.stdin.close()
    if p.wait():
        raise SystemExit("ffmpeg failed")
    print(f"{out}: {total / float(FPS):.1f} s")


if __name__ == "__main__":
    if sys.argv[1] == "demo":                           # demo CLIPS INTRO OUT.mp4: the captioned 1:1 cut
        demo(*sys.argv[2:5])
    else:
        main(*sys.argv[1:3])
