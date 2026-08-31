"""Engineering edge/integrity checks; these are not natural ROM parity."""
import argparse
import copy
import json
from pathlib import Path
import struct
import sys
import subprocess
import unittest
from unittest.mock import patch

import verify_setup_scheduler as verify

EXE = None
ROWS = None
NATIVE = None
ROM = None


def queued(head=0, tail=8, budget=200, size=1, mode=1, palette=0):
    records = bytearray(512)
    struct.pack_into('<B3sHH', records, head, mode, bytes.fromhex('563412'), size, 0x789)
    return f'queue {head} {tail} {budget} {palette} 9 1193046 ' + records.hex()


class CEdges(unittest.TestCase):
    def run_probe(self, commands):
        return verify.probe_lines(EXE, commands)

    def test_budget_boundary_is_inclusive(self):
        self.assertEqual(self.run_probe([queued(budget=109)]),
                         ['op 1 24 255 1193046 1 1929', 'end 0 8 0 0'])

    def test_insufficient_budget_preserves_cursor_and_budget(self):
        self.assertEqual(self.run_probe([queued(budget=108)]), ['end 1 0 108 0'])
        self.assertEqual(self.run_probe([queued(budget=107)]), ['end 1 0 107 0'])

    def test_ring_wrap(self):
        self.assertEqual(self.run_probe([queued(head=504, tail=0, budget=109)]),
                         ['op 1 24 255 1193046 1 1929', 'end 0 0 0 0'])

    def test_native_palette_low_byte_quirk(self):
        self.assertEqual(self.run_probe([queued(tail=0, palette=256)]), ['end 0 0 200 256'])

    def test_native_palette_subtraction_borrow(self):
        self.assertEqual(self.run_probe([queued(tail=0, budget=1, palette=2)]),
                         ['op 0 34 255 1193046 2 9', 'end 0 0 65454 0'])

    def test_fixed_source_modes(self):
        self.assertEqual(self.run_probe([queued(mode=0xFD, budget=121)]),
                         ['op 8 24 0 1193046 1 1929', 'end 0 8 0 0'])
        self.assertEqual(self.run_probe([queued(mode=0xFC, budget=120)]),
                         ['op 8 25 255 1193046 1 1929', 'end 0 8 0 0'])

    def test_increment32_source_translation_only(self):
        self.assertEqual(self.run_probe([queued(mode=0xD6, budget=139)]),
                         ['op 1 24 129 1193046 1 1929', 'end 0 8 0 0'])

    def test_untranslated_record_rejected(self):
        self.assertEqual(self.run_probe([queued(mode=0xFE)]), ['end 2 0 200 0'])

    def test_epoch_wait_is_not_ready_during_nmi(self):
        self.assertEqual(self.run_probe(['load 71', 'state 71 0 1', 'increment',
                                        'state 72 0 1', 'state 72 0 0']),
                         ['ready 0', 'epoch 72', 'ready 0', 'ready 1'])

    def test_guard_and_epoch_wrap(self):
        self.assertEqual(self.run_probe(['load 65535', 'state 65535 1 1', 'increment',
                                        'state 65535 0 1', 'increment', 'state 0 0 0']),
                         ['ready 0', 'epoch 65535', 'ready 0', 'epoch 0', 'ready 1'])

    def test_native_m1_wait_preserves_byte_comparison(self):
        self.assertEqual(self.run_probe(['load8 397', 'state 653 0 0',
                                        'state 398 0 0']), ['ready 0', 'ready 1'])


