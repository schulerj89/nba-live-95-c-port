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
    600: "40dbdf3634307b63e0f1e0407617c00ff5c5cce845a6a2e961050441e593ccf8",
    1300: "8cef9da7b960488d615fc9924cb58a131f67040895e04c4cab99b1905ffc55a8",
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
        expected_dormant_fouls = {
            "event_raw": 0, "shooting_raw": 0,
            "offender_raw": -1, "victim_raw": -1,
            "team_raw": [0, 0], "personal_raw": [0] * 10,
            "free_throw_state_raw": 0, "free_throw_sequence_raw": 0,
        }
        activated_foul = next((row for row in rows
                               if row.get("fouls") != expected_dormant_fouls), None)
        if activated_foul:
            raise AssertionError(
                "foul scaffold activated without a verified ROM collision "
                f"predicate at frame {activated_foul['frame']}: "
                f"{activated_foul.get('fouls')}")

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

        pack_raw = Path(args.pack).read_bytes()
        pack_count = int.from_bytes(pack_raw[12:16], "little")

        def packed_asset(wanted):
            for asset_index in range(pack_count):
                entry = 16 + asset_index * 24
                asset_id = int.from_bytes(pack_raw[entry:entry + 4], "little")
                if asset_id == wanted:
                    offset = int.from_bytes(pack_raw[entry + 4:entry + 8], "little")
                    size = int.from_bytes(pack_raw[entry + 8:entry + 12], "little")
                    return pack_raw[offset:offset + size]
            return None

        formation_asset = packed_asset(274)
        if formation_asset is None:
            raise AssertionError("NBFORM1 gameplay formation asset is missing")

        def signed_word(payload, offset):
            value = int.from_bytes(payload[offset:offset + 2], "little")
            return value - 0x10000 if value & 0x8000 else value

        def formation_target(play, role, step, mirror_y, side):
            entry = 48 + (play * 5 + role) * 8
            count = int.from_bytes(formation_asset[entry + 2:entry + 4], "little")
            offset = int.from_bytes(formation_asset[entry + 4:entry + 8], "little")
            if not 0 <= step < count:
                raise AssertionError(
                    f"runtime requested invalid formation play={play} role={role} step={step}")
            x = signed_word(formation_asset, offset + step * 4)
            y = signed_word(formation_asset, offset + step * 4 + 2)
            if mirror_y:
                y = -y
            if side == 0:
                x = -x
                if play >= 0x0E:
                    y = -y
            return x, y

        installs = [0, 0]
        completions = [0, 0]
        for previous, current in zip(rows[218:], rows[219:]):
            # Actor pass runs before the rebound/catch commit in the same
            # host tick. If possession changes, a bit-$08 rise still belongs
            # to the previous play graph, exactly like `$87:8F01` ordering.
            possession = previous["possession"] if \
                previous["possession"]["team"] != current["possession"]["team"] \
                else current["possession"]
            for before, after in zip(previous["actors"], current["actors"]):
                old_flags = before["raw"]["behavior_flags"]
                new_flags = after["raw"]["behavior_flags"]
                if not old_flags & 0x08 and new_flags & 0x08:
                    side = after["id"] // 5
                    expected = formation_target(
                        possession["play_code_raw"], after["id"] % 5,
                        possession["play_step_raw"],
                        possession["play_mirror_raw"] != 0, side)
                    actual = (after["raw"]["target_x_56"],
                              after["raw"]["target_y_58"])
                    if actual != expected:
                        raise AssertionError(
                            f"$85:AD6B target install changed: {actual} != {expected}")
                    installs[side] += 1
                if not old_flags & 0x40 and new_flags & 0x40:
                    completions[after["id"] // 5] += 1
        if min(installs) < 10 or min(completions) < 10:
            raise AssertionError(
                f"formation install/arrival lifecycle was not sustained: "
                f"installs={installs} completions={completions}")
        special_rows = [row for row in rows[219:]
                        if row["possession"]["special_actor_raw"] != 0xFFFF]
        if any(not 0 <= row["possession"]["special_actor_raw"] < 10
               for row in special_rows):
            raise AssertionError("$85:B4B9 produced an unbounded $09A2 cutter")
        anchor_rows = []
        for row in special_rows:
            actor_id = row["possession"]["special_actor_raw"]
            actor = row["actors"][actor_id]
            expected = (-336 if actor_id < 5 else 336, 0)
            actual = (actor["raw"]["target_x_56"],
                      actor["raw"]["target_y_58"])
            if actual == expected:
                if actor["raw"]["behavior_flags"] & 0x08:
                    raise AssertionError("$09A2 cutter retained formation bit $08")
                anchor_rows.append(row)
        if special_rows and not anchor_rows:
            raise AssertionError("$85:AE1F cutter anchor was not represented")

        play_codes = {row["possession"]["play_code_raw"] for row in rows[219:]}
        # BAA2 sets `$0994`; B128 now selects from the asset-packed team
        # strategy ranges instead of rotating four host-authored fixtures.
        if 0x35 not in play_codes or 0x01 not in play_codes or \
                len(play_codes) < 8 or any(not 0 <= code < 61 for code in play_codes):
            raise AssertionError(f"ROM strategy play selection did not sustain: {play_codes}")
        play_35 = frame(220)["possession"]
        expected_35 = {
            "play_step_raw": 0, "play_countdown_raw": -2,
            "play_event_wait_raw": 1, "play_selector_raw": [-1, -1, -1],
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
        if assignments != [-1, -1, -1, -1, -1, -1, -1, -1, -1, -1]:
            raise AssertionError(f"actor +$16 ownership mapping changed: {assignments}")
        if any(row["possession"]["play_request_raw"] not in (0, 1)
               for row in rows):
            raise AssertionError("$0994 escaped its word-boolean contract")
        requested_01 = [(index, row) for index, row in enumerate(rows)
                        if row["possession"]["play_request_raw"] == 1]
        if not requested_01:
            raise AssertionError("$86:BB5F acquisition never requested a play")
        for index, requested in requested_01:
            possession = requested["possession"]
            next_due = next((row for row in rows[index + 1:index + 4]
                             if row["scheduler"]["due_raw"]), None)
            if next_due is None or \
                    next_due["possession"]["play_request_raw"] != 0 or \
                    not 0 <= next_due["possession"]["play_code_raw"] < 61 or \
                    next_due["possession"]["play_step_raw"] != 0:
                raise AssertionError(
                    f"$85:B128 did not consume $0994 on the next actor pass: "
                    f"{requested} -> {next_due}")
            if requested["match"]["live_state_raw"] == 0x82 and \
                    (next_due["possession"]["play_code_raw"] != 0x01 or
                     next_due["possession"]["play_countdown_raw"] != 120):
                raise AssertionError(
                    f"made-score $0994 did not preserve play $01: {next_due}")
        # The score writer changes `$0996` immediately, but B377 does not load
        # record zero until `$0994` is consumed on the next logical pass.
        play_01_rows = [row["possession"] for row in rows
                        if row["possession"]["play_code_raw"] == 0x01 and
                        row["possession"]["play_request_raw"] == 0]
        if not play_01_rows or play_01_rows[0]["play_step_raw"] != 0 or \
                play_01_rows[0]["play_countdown_raw"] != 120 or \
                play_01_rows[0]["play_selector_raw"] != [3, 4, -1]:
            raise AssertionError(f"play $01 record zero changed: {play_01_rows[:1]}")
        if any(row["play_step_raw"] not in (0, 1, 2) for row in play_01_rows):
            raise AssertionError("play $01 escaped its ROM control stream")
        relative_01 = {0: [3, 4, -1], 1: [4, 3, -1], 2: [3, 4, -1]}
        for row in play_01_rows:
            left = relative_01[row["play_step_raw"]]
            right = [value + 5 if value >= 0 else value for value in left]
            # Mode-11 `$85:B50E` consumes `$09AA/$09AC/$09AE` after its
            # decision; the next play-control record reloads them.
            if row["play_selector_raw"] not in (left, right, [-1, -1, -1]):
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
        acquisitions = [(before, after) for before, after in zip(rows, rows[1:])
                        if before["ball"]["state"] in (3, 6) and
                        after["ball"]["state"] == 4]
        if len(acquisitions) < 10:
            raise AssertionError("$86:BAA2 catch/rebound path was not sustained")
        for _, acquired in acquisitions:
            match = acquired["match"]
            if acquired["ball"]["activity_raw"] != 0 or \
                    match["rim_context_raw_097c"] != 0 or \
                    not match["event_bits_raw_13e7"] & 0x0010 or \
                    acquired["possession"]["play_request_raw"] != 1:
                raise AssertionError(
                    f"$86:BC81-$BC90 acquisition reset changed: {acquired}")
        mode12_attached = [row for row in rows
                           if row["ball"]["state"] == 4 and
                           row["ball"]["activity_raw"] == 0xFFFF]
        if not any(row["ball"]["state"] == 5 and
                   row["ball"]["activity_raw"] == 0xFFFF for row in rows) or \
                not mode12_attached or \
                any(row["actors"][row["ball"]["owner"]]["raw"]["control_mode"] != 12
                    for row in mode12_attached) or \
                any(row["ball"]["state"] == 4 and
                    row["ball"]["activity_raw"] not in (0, 0xFFFF)
                    for row in rows):
            raise AssertionError("$0948 canonical shot/attach lifecycle changed")

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
                # `$85:A656-$A726` can independently cancel one outward axis
                # after +$4C was written when the record touches ±394/±224.
                # Telemetry rounds 24.8 coordinates, while the ROM compares
                # their signed integer word. A clamped -394 record with a
                # positive fraction can therefore be reported as -393.
                on_rectangular_edge = abs(actor["x"]) >= 393 or \
                    abs(actor["y"]) >= 223
                on_isometric_edge = actor["x"] <= -556 - actor["y"] + 1 \
                    if actor["y"] < 0 else \
                    actor["x"] >= 561 - actor["y"] - 1
                stable_movement_mode = previous is not None and \
                    previous["actors"][actor["id"]]["raw"]["control_mode"] == \
                    raw["control_mode"] and raw["control_mode"] in (1, 2, 3, 4, 5, 6)
                if (actor["vx"] or actor["vy"]) and \
                        raw["movement_magnitude_4c"] != expected_magnitude and \
                        not on_rectangular_edge and not on_isometric_edge and \
                        stable_movement_mode:
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
        if not {(0x0B, 0x03), (0x16, 0x32)}.issubset(mismatch_pairs):
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
        shot_starts = 0
        shot_releases = 0
        mode11_fallbacks = 0
        for previous, current in zip(due_rows, due_rows[1:]):
            for before, after in zip(previous["actors"], current["actors"]):
                old_mode = before["raw"]["control_mode"]
                new_mode = after["raw"]["control_mode"]
                actor_id = after["id"]
                ball = current["ball"]
                if old_mode == 11 and new_mode == 12:
                    if after["animation"] != 0x16 or \
                            after["lower_animation"] != 0x32 or \
                            after["vz"] != 0x1E0 or ball["state"] != 4 or \
                            ball["owner"] != actor_id or \
                            ball["activity_raw"] != 0xFFFF:
                        raise AssertionError(
                            f"$86:B625 mode-12 initialization changed: {current}")
                    shot_starts += 1
                elif old_mode == 12 and new_mode == 12:
                    if after["vz"] != before["vz"] - 0x30 or \
                            ball["state"] != 4 or ball["owner"] != actor_id:
                        raise AssertionError(
                            f"$85:96B5 mode-12 jump cadence changed: "
                            f"{before['vz']}->{after['vz']}")
                elif old_mode == 12 and new_mode == 11:
                    signed_gate = before["vz"] < 0
                    low_rng_gate = 0 <= before["vz"] < 0x60 and \
                        (previous["possession"]["rng_state_raw"] & 0x70) == 0
                    free_throw_gate = 0 <= before["vz"] < 0x60 and \
                        previous["fouls"]["free_throw_state_raw"] != 0
                    if not (signed_gate or low_rng_gate or free_throw_gate) or \
                            after["animation"] != 0x17 or \
                            ball["state"] != 5 or ball["owner"] != -1 or \
                            current["possession"]["actor"] != -1:
                        raise AssertionError(
                            f"$86:9D6E shot release changed: {current}")
                    shot_releases += 1
                elif old_mode == 11 and new_mode == 1 and \
                        previous["possession"]["actor"] == -1:
                    mode11_fallbacks += 1
        if min(shot_starts, shot_releases, mode11_fallbacks) < 2:
            raise AssertionError(
                "mode 11->12->11->1 shot lifecycle was not sustained: "
                f"starts={shot_starts} releases={shot_releases} "
                f"fallbacks={mode11_fallbacks}")

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
        if not misses or any(not 0 <= shot["index"] < 16 for shot in misses):
            raise AssertionError(f"$86:A110/$A17D miss path missing: {misses}")
        if any(shot["rebound"] is None for shot in misses):
            raise AssertionError(f"miss did not reach collision-owned rebound: {misses}")
        if any(not 0 <= owner < 10 or team != owner // 5 for shot in misses
               for owner, team in [shot["rebound"]]):
            raise AssertionError(f"miss rebound ownership was inconsistent: {misses}")

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

        animation = packed_asset(256)
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

        # `$86:AB2D-$AF65/$86:A6B3-$A790`: mode 15 installs one of the
        # live-covered pass states, keeps the ball attached through the
        # resource phase threshold, then releases it via the ROM table.
        pass_rows = []
        release_rows = []
        for index, row in enumerate(rows[219:], 219):
            possession = row["possession"]
            actor_id = possession["pass_actor_raw"]
            if not possession["pass_active_raw"]:
                continue
            receiver_id = possession["pass_receiver_raw"]
            if actor_id < 0 or receiver_id < 0:
                # `$85:A656-$A726 -> $86:A613` clears both words when any
                # integrated record touches a rectangular boundary; `$09C4`
                # and the installed mode-15 executor intentionally survive.
                if actor_id != -1 or receiver_id != -1:
                    raise AssertionError(
                        f"partial `$0942/$0946` boundary clear: {possession}")
                actor_id = possession["candidate_raw"]
            if actor_id < 0 or actor_id >= 10 or receiver_id >= 10:
                raise AssertionError(f"invalid mode-15 pass actor: {possession}")
            actor = row["actors"][actor_id]
            raw = actor["raw"]
            expected_band = 0 if possession["pass_distance_raw"] < 0x41 else \
                6 if possession["pass_distance_raw"] < 0x79 else \
                12 if possession["pass_distance_raw"] < 0xC9 else \
                18 if possession["pass_distance_raw"] < 0x119 else \
                24 if possession["pass_distance_raw"] < 0x191 else 30
            if raw["pass_band_62"] != expected_band or \
                    raw["mode_saved_62"] != expected_band or \
                    raw["pass_direction_66"] >= 8 or \
                    raw["saved_mode_84"] != raw["control_mode_saved"] or \
                    actor["animation"] not in (0x2D, 0x2E, 0x2F, 0x30, 0x31):
                raise AssertionError(f"mode-15 pass metadata diverged: {actor}")
            pass_rows.append((index, row, actor))
            if raw["pass_released"] and index and \
                    not rows[index - 1]["actors"][actor_id]["raw"]["pass_released"]:
                release_rows.append((index, row, actor, rows[index - 1]))
        if len(pass_rows) < 100 or not release_rows:
            raise AssertionError("ROM mode-15 pass lifecycle was not sustained")
        for _, row, actor, before in release_rows:
            raw = actor["raw"]
            before_actor = before["actors"][actor["id"]]
            if before_actor["raw"]["upper_phase"] <= \
                    raw["pass_release_threshold"] or \
                    row["ball"]["state"] != 3 or row["ball"]["owner"] != -1:
                raise AssertionError("pass released before `$86:A736-$A747` phase gate")
        if not {0x2D, 0x2F, 0x30}.issubset(
                {actor["animation"] for _, _, actor in pass_rows}):
            raise AssertionError("live-covered pass-animation families regressed")

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
                    "$86:A110", "$86:A17D", "$86:BAA2-$BC99",
                   "$86:CCCD-$D5DA", "cpu_first_loose_ball_contact",
                   "nba_gameplay_ball_pose_contact",
                    "$86:E923-$E96E", "$86:B0F7-$B153",
                   "$85:A82C-$AB16", "nba_gameplay_velocity_step",
                   "$86:AB2D-$AF65", "$86:A6B3-$A790",
                   "$86:9DDB-$9DE4", "$86:9B84-$9B8F",
                   "nba_gameplay_pass_direction", "cpu_begin_rom_pass",
                   "nba_gameplay_select_pass_receiver",
                   "$85:B50E-$B60A", "$85:B60B-$B677",
                   "cpu_update_rom_passer",
                   "nba_player_gameplay_movement_profile",
                    "nba_player_gameplay_shot_ratings",
                    "cpu_commit_ball_acquisition"):
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
