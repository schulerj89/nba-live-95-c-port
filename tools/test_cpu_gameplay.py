"""Long-running regression checks for ROM-derived CPU-versus-CPU gameplay."""

import argparse
import hashlib
import json
import math
import subprocess
import sys
import tempfile
from pathlib import Path

from PIL import Image


FORMATION = [
    (8, 3), (-16, -83), (-24, 80), (104, -56), (96, 59),
    (-8, -3), (16, 83), (24, -80), (-104, 56), (-96, -59),
]
EXPECTED_RGB = {
    600: "360d3daee6e864a067b082ec976927bb076809f3d10dba483da16230a6a43e83",
    1300: "be5cd8c2ef4234369b095f55bff2fd788e2e4a2201cd024fb8b8030668e2d074",
}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pack", required=True)
    parser.add_argument("--exe", required=True)
    parser.add_argument("--rom", required=True)
    args = parser.parse_args()

    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        trace = root / "cpu_gameplay.jsonl"
        command = [
            args.exe, "--headless", "--rom", args.rom, "--assets", args.pack,
            "--tipoff-only", "--frames", "35000", "--gameplay-trace", str(trace),
            "--debug-state",
        ]
        result = subprocess.run(command, capture_output=True, text=True, check=False)
        if result.returncode or "INT:$85:963D" not in result.stdout or \
                "BALL M:" not in result.stdout:
            raise AssertionError(result.stdout + result.stderr)
        rows = [json.loads(line) for line in trace.read_text().splitlines()]
        if len(rows) != 35000:
            raise AssertionError(f"expected 35000 CPU frames, got {len(rows)}")

        def frame(number):
            return rows[number - 1]

        live = frame(240)
        if any(actor["control"] != 0 for actor in frame(1900)["actors"]):
            raise AssertionError("CPU-versus-CPU mode assigned a human actor")
        if any(row["control"]["actor"] != 0xFF for row in rows[219:]):
            raise AssertionError("CPU-versus-CPU installed a human-selected actor")
        moved = sum((actor["x"], actor["y"]) != FORMATION[index]
                    for index, actor in enumerate(live["actors"]))
        if moved < 8:
            raise AssertionError(f"only {moved}/10 CPU actors broke formation")
        if (live["actors"][0]["x"], live["actors"][0]["y"]) != FORMATION[0]:
            raise AssertionError("center reaction delay no longer matches the ROM trace")

        play_codes = {row["possession"]["play_code_raw"] for row in rows[219:]}
        if not {0x35, 0x01, 0x0F, 0x26}.issubset(play_codes):
            raise AssertionError(f"recurring ROM play codes missing: {play_codes}")
        teams = {row["possession"]["team"] for row in rows[219:]}
        if not {0, 1}.issubset(teams):
            raise AssertionError(f"CPU offense did not change sides: {teams}")
        modes = {row["ball"]["state"] for row in rows[219:]}
        if not {3, 4, 5, 8}.issubset(modes):
            raise AssertionError(f"ball physics modes missing: {modes}")
        owners = {row["ball"]["owner"] for row in rows[219:]}
        if len(owners - {-1}) < 4:
            raise AssertionError(f"ballhandler did not rotate: {owners}")

        behavior_targets = [
            0x879C1B, 0x86F1B0, 0x86F6CD, 0x86F23F, 0x86F794,
            0x86F2CA, 0x86F8CD, 0x86994C, 0x86C6AD, 0x86F0B7,
            0x86A5B0, 0x86F34F, 0x86B769, 0x86A7DA, 0x86B154,
            0x86A6B3, 0x86B0F7, 0x86B979,
        ]
        for row in rows[219:]:
            changed_timers = 0
            previous = rows[row["frame"] - 2] if row["frame"] > 220 else None
            for actor in row["actors"]:
                mode = actor["raw"]["control_mode"]
                if mode >= len(behavior_targets) or \
                        actor["ai_routine"] != behavior_targets[mode]:
                    raise AssertionError(f"$87:9245 mode dispatch changed: {actor}")
                if actor["actor_routine"] != 0x85963D:
                    raise AssertionError("actor integration lost $85:963D")
                if actor["raw"]["upper_resource"] == 0xFFFF or \
                        actor["raw"]["lower_resource"] == 0xFFFF:
                    raise AssertionError("independent animation resource missing")
                if previous and actor["raw"]["action"] != \
                        previous["actors"][actor["id"]]["raw"]["action"]:
                    changed_timers += 1
            if changed_timers not in (0, 10):
                raise AssertionError("$87:8F01 actor pass split across C actors")
        mismatch_pairs = {(actor["animation"], actor["lower_animation"])
                          for row in rows[219:] for actor in row["actors"]
                          if actor["animation"] != actor["lower_animation"]}
        if not {(0x0B, 0x03), (0x31, 0x03)}.issubset(mismatch_pairs):
            raise AssertionError(f"independent animation pairs missing: {mismatch_pairs}")
        if any(row["possession"]["rng_state_raw"] in (0, 0xFFFF)
               for row in rows[219:]):
            raise AssertionError("$80:CEE7 RNG state was not retained")

        # `$85:A079-$A345`: `$094C` is added to `$4711/$4791`, then
        # `$0936=$82` holds the dead ball until the inbound reset.
        score_changes = []
        previous_score = (0, 0)
        for row in rows[219:]:
            match = row["match"]
            score = (match["score_left_raw"], match["score_right_raw"])
            if score != previous_score:
                delta = (score[0] - previous_score[0],
                         score[1] - previous_score[1])
                if sorted(delta) not in ([0, 2], [0, 3]):
                    raise AssertionError(f"invalid ROM score increment {delta}")
                if match["shot_value_raw"] not in (2, 3) or \
                        match["live_state_raw"] != 0x82 or \
                        row["ball"]["state"] != 5 or \
                        not 74 <= row["ball"]["z"] <= 82:
                    raise AssertionError(f"made basket state incomplete: {row}")
                scoring_side = 0 if delta[0] else 1
                expected_group = (scoring_side ^ 1) * 5
                if match["inbound_state_raw"] != expected_group or \
                        match["inbound_actor_raw"] != expected_group + 2:
                    raise AssertionError(f"$0952/$0954 inbound mapping changed: {row}")
                score_changes.append((row["frame"], score))
                previous_score = score
        if len(score_changes) < 4 or previous_score[0] < 4 or previous_score[1] < 4:
            raise AssertionError(f"CPU scoring did not sustain both teams: {score_changes}")
        dead_runs = []
        run = []
        for row in rows:
            if row["match"]["live_state_raw"] == 0x82:
                run.append(row["match"]["inbound_timer_raw"])
            elif run:
                dead_runs.append(run)
                run = []
        if not dead_runs or any(values[0] != 300 or values[-1] != 0 or
                                not {240, 120, 60, 0}.issubset(values) or
                                any(a < b for a, b in zip(values, values[1:]))
                                for values in dead_runs):
            raise AssertionError("$092E inbound thresholds changed: " +
                                 repr([(len(v), v[0], v[-1]) for v in dead_runs]))

        shots = []
        last_state = rows[218]["ball"]["state"]
        for row in rows[219:]:
            state = row["ball"]["state"]
            if state == 5 and last_state != 5:
                match = row["match"]
                shots.append({"frame": row["frame"],
                              "team": row["possession"]["team"],
                              "veto": match["shot_inner_veto_raw"],
                              "index": match["shot_miss_index_raw"],
                              "rebound": None})
            if state == 4 and last_state == 6 and shots:
                shots[-1]["rebound"] = (row["ball"]["owner"],
                                        row["possession"]["team"])
            last_state = state
        misses = [shot for shot in shots if shot["veto"]]
        if len(misses) < 2 or any(not 0 <= shot["index"] < 16 for shot in misses):
            raise AssertionError(f"$86:A110/$A17D miss path missing: {misses}")
        if any(shot["rebound"] is None for shot in misses):
            raise AssertionError(f"miss did not reach collision-owned rebound: {misses}")
        if not any(owner // 5 == shot["team"] for shot in misses
                   for owner, _ in [shot["rebound"]]):
            raise AssertionError("offensive rebounds were replaced by forced turnovers")

        def signed16(value):
            return value - 0x10000 if value & 0x8000 else value

        prior_dx = prior_dy = 0
        camera_positions = set()
        for row in rows[199:]:
            camera = row["camera"]
            if camera["routine"] != 0x859192:
                raise AssertionError("camera telemetry lost $85:9192 attribution")
            x, y = camera["x"], camera["y"]
            if not (-582 <= x <= 328 and -242 <= y <= -53):
                raise AssertionError(f"camera escaped ROM bounds: {(x, y)}")
            dx = abs(signed16(camera["raw_085c"]) -
                     signed16(camera["raw_085e"]))
            dy = abs(signed16(camera["raw_0860"]) -
                     signed16(camera["raw_0862"]))
            if dx > 22 or dy > 22 or dx > prior_dx + 2 or dy > prior_dy + 2:
                raise AssertionError(
                    f"$85:9352 camera cadence changed: {prior_dx, prior_dy} -> {dx, dy}")
            prior_dx, prior_dy = dx, dy
            expected_source = (0x8006 + camera["raw_086c"] * 104 +
                               camera["raw_086e"] * 2) & 0xFFFF
            if camera["raw_0876"] != expected_source:
                raise AssertionError("$85:8EE6 court source pointer changed")
            camera_positions.add((x, y))
        if len(camera_positions) < 25:
            raise AssertionError("camera did not follow CPU play")

        attached = []
        for row in rows[219:]:
            owner = row["ball"]["owner"]
            if row["ball"]["state"] == 4 and owner >= 0:
                actor = row["actors"][owner]
                attached.append(math.hypot(row["ball"]["x"] - actor["x"],
                                           row["ball"]["y"] - actor["y"]))
        if not attached or max(attached) > 20.0:
            raise AssertionError("ball is not attached at the actor hand offset")

        for first in range(220, 1900, 240):
            last = min(first + 239, len(rows) - 1)
            counts = []
            for actor in range(10):
                counts.append(sum(
                    (rows[index]["actors"][actor]["x"],
                     rows[index]["actors"][actor]["y"]) !=
                    (rows[index - 1]["actors"][actor]["x"],
                     rows[index - 1]["actors"][actor]["y"])
                    for index in range(first + 1, last + 1)))
            if min(sum(counts[:5]), sum(counts[5:])) < 60:
                raise AssertionError(
                    f"CPU team became stationary in frames {first}-{last}: {counts}")

        analyzer = Path(__file__).with_name("analyze_cpu_gameplay_trace.py")
        analyzed = subprocess.run(
            [sys.executable, str(analyzer), str(trace), "--require-sustained"],
            capture_output=True, text=True, check=False)
        if analyzed.returncode or "PASS" not in analyzed.stdout:
            raise AssertionError(analyzed.stdout + analyzed.stderr)

        for number, expected in EXPECTED_RGB.items():
            output = root / f"cpu_{number}.bmp"
            render = subprocess.run([
                args.exe, "--headless", "--rom", args.rom, "--assets", args.pack,
                "--tipoff-only", "--frames", str(number), "--dump-frame", str(output),
            ], capture_output=True, text=True, check=False)
            if render.returncode:
                raise AssertionError(render.stdout + render.stderr)
            digest = hashlib.sha256(Image.open(output).convert("RGB").tobytes()).hexdigest()
            if digest != expected:
                raise AssertionError(f"CPU gameplay frame {number} changed: {digest}")

    source = Path(__file__).parents[1]
    implementation = "\n".join((source / relative).read_text() for relative in (
        "src/nba_tipoff.c", "src/nba_gameplay_ball.c", "src/nba_player_lab.c"))
    for marker in ("$85:963D-$985F", "$85:BC43-$BC81", "$85:B95C",
                   "$87:B832", "$87:B649", "$87:B66A", "$85:9192",
                   "$87:8F01-$8F8D", "nba_gameplay_camera_update",
                   "cpu_begin_possession", "cpu_update_possession",
                   "ball_attach_to_actor", "ball_launch",
                   "$85:A079-$A345", "$4711/$4791", "score_made_basket",
                   "$86:A110", "$86:A17D", "$86:BAA2/$86:BAEE",
                   "nba_player_gameplay_shot_ratings", "cpu_commit_rebound"):
        if marker not in implementation:
            raise AssertionError(f"CPU gameplay implementation lost {marker}")
    for relative in ("include/nba_gameplay_ai.h", "src/nba_gameplay_ai.c",
                     "include/nba_gameplay_ball.h", "src/nba_gameplay_ball.c",
                     "tools/ghidra/DumpCpuGameplay.java",
                     "tools/ghidra/Run-CpuGameplayAnalysis.ps1"):
        if not (source / relative).is_file():
            raise AssertionError(f"CPU Ghidra evidence tool missing: {relative}")
    print("CPU-versus-CPU gameplay regression checks passed")


if __name__ == "__main__":
    main()
