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
    600: "6c103ff0fe22b29384806281dd9a04f61e0ed867eb732f36bb32c168b4eda900",
    1300: "82262f0757290dd70dc1128814b1502025f7170f5b7313ecab605d120a2243d3",
}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pack", required=True)
    parser.add_argument("--exe", required=True)
    parser.add_argument("--rom", required=True)
    args = parser.parse_args()

    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        whistle = root / "gameplay_whistle.wav"
        whistle_run = subprocess.run([
            args.exe, "--headless", "--rom", args.rom, "--assets", args.pack,
            "--frames", "0", "--dump-gameplay-whistle", str(whistle),
        ], capture_output=True, text=True, check=False)
        if whistle_run.returncode or (
                "command $44 SRCN $12 pitch=$0556 voice=4 "
                "ADSR1/2=$8E/$A0 VOL=$14/$14 peak=3598"
                not in whistle_run.stdout):
            raise AssertionError(whistle_run.stdout + whistle_run.stderr)
        wav = whistle.read_bytes()
        if len(wav) != 96044 or wav[:4] != b"RIFF" or wav[36:40] != b"data":
            raise AssertionError("gameplay whistle WAV shape changed")
        whistle_hash = hashlib.sha256(wav[44:]).hexdigest()
        if whistle_hash != "3c8bfb56d2bffd1a0a8b85136c316d0d1f5b1b329ffc1daff1693a5628397846":
            raise AssertionError(
                f"gameplay command-$44 whistle PCM changed: {whistle_hash}")
        trace = root / "cpu_gameplay.jsonl"
        command = [
            args.exe, "--headless", "--rom", args.rom, "--assets", args.pack,
            "--tipoff-only", "--frames", "61990", "--gameplay-trace", str(trace),
            "--debug-state",
        ]
        result = subprocess.run(command, capture_output=True, text=True, check=False)
        if result.returncode or "INT:$85:963D" not in result.stdout or \
                "BALL M:" not in result.stdout:
            raise AssertionError(result.stdout + result.stderr)
        rows = [json.loads(line) for line in trace.read_text().splitlines()]
        if len(rows) != 61990:
            raise AssertionError(f"expected 61990 CPU frames, got {len(rows)}")
        initial_fouls = {
            "event_raw": 0, "shooting_raw": 0,
            "offender_raw": -1, "victim_raw": -1,
            "team_raw": [0, 0], "personal_raw": [0] * 10,
            "free_throw_state_raw": 0, "free_throw_sequence_raw": 0,
            "free_throw_start_tick_raw_09be": 0,
            "free_throw_aim_x_raw_0980": 0,
            "free_throw_aim_y_raw_0982": 0,
            "free_throw_flight_timer_raw_0930": 0,
            "deferred_shot_foul_phase_raw_0a02": 0,
            "latched_event_raw_08f0": 0,
            "whistle_active_raw_09b6": 0,
            "whistle_timer_raw_08de": 0xFFFF,
            "presentation_gate_raw_08e2": 0,
            "whistle_presentation_queued_raw": 0,
        }
        activated_foul = next((row for row in rows
                               if sum(row["fouls"]["team_raw"])), None)
        if activated_foul:
            foul = activated_foul["fouls"]
            collision = activated_foul["collision"]
            owned_foul = collision["routine"] == 0x86D12D and \
                foul["offender_raw"] == collision["a"] and \
                foul["victim_raw"] == collision["b"]
            body_foul = collision["player_routine"] in \
                (0x86BFBA, 0x86C91E) and \
                foul["offender_raw"] == collision["player_b"] and \
                foul["victim_raw"] == collision["player_a"]
            event_one = foul["event_raw"] == 1 or \
                (foul["latched_event_raw_08f0"] == 1 and
                 foul["whistle_active_raw_09b6"] == 1)
            if not (owned_foul or body_foul) or not event_one or \
                    foul["shooting_raw"] != 0 or \
                    sum(foul["team_raw"]) != 1 or \
                    sum(foul["personal_raw"]) != 1:
                raise AssertionError(
                    "$86:C4FE/$86:D12D defensive-foul bookkeeping diverged: "
                    f"{activated_foul}")
        for row in rows[219:]:
            collision = row["collision"]
            if collision["routine"] not in (
                    0, 0x86CE1E, 0x86D12D, 0x86D1D9,
                    0x86D25A, 0x86D43E):
                continue
            if collision["routine"] in (0x86CE1E, 0x86D25A):
                continue
            if collision["routine"] and (
                    not 0 <= collision["a"] < 10 or
                    not 0 <= collision["b"] < 10 or
                    collision["a"] // 5 == collision["b"] // 5):
                raise AssertionError(
                    "$86:CCFC owned-ball contact accepted a same-team or "
                    f"invalid pair: {collision}")
        player_contacts = [row["collision"] for row in rows[219:]
                           if row["collision"]["player_count"]]
        if not player_contacts:
            raise AssertionError("$86:D652 never produced player/player contact")
        for contact in player_contacts:
            if contact["player_routine"] not in (
                    0x86BD41, 0x86BF0B, 0x86BFBA, 0x86C91E) or \
                    not 0 <= contact["player_a"] < 10 or \
                    not 0 <= contact["player_b"] < 10:
                raise AssertionError(f"invalid player contact telemetry: {contact}")
            same_team = contact["player_a"] // 5 == contact["player_b"] // 5
            if same_team != (contact["player_routine"] == 0x86BD41):
                raise AssertionError(f"player contact classifier changed: {contact}")

        def frame(number):
            return rows[number - 1]

        live = frame(240)
        for number in (220, 400, 1800):
            expected_clock = 43200 - max(0, number - 220)
            if frame(number)["match"]["match_clock_raw_0928"] != expected_clock:
                raise AssertionError(
                    f"$0928 outer-frame clock changed at {number}: "
                    f"{frame(number)['match']}")
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
        expected_assignments = [14, 10, 12, 16, 18, 4, 0, 2, 6, 8]
        if [actor["raw"]["assignment_base"] for actor in live["actors"]] != \
                expected_assignments:
            raise AssertionError("lineup-permuted `$86:D86C` assignments changed")
        current_assignments = [
            actor["raw"]["assignment_current"] for actor in live["actors"]]
        if current_assignments == expected_assignments:
            raise AssertionError("`$85:BE06-$C0F5` never replanned assignments")
        if any(value != 0xFFFF and
               (value & 1 or value > 18) for value in current_assignments):
            raise AssertionError("defensive planner emitted an invalid actor offset")
        if 4 not in [actor["raw"]["control_mode"] for actor in live["actors"]]:
            raise AssertionError("`$85:BF81-$BFDC` did not promote a primary defender")
        if not any(any(actor["raw"]["control_mode"] == 6
                       for actor in row["actors"]) for row in rows[219:]):
            raise AssertionError("`$85:C018-$C036` never selected a help defender")
        match = live["match"]
        if match["team_context_mode_raw_30"] != [4, 4] or \
                match["team_context_flags_raw_32"] != [1, 1] or \
                match["team_context_activity_raw_39"] != [1, 1]:
            raise AssertionError("`$46EB/$476B` team context initialization changed")

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
            # `$87:8F01` actor dispatch and the surrounding play controller
            # can straddle a trace row: frame-220 possession initialization
            # precedes actor dispatch, while ordinary play advance and a
            # rebound/catch commit follow it. Accept either adjacent state,
            # but require the target to come from an exact packed ROM row.
            for before, after in zip(previous["actors"], current["actors"]):
                old_flags = before["raw"]["behavior_flags"]
                new_flags = after["raw"]["behavior_flags"]
                if not old_flags & 0x08 and new_flags & 0x08:
                    if current["match"]["live_state_raw"] == 0x82 and \
                            after["id"] == current["match"]["inbound_actor_raw"]:
                        continue
                    side = after["id"] // 5
                    expected_targets = {
                        formation_target(
                            possession["play_code_raw"], after["id"] % 5,
                            possession["play_step_raw"],
                            possession["play_mirror_raw"] != 0, side)
                        for possession in (previous["possession"],
                                           current["possession"])
                    }
                    actual = (after["raw"]["target_x_56"],
                              after["raw"]["target_y_58"])
                    if actual not in expected_targets:
                        raise AssertionError(
                            f"$85:AD6B target install changed at frame "
                            f"{current['frame']} actor {after['id']}: "
                            f"{actual} not in {expected_targets}; previous possession="
                            f"{previous['possession']} current possession="
                            f"{current['possession']}")
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
                    requested["match"]["inbound_layout_raw"] == 0 and \
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
                play_01_rows[0]["play_selector_raw"] not in (
                    [3, 4, -1], [8, 9, -1]):
            raise AssertionError(f"play $01 record zero changed: {play_01_rows[:1]}")
        if any(row["play_step_raw"] not in (0, 1, 2) for row in play_01_rows):
            raise AssertionError("play $01 escaped its ROM control stream")
        relative_01 = {0: [3, 4, -1], 1: [4, 3, -1], 2: [3, 4, -1]}
        for row in play_01_rows:
            left = relative_01[row["play_step_raw"]]
            right = [value + 5 if value >= 0 else value for value in left]
            # B50E and the inbound F5C7 selector retain these words; the
            # play-control stream or BAA2 acquisition reloads/clears them.
            if row["play_selector_raw"] not in (left, right, [-1, -1, -1]):
                raise AssertionError(
                    f"play $01 side-relative selectors changed: {row}")
        teams = {row["possession"]["team"] for row in rows[219:]}
        if not {0, 1}.issubset(teams):
            raise AssertionError(f"CPU offense did not change sides: {teams}")
        modes = {row["ball"]["state"] for row in rows[219:]}
        if not {3, 4, 5, 6}.issubset(modes):
            raise AssertionError(f"ball physics modes missing: {modes}")
        owners = {row["ball"]["owner"] for row in rows[219:]}
        if len(owners - {-1}) < 4:
            raise AssertionError(f"ballhandler did not rotate: {owners}")
        acquisitions = [(before, after) for before, after in zip(rows, rows[1:])
                        if before["ball"]["state"] in (3, 6) and
                        after["ball"]["state"] == 4 and
                        after["match"]["live_state_raw"] != 0x82]
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
                    row["ball"]["activity_raw"] not in (0, 0xFFFF) and
                    not (row["ball"]["activity_raw"] == 1 and
                         row["fouls"]["free_throw_state_raw"] != 0)
                    for row in rows):
            raise AssertionError("$0948 canonical shot/attach lifecycle changed")
        # `$86:9DBF/$9DFF` install these latches at the mode-12 release
        # boundary. Later ROM rim/impact branches may clear `$096A` while the
        # ball still retains its shot-mode renderer state, so validate the
        # transition rather than every subsequent flight row.
        detached_shots = [rows[index] for index in range(1, len(rows))
                          if rows[index]["ball"]["state"] == 5 and
                          rows[index]["ball"]["activity_raw"] == 0xFFFF and
                          rows[index - 1]["ball"]["state"] == 4 and
                          any(actor["raw"]["control_mode"] == 12
                              for actor in rows[index - 1]["actors"])]
        if not detached_shots or any(
                not 0 <= row["match"]["shot_actor_raw_09c8"] < 10 or
                row["match"]["interference_value_raw_096a"] not in (1, 2, 3)
                for row in detached_shots):
            raise AssertionError(
                "$86:9DBF/$9DFF detached-shot latches changed")

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
                # `$86:BD41/$BF0B` likewise rewrites planar velocity after
                # actor integration without recomputing +$4C; +$7A marks
                # that post-resolver player/player response.
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
                stable_movement_mode = row["scheduler"]["due_raw"] != 0 and \
                    previous is not None and \
                    previous["actors"][actor["id"]]["raw"]["control_mode"] == \
                    raw["control_mode"] and raw["control_mode"] in (1, 2, 3, 4, 5, 6)
                post_resolver_contact = row["collision"]["player_count"] != 0 or \
                    (previous is not None and
                     previous["collision"]["player_count"] != 0)
                if (actor["vx"] or actor["vy"]) and \
                        raw["movement_magnitude_4c"] != expected_magnitude and \
                        not on_rectangular_edge and not on_isometric_edge and \
                        stable_movement_mode and not post_resolver_contact and \
                        row["fouls"]["free_throw_state_raw"] == 0:
                    raise AssertionError(
                        f"actor +$4C magnitude changed at frame {row['frame']} "
                        f"actor {actor['id']}: {raw['movement_magnitude_4c']} "
                        f"!= {expected_magnitude}; collision={row['collision']}")
                mode = actor["raw"]["control_mode"]
                if row["fouls"]["free_throw_state_raw"] == 0 and \
                        (mode >= len(behavior_targets) or
                         actor["ai_routine"] != behavior_targets[mode]):
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
        mode13_carried_frames = 0
        mode13_finishes = 0
        for previous, current in zip(due_rows, due_rows[1:]):
            for before, after in zip(previous["actors"], current["actors"]):
                old_mode = before["raw"]["control_mode"]
                new_mode = after["raw"]["control_mode"]
                actor_id = after["id"]
                ball = current["ball"]
                if new_mode == 13:
                    if after["animation"] not in (
                            0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E) or \
                            after["lower_animation"] != 0x1F or \
                            ball["state"] != 4 or ball["owner"] != actor_id or \
                            not 0 < after["raw"]["mode13_timer_60"] <= 0x28 or \
                            after["raw"]["mode13_selector_56"] not in range(-1, 7) or \
                            after["raw"]["mode13_variant_58"] not in (0, 2, 4, 6) or \
                            (after["raw"]["mode13_baseline_vx_ba"] == 0 and
                             after["raw"]["mode13_baseline_vy_bc"] == 0):
                        raise AssertionError(
                            f"$86:B34F/A7DA carried finish changed: {after} {ball}")
                    mode13_carried_frames += 1
                if old_mode == 13 and new_mode == 1:
                    if ball["state"] != 5 or ball["owner"] != -1 or \
                            current["match"]["shot_value_raw"] != 2:
                        raise AssertionError(
                            f"$86:A9D0 close-finish release changed: {after} {ball}")
                    mode13_finishes += 1
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
        if mode13_carried_frames < 8 or mode13_finishes < 1:
            raise AssertionError(
                "$86:B34F->$86:A7DA mode-13 lifecycle not exercised: "
                f"carried={mode13_carried_frames} finishes={mode13_finishes}")

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
        if len(score_changes) < 4 or previous_score[0] == 0 or previous_score[1] == 0:
            raise AssertionError(f"CPU scoring did not sustain both teams: {score_changes}")
        dead_runs = []
        run = []
        for row in rows:
            if row["match"]["live_state_raw"] == 0x82 and \
                    row["fouls"]["free_throw_state_raw"] == 0:
                run.append(row)
            elif run:
                dead_runs.append(run)
                run = []
        if not dead_runs:
            raise AssertionError("$092E inbound executor was not exercised")
        replaced_provisional = 0
        for inbound in dead_runs:
            first = inbound[0]
            match = first["match"]
            provisional_actor = match["inbound_actor_raw"]
            layout = match["inbound_layout_raw"]
            if layout == 0:
                expected_target = (394, -64, 6) if provisional_actor == 2 \
                    else (-394, 64, 2)
            elif layout in (3, 4):
                # `$85:C37D` consumes the rounded live ball record; `$09B0/B2`
                # separately preserve its signed integer-word floor.
                source_x = first["ball"]["x"]
                source_y = first["ball"]["y"]
                side_anchor = -336 if provisional_actor == 2 else 336
                if layout == 4:
                    target_x = (-40 if side_anchor < 0 else 40) if \
                        (side_anchor ^ source_x) < 0 else source_x
                else:
                    target_x = max(-332, min(337, source_x))
                target_y = -224 if source_y < 0 else 224
                target_x = max(target_x, -556 - target_y) if target_y < 0 \
                    else min(target_x, 561 - target_y)
                expected_target = (
                    target_x, target_y, 0 if source_y < 0 else 4)
            else:
                raise AssertionError(f"unexpected dead-ball layout: {first}")
            actual_target = (match["inbound_target_x_raw"],
                             match["inbound_target_y_raw"],
                             match["inbound_direction_raw"])
            if match["inbound_timer_raw"] != 300 or provisional_actor not in (2, 7) or \
                    actual_target != expected_target:
                raise AssertionError(f"$85:A1E9/C37D inbound seed changed: {first}")
            installed = [row for row in inbound
                         if row["possession"]["actor"] >= 0]
            if not installed:
                raise AssertionError("pose collision never installed the inbound owner")
            installed_actor = installed[0]["match"]["inbound_actor_raw"]
            if installed_actor != installed[0]["possession"]["actor"] or \
                    installed[0]["ball"]["owner"] != -1:
                raise AssertionError(
                    "$093E dead-ball owner was conflated with logical ball ownership")
            if installed_actor != provisional_actor:
                replaced_provisional += 1
            ready = [row for row in inbound
                     if row["match"]["inbound_ready_raw"]]
            if not ready:
                raise AssertionError("$86:F4F2 inbound never reached raw target box")
            first_ready = ready[0]
            if first_ready["match"]["dead_ball_raw_0968"] != 2:
                raise AssertionError(
                    "$86:F54F inbound arrival did not write $0968=2")
            actor_id = first_ready["match"]["inbound_actor_raw"]
            actor = first_ready["actors"][actor_id]
            dx = first_ready["match"]["inbound_target_x_raw"] - actor["x"]
            dy = first_ready["match"]["inbound_target_y_raw"] - actor["y"]
            if not (-9 <= dx < 9 and -9 <= dy < 9):
                raise AssertionError(f"$86:F4F2 accepted outside [-9,+8]: {(dx, dy)}")
            before_ready = [row["match"]["inbound_timer_raw"]
                            for row in inbound
                            if row["possession"]["actor"] >= 0 and
                            not row["match"]["inbound_ready_raw"]]
            if any(value not in (299, 300) for value in before_ready):
                raise AssertionError("$86:F654 stopped reloading 300 before arrival")
            for before, after in zip(ready, ready[1:]):
                if before["match"]["inbound_timer_raw"] >= \
                        after["match"]["inbound_timer_raw"]:
                    continue
                # Exact `$86:A613` can cancel a boundary-crossing inbound
                # transfer. `$87:9AA6` then seeds the opposite-side inbound
                # without an intervening non-$82 render frame; only that
                # state-side change may restart the visible countdown. The
                # earlier `$86:F4F2-$F4FF` arrival test also precedes the
                # ready latch: contact can displace the frozen inbounder and
                # make `$86:F654` reload 300 for the same actor.
                side_reset = before["match"]["inbound_state_raw"] != \
                    after["match"]["inbound_state_raw"]
                displaced_reload = \
                    before["match"]["inbound_actor_raw"] == \
                        after["match"]["inbound_actor_raw"] and \
                    after["match"]["inbound_timer_raw"] in (299, 300)
                if not (side_reset or displaced_reload) or \
                        after["match"]["inbound_transfer_raw"] != 0 or \
                        after["possession"]["actor"] < 0:
                    raise AssertionError("arrived inbound timer increased")
            transfer = [row for row in ready
                        if row["match"]["inbound_transfer_raw"]]
            transfer_actor = transfer[0]["match"]["inbound_actor_raw"] \
                if transfer else -1
            if not transfer or transfer[0]["match"]["inbound_timer_raw"] >= 240 or \
                    transfer[0]["possession"]["pass_actor_raw"] != transfer_actor or \
                    not transfer[0]["possession"]["pass_active_raw"]:
                raise AssertionError(f"$86:F59F/F64F transfer gate changed: {transfer[:1]}")
        if replaced_provisional == 0:
            raise AssertionError(
                "$86:CCFC pose collision never replaced provisional actor 2/7")

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
            # `$86:CCFC-$D1D6` may acquire a descending miss directly from
            # shot mode 5, while floor/rim misses first become bounce mode 6.
            if state == 4 and last_state in (5, 6) and shots:
                shots[-1]["rebound"] = (row["ball"]["owner"],
                                        row["possession"]["team"])
            last_state = state
        misses = [shot for shot in shots if shot["veto"]]
        if any(not 0 <= shot["index"] < 16 for shot in misses):
            raise AssertionError(f"$86:A110/$A17D miss path missing: {misses}")
        if any((shot["index"] != 255) != bool(shot["veto"])
               for shot in shots):
            raise AssertionError(
                f"$86:A110 veto/miss-index contract changed: {shots}")
        # Do not classify a shot begun inside the final 600-frame capture
        # tail as a failed rebound; the trace ended before its resolution
        # horizon. Every miss with a complete horizon must still resolve.
        settled_misses = [shot for shot in misses
                          if shot["frame"] <= rows[-1]["frame"] - 600]
        if any(shot["rebound"] is None for shot in settled_misses):
            raise AssertionError(
                f"miss did not reach collision-owned rebound: {settled_misses}")
        if any(not 0 <= owner < 10 or team != owner // 5
               for shot in settled_misses
               for owner, team in [shot["rebound"]]):
            raise AssertionError(
                f"miss rebound ownership was inconsistent: {settled_misses}")

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
            if row["ball"]["state"] == 4 and owner >= 0 and \
                    row["fouls"]["free_throw_state_raw"] == 0:
                actor = row["actors"][owner]
                actual = (row["ball"]["x"] - actor["x"],
                          row["ball"]["y"] - actor["y"],
                          row["ball"]["z"] - actor["z"])
                expected = expected_attachment(actor)
                attached.append((row["frame"], actual, expected))
        def attachment_matches(pair):
            _, actual, expected = pair
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
                # `$86:A613` has destroyed the only global identities. The
                # released passer may already have normalized out of mode 15,
                # so `$093E` is not a valid substitute for `$0942` here.
                continue
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
                    (not raw["pass_released"] and
                     actor["animation"] not in
                     (0x2D, 0x2E, 0x2F, 0x30, 0x31)):
                raise AssertionError(
                    f"mode-15 pass metadata diverged: possession={possession} "
                    f"actor={actor}")
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
        pass_animations = {actor["animation"] for _, _, actor in pass_rows}
        if not pass_animations.intersection((0x2D, 0x2E)) or \
                0x2F not in pass_animations or \
                not pass_animations.intersection((0x30, 0x31)):
            raise AssertionError("live-covered pass-animation families regressed")

        # `$87:9C3A -> $86:A5B0 -> $86:9846`: after A613 invalidates
        # `$0946`, a normal mode-10 receiver must return to team mode on the
        # next 30-Hz actor pass. Two rendered rows are the maximum observable
        # scheduling latency; longer runs reproduce the retired edge drift.
        stale_receiver_run = 0
        for row in rows[219:]:
            stale = row["possession"]["pass_receiver_raw"] < 0 and any(
                actor["raw"]["control_mode"] == 10
                for actor in row["actors"])
            stale_receiver_run = stale_receiver_run + 1 if stale else 0
            if stale_receiver_run > 2:
                raise AssertionError(
                    f"$86:A5B0 stale mode-10 receiver at frame {row['frame']}")

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
        "src/nba_gameplay_ball.c", "src/nba_gameplay_effect.c",
        "src/nba_gameplay_foul.c", "src/nba_player_lab.c"))
    for marker in ("$85:963D-$985F", "$85:BC52-$BC81", "$85:B95C",
                   "$87:B832", "$87:B649", "$87:B66A", "$85:9192",
                   "$87:8F01-$8F8D", "nba_gameplay_camera_update",
                   "cpu_begin_possession", "cpu_update_possession",
                   "ball_attach_to_actor", "ball_launch",
                   "$85:A079-$A345", "$4711/$4791", "score_made_basket",
                    "$86:A110", "$86:A17D", "$86:BAA2-$BC99",
                   "$86:CCCD-$D5DA", "cpu_first_loose_ball_contact",
                   "nba_gameplay_ball_pose_contact",
                    "$86:E923-$E96E", "$86:B0F7-$B153",
                    "$86:B154-$B334", "cpu_update_rom_special_receiver",
                    "cpu_finish_rom_close_shot", "cpu_restore_normal_mode",
                    "$85:A82C-$AB16", "nba_gameplay_velocity_step",
                   "$85:B734-$B820", "nba_gameplay_mode11_shot_decision",
                   "nba_gameplay_same_x_half",
                   "$86:E82F-$E8F6", "$86:E8F7-$E922",
                   "nba_gameplay_defense_pair_target",
                   "nba_gameplay_defense_mode_target",
                   "$87:8FA1-$8FA9", "cpu_refresh_team_roles_end_frame",
                   "$85:A5F4-$A655", "nba_gameplay_ball_apply_settle",
                   "$85:A43A-$A44B",
                   "nba_gameplay_ball_apply_ground_impact",
                   "$87:A9E3-$AA01", "$87:AA02-$AAB1",
                   "nba_gameplay_effect_step",
                   "$86:AB2D-$AF65", "$86:A6B3-$A790",
                   "$86:9DDB-$9DE4", "$86:9B84-$9B8F",
                   "nba_gameplay_pass_direction", "cpu_begin_rom_pass",
                   "nba_gameplay_select_pass_receiver",
                   "$85:B50E-$B60A", "$85:B60B-$B677",
                   "cpu_update_rom_passer",
                   "$86:D035-$D205", "nba_gameplay_owned_contact_attempt",
                   "cpu_try_owned_ball_contact",
                   "$86:D5DB", "$86:D652-$D728", "$86:BD41-$BF08",
                   "$86:BF0B-$C475", "$86:C88F-$C91D",
                   "$86:BFBA-$C238", "$86:C91E-$CB83",
                   "cpu_try_player_knockdown_contact",
                   "$86:C4FE-$C6AC", "nba_gameplay_foul_classify_contact",
                   "cpu_classify_player_contact",
                   "$87:9C67", "$86:C6AD-$C74D",
                   "cpu_update_knockdown_actor",
                   "cpu_update_player_contacts",
                   "$86:CD97-$D1D6", "cpu_try_detached_shot_contact",
                   "nba_gameplay_detached_shot_contact_attempt",
                   "nba_player_gameplay_contact_rating",
                   "nba_player_gameplay_movement_profile",
                    "nba_player_gameplay_shot_ratings",
                    "cpu_commit_ball_acquisition"):
        if marker not in implementation:
            raise AssertionError(f"CPU gameplay implementation lost {marker}")
    for relative in ("include/nba_gameplay_ai.h", "src/nba_gameplay_ai.c",
                     "include/nba_gameplay_ball.h", "src/nba_gameplay_ball.c",
                     "include/nba_gameplay_effect.h",
                     "src/nba_gameplay_effect.c",
                     "tools/ghidra/DumpCpuGameplay.java",
                     "tools/ghidra/Run-CpuGameplayAnalysis.ps1"):
        if not (source / relative).is_file():
            raise AssertionError(f"CPU Ghidra evidence tool missing: {relative}")
    print("CPU-versus-CPU gameplay regression checks passed")


if __name__ == "__main__":
    main()
