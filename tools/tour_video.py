#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Theo Dainis (melodee-tess)
"""The 3.3 narrated tour's clips, 16:9, from the firmware's own audio and screens.

  build/host/demo_projects tour build/tour          (tests/demo_projects.c, built with -DMELODEE_LOOK=1)
  python3 tools/tour_video.py build/tour OUTDIR      (needs Pillow and imageio-ffmpeg, or ffmpeg on the PATH)
  python3 tools/tour_video.py narrated build/tour INTRO OUT.mp4   (the 16:9 tour to narrate, chapters and timing)
  python3 tools/tour_video.py short build/tour INTRO OUT.mp4   (the 9:16 short, about a minute, what is new)
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
    ("b07c-kites-piece-0", 1, 3.5, [(0, "PIECE: the foot of a wave...")]),
    ("b07c-kites-piece-1", 1, 3.5, [(0, "...halfway up...")]),
    ("b07c-kites-piece-2", 1, 4.5, [(0, "...and its top")]),
    ("b09b-undertow-close", 0, 6, [(0, "Nine inks"), (3, "Four-colour palettes, three new")], "grids"),
    ("b09b-undertow-close", 6, 18, [(0, "LANDSCAPE: rain on water, drawn by the music"),
                                    (9, "It changes nothing. It's just nice to watch.")]),
]
END_CLIP = ("b09b-undertow-close", 24)                  # the end card's music: UNDERTOW, on from the last clip
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



# The short (YouTube Shorts, Instagram Reels): 1080 x 1920, about a minute, what is new in 3.3. The screen 4x across
# the top, one large caption under it. Each entry: (clip, from s, for s, caption); "grid" as a fifth: the four MOSAIC
# palettes over that part of the clip's sound. The splash plays over the first clip's opening.
SHORT = [
    ("b00-undertow-open", None, 5, "Tessera 3.3: the long form"),
    ("b07a-chain-key", 4, 6, "Sections change together"),
    ("b07b-kites-entries", 0, 5, "Parts come and go"),
    ("b07c-kites-piece-0", 1, 2, "A shape for the whole piece"),
    ("b07c-kites-piece-1", 1, 2, "A shape for the whole piece"),
    ("b07c-kites-piece-2", 1, 2.5, "A shape for the whole piece"),
    ("a04c-split", 4, 5, "Steps that split in two"),
    ("b06a-lfo-on-lfo", 6, 6, "Mods that move mods"),
    ("b06b-perf", 1, 5, "One knob, many moves"),
    ("b09b-undertow-close", 3, 3, "New colours", "grid"),
    ("b09b-undertow-close", 6, 6, "Rain on water, drawn by the music"),
]
SW, SH, SS = 1080, 1920, 4                              # the frame; the screen 4x: 960
SX, SY = (SW - 240 * SS) // 2, 330
SHORT_END = ("b09b-undertow-close", 12, 8.0)            # the end card over the last clip's music: 2 s, then fading


def short_frame(caption, ink):
    from gen_brand import centred
    im = Image.new("RGB", (SW, SH), (0, 0, 0))
    d = ImageDraw.Draw(im)
    mark(d, SW // 2, 170, 28)
    centred(d, SW, 236, "TESSERA 3.3", 5, mix((0, 0, 0), ink, 70))
    d.rectangle([SX - 5, SY - 5, SX + 240 * SS + 4, SY + 240 * SS + 4], outline=mix((0, 0, 0), ink, 23), width=2)
    y = SY + 240 * SS + 60
    for line in wrap(caption, 20)[:3]:
        centred(d, SW, y, line, 8, ink)
        y += 80
    return im


def short_card(ink):
    from gen_brand import centred
    im = Image.new("RGB", (SW, SH), (0, 0, 0))
    d = ImageDraw.Draw(im)
    mark(d, SW // 2, 640, 70)
    centred(d, SW, 860, "TESSERA 3.3", 12, ink)
    for i, (s, px) in enumerate((("Free firmware for the", 5), ("M-Vave FM-1", 5), ("rastapopolix.github.io/tessera", 4))):
        centred(d, SW, 1030 + i * 60 + (20 if i == 2 else 0), s, px, mix((0, 0, 0), ink, 75))
    return im


def short(src, intro, out):
    src, intro, out = Path(src), Path(intro), Path(out)
    work = out.with_suffix(".work")
    work.mkdir(parents=True, exist_ok=True)
    nf = lambda s: round(s * float(FPS))
    audio, plan = array.array("h"), []

    def clip_audio(name, f0, n):
        with wave.open(str(src / f"{name}.wav")) as r:
            a = array.array("h", r.readframes(r.getnframes()))
        if name.startswith(LEVELLED):
            a = level(a)
        a = a[f0 * SPF * 2:(f0 + n) * SPF * 2]
        a.extend([0] * max(0, n * SPF * 2 - len(a)))
        return a

    def fade(a, k_in, k_out):
        m = len(a) // 2
        for i in range(max(k_in, k_out)):
            for kk, frm in ((k_in, i), (k_out, m - 1 - i)):
                if i < kk:
                    g = math.sin(0.5 * math.pi * i / kk) ** 2
                    a[2 * frm] = int(a[2 * frm] * g)
                    a[2 * frm + 1] = int(a[2 * frm + 1] * g)
        return a

    # the splash (splash.c's frames, YELLOW at its own speed), the first clip's music already under it
    t, frames_in = 0.0, []
    for line in (intro / "frames.txt").read_text().split("\n"):
        if line.strip():
            name, ms = line.split()
            n = nf(t + int(ms) / 1000) - nf(t)
            t += int(ms) / 1000
            big = Image.new("RGB", (SW, SH), (0, 0, 0))
            big.paste(Image.open(intro / name).convert("RGB").resize((240 * SS, 240 * SS), Image.NEAREST), (SX, SY))
            frames_in.append((big, n))
    n_intro = sum(n for _, n in frames_in)
    first = SHORT[0][0]
    audio.extend(fade(clip_audio(first, 0, n_intro), int(2.0 * FS), 0))   # (in over 2 s, as the page video)
    for big, n in frames_in:
        plan.append(("still", big, n))
    pos = {first: n_intro}                               # (where each clip's sound has got to, frames)
    for idx, e in enumerate(SHORT):
        name, start, dur, cap = e[:4]
        f0 = pos.get(name, 0) if start is None else nf(start)
        n = nf(dur)
        ink = INK[CHAPTER_INK.get(name[1:3], "YELLOW")]
        prev, nxt = SHORT[idx - 1] if idx else None, SHORT[idx + 1] if idx + 1 < len(SHORT) else None
        cont_in = idx == 0 or (prev and prev[0] == name and prev[1] is not None and abs(prev[1] + prev[2] - start) < 1e-6)
        cont_out = (nxt and nxt[0] == name and abs(start + dur - nxt[1]) < 1e-6) or (
            not nxt and (name, start + dur) == SHORT_END[:2])
        k = int(0.5 * FS)                               # (between pieces: a dip, not a jolt)
        audio.extend(fade(clip_audio(name, f0, n), 0 if cont_in else k, 0 if cont_out else k))
        if len(e) > 4:
            g = grid(src, GRID4, 2, 430, 24, cap, ink)
            card = short_frame(cap, ink)
            w = 2 * 430 + 24                            # (the four tiles as grid() lays them)
            x0, y0 = (DW - w) // 2, (DW - 120 - w) // 2
            sq = g.crop((x0, y0, x0 + w, y0 + w))
            card.paste(sq.resize((240 * SS, 240 * SS)), (SX, SY))
            plan.append(("still", card, n))
        else:
            ppm = sorted((src / name).glob("f*.ppm"))[f0:f0 + n]
            plan.append(("clip", (ppm, cap, ink), n))
        pos[name] = f0 + n
    name, at, secs = SHORT_END
    n = nf(secs)
    a = clip_audio(name, nf(at), n)
    m = len(a) // 2
    h = int(2.0 * FS)
    for i in range(m):
        g = 1.0 if i < h else max(0.0, (m - 1 - i) / (m - h)) ** 1.6
        a[2 * i] = int(a[2 * i] * g)
        a[2 * i + 1] = int(a[2 * i + 1] * g)
    audio.extend(a)
    plan.append(("still", short_card(INK["YELLOW"]), n))
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
            ppm, cap, ink = payload
            card = short_frame(cap, ink)
            for f in ppm:
                im = card.copy()
                im.paste(Image.open(f).convert("RGB").resize((240 * SS, 240 * SS), Image.NEAREST), (SX, SY))
                yield im

    cmd = [ffmpeg(), "-y", "-loglevel", "error", "-f", "rawvideo", "-pix_fmt", "rgb24", "-s", f"{SW}x{SH}",
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


# The narrated tour (16:9, music only: Theo adds his voice): the whole of Tessera, chapter by chapter, from the page
# video's refinements and the n clips. Each entry: ("card", title, line), ("still", file in CLIPS, secs) or
# (clip, from s or None: on from the splash, for s, chapter[, "grid9" | "grid4"]). Labels: each clip's own captions.
NARR = [
    ("b00-undertow-open", None, 12, ""),           # (the opening and HOME: no chapter name on screen)
    ("card", "TESSERA 3.3", "Composing through systems, on the M-Vave FM-1"),
    ("n01-home", 0, 15, ""),
    ("card", "Four tracks, real sounds", "FM6, ORACLE, TRIO, PHYS and drums"),
    ("a02a-fm6-harp", 0, 6, "FOUR TRACKS, REAL SOUNDS"),
    ("a02b-oracle-vox", 0, 8, "FOUR TRACKS, REAL SOUNDS"),
    ("a02c-trio-pad", 0, 6, "FOUR TRACKS, REAL SOUNDS"),
    ("n02d-phys-marimba", 0, 8, "FOUR TRACKS, REAL SOUNDS"),
    ("a02e-808-hand-drums", 0, 6, "FOUR TRACKS, REAL SOUNDS"),
    ("card", "Generators write the notes", "Thirteen ways to write a part"),
    ("n03a-mode-dial", 0, 16, "GENERATORS"),
    ("a03b-turing-lock", 0, 16, "GENERATORS"),
    ("a03c-euclid-pulses", 0, 12, "GENERATORS"),
    ("a03d-markov-then-all", 0, 16, "GENERATORS"),
    ("b03f-constellation", 0, 10, "GENERATORS"),
    ("a03e-bounce", 0, 8, "GENERATORS"),
    ("n03g-atlas", 0, 10, "GENERATORS"),
    ("n03h-chord", 0, 10, "GENERATORS"),
    ("n03i-quad", 0, 9, "GENERATORS"),
    ("card", "Shaping the line", "How the notes are played"),
    ("a04a-arp-lane", 0, 8, "SHAPING THE LINE"),
    ("a04b-drop", 0, 12, "SHAPING THE LINE"),
    ("a04c-split", 0, 16, "SHAPING THE LINE"),
    ("a04d-adorn", 0, 12, "SHAPING THE LINE"),
    ("n04e-gravity", 0, 12, "SHAPING THE LINE"),
    ("card", "Tracks that listen", "Parts that answer and follow"),
    ("a05a-call-answer", 0, 32, "TRACKS THAT LISTEN"),
    ("a05b-answers-overlap", 0, 15, "TRACKS THAT LISTEN"),
    ("a05c-follow", 0, 16, "TRACKS THAT LISTEN"),
    ("n05d-seed", 0, 12, "TRACKS THAT LISTEN"),
    ("card", "Modulate the modulators", "Movement, and hands on"),
    ("b06a-lfo-on-lfo", 0, 16, "MODULATE THE MODULATORS"),
    ("b06b-perf", 0, 12, "MODULATE THE MODULATORS"),
    ("b06c-stone-pier", 0, 10, "MODULATE THE MODULATORS"),
    ("n06d-organic", 0, 10, "MODULATE THE MODULATORS"),
    ("n06e-matrix", 0, 8, "MODULATE THE MODULATORS"),
    ("card", "From a loop to a piece", "The long form"),
    ("still", "layers.png", 12),
    ("n07a-form", 0, 12, "FROM A LOOP TO A PIECE"),
    ("b07a-chain-key", 2, 18, "FROM A LOOP TO A PIECE"),
    ("b07b-kites-entries", 0, 16, "FROM A LOOP TO A PIECE"),
    ("b07c-kites-piece-0", 1, 4, "FROM A LOOP TO A PIECE"),
    ("b07c-kites-piece-1", 1, 4, "FROM A LOOP TO A PIECE"),
    ("b07c-kites-piece-2", 1, 4, "FROM A LOOP TO A PIECE"),
    ("n07e-energy", 0, 10, "FROM A LOOP TO A PIECE"),
    ("card", "Playing it", "By hand, live, and other gear"),
    ("a08-mixer", 0, 14, "PLAYING IT"),
    ("n08b-fill", 0, 10, "PLAYING IT"),
    ("n08c-midi", 0, 50, "PLAYING IT"),
    ("card", "Colour, and a page to watch", "Nine inks, four palettes, LANDSCAPE"),
    ("b09b-undertow-close", 0, 4, "COLOUR", "grid9"),
    ("b09b-undertow-close", 4, 4, "COLOUR", "grid4"),
    ("b09b-undertow-close", 8, 18, "A PAGE TO WATCH"),
]
NARR_END = ("b09b-undertow-close", 26, 10.0)
NW, NH = 1920, 1080
NX, NY, NS = 1920 - 120 - 240 * 4, 60, 4               # the screen at the right, the text at the left (Theo)
NTX = 120                                              # the text column's left edge
NCARD_S = 3.0
NARR_INK = dict(CHAPTER_INK, **{"01": "YELLOW", "02": "YELLOW", "08": "NEON"})
NARR_LABELS = {"b09b-undertow-close": {8: "LANDSCAPE", 17: "IT CHANGES NOTHING. IT'S JUST NICE TO WATCH."},
               "n08c-midi": {0: "GLO > SYSTEM", 16: "MENU: NOTES OUT", 22: "CLOCK OUT", 27: "MIDI CC IN",
                             31: "MIDI LEARN", 35: "USB HOST"}}


def narr_panel(chapter, label, ink):
    """the narrated tour's card without the screen: the text column at the left, the screen's frame at the right"""
    im = Image.new("RGB", (NW, NH), (0, 0, 0))
    d = ImageDraw.Draw(im)
    dim = mix((0, 0, 0), ink, 60)
    mark(d, NTX + 14, NY + 14, 7)
    text(d, NTX + 40, NY + 2, "TESSERA 3.3", 4, dim)
    y = NY + 120
    for line in wrap(chapter, 22):
        text(d, NTX, y, line, 4, dim)
        y += 40
    y += 40 if chapter else 0
    for line in wrap(label, 15)[:5]:
        text(d, NTX, y, line, 6, ink)
        y += 60
    d.rectangle([NX - 6, NY - 6, NX + 240 * NS + 5, NY + 240 * NS + 5], outline=mix((0, 0, 0), ink, 23), width=2)
    return im


def narr_card(title, line, ink):
    from gen_brand import centred
    im = Image.new("RGB", (NW, NH), (0, 0, 0))
    d = ImageDraw.Draw(im)
    mark(d, NW // 2, 360, 56)
    centred(d, NW, 520, title.upper(), 9, ink)
    if line:
        centred(d, NW, 640, line, 5, mix((0, 0, 0), ink, 70))
    return im


def narr_screen(img240, ink):
    """a 240 x 240 image or an already-composed square, on the panel's screen place"""
    return img240.resize((240 * NS, 240 * NS), Image.NEAREST if img240.width == 240 else Image.LANCZOS)


