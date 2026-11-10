#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The web simulator builds the device's engine set: every engine flag tools/build.py gives the firmware,
web/sim/build.sh gives the simulator, with the same default (issue #593: the simulator kept DRUM's SID KIT
and SID DIGI, which the device leaves out)."""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ENGINE_FLAGS = ("MELODEE_FM6", "MELODEE_SID", "MELODEE_SID_KIT", "MELODEE_PHYS", "MELODEE_PLAITS",
                "MELODEE_TRIO", "MELODEE_ACCESS", "MELODEE_LOOK")


def main():
    py = (ROOT / "tools/build.py").read_text()
    sh = (ROOT / "web/sim/build.sh").read_text()
    dev = dict(re.findall(r'\(\s*"(MELODEE_[A-Z0-9_]+)",\s*"([01])"\s*\)', py))            # the engine set's tuples
    dev.update(re.findall(r'os\.environ\.setdefault\(\s*"(MELODEE_[A-Z0-9_]+)",\s*"([01])"\s*\)', py))
    sim = dict(re.findall(r"-D(MELODEE_[A-Z0-9_]+)=\$\{\1:-([01])\}", sh))
    fails = 0
    for f in ENGINE_FLAGS:
        if f not in dev:
            continue                                     # the firmware takes core.h's default: so does the simulator
        ok = sim.get(f) == dev[f]
        print(f"sim flags: {f:<16} device {dev[f]}, simulator {sim.get(f, 'core.h default')}  {'ok' if ok else 'FAIL'}")
        fails += not ok
    if not dev.get("MELODEE_SID_KIT"):
        print("sim flags: tools/build.py no longer names MELODEE_SID_KIT  FAIL"); fails += 1
    print("sim flags:", "FAILED" if fails else "all checks ok")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
