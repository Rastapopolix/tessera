#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Theo Dainis (melodee-tess)
"""The parameters' help texts (firmware/src/pid.c PIDS) and the help card's page texts (firmware/src/ui_help.c
HELP_PAGE), packed for the device build.

  tools/gen_help.py OUT.h

pid.c stays the source: this reads its T_ / G_ / X_ / XP_ entries (and the text macros defined among them) in order and
writes OUT.h with the texts byte-pair coded, then Huffman coded: every symbol from PID_HELP_LO up stands for two
symbols (either of which may be a pair again), the pairs chosen most frequent first, up to PID_HELP_NSYM symbols
(10 bits each in the pair table); each text is then a byte-aligned bit stream of canonical Huffman codes of its
symbols, ending in symbol 0. Texts that are equal are kept once. The help gesture expands one text at a time (pid_help_unpack.h); the device build leaves the plain texts
out (MELODEE_HELP_PACKED, firmware/src/pid.c), the host build keeps them, and tests/help_pack_test.c checks every
expanded text against them (tests/ui_test.c the page texts). The round trip is checked here too.
HELP_PAGE's texts follow PIDS' (text PID_HELP_N + hp), its H_ entries read in order: they must be in HP_ order.
"""
import argparse
import heapq
import re
import sys
from collections import Counter
from pathlib import Path

SRC = Path(__file__).resolve().parents[1]
PID = SRC / "firmware" / "src" / "pid.c"
PAGE = SRC / "firmware" / "src" / "ui_help.c"
STR = re.compile(r'"((?:[^"\\\n]|\\.)*)"')
ESC = {"\\\"": "\"", "\\\\": "\\", "\\'": "'", "\\n": "\n"}


def unquote(lit):
    out = re.sub(r"\\.", lambda m: ESC[m.group(0)], lit)          # (KeyError: an escape this does not know)
    return out


def strings(expr, macros):
    """a C expression of string literals and text macros -> its text"""
    rest, out = expr.strip(), []
    while rest:
        m = STR.match(rest)
        if m:
            out.append(unquote(m.group(1)))
        else:
            m = re.match(r"[A-Za-z_]\w*", rest)
            if not m or m.group(0) not in macros:
                raise SystemExit(f"gen_help: cannot read {rest[:60]!r}")
            out.append(macros[m.group(0)])
        rest = rest[m.end():].lstrip()
    return "".join(out)


def args_of(text, i):
    """the arguments of the call whose '(' is at text[i], and the index after its ')'"""
    depth, start, args, j = 0, i + 1, [], i
    while True:
        c = text[j]
        if c == '"':
            j = STR.match(text, j).end()
            continue
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
            if depth == 0:
                args.append(text[start:j])
                return args, j + 1
        elif c == "," and depth == 1:
            args.append(text[start:j])
            start = j + 1
        j += 1


def texts():
    src = PID.read_text()
    src = re.sub(r"/\*.*?\*/", lambda m: " " * len(m.group(0)), src, flags=re.S)
    begin = src.index("static const pid_entry_t PIDS[] = {")
    end = src.index("\n};", begin)
    body = src[begin:end].replace("\\\n", " ")
    macros, out, i = {}, [], body.index("{") + 1
    for m in re.finditer(r"^#define\s+(\w+)\s+(.*)$", body, re.M):
        macros[m.group(1)] = strings(m.group(2), macros)
    for m in re.finditer(r"\b(XP|[TGX])_\(", body):
        if body[:m.start()].rfind("#define") > body[:m.start()].rfind("\n"):
            continue
        a, _ = args_of(body, m.end() - 1)
        out.append(strings(a[-1], macros))
    return out


def page_texts():
    """ui_help.c HELP_PAGE: its H_(id, name, text) entries' texts, in order"""
    src = PAGE.read_text()
    src = re.sub(r"/\*.*?\*/", lambda m: " " * len(m.group(0)), src, flags=re.S)
    begin = src.index("static const help_t HELP_PAGE[HP_COUNT] = {")
    body = src[begin:src.index("\n};", begin)]
    return [strings(args_of(body, m.end() - 1)[0][-1], {}) for m in re.finditer(r"\bH_\(", body)]


NSYM = 1024                           # symbols: the text's bytes, then pairs; 10 bits each in PID_HELP_PAIR
W = 10
MAXL = 15                             # the longest Huffman code (a nibble in PID_HELP_LEN)


def pack(items):
    """byte-pair code the distinct texts, each ending in 0: (lo, pairs, symbol lists)"""
    raw = [t.encode("latin-1") for t in items]
    lo = max(max(r) for r in raw if r) + 1
    data, pairs = [list(r) + [0] for r in raw], []
    c = Counter((a, b) for d in data for a, b in zip(d[:-2], d[1:-1]))   # the pairs, the closing 0 left out
    for code in range(lo, NSYM):
        if not c:
            break
        pair, n = c.most_common(1)[0]
        if n < 2:
            break
        pairs.append(pair)
        for k, d in enumerate(data):
            if pair[0] not in d:
                continue
            o, i = [], 0
            while i < len(d):
                if i + 1 < len(d) - 1 and (d[i], d[i + 1]) == pair:
                    o.append(code)
                    i += 2
                else:
                    o.append(d[i])
                    i += 1
            if o != d:
                c.subtract(Counter(zip(d[:-2], d[1:-1])))
                c.update(Counter(zip(o[:-2], o[1:-1])))
                data[k] = o
        c = +c
    return lo, pairs, data


