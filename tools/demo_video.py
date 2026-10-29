#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Theo Dainis (melodee-tess)
"""The Tessera demo video from the firmware's own audio and screens, in the Mono look.

  build/host/demo_projects video build/video      (tests/demo_projects.c, built with -DMELODEE_LOOK=1: the clips' WAVs,
                                                   frames and captions)
  python3 tools/demo_video.py build/video OUT.mp4  (needs Pillow and imageio-ffmpeg, or ffmpeg on the PATH)

Each clip's frames (240 x 240, a frame every 1472 samples: 44100/1472 frames a second) are drawn 3x, nearest, on a
1080 x 1080 card, one ink on black as the device and the brand pictures (tools/gen_brand.py), its caption under the
screen in the device's 5 x 7 font. A title card, a card before the generators (clip g01-), before what is new (n01-)
and before the shaping pages (s1-) are added. The last clip (zz-) plays under the end card and fades out; every clip
fades in and out over a few milliseconds, so no cut clicks. The audio of every clip is padded to its frames x 1472
samples, so picture and sound stay together end to end. The generator and feature clips are levelled to the demos
(LEVEL_RMS, peaks under LEVEL_PEAK): a gain, nothing else; the demos play as made.
"""
import array
import math
import shutil
import subprocess
import sys
import wave
from fractions import Fraction
from pathlib import Path

from PIL import Image, ImageDraw

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gen_brand import BG, INK, LINE, MID, centred, mark, text_w   # noqa: E402  (the Mono look's ink, font and mark)

ROOT = Path(__file__).resolve().parents[1]
FS, SPF = 44100, 1472                                   # samples a frame (demo_projects.c V_BLOCKS x 32)
FPS = Fraction(FS, SPF)
W = H = 1080
LEVEL_RMS, LEVEL_PEAK = -24.0, -3.0                     # dBFS: the demos' RMS; the highest peak a levelled clip may reach
EDGE = 0.012                                            # s: each clip's fade in and out (no click at a cut)
END_HOLD = 2.0                                          # s: the last clip at full level under the end card, then a fade


def level(a):
    """a clip's samples at LEVEL_RMS, or as loud as LEVEL_PEAK allows"""
    peak = max(abs(v) for v in a) or 1
    rms = math.sqrt(sum(v * v for v in a) / len(a)) or 1
    gain = min(10 ** (LEVEL_RMS / 20) * 32768 / rms, 10 ** (LEVEL_PEAK / 20) * 32768 / peak)
    return array.array("h", (max(-32768, min(32767, round(v * gain))) for v in a)), 20 * math.log10(gain)


def fade(a, t_in, t_out, hold=0.0):
    """stereo a: in over t_in s, out over t_out s at the end (after hold s at full level when the fade is long)"""
    n = len(a) // 2
    k_in, k_out = int(t_in * FS), int(t_out * FS)
    start = max(n - k_out, int(hold * FS)) if k_out else n
    for i in range(n):
        g = 1.0
        if i < k_in:
            g = i / k_in
        if i >= start and n > start:
            g *= max(0.0, (n - 1 - i) / (n - start)) ** 1.6 if t_out > 1 else (n - 1 - i) / (n - start)
        if g != 1.0:
            a[2 * i] = int(a[2 * i] * g)
            a[2 * i + 1] = int(a[2 * i + 1] * g)
    return a


def wrap(text, width):
    lines, line = [], ""
    for word in text.split():
        t = (line + " " + word).strip()
        if len(t) <= width:
            line = t
        else:
            lines.append(line)
            line = word
    return lines + [line] if line else lines


