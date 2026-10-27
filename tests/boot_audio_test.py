#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Kerem Kilic (Ellic Studio)
"""Independent LZMA fixtures, malformed decoder inputs and the real packed image."""
import lzma
import os
from pathlib import Path
import random
import shlex
import struct
import subprocess
import sys
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import boot_audio


def elf_symbol(path, wanted):
    """Independent ELF32 reader: verify the load address the startup uses."""
    data = path.read_bytes()
    assert data[:6] == b'\x7fELF\x01\x01'
    offset = struct.unpack_from('<I', data, 32)[0]
    stride, count = struct.unpack_from('<HH', data, 46)
    sections = [struct.unpack_from('<10I', data, offset + i * stride) for i in range(count)]
    for section in sections:
        if section[1] != 2:
            continue
        strings_section = sections[section[6]]
        strings = data[strings_section[4]:strings_section[4] + strings_section[5]]
        for pos in range(section[4], section[4] + section[5], section[9]):
            name, value = struct.unpack_from('<II', data, pos)
            if strings[name:strings.find(b'\0', name)].decode() == wanted:
                return value
    raise AssertionError('missing ELF symbol ' + wanted)


class BootAudio(unittest.TestCase):
    def test_record_and_round_trip(self):
        for raw in (b'', b'A', bytes(random.Random(741).randbytes(512))):
            data = boot_audio.pack(raw)
            magic, n, size, reserved = struct.unpack('<4I', data[:16])
            self.assertEqual((magic, size, reserved), (boot_audio.MAGIC, len(raw), 0))
            self.assertEqual(len(data), 16 + n + (-n % 4))
            self.assertEqual(lzma.decompress(data[16:16 + n], format=lzma.FORMAT_RAW, filters=boot_audio.FILTERS), raw)
        images = [b'', b'xyz', bytes(range(256)) * 4]
        self.assertEqual([size for size, _ in boot_audio.unpack_images(boot_audio.pack_images(images), 3)],
                         [len(i) for i in images])
        with self.assertRaisesRegex(ValueError, 'no load image'):
            boot_audio.unpack_images(b'\0' * 16, 1)

    def test_dependency_gate(self):
        syms = '02001000 l F .text 00000020 boot_lzma\n02001040 l F .text 00000020 helper\n'
        dis = 'boot_lzma:\n 2001000: 00 00 \tcall 64 <helper : 2001040 >\nhelper:\n 2001040: 00 00 \trts\n'
        self.assertEqual(boot_audio.check_decoder(syms, dis, 0x2001000, 0x208d000), [])
        self.assertTrue(boot_audio.check_decoder(syms.replace('02001040', '01c01040'), dis, 0x2001000, 0x208d000))
        self.assertTrue(boot_audio.check_decoder(syms, dis.replace('call 64 <helper : 2001040 >', 'call r3'), 0x2001000, 0x208d000))
        self.assertTrue(boot_audio.check_decoder(syms + '01c01000 l O .data 00000020 boot_lzma.table\n', dis, 0x2001000, 0x208d000))

    def test_decoder(self):
        rng = random.Random(9025)
        payloads = [b'', b'A', bytes(range(256)), b'ABCD' * 32768,
                    bytes(rng.randrange(256) for _ in range(8192))]
        for name in boot_audio.SECTIONS:
            p = Path('build') / name
            if p.exists() and p.stat().st_size:
                payloads.append(p.read_bytes())
        with tempfile.TemporaryDirectory(prefix='melodee-lzma-') as temp:
            root = Path(temp)
            exe = root / 'lzma'
            flags = shlex.split(os.environ.get('BOOT_SANITIZE', '-fsanitize=address,undefined'))
            subprocess.run([*shlex.split(os.environ.get('CC', 'cc')), '-O1', '-g', *flags, '-fno-omit-frame-pointer',
                            'tests/boot_lzma_test.c', '-o', str(exe)], check=True)
            for raw in payloads:
                data = boot_audio.pack(raw)
                n = struct.unpack_from('<I', data, 4)[0]
                (root / 'packed').write_bytes(data[16:16 + n])
                (root / 'raw').write_bytes(raw)
                subprocess.run([str(exe), str(root / 'packed'), str(root / 'raw'), str(len(raw))], check=True)
            # Check bytes actually emitted by the packager, not another compressor invocation.
            sections = [Path('build') / name for name in boot_audio.SECTIONS]
            if all(p.exists() for p in sections) and Path('build/melodee.bin').exists():
                image = Path('build/melodee.bin').read_bytes()
                # ROM entry maps app-file offset zero to 0x02000120.
                offset = elf_symbol(Path('build/melodee.elf'), '_rt_load') - 0x02000120
                self.assertGreaterEqual(offset, 3808)
                if image[offset:offset + 4] != struct.pack('<I', boot_audio.MAGIC):
                    # MELODEE_COMPRESS_AUDIO=0 is the raw-copy reference profile: each at its load address.
                    for name, sym in zip(boot_audio.SECTIONS, ('_rt_load', '_dsp_load', '_dt_load', '_data_load', '_audio_load')):
                        at = elf_symbol(Path('build/melodee.elf'), sym) - 0x02000120
                        raw = (Path('build') / name).read_bytes()
                        self.assertEqual(image[at:at + len(raw)], raw)
                    return
                records = boot_audio.unpack_images(image[offset:], len(sections))
                self.assertLessEqual(offset + len(boot_audio.pack_images([p.read_bytes() for p in sections])), len(image))
                for (size, data), p in zip(records, sections):
                    raw = p.read_bytes()
                    self.assertEqual(size, len(raw))
                    self.assertEqual(lzma.decompress(data, format=lzma.FORMAT_RAW, filters=boot_audio.FILTERS), raw)
                    (root / 'packed').write_bytes(data)
                    (root / 'raw').write_bytes(raw)
                    subprocess.run([str(exe), str(root / 'packed'), str(root / 'raw'), str(size)], check=True)


if __name__ == '__main__':
    unittest.main()
