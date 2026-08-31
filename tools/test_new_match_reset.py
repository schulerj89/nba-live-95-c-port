"""Synthetic integrity tests for the verifier, not ROM-parity evidence."""
from copy import deepcopy
import json
from pathlib import Path
import unittest

from verify_new_match_reset import (compare_projection, parse_rows,
                                    strict_json, validate_fixture)


class NewMatchVerifierTests(unittest.TestCase):
    def setUp(self):
        self.fixture = strict_json((Path(__file__).resolve().parents[1] /
            'tests/fixtures/new-match-native-start.json').read_text())

    def test_original_fixture_is_valid(self):
        validate_fixture(self.fixture)

    def test_duplicate_keys_rejected(self):
        for text in ('{"period":0,"period":5}',
                     '{"native":{"scores":[0,0],"scores":[1,1]}}'):
            with self.assertRaises(ValueError):
                strict_json(text)

    def test_every_projected_value_must_match(self):
        original = self.fixture['native']
        mutated = deepcopy(original)
        mutated['period'] += 1
        with self.assertRaises(ValueError):
            compare_projection(mutated, original)
        for key in ('scores', 'timeouts'):
            for i in range(2):
                mutated = deepcopy(original)
                mutated[key][i] += 1
                with self.assertRaises(ValueError):
                    compare_projection(mutated, original)
        for side in range(2):
            for i in range(12):
                # Preserve a valid permutation while changing every slot.
                mutated = deepcopy(original)
                order = mutated['roster_order'][side]
                other = (i + 1) % 12
                order[i], order[other] = order[other], order[i]
                with self.assertRaises(ValueError):
                    compare_projection(mutated, original)

    def test_shapes_and_extra_fields_rejected(self):
        for key in ('scores', 'timeouts', 'roster_order'):
            for value in (None, {}, [], [0], [0, 0, 0]):
                fixture = deepcopy(self.fixture)
                fixture['native'][key] = value
                with self.assertRaises(ValueError):
                    validate_fixture(fixture)
        fixture = deepcopy(self.fixture)
        fixture['native']['roster_order'][0].pop()
        with self.assertRaises(ValueError):
            validate_fixture(fixture)
        fixture = deepcopy(self.fixture)
        fixture['native']['extra'] = 0
        with self.assertRaises(ValueError):
            validate_fixture(fixture)

    def test_uint16_types_and_permutation(self):
        for value in (True, False, -1, 65536, 0.0, '0', None):
            for key in ('period', 'scores', 'timeouts', 'roster_order'):
                fixture = deepcopy(self.fixture)
                if key == 'period':
                    fixture['native'][key] = value
                elif key == 'roster_order':
                    fixture['native'][key][0][0] = value
                else:
                    fixture['native'][key][0] = value
                with self.assertRaises(ValueError):
                    validate_fixture(fixture)

    def test_provenance_rejected(self):
        for key, value in (('snapshot_sha256', 'bad'),
                           ('run_manifest_sha256', 'g' * 64),
                           ('boundary', 'other'), ('kind', 'C-only'),
                           ('snapshot', '../elsewhere'), ('caveat', '')):
            fixture = deepcopy(self.fixture)
            fixture['source'][key] = value
            with self.assertRaises(ValueError):
                validate_fixture(fixture)
        fixture = deepcopy(self.fixture)
        del fixture['source']['controlled_setup']
        with self.assertRaises(ValueError):
            validate_fixture(fixture)
        fixture = deepcopy(self.fixture)
        fixture['schema'] = True
        with self.assertRaises(ValueError):
            validate_fixture(fixture)

    def test_output_rows_are_strict(self):
        rows = [dict(self.fixture['native'], side=side) for side in (0, 1)]
        def output(values):
            return '\n'.join('NEW_MATCH_PROJECTION ' + json.dumps(value)
                             for value in values)
        self.assertEqual(len(parse_rows(output(rows))), 2)
        for values in ([], rows[:1], rows + rows, rows[::-1]):
            with self.assertRaises(ValueError):
                parse_rows(output(values))
        for value in (True, '0', None, 2):
            changed = deepcopy(rows)
            changed[0]['side'] = value
            with self.assertRaises(ValueError):
                parse_rows(output(changed))
        with self.assertRaises(ValueError):
            parse_rows('NEW_MATCH_PROJECTION {broken}')


if __name__ == '__main__':
    unittest.main()
