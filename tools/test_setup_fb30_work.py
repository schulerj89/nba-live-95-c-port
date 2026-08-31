"""Integrity and malformed-protocol tests; never mutate native evidence."""
import argparse
import json
from analyze_setup_fb30_semantics import decode
import copy
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

import verify_setup_fb30_work as verify


def number_bits(value):
    if value < 4: return '1' + format(value, '02b')
    width = (value + 4).bit_length() - 1
    return '0' * (width - 2) + '1' + format(value - ((1 << width) - 4), f'0{width}b')


def synthetic_stream(depth):
    # Complete comb tree: one symbol at every shorter depth, two at the last.
    counts = [1] * (depth - 1) + [2]
    symbols = list(range(depth)) + [254]
    bits = ''.join(number_bits(v) for v in counts)
    used, previous = set(), 255
    for value in symbols:
        rank, cursor = 0, previous
        while True:
            cursor = (cursor + 1) & 255
            if cursor == value: break
            if cursor not in used: rank += 1
        bits += number_bits(rank)
        used.add(value); previous = value
    codes, first, index = {}, 0, 0
    for length, count in enumerate(counts, 1):
        first *= 2
        for code in range(first, first + count):
            codes[symbols[index]] = format(code, f'0{length}b'); index += 1
        first += count
    # Every code depth, raw literal escape, repeat-last, then peeked termination.
    bits += ''.join(codes[v] for v in range(depth))
    bits += codes[254] + number_bits(0) + '0' + format(237, '08b')
    bits += codes[254] + number_bits(7)
    bits += codes[254] + number_bits(0) + '1'
    bits += '0' * ((-len(bits)) % 8) + '0' * 32
    payload = bytes(range(depth)) + bytes([237]) * 8
    raw = bytes.fromhex('30fb00') + len(payload).to_bytes(2, 'big') + bytes([254])
    raw += bytes(int(bits[i:i + 8], 2) for i in range(0, len(bits), 8))
    return raw, payload


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
        for name in ('script', 'base_script', 'runner'):
            with self.subTest(name=name):
                self.reject_manifest(lambda m: m['sources'][name].update(sha256='0' * 64))

    def test_missing_trace_script_runner_or_snapshot_identity(self):
        for name in ('scheduler.jsonl', 'codec_instructions.jsonl', 'codec_writes.jsonl',
                     'codec_boundaries.jsonl', 'capture.lua', 'capture_runner.py',
                     'scheduler_base.lua', 'codec_20_exit.wram', 'header_01_before_wait.wram'):
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

    def test_nmi_exclusion_requires_exact_protocol(self):
        instructions = [r for r in self.instructions if r['call'] == 1]
        writes = [r for r in self.writes if r['call'] == 1]
        nmi = next(r for r in instructions if r['pc'] == 0x80815a)
        for field in ('value', 'address', 'pc'):
            bad = copy.deepcopy(writes)
            row = next(r for r in bad if r['cpu_cycles'] == nmi['cpu_cycles'] - 9)
            row[field] ^= 1
            with self.assertRaises(ValueError): verify.separate_native_effects(1, instructions, bad)

    def test_induced_wram_write_must_match_cpu_write(self):
        bad = copy.deepcopy(self.writes)
        row = next(r for r in bad if r['call'] == 1 and r['address'] == 0x7f2000)
        row['value'] ^= 1
        with self.assertRaises(ValueError): verify.separate_native_effects(1, self.instructions, bad)

    def test_trace_numeric_domains(self):
        with self.assertRaises(ValueError): verify.integer_rows([{'event': False}], {'event': 10}, 'event')
        with self.assertRaises(ValueError): verify.integer_rows([{'event': 0.0}], {'event': 10}, 'event')

    def test_source_continuation_self_tests(self):
        p = subprocess.run([str(self.exe), '--self-test', str(self.rom)], text=True, capture_output=True)
        self.assertEqual(p.returncode, 0, p.stdout + p.stderr)
        self.assertEqual(p.stdout, 'PASS: 14 FB30 continuation contract cases\n')

    def test_synthetic_canonical_depths_escape_run_and_terminator(self):
        canonical = self.rom.read_bytes()
        with tempfile.TemporaryDirectory() as directory:
            directory = Path(directory)
            rom = directory / 'synthetic.sfc'; output = directory / 'output.wram'
            for depth in range(1, 16):
                with self.subTest(depth=depth):
                    raw, expected = synthetic_stream(depth)
                    semantic = decode(raw)
                    self.assertEqual(semantic['payload'], expected)
                    blob = bytearray(canonical)
                    blob[0x170000:0x170000 + len(raw)] = raw
                    rom.write_bytes(blob)
                    result = subprocess.run([str(self.exe), str(rom), 'AE8000', str(output)],
                                            text=True, capture_output=True)
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    report = json.loads(result.stdout)
                    self.assertEqual(report['output_bytes'], len(expected))
                    scratch = output.read_bytes()
                    self.assertEqual(scratch[0x12000:0x12000 + len(expected)], expected)
                    self.assertEqual(scratch[0x100:0x100 + len(semantic['symbols'])], semantic['symbols'])
                    self.assertEqual(scratch[0x300:0x400], semantic['fast_symbols'])
                    self.assertEqual(scratch[0x400:0x500], semantic['fast_lengths'])
                    self.assertEqual(int.from_bytes(scratch[0x0c:0x0e], 'little'), 0x8000 + semantic['prefetch_offset'])
                    self.assertEqual(int.from_bytes(scratch[0x1e:0x20], 'little'), semantic['bit_buffer'])

    def test_source_preserves_zero_shift_carry_at_depth_sixteen(self):
        # $BEBE loads $14=0; $BEC0 clears carry; $BEC1 skips all shifts.
        # $BECD therefore continues construction after the canonical sum wraps.
        # Preserve that source edge behavior, bounded here by the work limit.
        raw, _ = synthetic_stream(16)
        blob = bytearray(self.rom.read_bytes()); blob[0x170000:0x170000 + len(raw)] = raw
        with tempfile.TemporaryDirectory() as directory:
            directory = Path(directory); rom = directory / 'synthetic.sfc'
            rom.write_bytes(blob)
            result = subprocess.run([str(self.exe), str(rom), 'AE8000', str(directory / 'out.wram')],
                                    text=True, capture_output=True)
            self.assertEqual(result.returncode, 1)
            report = json.loads(result.stdout)
            self.assertEqual(report['status'], 3)
            self.assertEqual(report['instructions'], 1000000)
            self.assertEqual(report['output_bytes'], 0)
            self.assertGreater(report['counts']['80BE91'], 16)

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

    def test_bad_address_tokens_reject_before_output(self):
        with tempfile.TemporaryDirectory() as directory:
            out = Path(directory) / 'output.wram'
            for source in ('AEC44g', '-EC446', '+EC446', 'AEC4460', 'AEC44', '0xAEC446'):
                p = subprocess.run([str(self.exe), str(self.rom), source, str(out)], capture_output=True)
                self.assertEqual(p.returncode, 2, source)
                self.assertFalse(out.exists())

    def test_bad_numeric_tokens_reject_before_output(self):
        with tempfile.TemporaryDirectory() as directory:
            out = Path(directory) / 'output.wram'; trace = Path(directory) / 'trace.jsonl'
            for state in ('-0,0,0,8175,128,0,0', '+0,0,0,8175,128,0,0',
                          '4294967296,0,0,8175,128,0,0', '65536,0,0,8175,128,0,0',
                          '0,0,0,8175,256,0,0', '0,0,0,8175,128,0,65536',
                          '0g,0,0,8175,128,0,0', '0,0,0,8175,128,0', '0,0,0,8175,128,0,0,0'):
                p = subprocess.run([str(self.exe), str(self.rom), 'AEA0AF', str(out), str(trace), state], capture_output=True)
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
