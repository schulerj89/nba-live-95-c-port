"""Compact native `$87:A52C-$A5FA` calls into a durable selector fixture."""

import argparse
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--capture", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    calls = []
    for line in Path(args.capture).read_text().splitlines():
        row = json.loads(line)
        source = row["input"]
        calls.append({
            "call": row["call"],
            "input": [source["move_facing"], source["mode"],
                      source["status"], source["upper_state"],
                      source["anchor_direction"], source["candidate_valid"],
                      source["candidate_dx"], source["candidate_dy"]],
            "expected": row["expected"]["selected_direction"],
        })
    document = {
        "routine": "$87:A52C-$A5FA",
        "provenance": "natural-ROM: Mesen CPU-vs-CPU draw-preparation calls",
        "raw_calls": len(calls),
        "calls": calls,
    }
    Path(args.output).write_text(json.dumps(document, separators=(",", ":")) + "\n")


if __name__ == "__main__":
    main()
