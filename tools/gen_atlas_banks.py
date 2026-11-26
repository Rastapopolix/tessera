#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Theo Dainis
"""ATLAS's genre banks (firmware/src/gen_atlas_banks.h): one 5 x 5 map a genre, in the layout of Grids' own map
(gen_grids.h: 25 patterns of kick, snare and hat, 32 steps each, a hit's chance 0..255), so X and Y scan and blend a
bank as they do Grids'. Written for Tessera from the genres' common conventions, not taken from records or other
makers' pattern sets.

Every bank keeps the same axes, so a turn means the same thing in each: Y picks one of the genre's five feels (a row
of FEELS below), X goes from its bare skeleton (left) to its fullest (right). A feel is three lanes of 16 sixteenths,
each hit an importance 1..9 ('.' none); column x keeps the hits of importance KEEP[x] and up, so a hit fades in as X
moves past its column. A kept hit's chance is CHANCE(importance): 9 the strongest and an accent (over 192, gen.c
gen_grids_hit), 1 a ghost DENS has to be high to let through. The 32nds between (odd steps) are left empty.

  python3 tools/gen_atlas_banks.py           writes firmware/src/gen_atlas_banks.h
  python3 tools/gen_atlas_banks.py --check   fails when the header is not this script's (tests/run_tests.sh)
"""
import os
import sys

KEEP = (8, 6, 4, 2, 1)


def CHANCE(imp):
    return 95 + imp * 17


