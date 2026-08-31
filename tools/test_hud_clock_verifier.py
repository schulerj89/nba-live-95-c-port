"""Synthetic corruption checks for the native clock gate; not ROM parity."""
import argparse
import copy
from pathlib import Path
import unittest

from verify_hud_clock_formatters import (compare_rows, parse, read_output,
                                         validate_rows)


class ClockVerifierIntegrity(unittest.TestCase):
    def setUp(self):
        self.rows = copy.deepcopy(NATIVE)
        self.actual = [[r['case'], r['routine'], *r['exit'], *r['exit_text']] for r in self.rows]

    def test_positive_native_shape_and_protocol(self):
        validate_rows(self.rows)
        text = '\n'.join('HUD_CLOCK ' + ' '.join(map(str, r)) for r in self.actual)
        self.assertEqual(read_output(text), self.actual)
        self.assertFalse(compare_rows(self.rows, self.actual))

    def test_duplicate_json_key_rejected(self):
        with self.assertRaises(ValueError):
            parse('{"case":1,"case":1}')

    def test_missing_duplicate_reordered_boundary_rejected(self):
        for rows in (self.rows[:-1], self.rows + self.rows[:1],
                     self.rows[:1] + self.rows[:-1],
                     [self.rows[1], self.rows[0], *self.rows[2:]]):
            with self.assertRaises(ValueError):
                validate_rows(rows)

    def test_all_native_fields_are_typed_and_shaped(self):
        for field in ('case', 'routine', 'entry_frame', 'exit_frame'):
            for bad in (True, -1, None, '1'):
                rows = copy.deepcopy(self.rows)
                rows[0][field] = bad
                with self.subTest(field=field, bad=bad), self.assertRaises(ValueError):
                    validate_rows(rows)
        for field in ('entry', 'exit', 'entry_text', 'exit_text'):
            for bad in ([], [0] * 7, [0] * 9, [True] * 8, [-1] * 8, [65536] * 8):
                rows = copy.deepcopy(self.rows)
                rows[0][field] = bad
                with self.subTest(field=field, bad=bad), self.assertRaises(ValueError):
                    validate_rows(rows)

    def test_every_exit_word_and_byte_is_compared(self):
        for row in range(35):
            for column in range(2, 18):
                altered = copy.deepcopy(self.actual)
                altered[row][column] ^= 1
                with self.subTest(row=row, column=column):
                    self.assertTrue(compare_rows(self.rows, altered))
        with self.assertRaises(ValueError):
            compare_rows(self.rows, self.actual[:-1])

    def test_malformed_c_protocol_rejected(self):
        lines = ['HUD_CLOCK ' + ' '.join(map(str, r)) for r in self.actual]
        variants = [lines[:-1], lines + lines[:1], [lines[1], lines[0], *lines[2:]],
                    [lines[0] + ' 0', *lines[1:]], [lines[0] + 'garbage', *lines[1:]],
                    [lines[0].replace('HUD_CLOCK ', 'HUD_CLOCK -', 1), *lines[1:]]]
        for values in variants:
            with self.assertRaises(ValueError):
                read_output('\n'.join(values))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', required=True, type=Path)
    args, rest = parser.parse_known_args()
    NATIVE = validate_rows([parse(line) for line in
                           (args.native / 'clock-cases.jsonl').read_text().splitlines()])
    unittest.main(argv=['test_hud_clock_verifier.py', *rest])
