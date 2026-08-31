"""Independent protocol mutations for the CLI input evidence reader.

Synthetic protocol doubles test rejection and exact latches, not native game
parity. The production CLI/native replay remains a separate executable gate.
"""
import csv
from pathlib import Path
import tempfile
import unittest

import test_headless_input as verifier


COLUMNS = (
    'step held pressed released native state page row '
    'working_mode working_style working_level working_quarter '
    'committed_mode committed_style committed_level committed_quarter '
    'previous pending delay speed fast'
).split() + [f'rules{i}' for i in range(13)] + [f'options{i}' for i in range(7)] + [
    f'custom{i}' for i in range(13)
] + [f'working{i}' for i in range(13)]


class HeadlessInputProtocolTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.path = Path(self.directory.name) / 'trace.csv'

    def rows(self):
        row = dict.fromkeys(COLUMNS, 0)
        row.update(step=1, state=5, page=0, row=0, working_quarter=3,
                   committed_quarter=3, working3=3)
        for i in range(4, 13):
            row[f'working{i}'] = -1
        for i, value in enumerate([0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0]):
            row[f'rules{i}'] = value
        for i, value in enumerate([30, 30, 2, 1, 0, 0, 0]):
            row[f'options{i}'] = value
        for i, value in enumerate([45, 45, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0]):
            row[f'custom{i}'] = value
        return [row]

    def write(self, rows, columns=COLUMNS):
        with self.path.open('w', newline='') as stream:
            writer = csv.writer(stream)
            writer.writerow(columns)
            for row in rows:
                writer.writerow([row.get(key, 0) for key in columns])

    def test_declared_schema_accepts_valid_protocol_double(self):
        rows = self.rows()
        self.write(rows)
        self.assertEqual(verifier.read_trace(self.path, 1), rows)

    def test_every_missing_column_is_rejected(self):
        for missing in COLUMNS:
            self.write(self.rows(), [key for key in COLUMNS if key != missing])
            with self.subTest(missing=missing), self.assertRaises(ValueError):
                verifier.read_trace(self.path, 1)

    def test_extra_and_duplicate_columns_are_rejected(self):
        for extra in ['invented', 'delay', 'step']:
            self.write(self.rows(), COLUMNS + [extra])
            with self.subTest(extra=extra), self.assertRaises(ValueError):
                verifier.read_trace(self.path, 1)

    def test_missing_extra_duplicate_and_reordered_frames_are_rejected(self):
        one = self.rows()[0]
        two = dict(one, step=2)
        for rows in [[], [one, two], [dict(one, step=2)], [one, one], [two, one]]:
            self.write(rows)
            with self.subTest(steps=[row['step'] for row in rows]), self.assertRaises(ValueError):
                verifier.read_trace(self.path, 1)

    def test_malformed_row_population_and_numeric_text_are_rejected(self):
        self.write(self.rows())
        original = self.path.read_text()
        for altered in [original.replace(',0,', ',not-an-integer,', 1),
                        original.rstrip() + ',0\n',
                        original.rsplit(',', 1)[0] + '\n']:
            self.path.write_text(altered)
            with self.subTest(value=altered[-100:]), self.assertRaises(ValueError):
                verifier.read_trace(self.path, 1)

    def test_every_input_latch_is_compared_with_complete_words(self):
        # Change buttons without releasing, then release/repress; adjacent equal
        # words remain held. All twelve SNES bits occur in the first held word.
        words = [0, 0xfff0, 0xfff0, 0x0300, 0x0100, 0, 0x0100, 0]
        rows = []
        previous = 0
        for step, native in enumerate(words, 1):
            held = verifier.host_word(native)
            rows.append(dict(step=step, held=held, pressed=held & ~previous,
                             released=previous & ~held, native=native))
            previous = held
        verifier.verify_inputs(rows, words)
        for index in range(len(rows)):
            for field in ['held', 'pressed', 'released', 'native']:
                changed = [dict(row) for row in rows]
                changed[index][field] ^= 1
                with self.subTest(index=index, field=field), self.assertRaises(AssertionError):
                    verifier.verify_inputs(changed, words)

    def test_input_population_difference_is_rejected(self):
        with self.assertRaises(ValueError):
            verifier.verify_inputs([], [0])


if __name__ == '__main__':
    unittest.main()
