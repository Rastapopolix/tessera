#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The header messages fit: every literal text ui_message() or ui_say() shows is at most 15 characters, what the
Mono header has room for (ui_mono.c mo_cut(.., ui.msg, 15): a longer one is cut, not ellipsised, so the render lint
does not see it). ui_say's two literals count together; the second message (a literal copied into ui.msg2) too."""
import glob
import re
import sys

ROOM = 15
CALL = re.compile(r'\bui_(message|say)\s*\(')
MSG2 = re.compile(r'\bstr_cpy\s*\(\s*ui\.msg2\s*,\s*"((?:[^"\\]|\\.)*)"')
LIT = re.compile(r'"((?:[^"\\]|\\.)*)"')


def args_of(src, at):
    """the text of the call's arguments, from its '(' at `at` to the matching ')'"""
    depth, i, quote = 0, at, False
    while i < len(src):
        c = src[i]
        if quote:
            if c == '\\':
                i += 1
            elif c == '"':
                quote = False
        elif c == '"':
            quote = True
        elif c == '(':
            depth += 1
        elif c == ')':
            depth -= 1
            if not depth:
                return src[at + 1:i]
        i += 1
    return ''


def split_top(args):
    """the arguments at the top level (commas outside parentheses and strings)"""
    out, depth, quote, cur, i = [], 0, False, '', 0
    while i < len(args):
        c = args[i]
        cur += c
        if quote:
            if c == '\\':
                i += 1
                cur += args[i] if i < len(args) else ''
            elif c == '"':
                quote = False
        elif c == '"':
            quote = True
        elif c in '([':
            depth += 1
        elif c in ')]':
            depth -= 1
        elif c == ',' and not depth:
            out.append(cur[:-1])
            cur = ''
        i += 1
    out.append(cur)
    return out


def texts(arg):
    """the literals an argument may show: each alternative of a ?: (adjacent literals joined)"""
    alts = re.split(r'[?:]', re.sub(r'"(?:[^"\\]|\\.)*"', lambda m: m.group(0).replace('?', '\x01').replace(':', '\x02'), arg))
    out = []
    for a in alts:
        lits = LIT.findall(a.replace('\x01', '?').replace('\x02', ':'))
        if lits:
            out.append(bytes(''.join(lits), 'utf-8').decode('unicode_escape'))
    return out


def main():
    bad = 0
    for path in sorted(glob.glob('firmware/src/*.c')):
        src = open(path, encoding='utf-8', errors='replace').read()
        for m in CALL.finditer(src):
            if src[max(0, m.start() - 12):m.start()].rstrip().endswith('void'):   # (the definitions)
                continue
            args = split_top(args_of(src, m.end() - 1))
            line = src.count('\n', 0, m.start()) + 1
            if m.group(1) == 'message':
                shown = texts(args[0])
            else:                                   # ui_say(a, b): both literals, or the literal part
                a, b = texts(args[0]), texts(args[1]) if len(args) > 1 else []
                shown = [x + y for x in (a or ['']) for y in (b or [''])]
            for s in shown:
                if len(s) > ROOM:
                    print(f'{path}:{line}: "{s}" is {len(s)} characters (room: {ROOM})')
                    bad += 1
        for m in MSG2.finditer(src):
            s = bytes(m.group(1), 'utf-8').decode('unicode_escape')
            if len(s) > ROOM:
                line = src.count('\n', 0, m.start()) + 1
                print(f'{path}:{line}: "{s}" (second message) is {len(s)} characters (room: {ROOM})')
                bad += 1
    print(f'messages: {bad} too long' if bad else 'messages: every literal fits the header')
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
