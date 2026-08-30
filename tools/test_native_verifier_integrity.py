"""Adversarial protocol tests; synthetic probe output is never a native golden.

Run separately from the real executable replay gates. These tests deliberately
corrupt copies in memory; no retained fixture, native capture, or C source changes.
"""

import copy
import io
import json
import subprocess
import sys
import unittest
from contextlib import redirect_stdout
from pathlib import Path
from unittest.mock import patch

import verify_actor_commit_vectors as actor
import verify_violation_parent_vectors as violation

ROOT = Path(__file__).resolve().parents[1]


def output(fixture):
    return ('\n'.join(' '.join(f'{value:04X}' for value in call['expected'])
                      for call in fixture['calls']) + '\n').encode()


class VerifierIntegrity(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.fixtures = [
            (actor, json.loads((ROOT / 'tests/fixtures/actor-commit-edge-witnesses.json').read_text())),
            (violation, json.loads((ROOT / 'tests/fixtures/violation-oob-witnesses.json').read_text())),
            (violation, json.loads((ROOT / 'tests/fixtures/violation-parent-witnesses.json').read_text())),
        ]

    def replay(self, module, fixture, stdout=None, error=None):
        arguments = ['verify', '--vectors', 'in-memory.json', '--probe', 'not-executed']
        if module is violation:
            arguments += ['--pack', 'not-opened']
        completed = subprocess.CompletedProcess(arguments, 0,
                                                output(fixture) if stdout is None else stdout, b'')
        with patch.object(Path, 'read_text', return_value=json.dumps(fixture)), \
                patch.object(sys, 'argv', arguments), \
                patch.object(module.subprocess, 'run', return_value=completed, side_effect=error), \
                redirect_stdout(io.StringIO()) as messages:
            module.main()
            return messages.getvalue()

    def reject(self, module, fixture, stdout=None, error=None):
        with self.assertRaises((ValueError, KeyError, TypeError, SystemExit,
                                subprocess.CalledProcessError)):
            self.replay(module, fixture, stdout, error)

    def test_valid_protocol_for_all_retained_schemas(self):
        for module, fixture in self.fixtures:
            with self.subTest(schema=fixture['schema']):
                self.assertIn('PASS', self.replay(module, fixture))

    def test_empty_fixture_and_missing_case_rejected(self):
        for module, original in self.fixtures:
            for calls in ([], original['calls'][:-1]):
                with self.subTest(schema=original['schema'], calls=len(calls)):
                    fixture = copy.deepcopy(original)
                    fixture['calls'] = calls
                    self.reject(module, fixture, b'')

    def test_missing_output_and_missing_row_rejected(self):
        for module, fixture in self.fixtures:
            for stdout in (b'', b'\n', b'\n'.join(output(fixture).splitlines()[:-1])):
                with self.subTest(schema=fixture['schema'], output_bytes=len(stdout)):
                    self.reject(module, fixture, stdout)

    def test_every_output_word_required_including_actor_timer(self):
        for module, fixture in self.fixtures:
            lines = output(fixture).splitlines()
            for field in range(len(fixture['calls'][0]['expected'])):
                with self.subTest(schema=fixture['schema'], missing_field=field):
                    truncated = lines[0].split()
                    del truncated[field]
                    self.reject(module, fixture, b'\n'.join([b' '.join(truncated)] + lines[1:]))

    def test_extra_words_rows_and_unknown_logs_rejected(self):
        for module, fixture in self.fixtures:
            original = output(fixture)
            for stdout in (b'FFFF ' + original, original + original.splitlines()[0],
                           b'[UNEXPECTED] error\n' + original):
                with self.subTest(schema=fixture['schema'], prefix=stdout[:30]):
                    self.reject(module, fixture, stdout)

    def test_malformed_probe_words_rejected(self):
        for module, fixture in self.fixtures:
            original = output(fixture)
            for token in (b'GGGG', b'-001', b'10000', b'0', b'0x00'):
                with self.subTest(schema=fixture['schema'], token=token):
                    self.reject(module, fixture, token + original[4:])

    def test_expected_width_and_word_type_rejected(self):
        for module, original in self.fixtures:
            first = original['calls'][0]['expected']
            for bad in (first[:-1], first + [0], [True] + first[1:],
                        [-1] + first[1:], [65536] + first[1:], ['0'] + first[1:]):
                with self.subTest(schema=original['schema'], first=bad[:1], width=len(bad)):
                    fixture = copy.deepcopy(original)
                    fixture['calls'][0]['expected'] = bad
                    self.reject(module, fixture, output(original))

    def test_value_mutation_is_failure_not_protocol_success(self):
        for module, original in self.fixtures:
            fixture = copy.deepcopy(original)
            fixture['calls'][0]['expected'][-1] ^= 1
            with self.subTest(schema=original['schema']):
                self.reject(module, fixture, output(original))

    def test_short_images_rejected_even_when_probe_would_match(self):
        for module, original in self.fixtures:
            fixture = copy.deepcopy(original)
            if 'base_input' in fixture:
                fixture['base_input'] = fixture['base_input'][:-2]
            else:
                fixture['calls'][0]['input'] = fixture['calls'][0]['input'][:-2]
            with self.subTest(schema=fixture['schema']):
                self.reject(module, fixture, output(original))

    def test_bad_delta_controls_and_provenance_rejected(self):
        for module, original in self.fixtures[:2]:
            for mutation in ('duplicate', 'negative', 'outside', 'byte', 'hash',
                             'case', 'native_call', 'controlled', 'label'):
                fixture = copy.deepcopy(original)
                first = fixture['calls'][0]
                if mutation == 'duplicate':
                    first['patches'] += [[0, 0], [0, 0]]
                elif mutation == 'negative':
                    first['patches'].append([-1, 0])
                elif mutation == 'outside':
                    first['patches'].append([actor.WRAM_SIZE, 0])
                elif mutation == 'byte':
                    first['patches'].append([0, 256])
                elif mutation == 'hash':
                    fixture['provenance']['vectors_sha256'] = '0' * 64
                elif mutation == 'controlled':
                    first['controlled'] = False
                elif mutation == 'label':
                    first['x'] += 1
                else:
                    first[mutation] = 2
                with self.subTest(schema=fixture['schema'], mutation=mutation):
                    self.reject(module, fixture, output(original))

    def test_failed_probe_process_cannot_pass(self):
        module, fixture = self.fixtures[0]
        self.reject(module, fixture, error=subprocess.CalledProcessError(1, 'probe'))

    def test_known_violation_logs_do_not_hide_data(self):
        module, fixture = self.fixtures[1]
        logs = ("[ASSETS] Loaded asset pack: 'test.pak' (1 bytes, 1 assets)\n" +
                violation.TIPOFF_LOG + '\n').encode()
        self.assertIn('PASS', self.replay(module, fixture, logs + output(fixture)))
        self.reject(module, fixture, logs)

    def test_memory_ranges_cannot_silently_zero_fill(self):
        for snapshot in ({'mem': {}}, {'mem': {'0000': '00'}},
                         {'mem': {'4AFF': '0000'}},
                         {'mem': {'0000': '0000', '0001': '00'}}):
            with self.subTest(snapshot=snapshot), self.assertRaises(ValueError):
                actor.memory(snapshot, ((0, 0xFF),))

    def test_raw_empty_truncated_and_missing_capture_rejected(self):
        path = Path('sample.vectors.jsonl')
        entry = {'mem': {'0000': '00' * 256}}
        vector = {'call': 1, 'entry_frame': 1, 'exit_frame': 1,
                  'exit_pc': '85990f', 'entry': entry, 'exit': entry}
        meta = {'entry': '8596b5', 'exits': ['85990f'], 'max_calls': 2,
                'reads': ['0000-00ff'], 'writes': ['0000-00ff']}
        complete = 'label=sample vectors=1 orphan_exits=0 shared_exit_callbacks=0'

        def read_text(target, **kwargs):
            return json.dumps(meta) if target.name.endswith('.meta.json') else complete

        with patch.object(Path, 'read_text', read_text):
            for payload in (b'', b'\n', b'{"call":',
                            (json.dumps(vector) + '\n' + json.dumps(vector)).encode()):
                with self.subTest(payload=payload[:40]), \
                        patch.object(Path, 'read_bytes', return_value=payload), \
                        self.assertRaises(ValueError):
                    actor.raw_capture(path, {0x8596B5}, actor.ACTOR_EXITS)
            with patch.object(Path, 'read_bytes', return_value=json.dumps(vector).encode()):
                self.assertEqual(len(actor.raw_capture(path, {0x8596B5}, actor.ACTOR_EXITS)), 1)
                complete = complete.replace('vectors=1', 'vectors=2')
                with self.assertRaises(ValueError):
                    actor.raw_capture(path, {0x8596B5}, actor.ACTOR_EXITS)
        with patch.object(Path, 'read_bytes', side_effect=FileNotFoundError), \
                self.assertRaises(FileNotFoundError):
            actor.raw_capture(path, {0x8596B5}, actor.ACTOR_EXITS)

    def test_cross_frame_filter_cannot_produce_empty_pass(self):
        arguments = ['verify', '--vectors', 'sample.vectors.jsonl', '--probe', 'unused', '--pack', 'unused']
        with patch.object(sys, 'argv', arguments), \
                patch.object(violation, 'raw_capture', return_value=[{'entry_frame': 1, 'exit_frame': 2}]), \
                self.assertRaisesRegex(ValueError, 'no same-frame'):
            violation.main()


if __name__ == '__main__':
    unittest.main(verbosity=2)
