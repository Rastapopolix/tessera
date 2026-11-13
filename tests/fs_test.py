#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The sample rate as one constant (FS): every engine plays the same pitch at 48 kHz as at 44.1 kHz.

The FM-1 runs at 44.1 kHz; the Daisy Seed (Andamento) runs its codec at 48 kHz. FS comes from
tools/gen_tables.py (MELODEE_FS), and every rate-dependent number in the firmware is derived from it.
This builds tests/hostsim.c twice, with build/gen's tables and with tables made at MELODEE_FS=48000,
renders each engine's held note and compares the pitch (YIN, tools/sampleio.py).

  tests/fs_test.py OUTDIR        (after ./build.sh: build/gen; CC from the environment, default cc)

Pitch is the gate. Timbre may still move at 48 kHz where a filter's coefficients are stored as numbers
made at 44.1 kHz (the 808 kit's DRF_* filters, PHYS' damping and brightness): listed in the Andamento notes.
"""
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
    print(f"sample rate: {'FAILED' if fail else 'all engines in tune at 48 kHz'}")
    return 1 if fail else 0


if __name__ == "__main__":
    sys.exit(main())