class EvidenceIntegrity(unittest.TestCase):
    def mutated(self):
        return copy.deepcopy(ROWS)

    def test_duplicate_json_key_rejected(self):
        with self.assertRaises(ValueError):
            json.loads('{"event":1,"event":2}', object_pairs_hook=verify.pairs)

    def test_missing_queue_record_bytes_rejected(self):
        rows = self.mutated()
        next(r for r in rows if r['tag'] == 'nmi.before_publish')['queue_hex'] = '00'
        with self.assertRaisesRegex(ValueError, 'complete queue'):
            verify.queue_cases(rows)

    def test_missing_exit_rejected(self):
        rows = self.mutated()
        index = next(i for i, r in enumerate(rows) if r['tag'] == 'nmi.queue_completed')
        del rows[index]
        with self.assertRaisesRegex(ValueError, 'without observed queue'):
            verify.queue_cases(rows)

    def test_missing_dma_job_fails_semantic_comparison(self):
        rows = self.mutated()
        start, end, operations = next(case for case in verify.queue_cases(rows) if case[2])
        self.assertGreater(len(operations), 0)
        index = next(i for i, r in enumerate(rows) if r['tag'] == 'dma.submit' and
                     start['event'] < r['event'] < end['event'])
        del rows[index]
        with self.assertRaisesRegex(ValueError, 'publication differs'):
            verify.check_queues(rows, EXE)

    def test_expected_budget_is_actually_checked(self):
        rows = self.mutated()
        next(r for r in rows if r['tag'] == 'nmi.queue_completed')['queue_budget'] ^= 1
        with self.assertRaisesRegex(ValueError, 'queue state differs'):
            verify.check_queues(rows, EXE)

    def test_expected_cursor_is_actually_checked(self):
        rows = self.mutated()
        next(r for r in rows if r['tag'] == 'nmi.queue_completed')['cpu_x'] ^= 8
        with self.assertRaisesRegex(ValueError, 'queue state differs'):
            verify.check_queues(rows, EXE)

    def test_observed_palette_destination_is_checked(self):
        rows = self.mutated()
        next(r for r in rows if r['tag'] == 'dma.submit' and r['bbus'] == 0x22 and r['nmi_depth'] == 1)['ppu_cgram_address'] ^= 1
        with self.assertRaisesRegex(ValueError, 'publication differs'):
            verify.check_queues(rows, EXE)

    def test_observed_vmain_is_checked(self):
        rows = self.mutated()
        start, end, _ = next(case for case in verify.queue_cases(rows) if case[2])
        next(r for r in rows if r['tag'] == 'dma.submit' and
             start['event'] < r['event'] < end['event'])['ppu_vram_increment_high'] ^= 1
        with self.assertRaisesRegex(ValueError, 'publication differs'):
            verify.check_queues(rows, EXE)

    def test_wait_loaded_epoch_is_checked(self):
        rows = self.mutated()
        next(r for r in rows if r['tag'] == 'wait.loaded' and r['header_active'])['loaded_epoch'] ^= 1
        with self.assertRaisesRegex(ValueError, 'wait load'):
            verify.check_epochs(rows, EXE)

    def test_resume_before_interrupt_return_rejected(self):
        rows = self.mutated()
        next(r for r in rows if r['tag'] == 'wait.resume' and r['header_active'])['nmi_depth'] = 1
        with self.assertRaisesRegex(ValueError, 'must follow epoch change and RTI'):
            verify.check_epochs(rows, EXE)

    def test_byte_wait_loaded_value_is_checked(self):
        rows = self.mutated()
        next(r for r in rows if r['tag'] == 'wait.loaded' and r['cpu_ps'] & 32)['loaded_epoch'] ^= 1
        with self.assertRaisesRegex(ValueError, 'byte wait load'):
            verify.check_byte_epochs(rows, EXE)


