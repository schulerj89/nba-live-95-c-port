"""Summarize sustained CPU decisions, movement, and ball physics from JSONL."""

import argparse
import json
import math
from pathlib import Path


def load_rows(path):
    return [json.loads(line) for line in Path(path).read_text().splitlines()
            if line.strip()]


def transitions(rows, getter, start):
    result = []
    previous = object()
    for row in rows:
        if row["scene_frame"] < start:
            continue
        value = getter(row)
        if value != previous:
            result.append((row["scene_frame"], value))
            previous = value
    return result


def movement_count(rows, actor, first, last):
    count = 0
    for index in range(max(first + 1, 1), min(last + 1, len(rows))):
        current, previous = rows[index]["actors"][actor], rows[index - 1]["actors"][actor]
        count += (current["x"], current["y"]) != (previous["x"], previous["y"])
    return count


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("trace")
    parser.add_argument("--start", type=int, default=220)
    parser.add_argument("--window", type=int, default=240)
    parser.add_argument("--require-sustained", action="store_true")
    args = parser.parse_args()
    rows = load_rows(args.trace)
    if not rows:
        raise SystemExit("trace is empty")

    play_changes = transitions(
        rows, lambda row: (row["possession"]["play_code_raw"],
                           row["possession"]["team"]), args.start)
    ball_changes = transitions(
        rows, lambda row: (row["ball"]["state"], row["ball"]["owner"]),
        args.start)
    print(f"[CPU TRACE] frames={len(rows)} scene={rows[0]['scene_frame']}..{rows[-1]['scene_frame']}")
    print("[CPU TRACE] plays=" + ", ".join(
        f"f{frame}:${value[0]:02X}/team{value[1]}" for frame, value in play_changes))
    print("[CPU TRACE] ball=" + ", ".join(
        f"f{frame}:mode{value[0]}/owner{value[1]}" for frame, value in ball_changes[:24]))

    weak_windows = []
    final_frame = rows[-1]["scene_frame"]
    for first in range(args.start, final_frame + 1, args.window):
        last = min(first + args.window - 1, final_frame)
        counts = [movement_count(rows, actor, first, last) for actor in range(10)]
        team_counts = (sum(counts[:5]), sum(counts[5:]))
        print(f"[CPU TRACE] movement f{first}-{last} teams={team_counts[0]}/{team_counts[1]} "
              f"actors={'/'.join(map(str, counts))}")
        span = last - first + 1
        if min(team_counts) < max(4, span // 4):
            weak_windows.append((first, last, team_counts))

    attached_distances = []
    for row in rows:
        owner = row["ball"]["owner"]
        if row["ball"]["state"] == 4 and 0 <= owner < 10:
            actor = row["actors"][owner]
            attached_distances.append(math.hypot(
                row["ball"]["x"] - actor["x"],
                row["ball"]["y"] - actor["y"]))
    if attached_distances:
        print(f"[CPU TRACE] attached-distance max={max(attached_distances):.2f} "
              f"samples={len(attached_distances)}")

    if args.require_sustained:
        errors = []
        if final_frame < 1500:
            errors.append("trace does not cover 1,500 gameplay frames")
        play_codes = {value[0] for _, value in play_changes}
        if len(play_codes) < 4:
            errors.append(f"only {len(play_codes)} recurring play codes")
        modes = {row["ball"]["state"] for row in rows[args.start:]}
        if not {3, 4, 5, 6}.issubset(modes):
            errors.append(f"missing pass/attach/shot/bounce modes: {sorted(modes)}")
        if weak_windows:
            errors.append(f"stationary team windows: {weak_windows}")
        # ROM animation-resource poses reach roughly 37 world pixels from the
        # actor origin; the exact per-frame table contract is checked by
        # test_cpu_gameplay.py. This summary only rejects true detachment.
        if not attached_distances or max(attached_distances) > 40.0:
            errors.append("ball is not physically attached to its owner")
        if errors:
            print("[CPU TRACE] FAIL: " + "; ".join(errors))
            raise SystemExit(1)
        print("[CPU TRACE] PASS: sustained CPU decisions, movement, and ball physics")


if __name__ == "__main__":
    main()
