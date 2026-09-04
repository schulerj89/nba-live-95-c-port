"""Replay Mesen defense-context witnesses through the compiled C port."""

import argparse
import json
from pathlib import Path
import subprocess


EXPECTED_PATH = {
    "early_trailing": "85b161",
    "late_trailing_nonzero": "85b166",
    "late_trailing_zero": "85b16b",
    "tied_rng_odd": "85b161",
    "tied_rng_even": "85b170",
    "inactive_preserves": "85b176",
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--vectors", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    args = parser.parse_args()
    fixture = json.loads(args.vectors.read_text())
    calls = fixture["calls"]
    if fixture.get("schema") != 1 or len(calls) != len(EXPECTED_PATH):
        raise ValueError("unexpected defense-context fixture schema")
    if {call["name"] for call in calls} != set(EXPECTED_PATH):
        raise ValueError("defense-context fixture matrix is incomplete")
    for call in calls:
        if call["entry_pc"] != "85b130" or call["exit_pc"] != "85b176":
            raise ValueError("fixture boundary differs from audited native code")
        if EXPECTED_PATH[call["name"]] not in call["native"]["executed"]:
            raise ValueError(f"native path missing for {call['name']}")
    payload = "".join(
        "{current_score:04x} {opponent_score:04x} {period_raw_0926:04x} "
        "{opponent_activity_raw_39:04x} {rng_seed:04x} "
        "{initial_mode_raw_30:04x}\n".format(**call["input"])
        for call in calls
    )
    run = subprocess.run(
        [str(args.probe)], input=payload, text=True, capture_output=True,
        check=True)
    rows = [line.split() for line in run.stdout.splitlines() if line.strip()]
    if len(rows) != len(calls):
        raise AssertionError("probe returned the wrong number of rows")
    failures = []
    for call, row in zip(calls, rows):
        actual = {
            "random_word": int(row[0], 16),
            "selected": bool(int(row[1])),
            "mode_raw_30": int(row[2], 16),
            "rng_state": int(row[3], 16),
        }
        expected = {
            "random_word": call["native"]["random_word"],
            "selected": call["native"]["selected"],
            "mode_raw_30": call["native"]["mode_raw_30"],
            "rng_state": call["native"]["random_word"],
        }
        if actual != expected:
            failures.append((call["name"], expected, actual))
    status = "PASS" if not failures else "FAIL"
    print(f"[CPU DEFENSE CONTEXT] {status}: calls={len(calls)} "
          f"mismatches={len(failures)}")
    for failure in failures:
        print(failure)
    if failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
