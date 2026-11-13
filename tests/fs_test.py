#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The sample rate as one constant (FS): every engine plays the same pitch at 48 kHz as at 44.1 kHz, and the
sounds whose numbers were set per sample keep their tuning and decays.

The FM-1 runs at 44.1 kHz; the Daisy Seed (Andamento) runs its codec at 48 kHz. FS comes from
tools/gen_tables.py (MELODEE_FS), and every rate-dependent number in the firmware is derived from it.
This builds tests/hostsim.c twice, with build/gen's tables and with tables made at MELODEE_FS=48000,
renders each engine's held note and compares the pitch (YIN, tools/sampleio.py). Then the 808 cowbell's two
oscillators (their phase steps, DR_OSC, and the band-pass after them: drum_808.c) and the decay of PHYS' modal
sounds (the modes' Q scale: phys_dsp.c PX_Q_FS).

  tests/fs_test.py OUTDIR        (after ./build.sh: build/gen; CC from the environment, default cc)

Pitch and decay are the gate. Sounds from noise (the 808's hats, snare, clap; PHYS' plucks and bows) are not
compared sample for sample: their random sequences run differently at another rate.
"""
import math
import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from sampleio import read_any_wav, yin  # noqa: E402

ENGINES = {6: "TRIO", 10: "DRUM (808 BD)", 12: "FM6", 19: "ORACLE"}
CENTS = 5.0                                   # the most two renders' pitches may differ
COWBELL = {"OCT": "1", "NOTE": "15", "SECS": "1"}   # GM 56 on the 808 kit (DRUM preset 0)
MODAL = {0: "BELL TREE", 1: "MARIMBA", 4: "KALIMBA", 9: "VIBES"}   # PHYS presets
DECAY = 0.05                                  # the most their -30 dB times may differ (5 %)


def build(out, gen):
    exe = out / f"hostsim_{gen.name}"
    cc = os.environ.get("CC", "cc").split()
    subprocess.run([*cc, "-O2", "-w", f"-I{gen}", "-I" + str(ROOT / "firmware/src"), "-I" + str(ROOT / "tests"),
                    "-o", str(exe), str(ROOT / "tests/hostsim.c"), "-lm"], check=True)
    return exe


def pitch(exe, eng, out):
    wav = out / f"{exe.name}_{eng}.wav"
    subprocess.run([str(exe), str(eng), "0", "1", str(wav)], check=True, stdout=subprocess.DEVNULL)
    sr, x = read_any_wav(wav)
    a = int(0.25 * sr)
    return sr, yin(x[a:a + 4000], sr)


def render(exe, eng, preset, env, out):
    wav = out / f"{exe.name}_{eng}_{preset}_{'_'.join(env.values())}.wav"
    subprocess.run([str(exe), str(eng), str(preset), "1", str(wav), "0"], check=True, stdout=subprocess.DEVNULL,
                   env={**os.environ, **env})
    return read_any_wav(wav)                  # (mono)


def goertzel(x, sr, hz):
    w = 2 * math.pi * hz / sr
    c, s1, s2 = 2 * math.cos(w), 0.0, 0.0
    for v in x:
        s1, s2 = v + c * s1 - s2, s1
    return s1 * s1 + s2 * s2 - c * s1 * s2


def peaks(sr, x, lo, hi):
    """the two strongest partials between lo and hi Hz: a 2 Hz scan, then the peaks to 0.05 Hz"""
    a, n = int(0.06 * sr), int(0.12 * sr)
    seg = [v * (0.5 - 0.5 * math.cos(2 * math.pi * i / n)) for i, v in enumerate(x[a:a + n])]
    scan = [(goertzel(seg, sr, f), f) for f in range(lo, hi, 2)]
    tops = [scan[k] for k in range(1, len(scan) - 1) if scan[k][0] > scan[k - 1][0] and scan[k][0] >= scan[k + 1][0]]
    fine = []
    for _, f in sorted(tops, reverse=True)[:2]:
        fine.append(max((goertzel(seg, sr, f + d / 20), f + d / 20) for d in range(-40, 41))[1])
    return sorted(fine)


def decay(sr, x, db=30.0):
    """seconds from the loudest 5 ms to the first 5 ms db below it"""
    b = int(0.005 * sr)
    env = [math.sqrt(sum(v * v for v in x[i:i + b]) / b) for i in range(0, len(x) - b, b)]
    p = max(range(len(env)), key=env.__getitem__)
    th = env[p] * 10 ** (-db / 20)
    return next(((i - p) * b / sr for i in range(p, len(env)) if env[i] < th), float("nan"))


def main():
    out = Path(sys.argv[1] if len(sys.argv) > 1 else "build/host/fs")
    out.mkdir(parents=True, exist_ok=True)
    g44, g48 = ROOT / "build/gen", out / "gen48"
    g48.mkdir(exist_ok=True)
    for h in g44.glob("*.h"):
        shutil.copy(h, g48 / h.name)
    subprocess.run([sys.executable, str(ROOT / "tools/gen_tables.py"), str(g48 / "melodee_tables.h")], check=True,
                   env={**os.environ, "MELODEE_FS": "48000"}, stdout=subprocess.DEVNULL)
    a, b = build(out, g44), build(out, g48)
    fail = 0
    for eng, name in ENGINES.items():
        (sa, fa), (sb, fb) = pitch(a, eng, out), pitch(b, eng, out)
        cents = 1200 * __import__("math").log2(fb / fa)
        ok = sa == 44100 and sb == 48000 and abs(cents) <= CENTS
        fail += not ok
        print(f"{name:14} {fa:8.2f} Hz at {sa} / {fb:8.2f} Hz at {sb}: {cents:+5.1f} cents  {'ok' if ok else 'FAILED'}")
    (sa, xa), (sb, xb) = render(a, 10, 0, COWBELL, out), render(b, 10, 0, COWBELL, out)
    pa, pb = peaks(sa, xa, 400, 1000), peaks(sb, xb, 400, 1000)
    cents = [1200 * math.log2(q / p) for p, q in zip(pa, pb)]
    ok = len(cents) == 2 and all(abs(c) <= CENTS for c in cents)
    fail += not ok
    print(f"808 cowbell    {pa[0]:.1f} + {pa[1]:.1f} Hz / {pb[0]:.1f} + {pb[1]:.1f} Hz: "
          f"{', '.join(f'{c:+.1f}' for c in cents)} cents  {'ok' if ok else 'FAILED'}")
    for preset, name in MODAL.items():
        env = {"NOTE": "7", "SECS": "3"}
        da, db = decay(*render(a, 9, preset, env, out)), decay(*render(b, 9, preset, env, out))
        ok = abs(db / da - 1) <= DECAY
        fail += not ok
        print(f"PHYS {name:9} decays to -30 dB in {da:.3f} s / {db:.3f} s: {100 * (db / da - 1):+.1f} %  "
              f"{'ok' if ok else 'FAILED'}")
    print(f"sample rate: {'FAILED' if fail else 'all engines in tune at 48 kHz, the 808 metal and PHYS decays too'}")
    return 1 if fail else 0


if __name__ == "__main__":
    sys.exit(main())
