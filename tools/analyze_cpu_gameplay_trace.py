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


def is_live_play(row):
    """Movement cadence applies outside dead-ball and free-throw scenes."""
    return (row["match"]["live_state_raw"] < 0x80 and
            row["fouls"]["free_throw_state_raw"] == 0)


def is_dead_ball(row):
    """The ROM uses bit 7 of $0936 for dead-ball/inbound sequencing."""
    return row["match"]["live_state_raw"] >= 0x80


def visual_ball_carrier(row):
    """Actor whose retained OBJ point owns the visible ball this frame."""
    owner = row["ball"]["owner"]
    if row["ball"]["state"] == 4 and 0 <= owner < 10:
        return owner
    possession = row["possession"]["actor"]
    if row["match"]["live_state_raw"] == 0x82 and 0 <= possession < 10:
        return possession
    return -1


def movement_count(rows, actor, first, last):
    count = 0
    base_frame = rows[0]["scene_frame"]
    first_index = max(1, first - base_frame + 1)
    stop_index = min(len(rows), last - base_frame + 1)
    for index in range(first_index, stop_index):
        if not is_live_play(rows[index]) or not is_live_play(rows[index - 1]):
            continue
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
    skipped_dead_ball_windows = []
    final_frame = rows[-1]["scene_frame"]
    for first in range(args.start, final_frame + 1, args.window):
        last = min(first + args.window - 1, final_frame)
        base_frame = rows[0]["scene_frame"]
        first_index = max(1, first - base_frame + 1)
        stop_index = min(len(rows), last - base_frame + 1)
        live_pairs = sum(
            is_live_play(rows[index]) and is_live_play(rows[index - 1])
            for index in range(first_index, stop_index))
        counts = [movement_count(rows, actor, first, last) for actor in range(10)]
        team_counts = (sum(counts[:5]), sum(counts[5:]))
        print(f"[CPU TRACE] movement f{first}-{last} teams={team_counts[0]}/{team_counts[1]} "
              f"live_pairs={live_pairs} actors={'/'.join(map(str, counts))}")
        # A window containing only the one-frame boundary between dead-ball
        # and ordinary play is not a meaningful sustained-movement sample.
        # Require a full 32 comparable 60-Hz pairs before grading both
        # teams. Shorter fragments occur on either side of an inbound or
        # free-throw transition and do not contain a complete actor cadence.
        if live_pairs < 32:
            skipped_dead_ball_windows.append((first, last))
        elif min(team_counts) < max(4, live_pairs // 4):
            weak_windows.append((first, last, team_counts))

    dead_ball_runs = []
    run_start = None
    for row in rows:
        if is_dead_ball(row) and run_start is None:
            run_start = row["scene_frame"]
        elif not is_dead_ball(row) and run_start is not None:
            dead_ball_runs.append((run_start, row["scene_frame"] - 1))
            run_start = None
    if run_start is not None:
        dead_ball_runs.append((run_start, final_frame))
    if dead_ball_runs:
        print("[CPU TRACE] dead-ball=" + ", ".join(
            f"f{first}-{last}({last - first + 1})"
            for first, last in dead_ball_runs))
    if skipped_dead_ball_windows:
        print("[CPU TRACE] movement windows outside ordinary live play=" +
              ", ".join(f"f{first}-{last}"
                        for first, last in skipped_dead_ball_windows))

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

    retained_ball_frames = 0
    retained_ball_mismatches = []
    for previous, current in zip(rows, rows[1:]):
        carrier = visual_ball_carrier(current)
        if (carrier < 0 or carrier != visual_ball_carrier(previous) or
                current["simulation_tick"] & 1 == 0 or
                (current["camera"]["x"] == previous["camera"]["x"] and
                 current["camera"]["y"] == previous["camera"]["y"])):
            continue
        retained_ball_frames += 1
        old_actor = previous["actors"][carrier]
        new_actor = current["actors"][carrier]
        old_relative = (previous["ball"]["screen_x"] - old_actor["screen_x"],
                        previous["ball"]["screen_y"] - old_actor["screen_y"])
        new_relative = (current["ball"]["screen_x"] - new_actor["screen_x"],
                        current["ball"]["screen_y"] - new_actor["screen_y"])
        if old_relative != new_relative:
            retained_ball_mismatches.append(
                (current["scene_frame"], carrier, old_relative, new_relative))
    print(f"[CPU TRACE] retained-ball camera frames={retained_ball_frames} "
          f"relative mismatches={len(retained_ball_mismatches)}")

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
        # A native half-court possession may settle all five actors while the
        # mode-11 owner waits on its decision cadence. Treat isolated/short
        # runs as telemetry, but retain a 20-second (five-window) deadlock
        # guard so the original "players sit forever" regression cannot hide.
        weak_runs = []
        for window in weak_windows:
            if weak_runs and window[0] == weak_runs[-1][-1][1] + 1:
                weak_runs[-1].append(window)
            else:
                weak_runs.append([window])
        stalled_runs = [run for run in weak_runs if len(run) >= 5]
        if stalled_runs:
            errors.append(f"stationary team runs: {stalled_runs}")
        base_frame = rows[0]["scene_frame"]
        overlong_dead_ball = []
        for run in dead_ball_runs:
            first = max(0, run[0] - base_frame)
            last = min(len(rows), run[1] - base_frame + 1)
            includes_free_throws = any(
                row["fouls"]["free_throw_state_raw"] != 0
                for row in rows[first:last])
            # A multi-attempt free-throw presentation legitimately remains
            # in bit-7 dead-ball state beyond the ordinary inbound ceiling.
            # Extreme loose-ball coordinates can require several native
            # 300-tick target reloads before the inbounder reaches the strict
            # `$86:F654` box. Keep a finite 40-second guard while allowing
            # the observed 1,778-frame completion.
            if run[1] - run[0] + 1 > 2400 and not includes_free_throws:
                overlong_dead_ball.append(run)
        # A finite capture may end in the middle of an otherwise ordinary
        # inbound. Completed runs above enforce the duration ceiling; the
        # final right-censored run cannot prove a deadlock by itself.
        if overlong_dead_ball:
            errors.append(f"dead-ball state exceeded 2,400 frames: {overlong_dead_ball}")
        # ROM animation-resource poses reach roughly 37 world pixels from the
        # actor origin; the exact per-frame table contract is checked by
        # test_cpu_gameplay.py. This summary only rejects true detachment.
        if not attached_distances or max(attached_distances) > 40.0:
            errors.append("ball is not physically attached to its owner")
        if retained_ball_mismatches:
            errors.append("ball OBJ moved relative to retained carrier OBJ: " +
                          repr(retained_ball_mismatches[:8]))
        if errors:
            print("[CPU TRACE] FAIL: " + "; ".join(errors))
            raise SystemExit(1)
        print("[CPU TRACE] PASS: sustained CPU decisions, movement, and ball physics")


if __name__ == "__main__":
    main()
