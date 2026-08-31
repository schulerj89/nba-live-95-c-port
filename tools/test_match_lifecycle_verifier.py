"""Adversarial checks against the compiled lifecycle projection (C-only tests)."""
import argparse
import copy
import json
import tempfile
import unittest
from pathlib import Path

from verify_match_lifecycle import read_fixture, replay, audit_native

ROOT = Path(__file__).resolve().parents[1]


class LifecycleVerifierTests(unittest.TestCase):
    probe = None

    def setUp(self):
        self.path = ROOT / "tests/fixtures/match-lifecycle-expiry-witnesses.json"
        self.fixture = json.loads(self.path.read_text())
        self.cases = read_fixture(self.path)

    def validate_mutation(self, data):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "fixture.json"
            path.write_text(json.dumps(data))
            return read_fixture(path)

    def test_recorded_projection(self):
        self.assertEqual(replay(self.cases, self.probe), 4)

    def test_every_recorded_output_is_consumed(self):
        for index in range(4):
            for key in ("period", "clock", "latch"):
                with self.subTest(case=index, field=key):
                    changed = copy.deepcopy(self.cases)
                    changed[index]["result"][key] ^= 1
                    with self.assertRaisesRegex(ValueError, "native .* != C"):
                        replay(changed, self.probe)

    def test_missing_extra_duplicate_case_and_output_rejected(self):
        changes = []
        for count in (0, 3, 5):
            changed = copy.deepcopy(self.fixture)
            changed["cases"] = (changed["cases"] + changed["cases"])[:count]
            changes.append(changed)
        changed = copy.deepcopy(self.fixture)
        changed["cases"][1] = changed["cases"][0]
        changes.append(changed)
        for key in ("period", "clock", "latch"):
            changed = copy.deepcopy(self.fixture)
            del changed["cases"][0]["result"][key]
            changes.append(changed)
        changed = copy.deepcopy(self.fixture)
        changed["cases"][0]["result"]["unknown"] = 0
        changes.append(changed)
        for changed in changes:
            with self.assertRaises(ValueError):
                self.validate_mutation(changed)

    def test_non_integer_words_and_incomplete_provenance_rejected(self):
        for value in (True, 1.0, "1", -1, 65536, None):
            changed = copy.deepcopy(self.fixture)
            changed["cases"][0]["result"]["clock"] = value
            with self.assertRaises(ValueError):
                self.validate_mutation(changed)
        for field in ("system", "capture_script", "controlled_writes", "clock_setting"):
            changed = copy.deepcopy(self.fixture)
            del changed["source"][field]
            with self.assertRaises(ValueError):
                self.validate_mutation(changed)

    def test_native_event_boundary_word_type_and_order_rejected(self):
        # Synthetic event traces test the audit protocol, not ROM behavior.
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            for case in self.cases:
                directory = root / case['name']
                directory.mkdir()
                state = {'0926': case['seed']['period'], '0928': 1, '09b4': 0,
                         '4711': case['seed']['left_score'], '4791': case['seed']['right_score']}
                rows = [dict(case=case['name'], frame=i, pc=pc.replace('$', '').replace(':', '').lower(),
                             event='controlled_seed_before_native_clock_writer' if i == 0 else 'witness',
                             state=copy.deepcopy(state))
                        for i, pc in enumerate(case['ordered_witnesses'])]
                result = case['result']
                rows[-1]['state'].update({'0926': result['period'], '0928': result['clock'], '09b4': result['latch']})
                rows[-1]['event'] = 'final_postgame_handoff' if case['name'] == 'regulation_final' else 'next_period_clock_ready'
                (directory / 'events.jsonl').write_text(''.join(json.dumps(r) + '\n' for r in rows))
            self.assertEqual(len(audit_native(self.cases, root)), 4)
            path = root / 'q1/events.jsonl'
            original = path.read_text()
            for mutation in ('wrong_pc', 'boolean_word', 'reverse_frame'):
                rows = [json.loads(line) for line in original.splitlines()]
                if mutation == 'wrong_pc': rows[-1]['pc'] = '000000'
                elif mutation == 'boolean_word': rows[-1]['state']['0926'] = True
                else: rows[-1]['frame'] = 0
                path.write_text(''.join(json.dumps(r) + '\n' for r in rows))
                with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                    audit_native(self.cases, root)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", required=True)
    args = parser.parse_args()
    LifecycleVerifierTests.probe = str(Path(args.probe).resolve())
    unittest.main(argv=[__file__])
