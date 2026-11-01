#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Kerem Kilic (Ellic Studio)
"""Stack audit must account for callbacks, nesting and reject unknown paths."""
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import stack_budget

ROOTS = ['fm1_cstart', 'isr_alnk0', 'isr_timer5', 'fm1_core1_main', 'fm1_fault_c']


class StackBudget(unittest.TestCase):
    def analyze(self, bodies=None, extra_dis='', engine=None, usb=None):
        bodies = bodies or {}
        names = list(dict.fromkeys(ROOTS + list(bodies)))
        ir = '%struct.engine_t = type { void (i32)* }\n'
        dis = ''
        for i, name in enumerate(names):
            ir += f'define void @{name}() {{\n{bodies.get(name, "  ret void")}\n}}\n'
            dis += f'{name}:\n {0x1000+i*256:x}: 00 00 \t[--sp] = {{rets,r7-r4}}\n'
            dis += f' {0x1004+i*256:x}: 00 00 \tsp += -100\n'
        dis += extra_dis
        with tempfile.TemporaryDirectory() as directory:
            src = Path(directory)
            (src / 'core.h').write_text('typedef struct { void (*render)(int); } engine_t;')
            (src / 'seq.c').write_text('if (song.playing || cin_left\nif (started) seq_advance(0);')
            (src / 'usb_host.c').write_text(usb or '#define UH_HUBS 2u\n'
                                            'r = depth < UH_HUBS && !uh.hub[depth].addr ? uh_hub_probe(depth,\n'
                                            'r = uh_probe((uint8_t)(h + 1u), p, m);')
            (src / 'eng_test.c').write_text('.render=renderer,')
            # melodee-tess: the bank caches' and help's bounded re-entry (stack_budget.CACHE_GUARDED)
            (src / 'native_presets.c').write_text('native_fm_cached = (uint8_t)b;\n        native_index(ENGI_FM6, b);')
            (src / 'upreset.c').write_text('up_cached=(uint8_t)b;\n        memset(&up_cache,0,1);\n        up_bank_check(b,n);')
            (src / 'ui_help.c').write_text('if (!help_inner && list_help_row(&lp, &ls)) {\n help_inner = 1;\n help_find(ls);')
            if engine:
                (src / 'core.h').write_text(engine[0])
                ir = ir.replace('%struct.engine_t = type { void (i32)* }', engine[1])
            return stack_budget.analyze(ir, dis, src)

    def test_cache_guarded_reentry(self):
        report = self.analyze({'fm1_cstart': '  call void @native_fm_bank()\n  ret void',
                               'native_fm_bank': '  call void @native_index()\n  ret void',
                               'native_index': '  call void @native_fm_bank()\n  ret void'})
        self.assertEqual(report['paths']['fm1_cstart'][1],
                         ['fm1_cstart', 'native_fm_bank', 'native_index', 'native_fm_bank'])
        report = self.analyze({'fm1_cstart': '  call void @help_find()\n  ret void',
                               'help_find': '  call void @help_find()\n  ret void'})
        self.assertEqual(report['paths']['fm1_cstart'][0], 360)
        with self.assertRaisesRegex(ValueError, 'recursive stack path'):
            self.analyze({'fm1_cstart': '  call void @native_index()\n  ret void',
                          'native_index': '  call void @native_index()\n  ret void'})

    def test_inlined_calls_fold_into_callers(self):
        # audio_worker_poll is inlined everywhere (no disassembly): its job call is audio_worker_loop's
        bodies = {'fm1_core1_main': '  call void @audio_worker_loop()\n  ret void',
                  'audio_worker_loop': '  call void @audio_worker_poll()\n  ret void',
                  'job': '  call void @helper()\n  ret void', 'helper': '  ret void'}
        names = list(dict.fromkeys(ROOTS + list(bodies)))
        ir = '%struct.engine_t = type { void (i32)* }\n'
        ir += 'define void @audio_worker_poll() {\n  call void %f(i8* %c)\n  ret void\n}\n'
        dis = ''
        for i, name in enumerate(names):
            ir += f'define void @{name}() {{\n{bodies.get(name, "  ret void")}\n}}\n'
            dis += f'{name}:\n {0x1000+i*256:x}: 00 00 \tsp += -100\n'
        with tempfile.TemporaryDirectory() as directory:
            src = Path(directory)
            for f, text in (('core.h', 'typedef struct { void (*render)(int); } engine_t;'),
                            ('seq.c', 'if (song.playing || cin_left\nif (started) seq_advance(0);'),
                            ('eng_test.c', ''), ('jobs.c', 'audio_worker_submit(mode ? 0 : job, 0);'),
                            ('native_presets.c', 'native_fm_cached = (uint8_t)b; native_index(ENGI_FM6, b);'),
                            ('upreset.c', 'up_cached=(uint8_t)b; up_bank_check(b,n);'),
                            ('ui_help.c', 'if (!help_inner && list_help_row() help_inner = 1; help_find(ls);')):
                (src / f).write_text(text)
            report = stack_budget.analyze(ir, dis, src)
        self.assertEqual(report['paths']['fm1_core1_main'][1],
                         ['fm1_core1_main', 'audio_worker_loop', 'job', 'helper'])

    def test_unset_engine_callback(self):
        engine = ('typedef struct { void (*render)(int); void (*preset_make)(int); } engine_t;',
                  '%struct.engine_t = type { void (i32)*, void (i32)* }')
        report = self.analyze({'isr_alnk0': '  %a = getelementptr %struct.engine_t, %struct.engine_t* %engine, i32 0, i32 1\n  %b = load void (i32)*, void (i32)** %a\n  call void %b(i32 1)\n  ret void',
                               'renderer': '  ret void'}, engine=engine)
        self.assertEqual(report['unset_callbacks'], [('isr_alnk0', 'void (i32)')])

    def test_serial_build(self):
        # melodee-tess: MELODEE_DUAL_CORE=0 links no CPU1 root and reserves no CPU1 stacks
        report = self.analyze()
        del report['paths']['fm1_core1_main']
        self.assertEqual(stack_budget.check(report, 3000, 1000, 0), [])
        self.assertTrue(stack_budget.check(report, 3000, 1000, 4096))

    def test_nested_direct_and_tail(self):
        report = self.analyze({'fm1_cstart': '  tail call void @helper()\n  ret void', 'helper': '  ret void'})
        self.assertEqual(report['paths']['fm1_cstart'][0], 240)
        self.assertEqual(report['paths']['fm1_cstart'][1], ['fm1_cstart', 'helper'])
        self.assertEqual(stack_budget.check(report, 3000, 1000, 2000), [])
        self.assertTrue(stack_budget.check(report, 100, 100, 100))
        self.assertTrue(stack_budget.check(report, 3000, 1000, 4096, 512))
        self.assertTrue(stack_budget.check(report, 3000, 1000, 4096, 4000))
        del report['paths']['isr_timer5']
        self.assertTrue(stack_budget.check(report, 3000, 1000, 2000))

    def test_unknown_callback(self):
        with self.assertRaisesRegex(ValueError, 'unresolved stack callbacks'):
            self.analyze({'isr_alnk0': '  call void %callback(i32 1)\n  ret void'})

    def test_engine_callback(self):
        report = self.analyze({'isr_alnk0': '  %a = getelementptr %struct.engine_t, %struct.engine_t* %engine, i32 0, i32 0\n  %b = load void (i32)*, void (i32)** %a\n  call void %b(i32 1)\n  ret void',
                               'renderer': '  ret void'})
        self.assertEqual(report['paths']['isr_alnk0'][0], 240)
        self.assertIn('renderer', report['indirect']['isr_alnk0'])

    def test_unbounded_recursion(self):
        with self.assertRaisesRegex(ValueError, 'recursive stack path'):
            self.analyze({'fm1_cstart': '  call void @fm1_cstart()\n  ret void'})

    def test_bounded_hub_recursion(self):                # (two hub levels: the root probe and two below)
        report = self.analyze({'fm1_cstart': '  call void @uh_probe()\n  ret void',
                               'uh_probe': '  call void @uh_probe()\n  ret void'})
        self.assertEqual(report['paths']['fm1_cstart'][0], 480)

    def test_bounded_hub_port_helper(self):
        report = self.analyze({'fm1_cstart': '  call void @uh_probe()\n  ret void',
                               'uh_probe': '  call void @uh_hub_port_probe()\n  ret void',
                               'uh_hub_port_probe': '  call void @uh_probe()\n  ret void'})
        self.assertEqual(report['paths']['fm1_cstart'][0], 840)

    def test_idle_hub_port_helper(self):
        report = self.analyze({'fm1_cstart': '  call void @uh_hub_port_probe()\n  ret void',
                               'uh_probe': '  call void @uh_hub_port_probe()\n  ret void',
                               'uh_hub_port_probe': '  call void @uh_probe()\n  ret void'})
        self.assertEqual(report['paths']['fm1_cstart'][0], 720)

    def test_hub_levels_from_source(self):
        one = '#define UH_HUBS 1u\nr = depth < UH_HUBS && !uh.hub[depth].addr ? uh_hub_probe(depth,\n' \
              'r = uh_probe((uint8_t)(h + 1u), p, m);'
        report = self.analyze({'fm1_cstart': '  call void @uh_probe()\n  ret void',
                               'uh_probe': '  call void @uh_probe()\n  ret void'}, usb=one)
        self.assertEqual(report['paths']['fm1_cstart'][0], 360)
        with self.assertRaisesRegex(ValueError, 're-entry bounds'):
            self.analyze(usb=one.replace('UH_HUBS 1u', 'UH_HUBS 3u'))
        with self.assertRaisesRegex(ValueError, 're-entry bounds'):
            self.analyze(usb='#define UH_HUBS 2u\nreturn uh_hub_probe(depth')

    def test_unbounded_port_helper_recursion(self):
        with self.assertRaisesRegex(ValueError, 'recursive stack path'):
            self.analyze({'fm1_cstart': '  call void @uh_hub_port_probe()\n  ret void',
                          'uh_hub_port_probe': '  call void @uh_hub_port_probe()\n  ret void'})

    def test_bounded_recording_reentry(self):
        report = self.analyze({'isr_alnk0': '  call void @input_on()\n  ret void',
                               'input_on': '  call void @seq_advance()\n  ret void',
                               'seq_advance': '  call void @input_on()\n  ret void'})
        self.assertEqual(report['paths']['isr_alnk0'][0], 480)

    def test_dynamic_frame(self):
        with self.assertRaisesRegex(ValueError, 'dynamic stack frames'):
            self.analyze(extra_dis=' 2000: 00 00 \tsp += r0\n')


if __name__ == '__main__':
    unittest.main()
