"""Producer artifact/protocol tests; native evidence remains read-only."""
import argparse
import copy
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch
import verify_setup_header_work as verify


class IntegrityTests(unittest.TestCase):
    def reject_manifest(self, change):
        manifest = copy.deepcopy(self.manifest)
        change(manifest)
        with self.assertRaises((ValueError, AssertionError)):
            verify.validate_capture_manifest(manifest, self.native, self.rom)

    def test_missing_each_source(self):
        for name in self.manifest['sources']:
            with self.subTest(name=name):
                self.reject_manifest(lambda m: m['sources'].pop(name))

    def test_capture_source_revisions_are_pinned(self):
        for name in ('script', 'base_script', 'runner', 'codec_script', 'producer_script'):
            with self.subTest(name=name):
                self.reject_manifest(lambda m: m['sources'][name].update(sha256='0' * 64))

    def test_missing_trace_script_runner_or_snapshot_identity(self):
        for name in ('scheduler.jsonl', 'codec_instructions.jsonl', 'codec_writes.jsonl',
                     'codec_boundaries.jsonl', 'capture.lua', 'capture_runner.py',
                     'scheduler_base.lua', 'codec_base.lua', 'producer_instructions.jsonl', 'producer_bus.jsonl', 'producer_boundaries.jsonl', 'producer_04_exit.wram', 'producer_base.lua', 'header_work_instructions.jsonl', 'header_work_bus.jsonl', 'header_work_boundaries.jsonl', 'codec_20_exit.wram', 'header_01_before_wait.wram'):
            with self.subTest(name=name):
                self.reject_manifest(lambda m: m['artifacts'].pop(name))

    def test_bool_and_float_numeric_domains(self):
        for value in (True, 1.0):
            self.reject_manifest(lambda m: m.update(schema=value))
        for value in (False, 0.0):
            self.reject_manifest(lambda m: m.update(exit_code=value))
        self.reject_manifest(lambda m: m['artifacts']['codec_01_entry.wram'].update(bytes=True))

    def test_settings_cannot_change(self):
        self.reject_manifest(lambda m: m['isolation']['settings']['Snes'].update(EnableRandomPowerOnState=True))
        self.reject_manifest(lambda m: m['isolation'].pop('post_settings_sha256'))
        self.reject_manifest(lambda m: m['isolation'].update(post_settings_sha256='not a hash'))

    def test_persisted_settings_are_rehashed(self):
        real = verify.digest
        def changed(path):
            return '0' * 64 if Path(path).name == 'settings.json' else real(path)
        with patch.object(verify, 'digest', changed):
            with self.assertRaises(ValueError): verify.read_native(self.native, self.rom)

    def test_build_requires_all_sources(self):
        for name in [None, *self.build['sources']]:
            bad = copy.deepcopy(self.build)
            if name is None: bad['sources'] = {}
            else: bad['sources'].pop(name)
            with patch.object(verify, 'read_json', return_value=bad):
                with self.assertRaises(ValueError): verify.check_build(self.exe)

    def test_build_numeric_and_executable_identity(self):
        for field, value in [('schema', True), ('compiler_exit', False), ('compiler_exit', 0.0)]:
            bad = copy.deepcopy(self.build); bad[field] = value
            with patch.object(verify, 'read_json', return_value=bad):
                with self.assertRaises(ValueError): verify.check_build(self.exe)
        bad = copy.deepcopy(self.build); bad['executable']['sha256'] = '0' * 64
        with patch.object(verify, 'read_json', return_value=bad):
            with self.assertRaises(ValueError): verify.check_build(self.exe)

    def test_trace_numeric_domains(self):
        with self.assertRaises(ValueError): verify.integer_rows([{'event': False}], {'event': 10}, 'event')
        with self.assertRaises(ValueError): verify.integer_rows([{'event': 0.0}], {'event': 10}, 'event')

    def test_source_continuation_self_tests(self):
        p = subprocess.run([str(self.exe), '--self-test', str(self.rom)], text=True, capture_output=True)
        self.assertEqual(p.returncode, 0, p.stdout + p.stderr)
        self.assertEqual(p.stdout, 'PASS: 11 header continuation contract cases\n')

    def test_source_trace_rejects_backwards_and_fitted_clock_compensation(self):
        # Exact auditor failure shape: first interval negative, compensated by
        # one refresh quantum in the adjacent interval. Neither is legal work.
        base = dict(kind='instruction', pc=0x80c62b, a=0, x=0, y=0, sp=8175, db=128, ps=0)
        report = dict(cycles=12, master=80, instructions=3)
        rows = [dict(base, cycle=1, master=0), dict(base, cycle=5, master=26), dict(base, cycle=9, master=54)]
        verify.validate_source_events(rows, report)
        for change in (-40, 40, -2, 3):
            bad = copy.deepcopy(rows); bad[1]['master'] += change
            if change == -2: bad[0]['master'] = 2
            with self.assertRaises(ValueError): verify.validate_source_events(bad, report)

    def test_source_trace_rejects_mixed_event_reordering(self):
        base = dict(kind='instruction', pc=0x80c62b, a=0, x=0, y=0, sp=8175, db=128, ps=0)
        report = dict(cycles=8, master=52, instructions=2)
        first, second = dict(base, cycle=1, master=0), dict(base, cycle=5, master=26)
        write = dict(kind='write', pc=0x80c62b, cycle=4, address=0x1fef, value=0)
        verify.validate_source_events([first, write, second], report)
        with self.assertRaises(ValueError): verify.validate_source_events([first, second, write], report)
        with self.assertRaises(ValueError): verify.validate_source_events([write, first, second], report)

    def test_bad_numeric_tokens_reject_before_output(self):
        with tempfile.TemporaryDirectory() as directory:
            out = Path(directory) / 'output.wram'; trace = Path(directory) / 'trace.jsonl'
            for state in ('-0,0,0,8175,128,0,0', '+0,0,0,8175,128,0,0',
                          '4294967296,0,0,8175,128,0,0', '65536,0,0,8175,128,0,0',
                          '0,0,0,8175,256,0,0', '0,0,0,8175,128,0,65536',
                          '0g,0,0,8175,128,0,0', '0,0,0,8175,128,0', '0,0,0,8175,128,0,0,0'):
                p = subprocess.run([str(self.exe), str(self.rom), str(out), str(trace), state], capture_output=True)
                self.assertEqual(p.returncode, 2, state)
                self.assertFalse(out.exists())
                self.assertFalse(trace.exists())

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', required=True, type=Path)
    parser.add_argument('--rom', required=True, type=Path)
    parser.add_argument('--exe', required=True, type=Path)
    args = parser.parse_args()
    IntegrityTests.native = args.native.resolve()
    IntegrityTests.rom = args.rom.resolve()
    IntegrityTests.exe = args.exe.resolve()
    IntegrityTests.manifest = verify.read_json(args.native / 'manifest.json')
    IntegrityTests.build = verify.check_build(IntegrityTests.exe)
    verify.read_native(IntegrityTests.native, IntegrityTests.rom)
    _, IntegrityTests.instructions, IntegrityTests.writes = verify.native_records(IntegrityTests.native)
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(IntegrityTests))
    raise SystemExit(0 if result.wasSuccessful() else 1)


if __name__ == '__main__':
    main()
