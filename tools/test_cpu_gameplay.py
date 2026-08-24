"""Long-running regression checks for ROM-derived CPU-versus-CPU gameplay."""

import argparse
import hashlib
import json
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
    600: "a3e002966912947f3f2cb55e12e5bfa8588ddfae1a116addce44543a328a5448",
    1300: "7d0cabb2626d484b6762a59f9b396d2bb4f492b03f9a853c3b1bd0542ab6b06a",
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
            "--tipoff-only", "--frames", "50000", "--gameplay-trace", str(trace),
            "--debug-state",
        ]
        result = subprocess.run(command, capture_output=True, text=True, check=False)
        if result.returncode or "INT:$85:963D" not in result.stdout or \
                "BALL M:" not in result.stdout:
            raise AssertionError(result.stdout + result.stderr)
        rows = [json.loads(line) for line in trace.read_text().splitlines()]
        if len(rows) != 50000:
            raise AssertionError(f"expected 50000 CPU frames, got {len(rows)}")

        def frame(number):
            return rows[number - 1]

        live = frame(240)
        if [actor["roster"] for actor in live["actors"]] != \
                [2, 0, 1, 3, 4, 2, 0, 1, 3, 4]:
            raise AssertionError("active actor-to-roster mapping changed")
        free_camera_states = set()
        actor_camera_states = set()
        for row in rows[219:]:
            possession_actor = row["possession"]["actor"]
            camera = row["camera"]
            expected_subject = possession_actor if possession_actor >= 0 else -1
            if camera["subject_raw"] != expected_subject:
                raise AssertionError(
                    f"camera subject diverged from signed $093E: {camera}")
            if possession_actor >= 0:
                expected_group = 5 if possession_actor >= 5 else 0
                if camera["side_group_raw"] != expected_group:
                    raise AssertionError("actor camera did not refresh persistent $093A")
                actor_camera_states.add(row["ball"]["state"])
            else:
                free_camera_states.add(row["ball"]["state"])
        if not {3, 5, 6}.issubset(free_camera_states) or not actor_camera_states:
            raise AssertionError("camera did not cover actor and free-ball proxy paths")
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
        play_35 = frame(220)["possession"]
        expected_35 = {
            "play_step_raw": 0, "play_countdown_raw": -2,
            "play_event_wait_raw": 1, "play_selector_raw": [9, 7, -1],
        }
        if any(play_35[key] != value for key, value in expected_35.items()):
            raise AssertionError(f"$85:B377/$B2DC play $35 load changed: {play_35}")
        play_35_next = frame(222)["possession"]
        if play_35_next["play_step_raw"] != 0 or \
                play_35_next["play_countdown_raw"] != -4 or \
                play_35_next["play_event_wait_raw"] != 1:
            raise AssertionError(
                f"$85:B24C event barrier did not retain signed underflow: {play_35_next}")
        assignments = [actor["raw"]["controller_assignment_16"]
                       for actor in frame(220)["actors"]]
        if assignments != [-1, -1, -1, -1, -1, -1, -1, -1, 0, -1]:
            raise AssertionError(f"actor +$16 ownership mapping changed: {assignments}")
        if any(row["possession"]["play_request_raw"] not in (0, 1)
               for row in rows):
            raise AssertionError("$0994 escaped its word-boolean contract")
        requested_01 = [(index, row) for index, row in enumerate(rows)
                        if row["possession"]["play_request_raw"] == 1]
        if not requested_01:
            raise AssertionError("made baskets never requested play $01 through $0994")
        for index, requested in requested_01:
            possession = requested["possession"]
            if possession["play_code_raw"] != 0x01 or \
                    requested["match"]["live_state_raw"] != 0x82:
                raise AssertionError(f"invalid pending $0994 state: {requested}")
            next_due = next((row for row in rows[index + 1:index + 4]
                             if row["scheduler"]["due_raw"]), None)
            if next_due is None or \
                    next_due["possession"]["play_request_raw"] != 0 or \
                    next_due["possession"]["play_code_raw"] != 0x01 or \
                    next_due["possession"]["play_step_raw"] != 0 or \
                    next_due["possession"]["play_countdown_raw"] != 120:
                raise AssertionError(
                    f"$85:B128 did not consume $0994 on the next actor pass: "
                    f"{requested} -> {next_due}")
        # The score writer changes `$0996` immediately, but B377 does not load
        # record zero until `$0994` is consumed on the next logical pass.
        play_01_rows = [row["possession"] for row in rows
                        if row["possession"]["play_code_raw"] == 0x01 and
                        row["possession"]["play_request_raw"] == 0]
        if not play_01_rows or play_01_rows[0]["play_step_raw"] != 0 or \
                play_01_rows[0]["play_countdown_raw"] != 120 or \
                play_01_rows[0]["play_selector_raw"] != [3, 4, -1]:
            raise AssertionError(f"play $01 record zero changed: {play_01_rows[:1]}")
        if {row["play_step_raw"] for row in play_01_rows} != {0, 1, 2} or \
                not any(row["play_cycle_raw"] == 1 for row in play_01_rows):
            raise AssertionError("play $01 countdown did not traverse and cycle")
        relative_01 = {0: [3, 4, -1], 1: [4, 3, -1], 2: [3, 4, -1]}
        for row in play_01_rows:
            left = relative_01[row["play_step_raw"]]
            right = [value + 5 if value >= 0 else value for value in left]
            if row["play_selector_raw"] not in (left, right):
                raise AssertionError(
                    f"play $01 side-relative selectors changed: {row}")
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
                raw = actor["raw"]
                for field in ("controller_assignment_16",
                              "movement_magnitude_4c", "recovery_inhibit_7a"):
                    if field not in raw:
                        raise AssertionError(f"missing actor raw field {field}")
                expected_magnitude = max(abs(actor["vx"]), abs(actor["vy"])) + \
                    min(abs(actor["vx"]), abs(actor["vy"])) // 4
                # `+$4C` is written by the velocity resolver and remains
                # latched if a later branch zeros/skips velocity that frame.
                if (actor["vx"] or actor["vy"]) and \
                        raw["movement_magnitude_4c"] != expected_magnitude:
                    raise AssertionError(
                        f"actor +$4C magnitude changed: {raw['movement_magnitude_4c']} "
                        f"!= {expected_magnitude}")
                mode = actor["raw"]["control_mode"]
                if mode >= len(behavior_targets) or \
                        actor["ai_routine"] != behavior_targets[mode]:
                    raise AssertionError(f"$87:9244 mode dispatch changed: {actor}")
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

        # `$87:8EFB-$8F92`: one global, possession-independent 30-Hz pass,
        # fixed `$0938=2`, strict actor records 0 through 9.
        possession_changes = 0
        previous_team = rows[0]["possession"]["team"]
        for index, row in enumerate(rows):
            scheduler = row["scheduler"]
            due = (row["simulation_tick"] & 1) == 0
            expected_order = list(range(10)) if due else []
            if scheduler != {
                    "due_raw": int(due),
                    "actor_pass_dt_raw": 2 if due else 0,
                    "actor_pass_mask_raw": 0x3FF if due else 0,
                    "actor_pass_order_raw": expected_order}:
                raise AssertionError(f"$87:8EFB scheduler mismatch: {row}")
            team = row["possession"]["team"]
            if team != previous_team:
                possession_changes += 1
                if index and row["scheduler"]["due_raw"] == \
                        rows[index - 1]["scheduler"]["due_raw"]:
                    raise AssertionError("possession change rephased global actor pass")
                previous_team = team
        if possession_changes < 4:
            raise AssertionError("scheduler was not tested across possessions")

        due_rows = [row for row in rows if row["scheduler"]["due_raw"]]
        recovery_transitions = 0
        for previous, current in zip(due_rows, due_rows[1:]):
            for before, after in zip(previous["actors"], current["actors"]):
                old_mode = before["raw"]["control_mode"]
                new_mode = after["raw"]["control_mode"]
                old_timer = before["raw"]["reaction_threshold"]
                new_timer = after["raw"]["reaction_threshold"]
                if old_mode == new_mode and old_mode in (7, 16) and \
                        new_timer != old_timer - 2:
                    raise AssertionError(
                        f"mode {old_mode} +$60 cadence changed: {old_timer}->{new_timer}")
                if old_mode == 16 and new_mode == 7:
                    if old_timer != 0 or new_timer != 0xB4:
                        raise AssertionError(
                            f"$86:B10A recovery transition changed: {old_timer}->{new_timer}")
                    recovery_transitions += 1
        if recovery_transitions < 2:
            raise AssertionError("post-shot mode 16->7 lifecycle was not sustained")

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
        if not dead_runs or any(values[0] != 300 or values[-1] not in (61, 0) or
                                not {240, 120}.issubset(values) or
                                (values[-1] == 0 and
                                 not {60, 0}.issubset(values)) or
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

        pack_raw = Path(args.pack).read_bytes()
        pack_count = int.from_bytes(pack_raw[12:16], "little")
        animation = None
        for index in range(pack_count):
            entry = 16 + index * 24
            asset_id = int.from_bytes(pack_raw[entry:entry + 4], "little")
            if asset_id == 256:
                offset = int.from_bytes(pack_raw[entry + 4:entry + 8], "little")
                size = int.from_bytes(pack_raw[entry + 8:entry + 12], "little")
                animation = pack_raw[offset:offset + size]
                break
        if animation is None:
            raise AssertionError("NBPANIM1 attachment tables are missing")
        header = [int.from_bytes(animation[8 + i * 4:12 + i * 4], "little")
                  for i in range(15)]
        lower_table, upper_x_table, upper_y_table, upper_z_table = \
            header[4], header[12], header[13], header[14]
        signed8 = lambda value: value - 256 if value >= 128 else value

        def expected_attachment(actor):
            raw = actor["raw"]
            upper, lower = raw["upper_resource"], raw["lower_resource"]
            lower_y = signed8(animation[lower_table + lower])
            lower_z = signed8(animation[lower_table + 0x830 + lower])
            upper_x = signed8(animation[upper_x_table + upper])
            upper_y = signed8(animation[upper_y_table + upper])
            upper_z = signed8(animation[upper_z_table + upper])
            flags = 0x8000 if actor["direction"] < 3 else 0
            if flags & 0x8000:
                flags ^= 3
            if flags & 2:
                lower_y = -lower_y
            if flags & 1:
                upper_y = -upper_y
            midpoint = (lower_y + upper_y) // 2
            return (midpoint - 2 * upper_x, midpoint + 2 * upper_x,
                    upper_x - lower_z - upper_z)

        attached = []
        for row in rows[219:]:
            owner = row["ball"]["owner"]
            if row["ball"]["state"] == 4 and owner >= 0:
                actor = row["actors"][owner]
                actual = (row["ball"]["x"] - actor["x"],
                          row["ball"]["y"] - actor["y"],
                          row["ball"]["z"] - actor["z"])
                expected = expected_attachment(actor)
                attached.append((actual, expected))
        def attachment_matches(pair):
            actual, expected = pair
            return all(abs(a - e) <= 1 for a, e in zip(actual, expected))
        if not attached or any(not attachment_matches(pair) for pair in attached):
            raise AssertionError(
                "ball diverged from `$87:B832/$B953` resource attachment: " +
                repr(next((pair for pair in attached
                           if not attachment_matches(pair)), None)))

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
        "src/nba_tipoff.c", "src/nba_gameplay_ai.c",
        "src/nba_gameplay_ball.c", "src/nba_player_lab.c"))
    for marker in ("$85:963D-$985F", "$85:BC43-$BC81", "$85:B95C",
                   "$87:B832", "$87:B649", "$87:B66A", "$85:9192",
                   "$87:8F01-$8F8D", "nba_gameplay_camera_update",
                   "cpu_begin_possession", "cpu_update_possession",
                   "ball_attach_to_actor", "ball_launch",
                   "$85:A079-$A345", "$4711/$4791", "score_made_basket",
                   "$86:A110", "$86:A17D", "$86:BAA2/$86:BAEE",
                    "$86:E923-$E96E", "$86:B0F7-$B153",
                   "$85:A82C-$AB16", "nba_gameplay_velocity_step",
                   "nba_player_gameplay_movement_profile",
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