def card(lines):
    """a still card: the mark large, then (text, px, colour) lines centred, in the 5 x 7 font"""
    im = Image.new("RGB", (W, H), BG)
    d = ImageDraw.Draw(im)
    mark(d, W // 2, 330, 64)
    y = 520
    for text, px, colour in lines:
        for line in wrap(text, (W - 120) // (6 * px)):
            centred(d, W, y, line, px, colour)
            y += 7 * px + max(10, 3 * px)
        y += 2 * px
    return im


def caption_layer(text):
    im = Image.new("RGB", (W, H), BG)
    d = ImageDraw.Draw(im)
    mark(d, 76, 76, 14)
    from gen_brand import text as mono
    mono(d, 118, 62, "TESSERA", 4)
    d.rectangle([176, 146, 903, 873], outline=LINE, width=2)   # (the screen's frame)
    y = 914
    for line in wrap(text, (W - 80) // 24)[:3]:
        centred(d, W, y, line, 4, INK)
        y += 46
    return im


def clips(src):
    """the clips in captions order: (dir, wav, [(t0, t1, text)] in the clip's own time)"""
    rows = [l.split("\t") for l in (src / "captions.txt").read_text(encoding="utf-8").splitlines() if l.strip()]
    out = []
    for name, a, b, text in rows:
        if not out or out[-1][0] != name:
            out.append([name, float(a), []])
        out[-1][2].append((float(a) - out[-1][1], float(b) - out[-1][1], text))
    found = []
    for name, _, caps in out:
        for d in (src / name, src / "a" / name, src / "b" / name, src / "c" / name):
            if d.is_dir() and d.with_suffix(".wav").exists():
                found.append((d, d.with_suffix(".wav"), caps))
                break
        else:
            raise SystemExit(f"{name}: no frames or WAV in {src}")
    return found


def ffmpeg():
    exe = shutil.which("ffmpeg")
    if exe:
        return exe
    import imageio_ffmpeg
    return imageio_ffmpeg.get_ffmpeg_exe()


def main(src, dst):
    src, dst = Path(src), Path(dst)
    work = dst.with_suffix(".work")
    work.mkdir(parents=True, exist_ok=True)
    audio = array.array("h")
    segments = []                                       # ("card", image, frames) or ("clip", dir, caps, frames)

    def silence(frames):
        audio.extend([0] * (frames * SPF * 2))

    title = card([("TESSERA 3.0", 14, INK), ("Generative firmware for the M-Vave FM-1", 4, MID),
                  ("Every sound and screen here is the firmware's own", 3, MID)])
    gens = card([("The generators", 10, INK), ("Thirteen ways to write a part, their knobs turned", 4, MID)])
    new = card([("New in 3.0", 10, INK), ("ORACLE, the FILTER effect, GRAVITY, DEVELOP, STEP as seed, FILL, Accumulate, "
                                          "ENERGY", 4, MID)])
    shaping = card([("Shaping", 10, INK), ("TWEAK and TIMING, on any track", 4, MID)])
    before = {"g01": gens, "n01": new, "s1": shaping}  # (the clip a card comes before)
    end = card([("TESSERA 3.0", 12, INK), ("rastapopolix.github.io/tessera", 4, MID),
                ("Free, GPL-3.0. Installs from the browser.", 3, MID)])
    secs = lambda s: round(s * FS / SPF)
    segments.append(("card", title, secs(3.5)))
    silence(secs(3.5))
    for i, (d, w, caps) in enumerate(clips(src)):
        c = before.get(d.name.split("-")[0])
        if c:
            segments.append(("card", c, secs(2.5)))
            silence(secs(2.5))
        frames = len(list(d.glob("f*.ppm")))
        with wave.open(str(w)) as r:
            a = array.array("h", r.readframes(r.getnframes()))
        gain, last = 0.0, d.name.startswith("zz-")
        if d.parent == src and not last:                # (a generator or feature clip; the demos are in a/, b/, c/)
            a, gain = level(a)
        need = frames * SPF * 2
        if len(a) > need:
            raise SystemExit(f"{w}: {len(a) // 2} samples, more than {frames} frames x {SPF}")
        a.extend([0] * (need - len(a)))                 # (the last frame's tail: silence, so the clip ends in step)
        if last:
            a = fade(a, EDGE, len(a) / 2 / FS - END_HOLD, END_HOLD)   # (the music fades out under the end card)
        else:
            a = fade(a, EDGE, EDGE)
        audio.extend(a)
        segments.append(("end" if last else "clip", end if last else d, caps, frames))
        print(f"{d.name:20s} {frames} frames, {len(a) // 2} samples ({len(a) // 2 == frames * SPF and 'in step'}), "
              f"gain {gain:+.1f} dB")
    segments.append(("card", end, secs(1)))
    silence(secs(1))
    total = sum(s[-1] for s in segments)
    assert len(audio) == total * SPF * 2, (len(audio), total)
    wav = work / "audio.wav"
    with wave.open(str(wav), "wb") as w:
        w.setnchannels(2); w.setsampwidth(2); w.setframerate(FS)
        w.writeframes(audio.tobytes())
    cmd = [ffmpeg(), "-y", "-loglevel", "error", "-f", "rawvideo", "-pix_fmt", "rgb24", "-s", f"{W}x{H}",
           "-framerate", f"{FPS.numerator}/{FPS.denominator}", "-i", "-", "-i", str(wav),
           "-c:v", "libx264", "-preset", "medium", "-crf", "18", "-pix_fmt", "yuv420p",
           "-c:a", "aac", "-b:a", "192k", "-movflags", "+faststart", str(dst)]
    p = subprocess.Popen(cmd, stdin=subprocess.PIPE)
    n = 0
    for seg in segments:
        if seg[0] == "card" or seg[0] == "end":
            raw = seg[1].tobytes()
            for _ in range(seg[-1]):
                p.stdin.write(raw)
            n += seg[-1]
            continue
        _, d, caps, frames = seg
        layers = {}
        for k in range(frames):
            t = k * SPF / FS
            text = next((c[2] for c in caps if c[0] <= t < c[1]), caps[-1][2])
            if text not in layers:
                layers[text] = caption_layer(text)
            im = layers[text].copy()
            scr = Image.open(d / f"f{k:05d}.ppm").convert("RGB").resize((720, 720), Image.NEAREST)
            im.paste(scr, (180, 150))
            p.stdin.write(im.tobytes())
        n += frames
    p.stdin.close()
    if p.wait():
        raise SystemExit("ffmpeg failed")
    print(f"demo_video: {dst} ({n} frames, {n * SPF / FS:.1f} s)")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2])