# bank: (snare lane's note, [five feels, Y 0..4: (kick, snare, hat)]); '|' only marks the beats
BANKS = [
    ("DUB", 37, [  # the mixing desk's riddim: space, a rim for the snare, hats thin
        ("....|....|9...|....", "....|....|9...|....", "..5.|..3.|..5.|..3."),          # one drop, bare
        ("....|....|9...|....", "....|5...|9...|5.2.", "..6.|..3.|..6.|..3."),          # rim-led
        ("9...|....|9...|....", "....|7...|....|7...", "..6.|..6.|..6.|..6."),          # rockers dub
        ("9...|9...|9...|9...", "....|....|8...|....", "..7.|..4.|..7.|..4."),          # steppers dub
        ("9...|9...|9...|9..3", "....|....|9...|.2..", "..72|..52|..72|..53"),          # steppers, busier
    ]),
    ("REGGAE", 37, [  # the full drummer: one drop, rockers, steppers
        ("....|....|9...|..2.", "....|....|9...|.3..", "7141|7141|7141|7141"),          # one drop
        ("....|....|9...|....", "....|4...|9...|4..2", "8.5.|8.5.|8.5.|8.6."),          # one drop, cross-stick 2 and 4
        ("9...|....|9...|..3.", "....|8...|....|8...", "7151|7151|7151|7151"),          # rockers
        ("9...|9...|9...|9...", "....|....|9...|....", "4.8.|4.8.|4.8.|4.8."),          # steppers
        ("9...|9...|9...|9.3.", "..2.|....|9...|..3.", "4183|4183|4183|4183"),          # steppers, driving
    ]),
    ("TECHNO", 39, [
        ("9...|9...|9...|9...", "....|7...|....|7...", "1282|1282|1282|1282"),          # straight four
        ("9...|9...|9...|9...", "....|6...|....|6..3", "3.9.|3.9.|3.9.|3.9."),          # offbeat hat
        ("9...|9...|9...|9.2.", "....|7...|....|7...", "5373|5373|5373|5373"),          # rolling sixteenths
        ("9...|...6|..8.|....", "....|7...|....|7...", "3.8.|3.8.|3.8.|3.83"),          # broken kick
        ("9...|9...|9...|9.4.", "....|8..2|....|8.3.", "4383|4383|4383|4383"),          # peak
    ]),
    ("HIP-HOP", 38, [
        ("9..5|....|..8.|....", "....|9...|....|9...", "7.5.|7.5.|7.5.|7.5."),          # boom-bap
        ("9...|..5.|..8.|.4..", "....|9...|....|9..3", "6.4.|6.4.|6.4.|6.43"),          # lazy (with SWING)
        ("9..6|....|9.6.|...4", "....|9...|...3|9...", "7353|7353|7353|7353"),          # busier kick
        ("9...|....|....|..5.", "....|....|9...|....", "7.5.|7.5.|7.5.|7.5."),          # half-time
        ("9...|...6|..8.|....", "....|....|9...|...3", "7575|7575|7575|7575"),          # half-time, rolling hats
    ]),
    ("DNB", 38, [
        ("9...|....|..8.|....", "....|9...|....|9...", "6.5.|6.5.|6.5.|6.5."),          # two-step
        ("9...|....|..8.|....", "....|9..2|...3|9..2", "6353|6353|6353|6353"),          # two-step, ghosts
        ("9...|....|..7.|.5..", "....|9...|.3..|9.3.", "7574|7574|7574|7574"),          # rolling
        ("9.5.|....|..7.|..4.", "....|9..3|.4..|9..4", "6.4.|6.43|6.4.|6.43"),          # broken, jungle-leaning
        ("9...|....|....|..5.", "....|....|9...|....", "6.5.|6.5.|6.5.|6.5."),          # halftime
    ]),
    ("HOUSE", 39, [
        ("9...|9...|9...|9...", "....|7...|....|7...", "..8.|..8.|..8.|..8."),          # four on the floor
        ("9...|9...|9...|9...", "....|8...|....|8..2", "3.9.|3.9.|3.9.|3.9."),          # offbeat hat, shaker
        ("9...|9...|9...|9..3", "....|8...|..2.|8...", "5.8.|5.83|5.8.|5.83"),          # shuffled
        ("9...|9...|9...|9...", "....|5...|....|5...", "..6.|..4.|..6.|..4."),          # deep, sparse
        ("9...|9...|9...|9.3.", "....|8...|....|8.3.", "4385|4385|4385|4385"),          # peak
    ]),
    ("AMBIENT", 37, [  # mostly quiet: a pulse, clicks, brushes; DENS keeps it low
        ("9...|....|....|....", "....|....|....|....", "....|....|5...|...."),          # slow pulse
        ("9..6|....|....|....", "....|....|....|....", "....|..4.|....|..4."),          # heartbeat
        ("7...|....|....|....", "....|....|6...|....", "..4.|....|..4.|.2.."),          # soft clicks
        ("6...|....|..3.|....", "...3|....|.5..|...2", "3.2.|.3..|2..3|.2.."),          # brushes
        ("8...|....|6...|....", "....|5...|....|5..2", "4242|4242|4242|4242"),          # gentle motion
    ]),
    ("BREAKS", 38, [
        ("9...|....|..8.|....", "....|9...|....|9...", "7.6.|7.6.|7.6.|7.6."),          # plain break
        ("9.7.|....|..8.|.5..", "....|9..3|.4..|9..3", "7.5.|7.5.|7.5.|7.5."),          # funky
        ("9...|..6.|9...|....", "....|9...|....|9...", "7353|7353|7353|7353"),          # big beat
        ("9..6|..6.|....|..5.", "....|9...|..3.|9...", "6.5.|6.5.|6.5.|6.5."),          # syncopated
        ("9.7.|..6.|..8.|.6..", "...3|9..4|.4..|9.4.", "7575|7575|7575|7575"),          # busy
    ]),
    ("ELECTRO", 39, [
        ("9...|....|..8.|....", "....|9...|....|9...", "7.5.|7.5.|7.5.|7.5."),
        ("9...|..7.|...6|....", "....|9...|....|9...", "6363|6363|6363|6363"),          # skipping kick
        ("9..6|....|9...|..5.", "....|9...|....|9..3", "7.5.|7.5.|7.5.|7.55"),
        ("9...|...7|..6.|....", "....|9...|.3..|9...", "5353|5353|5353|5353"),
        ("9..7|..6.|...7|.5..", "....|9..3|....|9.3.", "7474|7474|7474|7474"),
    ]),
    ("GARAGE", 38, [
        ("9...|....|...7|....", "....|9...|....|9...", "..7.|..7.|..7.|..7."),          # two-step
        ("9...|....|..6.|.7..", "....|9...|....|9..3", "4.8.|4.83|4.8.|4.83"),
        ("9..5|....|...7|..4.", "....|9...|.3..|9...", "5.8.|5.8.|5.8.|5.8."),          # skippy
        ("9...|9...|9...|9...", "....|9...|....|9...", "4.8.|4.8.|4.8.|4.83"),          # four-four garage
        ("9..6|...5|..7.|.5..", "....|9..3|...3|9...", "4383|4383|4383|4383"),
    ]),
    ("HALFTIME", 38, [
        ("9...|....|....|....", "....|....|9...|....", "7.5.|7.5.|7.5.|7.5."),
        ("9...|....|....|..6.", "....|....|9...|....", "7.5.|7.5.|7.5.|7.5."),
        ("9...|..6.|....|.5..", "....|....|9...|...3", "6353|6353|6353|6353"),
        ("9...|....|.6..|....", "....|....|9...|..3.", "..6.|..6.|..6.|..6."),          # close to dub
        ("9..5|..6.|....|.5.4", "....|..2.|9...|.3..", "7575|7575|7575|7575"),
    ]),
    ("DOWNTEMPO", 38, [
        ("9...|....|..7.|....", "....|9...|....|9...", "6.4.|6.4.|6.4.|6.4."),
        ("9..5|....|8...|....", "....|9...|....|9...", "6.3.|6.3.|6.3.|6.3."),          # heavy
        ("9...|..5.|..7.|....", "....|9...|....|9..2", "5.4.|5.43|5.4.|5.43"),          # lazy (with SWING)
        ("9...|....|....|..5.", "....|8...|....|8...", "....|..4.|....|..4."),          # sparse
        ("9..5|..5.|..7.|.4..", "....|9..2|....|9.3.", "5353|5353|5353|5353"),
    ]),
    ("AFRO", 37, [  # a rim figure on the snare lane, a shaker on the hats
        ("9...|9...|9...|9...", "...6|..6.|...6|..6.", "7353|7353|7353|7353"),          # four-four, rim figure
        ("9...|..6.|.5..|..7.", "....|8...|....|8...", "6.5.|6.5.|6.5.|6.5."),          # off-kilter kick
        ("9..5|....|9...|.5..", "..6.|..5.|..6.|...6", "7353|7353|7353|7353"),
        ("9...|..7.|..6.|....", "...5|....|..5.|5...", "7.6.|7.6.|7.6.|7.6."),
        ("9..6|..7.|9..5|..6.", "..6.|.5..|..6.|.5.5", "7575|7575|7575|7575"),
    ]),
]

