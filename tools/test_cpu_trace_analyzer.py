"""Behavioral equivalence checks for compact sustained-gameplay analysis."""
import contextlib
import copy
import io
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import analyze_cpu_gameplay_trace as analyzer


def trace_rows():
    rows = []
    for index in range(2700):
        actors = [{"x": index + actor, "y": actor,
                   "screen_x": index + actor, "screen_y": actor,
                   "unused_raw": {"animation": [1, 2, 3]}}
                  for actor in range(10)]
        rows.append({
            "scene_frame": index + 1, "simulation_tick": index,
            "actors": actors,
            "possession": {"play_code_raw": index // 60 % 4,
                           "team": index // 120 % 2, "actor": 0},
            "ball": {"state": (3, 4, 5, 6)[index // 50 % 4], "owner": 0,
                     "x": index, "y": 0, "screen_x": index, "screen_y": 0},
            "camera": {"x": 0, "y": 0},
            "match": {"live_state_raw": 0},
            "fouls": {"free_throw_state_raw": 0},
            "scheduler": {"actor_pass_mask_raw": 1023},
        })
    return rows


def report(rows):
    output = io.StringIO()
    status = 0
    with patch.object(analyzer, "load_rows", return_value=rows), \
            patch("sys.argv", ["analyzer", "unused.jsonl", "--require-sustained"]), \
            contextlib.redirect_stdout(output):
        try:
            analyzer.main()
        except SystemExit as error:
            status = error.code
    return status, output.getvalue()


class CompactAnalyzerTests(unittest.TestCase):
    def test_reports_and_failures_match_full_rows(self):
        for scenario in ("valid", "stationary", "dead_ball", "detached",
                         "retained_ball", "missing_modes", "optional_scheduler"):
            with self.subTest(scenario=scenario):
                rows = trace_rows()
                for row in rows:
                    if scenario == "stationary":
                        for actor in row["actors"]:
                            actor["x"] = 0
                        row["ball"]["x"] = 0
                    elif scenario == "dead_ball" and row["scene_frame"] <= 2450:
                        row["match"]["live_state_raw"] = 0x82
                    elif scenario == "detached":
                        row["ball"]["x"] += 50
                    elif scenario == "retained_ball":
                        row["camera"]["x"] = row["scene_frame"]
                        row["ball"]["screen_x"] += row["simulation_tick"] % 2
                    elif scenario == "missing_modes":
                        row["ball"]["state"] = 4
                    elif scenario == "optional_scheduler":
                        row.pop("scheduler")
                original = copy.deepcopy(rows)
                compact = [analyzer.compact_row(row) for row in rows]
                expected = report(rows)
                self.assertEqual(report(compact), expected)
                self.assertEqual(expected[0], 0 if scenario in
                                 ("valid", "optional_scheduler") else 1)
                self.assertEqual(rows, original, "compaction mutated source rows")
                self.assertNotIn("unused_raw", compact[0]["actors"][0])

    def test_loader_blank_lines_and_malformed_json(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "trace.jsonl"
            row = trace_rows()[0]
            path.write_text("\n" + json.dumps(row) + "\n  \n", encoding="utf-8")
            self.assertEqual(analyzer.load_rows(path), [analyzer.compact_row(row)])
            path.write_text('{"scene_frame":', encoding="utf-8")
            with self.assertRaises(json.JSONDecodeError):
                analyzer.load_rows(path)


if __name__ == "__main__":
    unittest.main()