def huffman(count):
    """code lengths (at most MAXL) of the symbols counted"""
    heap = [(n, s, [s]) for s, n in count.items()]
    heapq.heapify(heap)
    length = {s: 0 for s in count}
    while len(heap) > 1:
        a, b = heapq.heappop(heap), heapq.heappop(heap)
        for s in a[2] + b[2]:
            length[s] += 1
        heapq.heappush(heap, (a[0] + b[0], min(a[1], b[1]), a[2] + b[2]))
    assert max(length.values()) <= MAXL, "gen_help: a Huffman code longer than MAXL"
    return length


def canonical(length):
    """symbol -> (code, length): codes in order of length, then of symbol"""
    codes, code, prev = {}, 0, 0
    for s in sorted(length, key=lambda s: (length[s], s)):
        code <<= length[s] - prev
        prev = length[s]
        codes[s] = (code, prev)
        code += 1
    return codes


def unpack(lo, pairs, length, d):
    """as pid_help_unpack.h: the bits decoded (canonical, by the lengths alone), each pair expanded on a stack;
    (the text, the deepest stack)"""
    count = Counter(length.values())
    out, depth, bit = [], 0, 0
    while True:
        code = first = 0
        for n in range(1, MAXL + 1):
            code |= d[bit >> 3] >> (7 - (bit & 7)) & 1
            bit += 1
            if code - first < count[n]:
                sym = sorted(s for s in length if length[s] == n)[code - first]
                break
            first = (first + count[n]) << 1
            code <<= 1
        if not sym:
            return bytes(out), depth
        stack = [sym]
        while stack:
            depth = max(depth, len(stack))
            c = stack.pop()
            if c >= lo:
                stack += [pairs[c - lo][1], pairs[c - lo][0]]
            else:
                out.append(c)


def bitpack(values, w):
    """values as w-bit fields, MSB first, and a byte of padding (the reader takes two bytes)"""
    acc, n = 0, 0
    for v in values:
        acc, n = acc << w | v, n + w
    acc <<= -n % 8
    return list(acc.to_bytes((n + 7) // 8, "big")) + [0]


def c_bytes(name, ctype, vals, per=24):
    lines = [f"static const {ctype} {name}[{max(1, len(vals))}] = {{"]
    for i in range(0, len(vals), per):
        lines.append("    " + ", ".join(map(str, vals[i:i + per])) + ",")
    return lines + ["};"]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("out")
    a = ap.parse_args()
    pids, pages = texts(), page_texts()
    items = pids + pages
    distinct = list(dict.fromkeys(items))
    lo, pairs, syms = pack(distinct)
    nsym = lo + len(pairs)
    length = huffman(Counter(s for d in syms for s in d))
    codes = canonical(length)
    data, at, deepest, longest = [], {}, 0, 0
    for t, d in zip(distinct, syms):
        acc, n = 0, 0
        for s in d:
            acc, n = acc << codes[s][1] | codes[s][0], n + codes[s][1]
        acc <<= -n % 8
        coded = list(acc.to_bytes((n + 7) // 8, "big"))
        got, depth = unpack(lo, pairs, length, coded)
        assert got == t.encode("latin-1"), f"gen_help: round trip fails for {t[:40]!r}"
        at[t] = len(data)
        data += coded
        deepest, longest = max(deepest, depth), max(longest, len(t))
    assert len(data) < 65536
    maxl = max(length.values())
    count = [sum(1 for s in length if length[s] == n) for n in range(maxl + 1)]
    nib = [length.get(s, 0) for s in range(nsym)] + [0] * (nsym & 1)
    lens = [nib[i] << 4 | nib[i + 1] for i in range(0, len(nib), 2)]
    pair_bits = bitpack([v for p in pairs for v in p], W)
    plain = sum(len(t) + 1 for t in distinct)
    lines = ["/* generated by tools/gen_help.py from firmware/src/pid.c and ui_help.c: the help texts, byte-pair coded,",
             " * then Huffman coded */",
             "#pragma once", "#include <stdint.h>", "",
             f"#define PID_HELP_N {len(pids)}            /* PIDS entries, in their order */",
             f"#define HELP_PAGE_N {len(pages)}            /* then ui_help.c HELP_PAGE's, in HP_ order */",
             f"#define PID_HELP_LO {lo}u           /* symbols from here up are pairs (PID_HELP_PAIR's 2 x {W} bits) */",
             f"#define PID_HELP_NSYM {nsym}u        /* the symbols (0 ends a text) */",
             f"#define PID_HELP_MAXL {maxl}u          /* the longest code, in bits */",
             f"#define PID_HELP_MAX {longest}         /* the longest text, without its 0 */",
             f"#define PID_HELP_DEPTH {deepest}       /* the expansion stack it needs */", ""]
    lines += c_bytes("PID_HELP_PAIR", "uint8_t", pair_bits)
    lines += ["/* each symbol's code length (0: unused), a nibble each, the even symbol's high */"]
    lines += c_bytes("PID_HELP_LEN", "uint8_t", lens)
    lines += c_bytes("PID_HELP_CNT", "uint16_t", count, 16)
    lines += c_bytes("PID_HELP_AT", "uint16_t", [at[t] for t in items], 16)
    lines += c_bytes("PID_HELP_DATA", "uint8_t", data)
    Path(a.out).parent.mkdir(parents=True, exist_ok=True)
    Path(a.out).write_text("\n".join(lines) + "\n")
    packed = len(data) + len(pair_bits) + len(lens) + 2 * len(count) + 2 * len(items)
    print(f"help: {len(pids)} + {len(pages)} texts ({len(distinct)} distinct), {plain} B -> {len(data)} B coded + "
          f"{len(pair_bits)} B pairs + {len(lens) + 2 * len(count)} B code + {2 * len(items)} B offsets = {packed} B "
          f"-> {a.out}")


if __name__ == "__main__":
    main()