def narrated(src, intro, out):
    src, intro, out = Path(src), Path(intro), Path(out)
    work = out.with_suffix(".work")
    work.mkdir(parents=True, exist_ok=True)
    nf = lambda s: round(s * float(FPS))
    audio, plan, rows, chapters = array.array("h"), [], [], []
    caps_of = {name: caps for name, _, _, caps in clips(src)} if (src / "captions.txt").exists() else {}

    def clip_audio(name, f0, n):
        with wave.open(str(src / f"{name}.wav")) as r:
            a = array.array("h", r.readframes(r.getnframes()))
        if name.startswith(LEVELLED):
            a = level(a)
        a = a[f0 * SPF * 2:(f0 + n) * SPF * 2]
        a.extend([0] * max(0, n * SPF * 2 - len(a)))
        return a

    def fade(a, k_in, k_out):
        m = len(a) // 2
        for i in range(max(k_in, k_out)):
            for kk, frm in ((k_in, i), (k_out, m - 1 - i)):
                if i < kk:
                    g = math.sin(0.5 * math.pi * i / kk) ** 2
                    a[2 * frm] = int(a[2 * frm] * g)
                    a[2 * frm + 1] = int(a[2 * frm + 1] * g)
        return a

    def now():
        return sum(p[2] for p in plan) / float(FPS)

    # the splash (YELLOW, its own speed) on the screen's place, the cold open's music already under it
    t, frames_in = 0.0, []
    for line in (intro / "frames.txt").read_text().split("\n"):
        if line.strip():
            name, ms = line.split()
            n = nf(t + int(ms) / 1000) - nf(t)
            t += int(ms) / 1000
            im = Image.new("RGB", (NW, NH), (0, 0, 0))
            im.paste(Image.open(intro / name).convert("RGB").resize((240 * NS, 240 * NS), Image.NEAREST),
                     ((NW - 240 * NS) // 2, NY))
            frames_in.append((im, n))
    n_intro = sum(n for _, n in frames_in)
    first = NARR[0][0]
    audio.extend(fade(clip_audio(first, 0, n_intro), int(2.0 * FS), 0))   # (in over 2 s)
    for im, n in frames_in:
        plan.append(("still", im, n))
    pos, ink = {first: n_intro}, INK["YELLOW"]
    cards, textable, cur_chapter = set(), set(), ""      # (plan places: the cards; the stills with the text column)
    for idx, e in enumerate(NARR):
        prev, nxt = (NARR[idx - 1] if idx else None), (NARR[idx + 1] if idx + 1 < len(NARR) else None)
        if CAPTIONS is not None and e[0] != "card":
            CAPTIONS.extend((now() + at, now() + at + cap_read(c), c) for at, c in cap_blocks(narr_key(e))[0])
        if e[0] == "card":
            nxt_name = nxt[0] if nxt and nxt[0] not in ("card", "still") else (NARR[idx + 2][0] if idx + 2 < len(NARR) else "")
            ink = INK[NARR_INK.get(nxt_name[1:3], "YELLOW")]
            if e[1] != "TESSERA 3.3":
                chapters.append((now(), e[1]))
            n = nf(NCARD_S)
            cards.add(len(plan))
            cur_chapter = e[1].upper() if e[1] != "TESSERA 3.3" else ""
            plan.append(("still", narr_card(e[1], e[2], ink), n))
            audio.extend([0] * (n * SPF * 2))
            rows.append([f"{now() - NCARD_S:.1f}", "card", e[1], e[2]])
            continue
        if e[0] == "still":
            n = nf(e[2])
            im = Image.new("RGB", (NW, NH), (0, 0, 0))
            pic = Image.open(src / e[1]).convert("RGB")
            if CAPTIONS is not None:                    # (captioned: on the screen's place, the text at its left)
                im = narr_panel(cur_chapter, "LAYERS", ink)
                k = min(240 * NS / pic.width, 240 * NS / pic.height)
                pic = pic.resize((int(pic.width * k), int(pic.height * k)), Image.LANCZOS)
                im.paste(pic, (NX + (240 * NS - pic.width) // 2, NY + (240 * NS - pic.height) // 2))
                textable.add(len(plan))
            else:
                pic = pic.resize((int(pic.width * 1.6), int(pic.height * 1.6)), Image.LANCZOS)
                im.paste(pic, ((NW - pic.width) // 2, (NH - pic.height) // 2))
            plan.append(("still", im, n))
            audio.extend([0] * (n * SPF * 2))
            rows.append([f"{now() - e[2]:.1f}", "still", e[1], ""])
            continue
        name, start, dur, chapter = e[:4]
        f0 = pos.get(name, 0) if start is None else nf(start)
        n = nf(dur)
        ink = INK[NARR_INK.get(name[1:3], "YELLOW")]
        same = lambda a, b: a and b and len(a) > 3 and len(b) > 3 and a[0] == b[0] and a[0] not in ("card", "still")
        cont_in = idx == 0 or (same(prev, e) and prev[1] is not None and abs(prev[1] + prev[2] - (start or 0)) < 1e-6)
        cont_out = (same(e, nxt) and abs((start or 0) + dur - nxt[1]) < 1e-6) or (not nxt and (name, start + dur) == NARR_END[:2])
        k_in = 0 if cont_in else int((1.0 if prev and prev[0] in ("card", "still") else CUT_N) * FS)
        k_out = 0 if cont_out else int(((2.0 if idx == 0 else 1.0) if not nxt or nxt[0] in ("card", "still") else CUT_N) * FS)
        audio.extend(fade(clip_audio(name, f0, n), k_in, k_out))
        t0 = f0 / float(FPS)
        labels = NARR_LABELS.get(name) or {}
        caps = [(at - t0, c) for at, _, c in caps_of.get(name, [])] if not labels else [(at - t0, c) for at, c in labels.items()]
        if len(e) > 4:
            names = GRID9 if e[4] == "grid9" else GRID4
            g = grid(src, names, 3 if e[4] == "grid9" else 2, 300 if e[4] == "grid9" else 430, 15 if e[4] == "grid9" else 24, "", ink)
            card = narr_panel(chapter, "NINE INKS" if e[4] == "grid9" else "MOSAIC, SUNSET, OCEAN, NEON", ink)
            card.paste(g.crop((0, 0, DW, DW - 120)).resize((240 * NS, 240 * NS - 107), Image.LANCZOS), (NX, NY + 53))
            textable.add(len(plan))
            plan.append(("still", card, n))
            rows.append([f"{now() - dur:.1f}", name, chapter, "NINE INKS" if e[4] == "grid9" else "FOUR PALETTES"])
        else:
            ppm = sorted((src / name).glob("f*.ppm"))[f0:f0 + n]
            plan.append(("clip", (ppm, caps, chapter, ink), n))
            rows.append([f"{now() - dur:.1f}", name, chapter, " | ".join(f"{max(0, a):.0f}s {c}" for a, c in caps if a < dur)])
        pos[name] = f0 + n
    name, at, secs = NARR_END
    n = nf(secs)
    a = clip_audio(name, nf(at), n)
    m, h = len(a) // 2, int((2.0 if CAPTIONS is None else max(2.0, secs - 6.0)) * FS)
    for i in range(m):
        g = 1.0 if i < h else max(0.0, (m - 1 - i) / (m - h)) ** 1.6
        a[2 * i] = int(a[2 * i] * g)
        a[2 * i + 1] = int(a[2 * i + 1] * g)
    audio.extend(a)
    chapters.append((now(), "Where to get it"))
    if CAPTIONS is not None:
        CAPTIONS.extend((now() + at, now() + at + cap_read(c), c) for at, c in cap_blocks("end")[0])
    end_idx = len(plan)
    cards.add(end_idx), textable.add(end_idx)
    plan.append(("still", narr_card("TESSERA 3.3", "rastapopolix.github.io/tessera", INK["YELLOW"]), n))
    rows.append([f"{now() - secs:.1f}", "end card", "WHERE TO GET IT", ""])
    total = sum(p[2] for p in plan)
    assert len(audio) == total * SPF * 2, (len(audio), total * SPF * 2)
    wav = work / "audio.wav"
    write_wav(wav, audio)

    def cap_at(t):
        return next((c for t0, t1, c in CAPTIONS if t0 <= t < t1), "") if CAPTIONS else ""

    def frames():
        fi = 0
        for pi, (kind, payload, n) in enumerate(plan):
            cache = {}
            v_out = nf(FADE_V_OUT) if CAPTIONS is not None and pi + 1 in cards else 0
            v_in = nf(FADE_V_IN) if CAPTIONS is not None and pi in cards else 0
            for j in range(n):
                body = cap_at(fi / float(FPS))
                fi += 1
                if kind == "still":
                    im = payload
                    if body and pi in textable:
                        if body not in cache:
                            cache[body] = cap_draw(payload.copy(), body, centre=pi == end_idx)
                        im = cache[body]
                else:
                    ppm, caps, chapter, ink = payload
                    tc = j / float(FPS)
                    cap = ([c for at, c in caps if at <= tc] or [caps[0][1] if caps else ""])[-1]
                    if (cap, body) not in cache:
                        p = narr_panel(chapter, cap, ink)
                        cache[(cap, body)] = cap_draw(p, body) if body else p
                    im = cache[(cap, body)].copy()
                    im.paste(Image.open(ppm[min(j, len(ppm) - 1)]).convert("RGB").resize((240 * NS, 240 * NS), Image.NEAREST),
                             (NX, NY))
                g = 1.0
                if v_out and j >= n - v_out:
                    g = (n - 1 - j) / float(v_out)
                if v_in and j < v_in:
                    g = min(g, j / float(v_in))
                if g < 1.0:
                    im = Image.eval(im, lambda v, g=g: int(v * g))
                yield im

    cmd = [ffmpeg(), "-y", "-loglevel", "error", "-f", "rawvideo", "-pix_fmt", "rgb24", "-s", f"{NW}x{NH}",
           "-framerate", f"{FPS.numerator}/{FPS.denominator}", "-i", "-", "-i", str(wav),
           "-c:v", "libx264", "-preset", "medium", "-crf", "18", "-pix_fmt", "yuv420p", "-r", "30",
           "-c:a", "aac", "-b:a", "192k", "-ar", "48000", "-movflags", "+faststart", str(out)]
    p = subprocess.Popen(cmd, stdin=subprocess.PIPE)
    for im in frames():
        p.stdin.write(im.tobytes())
    p.stdin.close()
    if p.wait():
        raise SystemExit("ffmpeg failed")
    stamp = lambda s: f"{int(s) // 60}:{int(s) % 60:02d}"
    (out.with_suffix(".chapters.txt")).write_text("0:00 Intro\n" + "".join(f"{stamp(s)} {c}\n" for s, c in chapters))
    with open(out.with_suffix(".timing.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["start (s)", "clip", "chapter", "labels (from the clip's start)"])
        w.writerows(rows)
    print(f"{out}: {total / float(FPS):.1f} s, {len(chapters)} chapters")


# The captioned tour (Theo 2026-11-07: no voice; the script's words on the screen, long enough to read while watching):
# the narrated cut with each beat's text in the left column. By the NARR entry (its clip; a still's file; the colour
# grids as CLIP:grid9 / CLIP:grid4, LANDSCAPE as CLIP:land; the end card as "end"): its blocks in order, each a text or
# (text, at: seconds into the entry, no earlier than that), and "play" seconds after the last block with the column
# empty (the script's [let it play]). The pictures run as long as their words need.
NARR_TEXT = {
    "b00-undertow-open": [("Tessera: free firmware for the M-Vave FM-1. Everything you hear and see here is the firmware "
                           "itself.", 3.0)],
    "n01-home": ["Tessera turns the FM-1 into an instrument for composing through systems.",
                 "Instead of programming every note, you set up how the music behaves, and it plays.",
                 "Play it by hand, let it play, or anything in between.",
                 "Four tracks, each with its own sound and its own part.",
                 "This tour goes the way I'd learn it: the sounds, the generators that write the notes, how tracks "
                 "listen to each other, modulation, and how a loop becomes a whole piece."],
    "a02a-fm6-harp": ["Each track has a sound engine. FM6 is six-operator FM: bells, keys, harps."],
    "a02b-oracle-vox": ["ORACLE is a two-oscillator synth with a four-pole filter. Here it's a choir."],
    "a02c-trio-pad": ["TRIO is a three-oscillator synth, for pads, bass and leads."],
    "n02d-phys-marimba": ["PHYS is physical modelling: bars, strings, drum heads. Here, a marimba."],
    "a02e-808-hand-drums": ["And there are 808 and 909 drum kits."],
    "n03a-mode-dial": ["Each track can play steps you write, an arpeggio, or one of thirteen generators.",
                       "On GEN, the first knob picks the mode: STEP, ARP, then the generators. Each makes music in "
                       "its own way."],
    "a03b-turing-lock": ["TURING plays a loop of notes. At full LOCK it repeats exactly. Turn it down and it drifts, "
                         "note by note.",
                         "A lot of Tessera is that idea: something that repeats, but not quite."],
    "a03c-euclid-pulses": ["EUCLID spreads hits as evenly as it can across the bar. More pulses, more hits."],
    "a03d-markov-then-all": ["MARKOV walks between nearby notes, like someone humming.", ("play", 6)],
    "b03f-constellation": ["CONSTELLATION plays the stars as they pass the ship. Move the ship, and other stars play."],
    "a03e-bounce": ["BOUNCE plays a note every time a ball hits a wall."],
    "n03g-atlas": ["ATLAS is a map of drum grooves, like Mutable Instruments' Grids. Move across it, and the groove "
                   "changes."],
    "n03h-chord": ["CHORD plays progressions in your scale, and the other tracks can follow its harmony."],
    "n03i-quad": ["And QUAD is a grid of notes that tracks share, each reading its own row."],
    "a04a-arp-lane": ["SEQ is what a track plays. The ARP button is how it's shaped: EVOLVE, FORM, TWEAK, LENGTH, "
                      "TIMING."],
    "a04b-drop": ["On TWEAK, DROP leaves notes out. A little space makes a line breathe."],
    "a04c-split": ["New in 3.3: hold OCT+ on TIMING for SPLIT.",
                   "Now and then a step breaks into two quicker notes, or lands a little late.", ("play", 5)],
    "a04d-adorn": ["On a step track, EVOLVE can ADORN it: small ornaments in the gaps, a little different each round."],
    "n04e-gravity": ["And when a line wanders too far, GRAVITY pulls it back home now and then."],
    "a05a-call-answer": ["This is where Tessera is different from a groovebox with a random button: the tracks listen "
                         "to each other.",
                         "Track 1 plays a melody I wrote: a call.",
                         ("Track 2 is the ANSWER generator, and its source is track 1. It plays the call back, changed "
                          "a little more each time.", 14.5), ("play", 5)],
    "a05b-answers-overlap": ["It never forgets, so the answers start to overlap and talk over each other."],
    "a05c-follow": [("Tracks can also FOLLOW. This bass plays its own rhythm, but takes its notes from the melody.", 6.0)],
    "n05d-seed": ["And a pattern you write can be the seed for a generator, so it varies your line instead of making "
                  "its own."],
    "b06a-lfo-on-lfo": ["Mods move things on their own. Here an LFO sweeps the filter.",
                        ("New in 3.3, you can modulate the modulators: a second LFO swells and fades the first one's "
                         "sweep.", 7.0)],
    "b06b-perf": ["Also new: PERF macros. One knob, several moves at once. Here it opens the filter and brings in a "
                  "tremolo."],
    "b06c-stone-pier": ["Slow sources, TIDE and SWARM, move things over minutes. Here they move a dub echo."],
    "n06d-organic": ["This page shows them as slow, organic curves."],
    "n06e-matrix": ["And MATRIX shows the whole patch at a glance."],
    "layers.png": ["Everything so far makes a good loop. These layers turn it into a piece, slowest at the top.",
                   "They all start switched off."],
    "n07a-form": ["FORM gives one track sections, A, B and C, and DEVELOP varies each return of A."],
    "b07a-chain-key": ["CHAIN does that for the whole project. Every track moves to its B together,",
                       "and each section can have its own key. Here B goes up a fourth.", ("play", 5)],
    "b07b-kites-entries": ["ENTRIES takes parts out and brings them back in a musical order: the melody leaves first, "
                           "the bass comes back first."],
    "b07c-kites-piece-0": ["And PIECE plans the whole thing: a length, from a minute to four hours, and a shape."],
    "b07c-kites-piece-1": ["This one is waves. The foot of a wave, halfway up..."],
    "b07c-kites-piece-2": ["...and the top."],
    "n07e-energy": ["ENERGY follows the shape: busier and louder near the top, sparser in the valleys.",
                    "Press PLAY again and the same piece plays again; ROLL gives you a new one."],
    "a08-mixer": ["You can still play all of this by hand. MIXER has every track's level, and mutes can wait for the "
                  "bar or the phrase."],
    "n08b-fill": ["Hold GLO and E4 for a fill.", ("play", 5)],
    "n08c-midi": ["To play other gear: GLO > SYSTEM. MIDI picks the input, USB or TRS; ROUT the channels the tracks "
                  "play on; DRUM the drum track's own channel.",
                  ("In the menu (hold HOME), NOTES OUT sends every track's notes, or just the keys, and CLOCK OUT "
                   "the clock. MIDI CC IN and MIDI LEARN let a controller turn the knobs.", 16.5),
                  ("And with USB HOST on, the FM-1 is the host: plug in a USB-MIDI synth, keyboard or MIDI-to-CV "
                   "module, no computer needed (power it from a hub).", 35.5)],
    "b09b-undertow-close:grid9": ["The screen comes in nine inks..."],
    "b09b-undertow-close:grid4": ["...and four palettes that give each track its own colour."],
    "b09b-undertow-close:land": ["And there's a page just to watch. LANDSCAPE draws rain on water from the music: every "
                                 "note is a drop.",
                                 "It changes nothing. It's just nice to watch.", ("play", 8)],
    "end": ["Tessera is free and open source. Install it from your browser, or try it there in the simulator, no "
            "FM-1 needed.",
            "It's experimental, in every sense of the word, so tell me how it goes."],
}
CAP_AT, CAP_AIR = 0.8, 1.2                              # s: the first block's place in an entry; after the last
CAP_Y, CAP_PX, CAP_COLS, CAP_LH = 460, 4, 27, 44        # the text column: its top, font pixel, characters a line, line
CUT_N, FADE_V_OUT, FADE_V_IN = 0.8, 0.5, 0.3            # s: the music between clips; the picture to black before a
                                                        # card, and the card up from black
CAPTIONS = None                                         # (captioned(): the text timeline, (from s, to s, text))


def cap_read(s):
    """seconds to read a block while watching the screen: 1.5 s, and 2 words a second"""
    return 1.5 + len(s.split()) / 2.0


def cap_blocks(key):
    """an entry's blocks as (at, text), and its need: the seconds its words (and any play after them) take"""
    t, out, need = CAP_AT, [], 0.0
    for b in NARR_TEXT.get(key, []):
        if isinstance(b, tuple) and b[0] == "play":
            need = t + b[1]
            continue
        s, at = (b, None) if isinstance(b, str) else b
        t = max(t, at or 0.0)
        out.append((t, s))
        t += cap_read(s)
        need = t + CAP_AIR
    return out, need


def narr_key(e):
    if e[0] == "still":
        return e[1]
    if e[0] == "b09b-undertow-close":
        return e[0] + (":" + e[4][:5] if len(e) > 4 else ":land")
    return e[0]


def cap_draw(im, s, centre=False):
    """a block in the text column (or centred under the end card's address)"""
    from gen_brand import centred
    d = ImageDraw.Draw(im)
    if centre:
        y = 760
        for line in wrap(s, 40):
            centred(d, NW, y, line, CAP_PX, (215, 215, 215))
            y += CAP_LH
        return im
    y = CAP_Y
    for line in wrap(s, CAP_COLS):
        text(d, NTX, y, line, CAP_PX, (215, 215, 215))
        y += CAP_LH
    return im


def captioned(src, intro, out):
    """the narrated cut, captioned: each entry as long as its words need, the words in the text column"""
    global NARR, NARR_END, NARR_LABELS, CAPTIONS
    src, out = Path(src), Path(out)
    frames_of = lambda name: len(list((src / name).glob("f*.ppm"))) / float(FPS)
    new = []
    for e in NARR:
        if e[0] == "card":
            new.append(e)
            continue
        _, need = cap_blocks(narr_key(e))
        new.append((e[0], e[1], round(max(e[2], need), 2)) + tuple(e[3:]))
    # one clip in several entries: each on from where the last stopped
    for i in range(1, len(new)):
        a, b = new[i - 1], new[i]
        if b[0] == a[0] and b[0] not in ("card", "still") and a[1] is not None:
            new[i] = (b[0], round(a[1] + a[2], 2)) + tuple(b[2:])
    last = new[-1]
    _, need_end = cap_blocks("end")
    end = (NARR_END[0], round(last[1] + last[2], 2), round(max(NARR_END[2], need_end + 4.0), 2))
    short_of = []
    for e in new:
        if e[0] not in ("card", "still") and (e[1] or 0) + e[2] > frames_of(e[0]) + 1e-6:
            short_of.append(f"{e[0]}: needs {(e[1] or 0) + e[2]:.1f} s, has {frames_of(e[0]):.1f}")
    if end[1] + end[2] > frames_of(end[0]) + 1e-6:
        short_of.append(f"{end[0]} (end): needs {end[1] + end[2]:.1f} s, has {frames_of(end[0]):.1f}")
    if short_of:
        raise SystemExit("captioned: clips too short:\n  " + "\n  ".join(short_of))
    saved = NARR, NARR_END, NARR_LABELS
    NARR, NARR_END = new, end
    NARR_LABELS = dict(NARR_LABELS, **{"b09b-undertow-close": {0: "LANDSCAPE"}})
    CAPTIONS = []
    try:
        narrated(src, intro, out)
    finally:
        NARR, NARR_END, NARR_LABELS = saved
        CAPTIONS = None


def read_voice(path):
    """a narration WAV as mono floats at FS (44.1 kHz; linear resampling is enough for a voice)"""
    import numpy as np
    w = wave.open(str(path))
    ch, sw, fs, n = w.getnchannels(), w.getsampwidth(), w.getframerate(), w.getnframes()
    raw = w.readframes(n)
    if sw == 3:
        b = np.frombuffer(raw, dtype=np.uint8).reshape(-1, 3)
        a = (b[:, 0].astype(np.int32) | (b[:, 1].astype(np.int32) << 8) | (b[:, 2].astype(np.int32) << 16))
        a = np.where(a >= 1 << 23, a - (1 << 24), a) / float(1 << 23)
    else:
        a = np.frombuffer(raw, dtype=np.int16) / 32768.0
    a = a.reshape(-1, ch).mean(axis=1)
    if fs != FS:
        t = np.arange(int(len(a) * FS / fs)) * (fs / FS)
        a = np.interp(t, np.arange(len(a)), a)
    return a


def voiced(src, intro, narration, beats_json, out):
    """the narrated tour cut to Theo's voice: each clip as long as its beats need (at least its NARR length), the voice
    at each beat's place, the music ducked under it. OUT.mp4 (voice and music), OUT-music.mp4 (the same picture, music
    only), OUT-voice.wav (the voice alone on the same timeline), OUT.chapters.txt"""
    import json
    import numpy as np
    global NARR, NARR_END
    src, out = Path(src), Path(out)
    beats = {k: v for k, v in json.load(open(beats_json)).items() if not k.startswith("_")}
    voice = read_voice(narration)
    AIR, LEAD = 1.5, 1.0
    keyof = lambda e: (e[0] + (":grid9" if len(e) > 4 and e[4] == "grid9" else "")
                       + (":land" if e[0] == "b09b-undertow-close" and len(e) == 4 else ""))
    new, placed, t = [], [], 0.0                         # (placed: voice spans in OUT's time, filled after the cut)
    for e in NARR:
        if e[0] in ("card", "still") or len(e) < 4:
            new.append(e)
            continue
        b = beats.get(keyof(e)) or []
        need = max([(bb[2] if len(bb) > 2 else LEAD) + (bb[1] - bb[0]) + AIR for bb in b] or [0])
        dur = max(e[2], need)
        new.append((e[0], e[1], round(dur, 2)) + tuple(e[3:]))
    # the stills with a beat: the layers diagram
    for i, e in enumerate(new):
        if e[0] == "still" and e[1] in beats:
            bb = beats[e[1]][0]
            new[i] = ("still", e[1], max(e[2], LEAD + bb[1] - bb[0] + AIR))
    # PIECE: one beat over the three moments; the last holds on till it ends
    pb = beats.get("b07c-kites-piece-0")
    if pb:
        need = LEAD + pb[0][1] - pb[0][0] + AIR
        idx = [i for i, e in enumerate(new) if e[0].startswith("b07c-kites-piece-")]
        have = sum(new[i][2] for i in idx)
        if need > have:
            e = new[idx[-1]]
            new[idx[-1]] = (e[0], e[1], round(e[2] + need - have, 2)) + tuple(e[3:])
        new[idx[0]] = new[idx[0]][:2] + (new[idx[0]][2],) + tuple(new[idx[0]][3:])
    eb = beats.get("end")
    end_secs = max(NARR_END[2], (eb[0][2] if len(eb[0]) > 2 else LEAD) + eb[0][1] - eb[0][0] + 3.0) if eb else NARR_END[2]
    saved, saved_end = NARR, NARR_END
    NARR, NARR_END = new, (NARR_END[0], NARR_END[1], round(end_secs, 2))
    try:
        music_mp4 = out.with_name(out.stem + "-music.mp4")
        narrated(src, intro, music_mp4)                  # (the picture and the music, as the narrated cut)
    finally:
        NARR, NARR_END = saved, saved_end
    # where each entry starts in the cut: the timing sheet narrated() wrote
    rows = list(csv.reader(open(music_mp4.with_suffix(".timing.csv"))))[1:]
    with wave.open(str(music_mp4.with_suffix(".work") / "audio.wav")) as r:
        music = np.frombuffer(r.readframes(r.getnframes()), dtype=np.int16).reshape(-1, 2).astype(np.float64) / 32768
    vtrack = np.zeros(len(music))
    starts = {}
    for row in rows:
        st, clip = float(row[0]), row[1]
        if clip == "card":
            continue
        if clip == "end card":
            key = "end"
        elif clip == "still":
            key = row[2]
        elif clip == "b09b-undertow-close":
            key = "b09b-undertow-close:grid9" if row[3] == "NINE INKS" else (None if row[3] == "FOUR PALETTES" else "b09b-undertow-close:land")
        else:
            key = clip
        if not key or key in starts:
            continue
        starts[key] = st
    for key, spans in beats.items():
        if key not in starts:
            print("voiced: no place for", key)
            continue
        for bb in spans:
            at = starts[key] + (bb[2] if len(bb) > 2 else LEAD)
            seg = voice[int(bb[0] * FS):int(bb[1] * FS)].copy()
            f = int(0.03 * FS)                           # (a few ms of fade: no click at the cut)
            seg[:f] *= np.linspace(0, 1, f); seg[-f:] *= np.linspace(1, 0, f)
            i0 = int(at * FS)
            vtrack[i0:i0 + len(seg)] += seg[:max(0, len(vtrack) - i0)]
            placed.append((at, at + len(seg) / FS))
    rms = np.sqrt(np.mean(vtrack[np.abs(vtrack) > 1e-4] ** 2)) or 1
    vtrack *= min(10 ** (-18 / 20) / rms, 10 ** (-1.5 / 20) / (np.abs(vtrack).max() or 1))   # (the voice at -18 dBFS)
    duck = np.ones(len(music))                           # (the music 9 dB down under the voice, 0.4 s ramps)
    on = np.zeros(len(music), dtype=bool)
    for a, b in placed:
        on[int(a * FS):int(b * FS)] = True
    target = np.where(on, 10 ** (-9 / 20), 1.0)
    r = int(0.4 * FS)
    kern = np.ones(r) / r
    duck = np.convolve(target, kern, mode="same")
    duck = np.minimum(duck, np.convolve(target[::-1], kern, mode="same")[::-1])
    mix = music * duck[:, None] + vtrack[:, None]
    peak = np.abs(mix).max()
    if peak > 0.97:
        mix *= 0.97 / peak
    vw = out.with_name(out.stem + "-voice.wav")
    with wave.open(str(vw), "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(FS)
        w.writeframes((np.clip(vtrack, -1, 1) * 32767).astype(np.int16).tobytes())
    mw = music_mp4.with_suffix(".work") / "mix.wav"
    with wave.open(str(mw), "wb") as w:
        w.setnchannels(2); w.setsampwidth(2); w.setframerate(FS)
        w.writeframes((np.clip(mix, -1, 1) * 32767).astype(np.int16).tobytes())
    subprocess.run([ffmpeg(), "-y", "-loglevel", "error", "-i", str(music_mp4), "-i", str(mw), "-map", "0:v", "-map", "1:a",
                    "-c:v", "copy", "-c:a", "aac", "-b:a", "192k", "-ar", "48000", "-movflags", "+faststart", str(out)], check=True)
    import shutil as _sh
    _sh.copy(music_mp4.with_suffix(".chapters.txt"), out.with_suffix(".chapters.txt"))
    print(f"{out}: voiced, {len(placed)} beats; {music_mp4}; {vw}")

if __name__ == "__main__":
    if sys.argv[1] == "demo":                           # demo CLIPS INTRO OUT.mp4: the captioned 1:1 cut
        demo(*sys.argv[2:5])
    elif sys.argv[1] == "short":                        # short CLIPS INTRO OUT.mp4: the 9:16 short
        short(*sys.argv[2:5])
    elif sys.argv[1] == "narrated":                     # narrated CLIPS INTRO OUT.mp4: the 16:9 tour
        narrated(*sys.argv[2:5])
    elif sys.argv[1] == "captioned":                    # captioned CLIPS INTRO OUT.mp4: the tour with its words
        captioned(*sys.argv[2:5])
    elif sys.argv[1] == "voiced":                       # voiced CLIPS INTRO NARRATION.wav BEATS.json OUT.mp4
        voiced(*sys.argv[2:7])
    else:
        main(*sys.argv[1:3])
