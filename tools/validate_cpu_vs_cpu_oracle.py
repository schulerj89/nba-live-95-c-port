"""Validate that a Mesen gameplay JSONL is a genuine controller-free oracle."""

import argparse
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("trace")
    args = parser.parse_args()
    rows = [json.loads(line) for line in Path(args.trace).read_text().splitlines()
            if line.strip()]
    if not rows:
        raise AssertionError("CPU-vs-CPU oracle is empty")
    bad_assignments = [row["frame"] for row in rows
                       if row["controllers"]["assignment_raw"] != [65535] * 5]
    controlled = [row["frame"] for row in rows
                  if row["control"]["actor"] != -1]
    human_actor_frames = sum(actor["control"] != 0 for row in rows
                             for actor in row["actors"])
    possession_changes = sum(
        rows[index]["possession"]["actor"] !=
        rows[index - 1]["possession"]["actor"]
        for index in range(1, len(rows)))
    cpu_mode_11 = sum(actor["raw"]["control_mode"] == 11 for row in rows
                      for actor in row["actors"])
    if bad_assignments or controlled or human_actor_frames:
        raise AssertionError(
            f"not CPU-only: assignments={bad_assignments[:8]} "
            f"controlled={controlled[:8]} human_actor_frames={human_actor_frames}")
    if possession_changes < 4 or cpu_mode_11 == 0:
        raise AssertionError(
            f"oracle lacks live CPU behavior: possession_changes={possession_changes} "
            f"mode11={cpu_mode_11}")
    print(f"CPU-vs-CPU ROM oracle PASS: frames={len(rows)} "
          f"possession_changes={possession_changes} mode11_actor_frames={cpu_mode_11}")


if __name__ == "__main__":
    main()
