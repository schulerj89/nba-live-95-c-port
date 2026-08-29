"""Compact native `$87:A47A-$A6A4` ordinary-player outputs."""

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
        source, expected = row["input"], row["expected"]
        calls.append({"call": row["call"], "input": [
            source["move_facing"], source["mode"], source["status"],
            source["upper_state"], source["anchor_direction"],
            source["candidate_valid"], source["candidate_dx"],
            source["candidate_dy"], source["upper"], source["lower"],
            source["world_x"], source["world_y"], source["z"],
            source["screen_x"], source["screen_y"], source["head_base"],
            source["palette_offset"]], "expected": [
            expected["selected_direction"], expected["flags"],
            expected["upper"], expected["lower"], expected["head"],
            expected["attr"], expected["x"], expected["y"]]})
    Path(args.output).write_text(json.dumps({
        "routine": "$87:A47A-$A6A4 ordinary player path",
        "provenance": "natural-ROM: Mesen CPU-vs-CPU compositor inputs",
        "raw_calls": len(calls), "calls": calls,
    }, separators=(",", ":")) + "\n")


if __name__ == "__main__":
    main()
