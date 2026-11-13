#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Theo Dainis
"""Andamento: choose the code that goes to ITCM, the Seed's 64 KB of code memory with no wait states.

    itcm_pick.py build/tessera_core.o build/itcm.ld

Reads the compiled core's function sizes (arm-none-eabi-objdump) and writes the linker lines tessera_daisy.lds
includes in .itcm_text, taking the PICK list below in order while it fits in BUDGET. What doesn't fit stays in
QSPI flash: slower on a cache miss, still correct. So a core that grows never breaks the Daisy build, and the
list it prints says what moved out. The order is the priority: code that runs for every sample first (the
FM-1's own choice, .audio_text and .dsp_text, then the mixer and the engines' render loops), code that runs once
a block (every 32 samples, which the 16 KB instruction cache keeps warm anyway) last."""
import fnmatch, os, subprocess, sys

BUDGET = 60 * 1024                     # of 64 KB: the rest for the linker's veneers (ITCM <-> QSPI calls) and growth

# Whole sections the FM-1 build already marks as its hot code (firmware/app.ld)
SECTIONS = ['.audio_text', '.dsp_text']
# Then functions of tessera_core.o by name (fnmatch patterns; .part / .constprop / .isra copies match too)
PICK = [
    # per sample: the mixer, the master, the copies the engines call
    'mix_part', 'mix_spread', 'mix_prepare_batch', 'mix_cache_update', 'mix_unheard', 'master_*',
    'melodee_memset', 'melodee_memcpy', 'track_render_audio', 'track_render_unheard',
    # the engines' render loops, the most used first
    'fm6_block', 'fm6_render', 'fm6_eg_*', 'fm6_peg_*',
    'trio_render', 'trio_naive', 'trio_blep*', 'trio_osc*',
    'drum_render*', 'drum_block', 'drum_amp', 'drum_source', 'dr8_run*', 'beat_samples', 'step_samples',
    'or_render', 'or_block', 'or_env_tick',
    'phys_render', 'px_*_block', 'px_*_run', 'px_sin', 'px_exp2',
    'd9_render*', 'd9_env_exp', 'drum909_render*',
    # once a block
    'mod_voice*', 'pl_env', 'cord_block', 'mix_block',
]


def main():
    obj, out = sys.argv[1], sys.argv[2]
    objdump = os.environ.get('OBJDUMP', 'arm-none-eabi-objdump')
    funcs, secs = {}, {}               # functions in a .text.NAME section of their own: name -> size; SECTIONS' sizes
    for line in subprocess.run([objdump, '-t', obj], capture_output=True, text=True, check=True).stdout.splitlines():
        p = line.split()
        if len(p) == 6 and p[2] == 'F' and p[3] == '.text.' + p[5]:
            funcs[p[5]] = int(p[4], 16)
    for line in subprocess.run([objdump, '-h', obj], capture_output=True, text=True, check=True).stdout.splitlines():
        p = line.split()
        if len(p) >= 3 and p[1] in SECTIONS:
            secs[p[1]] = int(p[2], 16)

    used, lines, left = 0, [], []
    for s in SECTIONS:
        n = (secs.get(s, 0) + 15) & ~15
        used += n
        lines.append('*(%s*)' % s)
    taken = set()
    for pat in PICK:
        for name in sorted(funcs):
            base = name.split('.')[0]
            if name in taken or not fnmatch.fnmatchcase(base, pat):
                continue
            n = (funcs[name] + 7) & ~7
            if used + n > BUDGET:
                left.append((name, funcs[name]))
                continue
            used += n
            taken.add(name)
            lines.append('*tessera_core.o(.text.%s)' % name)
    with open(out, 'w') as f:
        f.write('/* made by itcm_pick.py from %s: %d of %d bytes */\n' % (obj, used, BUDGET))
        f.write('\n'.join(lines) + '\n')
    print('ITCM: %d B of the audio path picked (budget %d B, %d functions)' % (used, BUDGET, len(taken)))
    if left:
        print('ITCM: left in QSPI for lack of room: ' + ', '.join('%s (%d B)' % x for x in left))


if __name__ == '__main__':
    main()