NAMES = ("ALL",) + tuple(b[0] for b in BANKS)   # (pid.c TX_N_GRBANK, gen.h GEN_AB_*: the same order)


def lane(text):
    s = text.replace("|", "")
    assert len(s) == 16 and all(c == "." or c in "123456789" for c in s), text
    return [0 if c == "." else int(c) for c in s]


def node(feel, x):
    out = []
    for text in feel:
        for imp in lane(text):
            out += [CHANCE(imp) if imp and imp >= KEEP[x] else 0, 0]
    return out


def header():
    rows = ["/* SPDX-License-Identifier: GPL-3.0-only",
            " * Copyright (C) 2026 Theo Dainis */",
            "/* ATLAS's genre banks (gen.c gen_grids_map), made by tools/gen_atlas_banks.py: edit that, not this. Each a",
            " * 5 x 5 map in Grids' layout (gen_grids.h), GEN_ATLAS_BANK[bank - 1][x * 5 + y]: Y the genre's feel, X its",
            " * skeleton to its fullest. GEN_AB_SNARE: the note the bank's snare lane plays (38 SD, 37 RIM, 39 CLAP) */",
            "#ifndef GEN_ATLAS_BANKS_H", "#define GEN_ATLAS_BANKS_H", "",
            f"_Static_assert(GEN_AB_N == {len(NAMES)}, \"gen.h GEN_AB_*: tools/gen_atlas_banks.py's banks\");",
            "static const uint8_t GEN_ATLAS_BANK[GEN_AB_N - 1u][25][96] = {"]
    for name, _, feels in BANKS:
        assert len(feels) == 5, name
        rows.append(f"    {{   /* {name} */")
        for x in range(5):
            for y in range(5):
                v = node(feels[y], x)
                rows.append("        {" + ",".join(str(b) for b in v) + f"}},   /* x {x}, y {y} */")
        rows.append("    },")
    rows.append("};")
    rows.append("static const uint8_t GEN_AB_SNARE[GEN_AB_N - 1u] = {" + ", ".join(str(b[1]) for b in BANKS) + "};")
    rows += ["", "#endif", ""]
    return "\n".join(rows)


def main():
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "firmware", "src", "gen_atlas_banks.h")
    text = header()
    if "--check" in sys.argv:
        with open(path) as f:
            if f.read() != text:
                sys.exit("gen_atlas_banks.h is not tools/gen_atlas_banks.py's: run it")
        return
    with open(path, "w") as f:
        f.write(text)
    print(f"gen_atlas_banks: {len(BANKS)} banks, {len(BANKS) * 25 * 96} B -> {os.path.normpath(path)}")


if __name__ == "__main__":
    main()
