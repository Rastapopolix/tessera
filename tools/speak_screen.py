#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Theo Dainis (melodee-tess)
"""The screen reader against the screen: every knob on every page, for every engine and MODE, as the screen draws it
and as the reader says it (tests/speak_test.c compare FILE). A value said differently from the one drawn is a reader
bug (3.4: GEN's MODE said STEP on an ARP track; TURING's PROB 0 % where the screen says LOCK).

  build/host/speak_test compare build/host/speak_screen.tsv && python3 tools/speak_screen.py build/host/speak_screen.tsv

A value matches when its numbers are all in what is said, or, a word, when its letters come in order in a word said
(TURNG in TURING). A knob drawn but not said, or said but not drawn, is reported too. Exit 1 when anything is."""
import re
import sys

NUM = re.compile(r'\d+(?:\.\d+)?')


def in_order(short, long):
    i = 0
    for ch in long:
        if i < len(short) and ch == short[i]:
            i += 1
    return i == len(short)


def clean(s):
    return re.sub(r'\s+', ' ', s.upper()).strip()


def main(path):
    found = {}
    for line in open(path, encoding='utf-8', errors='replace'):
        r = line.rstrip('\n').split('\t')
        if len(r) < 7:
            continue
        mode, eng, page, k, lab, drawn, said = r[:7]
        d, s = clean(drawn), clean(said)
        if lab == '(none)':
            found.setdefault(('said, not drawn', page, k), r)
            continue
        if not s:
            if d and d != '--':
                found.setdefault(('drawn, not said', page, k, lab), r)
            continue
        word = re.sub(r'[^A-Z0-9#+-]', '', d.split(' ')[0]) if d else ''
        nums = NUM.findall(d)
        if nums:
            ok = all(re.search(r'(?<![\d.])' + re.escape(n) + r'(?![\d])', s) for n in nums)
        elif word and word != '--':
            words = [re.sub(r'[^A-Z0-9#+-]', '', w) for w in s.split(' ')]
            ok = any(in_order(word, w) for w in words) or in_order(word, re.sub(r'[^A-Z0-9#+-]', '', s))
        else:
            ok = True
        if not ok:
            found.setdefault(('value differs', page, k, lab, d), r)
    for key, r in found.items():
        print(f'{key[0]}: {r[0]} {r[1]} {r[2]} KNOB {r[3]}: screen "{r[4]} {r[5].strip()}", said "{r[6]}"')
    print(f'speak vs screen: {len(found)} differences')
    return 1 if found else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1]))