class IdentityAndProtocolIntegrity(unittest.TestCase):
    def reject_manifest(self, name, change):
        target = NATIVE / 'manifest.json' if name == 'capture' else EXE.parent / 'build-manifest.json'
        actual = verify.read_json(target)
        mutated = copy.deepcopy(actual)
        change(mutated)
        original_reader = verify.read_json
        def reader(path):
            return mutated if Path(path).resolve() == target.resolve() else original_reader(path)
        with patch.object(verify, 'read_json', side_effect=reader), self.assertRaises(ValueError):
            verify.read_capture(NATIVE, ROM) if name == 'capture' else verify.check_build(EXE)

    def test_required_capture_artifacts_cannot_be_omitted(self):
        for name in ('scheduler.jsonl', 'observed_environment.txt', 'capture_complete.txt', 'capture.lua'):
            with self.subTest(name=name):
                self.reject_manifest('capture', lambda m: m['artifacts'].pop(name))

    def test_required_capture_sources_cannot_be_omitted(self):
        for name in ('script', 'runner', 'settings', 'rom', 'mesen'):
            with self.subTest(name=name):
                self.reject_manifest('capture', lambda m: m['sources'].pop(name))

    def test_required_build_sources_cannot_be_omitted(self):
        for name in sorted(verify.BUILD_SOURCES):
            with self.subTest(name=name):
                self.reject_manifest('build', lambda m: m['sources'].pop(name))
        self.reject_manifest('build', lambda m: m.update(sources={}))

    def test_manifest_numeric_fields_reject_bool_float_and_negative(self):
        for name, field in (('capture', 'schema'), ('capture', 'exit_code'),
                            ('build', 'schema'), ('build', 'compiler_exit')):
            for value in (True, False, 0.0, 1.0, -1):
                with self.subTest(manifest=name, field=field, value=value):
                    self.reject_manifest(name, lambda m: m.update({field: value}))

    def test_artifact_sizes_require_exact_integers(self):
        for value in (True, 1.0, -1, 2**64):
            with self.subTest(value=value):
                self.reject_manifest('capture', lambda m: m['artifacts']['scheduler.jsonl'].update(bytes=value))

    def test_settings_hash_and_values_are_required(self):
        self.reject_manifest('capture', lambda m: m['isolation'].update(post_settings_sha256='0' * 64))
        self.reject_manifest('capture', lambda m: m['isolation']['settings']['Preferences'].update(AutoLoadPatches=True))
        self.reject_manifest('capture', lambda m: m['isolation']['settings']['Snes'].update(DisableFrameSkipping=1))

    def test_source_paths_are_tied_to_the_named_files(self):
        self.reject_manifest('capture', lambda m: m['sources']['script'].update(path=m['sources']['runner']['path']))
        self.reject_manifest('build', lambda m: m['sources']['src/nba_setup_scheduler.c'].update(
            path=m['sources']['include/nba_setup_scheduler.h']['path']))

    def test_native_numeric_fields_reject_bool_float_and_wrap(self):
        for field, value in (('event', False), ('master_clock', 0.0), ('cpu_a', 65536),
                             ('cpu_ps', -1), ('queue_head', True), ('header_active', 2)):
            with self.subTest(field=field):
                rows = copy.deepcopy(ROWS)
                rows[0][field] = value
                with self.assertRaises(ValueError):
                    verify.validate_rows(rows)

    def test_probe_rejects_malformed_numeric_tokens_before_execution(self):
        for value in ('-0', '+1', '12x', '1.0', '1e0', '0x10', '65536', '4294967296', '18446744073709551616'):
            with self.subTest(value=value):
                run = subprocess.run([str(EXE)], input=f'load {value}\nstate 1 0 0\n',
                                     capture_output=True, text=True, timeout=10)
                self.assertEqual(run.returncode, 2)
                self.assertEqual(run.stdout, '')

    def test_probe_requires_exact_hex_pairs_and_line_arity(self):
        valid = queued()
        prefix, raw = valid.rsplit(' ', 1)
        for command in (prefix + ' 1g' + raw[2:], prefix + ' g1' + raw[2:],
                        prefix + ' ' + raw + '00', valid + ' trailing', 'increment extra',
                        'load 1\x00junk', 'load 1 2'):
            with self.subTest(command=command[:40]):
                run = subprocess.run([str(EXE)], input=command + '\n', capture_output=True, text=True, timeout=10)
                self.assertEqual(run.returncode, 2)
                self.assertEqual(run.stdout, '')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', required=True, type=Path)
    parser.add_argument('--rom', required=True, type=Path)
    parser.add_argument('--exe', required=True, type=Path)
    args = parser.parse_args()
    EXE = args.exe.resolve()
    NATIVE = args.native.resolve()
    ROM = args.rom.resolve()
    verify.check_build(EXE)
    ROWS = verify.read_capture(args.native, args.rom)
    unittest.main(argv=[sys.argv[0]], verbosity=2)
