"""Resource-verifier integrity checks; these do not claim game/ROM parity.

The small synthetic DMA stream deliberately writes zeroes into an already
zero snapshot. Snapshot-only deltas cannot express that observable write
intent when replayed over a different live canvas.
"""
import copy
import csv
import tempfile
import unittest
from pathlib import Path

from setup_transition_capture import digest
from setup_transition_resource_jobs import (JOB_KEYS, SEGMENT_KEYS,
                                            expand_native_writes,
                                            read_publications, read_native_differences)


class PublicationIntegrity(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.directory = Path(self.temp.name)
        self.addCleanup(self.temp.cleanup)
        self.jobs = [
            (1, 831, 0, 8, 24, 22, 2, 0, 2, 0, 1, 0, 0),
            (2, 833, 0, 1, 24, 0x7f2360, 4096, 0x8370 // 2, 1, 0, 1, 0, -1),
            (3, 833, 0, 1, 24, 0x7f3360, 1024, 0x9370 // 2, 1, 0, 1, 0, -1),
        ]
        self.segments = [(831, 1, 0, 1), (832, 1, 1, 1),
                         (833, 2, 0, 4096), (833, 3, 0, 1024)]
        self.environment = ["directory=" + self.directory.resolve().as_posix()]
        self.write_evidence()

    def write_evidence(self):
        for name, keys, rows in (("resource_jobs.csv", JOB_KEYS, self.jobs),
                                 ("resource_writes.csv", SEGMENT_KEYS, self.segments)):
            with (self.directory / name).open("w", newline="", encoding="ascii") as stream:
                writer = csv.writer(stream)
                writer.writerow(keys)
                writer.writerows(rows)
        (self.directory / "capture_environment.txt").write_text("\n".join(self.environment) + "\n")
        self.manifest = {"artifacts": {"files": {}},
                         "isolation": {"observed_environment": self.environment}}
        for name in ("resource_jobs.csv", "resource_writes.csv", "capture_environment.txt"):
            raw = (self.directory / name).read_bytes()
            self.manifest["artifacts"]["files"][name] = {"bytes": len(raw), "sha256": digest(raw)}

    def read(self):
        return read_publications(self.directory, self.manifest)

    def expand(self, differences=None):
        return expand_native_writes(self.directory, self.manifest, "return", 831, 833,
                                    bytes(65536), differences or {})

    def test_unchanged_zero_writes_and_split_transfer_are_retained(self):
        result = self.expand()
        self.assertEqual(result[0], [(0, 0, 0)])
        self.assertEqual(result[1], [(2, 0, 0)])
        self.assertEqual(len(result[2]), 5120)
        self.assertEqual(result[2][0], (0x8370, 0, 2))
        self.assertEqual(result[2][-1], (0x976f, 0, 2))

    def test_dropped_duplicate_reordered_or_overlong_segment_rejected(self):
        originals = copy.deepcopy(self.segments)
        variants = [originals[1:], originals[:1] + originals,
                    [originals[1], originals[0]] + originals[2:],
                    [(831, 1, 0, 3)] + originals[2:]]
        for rows in variants:
            with self.subTest(rows=rows):
                self.segments = rows
                self.write_evidence()
                with self.assertRaises(ValueError):
                    self.read()

    def test_missing_or_trailing_csv_fields_rejected(self):
        for suffix in (",", ",extra"):
            self.write_evidence()
            path = self.directory / "resource_jobs.csv"
            text = path.read_text().splitlines()
            text[1] += suffix
            path.write_text("\n".join(text) + "\n")
            raw = path.read_bytes()
            self.manifest["artifacts"]["files"][path.name] = {"bytes": len(raw), "sha256": digest(raw)}
            with self.assertRaises(ValueError):
                self.read()

    def test_raw_hash_and_observed_folder_are_required(self):
        self.manifest["artifacts"]["files"]["resource_jobs.csv"]["sha256"] = "0" * 64
        with self.assertRaises(ValueError):
            self.read()
        self.environment = ["directory=C:/different-capture"]
        self.write_evidence()
        with self.assertRaises(ValueError):
            self.read()

    def test_dma_contract_domains_rejected(self):
        for index, value in ((0, 2), (1, -1), (2, 8), (3, 9), (4, 23),
                             (5, 0x1000000), (6, 0), (7, 65536), (8, 1),
                             (9, 1), (10, 32), (11, 1), (12, 256)):
            with self.subTest(index=index):
                old = self.jobs[0]
                row = list(old)
                row[index] = value
                self.jobs[0] = tuple(row)
                self.write_evidence()
                with self.assertRaises(ValueError):
                    self.read()
                self.jobs[0] = old

    def test_duplicate_reordered_and_invalid_difference_bytes_rejected(self):
        for writes in [[(0, 0), (0, 0)], [(2, 0), (0, 0)], [(False, 0)],
                       [(65536, 0)], [(0, 256)], [(0, -1)]]:
            with self.subTest(writes=writes), self.assertRaises(ValueError):
                self.expand({0: writes})

    def test_raw_difference_rows_outside_selected_range_are_validated(self):
        path = self.directory / "differences.txt"
        for text in ["831 0000 00\n834 0000 00\n834 0000 00\n",
                     "831 0000 00\n834 10000 00\n", "831 0000 00 extra\n",
                     "831 0000 00\n834 0000 100\n"]:
            path.write_text(text)
            with self.assertRaises(ValueError):
                read_native_differences(path, 831, 833, 65536)
        path.write_text("831 0000 00\n834 0000 00\n")
        self.assertEqual(read_native_differences(path, 831, 833, 65536), {0: [(0, 0)]})

    def test_snapshot_changes_require_a_dma_owner(self):
        with self.assertRaisesRegex(ValueError, "lacks a native DMA owner"):
            self.expand({0: [(1, 255)]})

    def test_fixed_source_fill_must_match_resulting_native_byte(self):
        with self.assertRaisesRegex(ValueError, "fixed-source fill disagrees"):
            self.expand({0: [(0, 255)]})

    def test_unobserved_mode1_partial_boundary_is_not_exported(self):
        self.segments = self.segments[:2] + [(833, 2, 0, 1), (833, 2, 1, 4095), self.segments[3]]
        self.write_evidence()
        with self.assertRaisesRegex(ValueError, "split mode1 DMA"):
            self.expand()

    def test_font_job_order_and_size_are_checked(self):
        row = list(self.jobs[1])
        row[5] += 2
        row[7] += 1
        self.jobs[1] = tuple(row)
        self.write_evidence()
        with self.assertRaisesRegex(ValueError, "middle of a canvas"):
            self.expand()


if __name__ == "__main__":
    unittest.main()
