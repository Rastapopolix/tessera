#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Theo Dainis
"""Andamento: where the Daisy build put things, checked after the link.

    memcheck.py build/tessera_daisy.elf

Fails the build if a section would need copying at boot and nothing copies it: a RAM section with its contents
in flash, other than .data (libDaisy's startup), .itcm_text and .dtcm_data (tessera_memory_init in main.cpp).
That is what happens to a section name the linker script doesn't know (an "orphan"): the linker gives it a
place in RAM and its bytes in flash, and nothing ever moves them. Then prints where the audio path's main
pieces landed, so a hot function or array that has drifted out of ITCM or DTCM shows in the build log."""
import os, subprocess, sys

REGIONS = [  # (name, start, size), as tessera_daisy.lds
    ('ITCM', 0x00000000, 64 << 10), ('DTCM', 0x20000000, 128 << 10), ('AXI SRAM', 0x24000000, 512 << 10),
    ('RAM_D2_DMA', 0x30000000, 32 << 10), ('RAM_D2', 0x30008000, 256 << 10), ('RAM_D3', 0x38000000, 64 << 10),
    ('BACKUP', 0x38800000, 4 << 10), ('SDRAM', 0xC0000000, 64 << 20), ('QSPI', 0x90000000, 8 << 20),
]
COPIED = {'.data', '.itcm_text', '.dtcm_data'}
# what should be in the fast memories: (symbol, where)
EXPECT = [
    ('fm6_run', 'ITCM'), ('or_samples', 'ITCM'), ('fx_reverb', 'ITCM'), ('fx_svf', 'ITCM'), ('master_samples', 'ITCM'),
    ('trio_render', 'ITCM'), ('phys_render', 'ITCM'),
    ('trk', 'DTCM'), ('fx_state', 'DTCM'), ('FM6_SIN', 'DTCM'), ('DSP_TANH', 'DTCM'), ('SINE', 'DTCM'),
    ('resource_host_low', 'DTCM'), ('audio_batch_jobs', 'DTCM'),
    ('resource_host', 'AXI SRAM'), ('screen_buf', 'RAM_D2_DMA'),
]


def region(addr):
    for name, start, size in REGIONS:
        if start <= addr < start + size:
            return name
    return '?'


def main():
    elf = sys.argv[1]
    objdump = os.environ.get('OBJDUMP', 'arm-none-eabi-objdump')
    nm = os.environ.get('NM', 'arm-none-eabi-nm')
    bad = []
    lines = subprocess.run([objdump, '-h', '-w', elf], capture_output=True, text=True, check=True).stdout.splitlines()
    for line in lines:
        p = line.split()
        if len(p) < 7 or not p[0].isdigit():
            continue
        name, size, vma, lma, flags = p[1], int(p[2], 16), int(p[3], 16), int(p[4], 16), ' '.join(p[7:])
        if size and 'LOAD' in flags and 'ALLOC' in flags and vma != lma and name not in COPIED:
            bad.append('%s: %d B in %s with its contents in flash, and nothing copies it' % (name, size, region(vma)))
    syms = {}
    for line in subprocess.run([nm, '-C', elf], capture_output=True, text=True, check=True).stdout.splitlines():
        p = line.split()
        if len(p) >= 3:
            syms.setdefault(p[2].split('.')[0].split('::')[-1], int(p[0], 16))
    print('memcheck: where the audio path landed')
    for sym, want in EXPECT:
        if sym not in syms:
            print('  %-20s (not in the build)' % sym)
            continue
        got = region(syms[sym])
        print('  %-20s %-10s%s' % (sym, got, '' if got == want else '   << expected in %s' % want))
    for b in bad:
        print('memcheck: ERROR ' + b)
    sys.exit(1 if bad else 0)


if __name__ == '__main__':
    main()
