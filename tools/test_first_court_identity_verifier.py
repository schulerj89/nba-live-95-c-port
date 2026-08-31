"""Protocol/corruption tests only. Synthetic values are not ROM witnesses."""
import copy
import json
import unittest

from verify_first_court_identity import (
    KEYS, compare_projections, read_probe_output, strict_json, validate_projection,
)


def sample():
    return dict(context_teams=[9, 17], anchor_x=[65200, 336],
                actor_groups=[0] * 5 + [5] * 5, active_roster=list(range(5)) * 2,
                assignment_roles=list(range(5)) * 2, appearance_variants=[0] * 10,
                height_variants=[1] * 10, active_stamina_ratings=[6] * 10)


def protocol(row):
    return "FIRST_COURT_IDENTITY " + json.dumps(row) + "\n"


class Integrity(unittest.TestCase):
    def test_valid_protocol(self):
        self.assertEqual(sample(), read_probe_output("diagnostic\n" + protocol(sample())))
        self.assertEqual([], compare_projections(sample(), sample()))

    def test_every_projected_word_detects_corruption(self):
        checked = 0
        expected = sample()
        for name, count in KEYS.items():
            for i in range(count):
                actual = copy.deepcopy(expected)
                actual[name][i] ^= 1
                try:
                    self.assertTrue(compare_projections(expected, actual))
                except ValueError:
                    pass  # Invalid native domains fail before comparison.
                checked += 1
        self.assertEqual(64, checked)

    def test_every_array_has_exact_shape(self):
        for name in KEYS:
            for value in ([], sample()[name][:-1], sample()[name] + [0], {}, None, "0"):
                actual = sample(); actual[name] = value
                with self.subTest(name=name, value=value), self.assertRaises(ValueError):
                    validate_projection(actual)

    def test_strict_integer_types(self):
        for name in KEYS:
            for value in (True, False, -1, 65536, 1.0, "1", None):
                actual = sample(); actual[name][0] = value
                with self.subTest(name=name, value=value), self.assertRaises(ValueError):
                    validate_projection(actual)

    def test_native_domains_and_rank_permutation(self):
        for name, value in (("context_teams", 29), ("actor_groups", 1),
                            ("active_roster", 12), ("assignment_roles", 5),
                            ("active_stamina_ratings", 256), ("assignment_roles", 1)):
            actual = sample(); actual[name][0] = value
            with self.subTest(name=name, value=value), self.assertRaises(ValueError):
                validate_projection(actual)

    def test_output_population_and_field_schema(self):
        for output in ("", "diagnostic only", protocol(sample()) * 2,
                       "FIRST_COURT_IDENTITY {broken}"):
            with self.subTest(output=output), self.assertRaises(ValueError):
                read_probe_output(output)
        for name in KEYS:
            actual = sample(); del actual[name]
            with self.assertRaises(ValueError):
                read_probe_output(protocol(actual))
        actual = sample(); actual["unexpected"] = []
        with self.assertRaises(ValueError):
            read_probe_output(protocol(actual))

    def test_duplicate_json_keys(self):
        for text in ('{"context_teams":[],"context_teams":[]}',
                     '{"source":{"sha256":"a","sha256":"b"}}'):
            with self.assertRaises(ValueError):
                strict_json(text)


if __name__ == "__main__":
    unittest.main()
