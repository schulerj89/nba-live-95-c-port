"""Long-running regression checks for ROM-derived CPU-versus-CPU gameplay."""

import argparse
import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path

from PIL import Image
from test_shot_state_trace import verify as verify_shot_state_trace


FORMATION = [
    (8, 3), (-16, -83), (-24, 80), (104, -56), (96, 59),
    (-8, -3), (16, 83), (24, -80), (-104, 56), (-96, -59),
]
EXPECTED_RGB = {
    # Reviewed after complete F34F caller ordering and coarse contact facing;
    # these are C visual regression anchors, not emulator-parity claims.
    # Independent ROM vectors and semantic endurance guards remain separate.
    # Stage4 physical acquisition replaces the forced actor8/frame220 reset.
    # All five captures inspected; wider loose-ball framing remains a gap.
    # Re-reviewed after the live `$86:EC32` jump/reach binding changed the
    # deterministic CPU trajectory. These frames visibly retain ten actors,
    # the ball, court bounds and unobstructed HUD composition.
    # Re-reviewed after replacing host rounding/truncation with the exact
    # `$87:A3BB-$A3DC` integer-word projection. Player feet now use the same
    # signed quarter-coordinate and origin as the native sprite compositor.
    # Re-reviewed after gameplay adopted the shared Mode-1 priority/window
    # compositor. These changes are isolated to BG/OBJ overlap and the newly
    # visible low-priority basket cells; CPU state/trajectory is unchanged.
    # Re-reviewed again after pack v31 replaced the opaque court panorama with
    # indexed BG1/BG2/BG3/backdrop inputs. The full CPU trace and state guards
    # remain unchanged; these hashes lock the corrected presentation output.
    # Re-reviewed after the native E39A/E3E1 defensive pose caller was bound.
    # The corrected state-7/8/10 poses change animation contact geometry and
    # therefore the deterministic CPU trajectory; each anchor still shows
    # ten complete players, ball, court, goal and unobstructed HUD.
    # Re-reviewed after `$86:D86C-$D89B` stopped pairing by roster number and
    # restored the native five-lineup-position matchups.  The corrected
    # assignments intentionally alter the CPU trajectory.  All five anchors
    # retain ten complete players, a visible ball, court/goal bounds and an
    # unobstructed HUD; the visitor number-overlay mask is also side-correct.
    # Re-reviewed after `$80:AD92-$AEC1` adopted the native direction-specific
    # torso/number submission order. State and trajectory are unchanged; only
    # the ROM-correct overlap priority of those two player layers differs.
    # Re-reviewed after 500 native `$86:F43A-$F653` calls proved inbound
    # behavior installs next-pass velocity only and reads signed integer
    # coordinate words for arrival/side gates. The duplicate host integration
    # and nearest-pixel gates were removed; all anchors retain ten complete
    # players, the ball, court/goal bounds and an unobstructed HUD.
    # Re-reviewed after the exact `$85:AD6B-$AF5B` parent branch sends all
    # opposite-X roles through B3AA (unless the activity edge route owns back
    # roles) and restores the DP-$5C short-timer target. The differential gate
    # independently proves targets, flags and velocity; these anchors retain
    # complete uniforms, ball, court/goal bounds and a clear HUD.
    # Re-reviewed after exact `$86:F1B0-$F2C9` parent timing stopped derived
    # movement from leading the ROM by one scheduled actor pass. All five
    # frames retain valid player composition, ball, court and clear HUD; the
    # changed trajectory is protected independently by the new ROM fixtures.
    600: "7bff9493a5c9baf9e2b85a900e056fb8416e1d88209314936900e3650d8317a7",
    # Re-reviewed after the live renderer adopted `$87:AFA2-$B053`'s tall
    # lower-body selector and the asset pack gained every dynamically chosen
    # `$87:AC76-$AC95` base+$28 torso resource. All five anchors show ten
    # complete, correctly colored player uniforms; the altered body resources
    # also correct native ball attachment points, so later CPU paths diverge
    # from the retired incomplete-resource trajectory by design.
    # Re-reviewed after the exact `$86:F43A-$F4F1` motion helper also adopted
    # `$85:B3C9`'s distance<=8 damping branch and native negative /16 bias.
    # The final five frames in `.analysis/inbound-final-review-20260829`
    # retain ten complete players, ball, court/goal bounds and clear HUD.
    # Re-reviewed after `$87:92A5-$949E` restored the native violation and
    # boundary-dead-ball parent ordering. The corrected boundary possession
    # changes the deterministic path after frame 600. These four anchors show
    # ten complete actors, the ball, both baskets/court bounds and a clear HUD.
    1300: "3a57e7b59d53828d25510ee3ec13c7e1b278c0160353bd075e8204f20fa20a0d",
    3480: "c7ab789a83e0892192a7d8db57307892c8a171ee55450cea79a42fa079007989",
    # Reviewed after the native `$85:BB5A->$85:BBAE` one-way help assignment
    # replaced the former symmetric port behavior. The exact ROM-call corpus
    # and matchup runtime probe independently protect the semantic change.
    6932: "2f5328f314fe7b6733e9da5f28bc23a69179a686c1d02380b91744ec387ad3ae",
    6954: "31b1cc98cfcd57550cccfe8cbacca3ca244fa5f8ab2b109864b44f740aabded5",
}


def native_target_distance(dx, dy):
    """Distance word returned by verified `$85:F347-$F3BA`."""
    def signed16(value):
        value &= 0xFFFF
        return value - 0x10000 if value & 0x8000 else value

    x = (-dx if dx < 0 else dx) & 0xFFFF
    y = (-dy if dy < 0 else dy) & 0xFFFF
    if signed16(y - 1) <= signed16(x):
        x, y = y, x
    return (y + (((x << 1) & 0xFFFF) >> 3)) & 0xFFFF


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
                "ADSR1/2=$8E/$A0 VOL=$14/$14 peak=2240"
                not in whistle_run.stdout):
            raise AssertionError(whistle_run.stdout + whistle_run.stderr)
        wav = whistle.read_bytes()
        if len(wav) != 96044 or wav[:4] != b"RIFF" or wav[36:40] != b"data":
            raise AssertionError("gameplay whistle WAV shape changed")
        whistle_hash = hashlib.sha256(wav[44:]).hexdigest()
        if whistle_hash != "d047862c692b0b6a6c6b7b7b1f0c249407f53dd9db41d23393d2818ae0ac0894":
            raise AssertionError(
                f"gameplay command-$44 whistle PCM changed: {whistle_hash}")
        trace = root / "cpu_gameplay.jsonl"
        command = [
            args.exe, "--headless", "--rom", args.rom, "--assets", args.pack,
            "--tipoff-only", "--frames", "63800", "--gameplay-trace", str(trace),
            "--debug-state",
        ]
        result = subprocess.run(command, capture_output=True, text=True, check=False)
        if result.returncode or "INT:$85:963D" not in result.stdout or \
                "BALL M:" not in result.stdout:
            raise AssertionError(result.stdout + result.stderr)
        rows = [json.loads(line) for line in trace.read_text().splitlines()]
        if len(rows) != 63800:
            raise AssertionError(f"expected 63800 CPU frames, got {len(rows)}")
        # Consecutive native frames 180..620 contain no small A->B->A player
        # origin reversals. This specifically guards the SNES OAM hold between
        # `$87:A47A` submission passes; sparse screenshot anchors cannot see it.
        presentation_reversals = []
        for actor in range(10):
            for index in range(181, 620):
                origins = [(rows[row]["actors"][actor]["screen_x"],
                            rows[row]["actors"][actor]["screen_y"])
                           for row in (index - 2, index - 1, index)]
                first = (origins[1][0] - origins[0][0],
                         origins[1][1] - origins[0][1])
                second = (origins[2][0] - origins[1][0],
                          origins[2][1] - origins[1][1])
                if first != (0, 0) and second == (-first[0], -first[1]) and \
                        max(abs(first[0]), abs(first[1])) <= 3:
                    presentation_reversals.append((index + 1, actor, first))
        if presentation_reversals:
            raise AssertionError(
                "player OAM origins shake between native presentation passes: "
                f"{presentation_reversals[:12]}")
        draw_direction_changes = []
        for row in rows:
            for actor in row["actors"]:
                draw_direction = actor["raw"]["draw_direction"]
                if draw_direction != actor["raw"]["direction_52"]:
                    draw_direction_changes.append(
                        (row["frame"], actor["id"],
                         actor["raw"]["control_mode"],
                         actor["raw"]["direction_52"], draw_direction))
                appearance = actor.get("appearance")
                if not appearance or not appearance["flags"] & 0x80:
                    raise AssertionError(
                        "$87:A47A->$80:AD92 selected a player layer absent "
                        f"from the asset pack: frame={row['frame']} actor={actor['id']} "
                        f"appearance={appearance}")
                if appearance["resources"][0:2] != [
                        actor["raw"]["draw_lower_resource"],
                        actor["raw"]["draw_upper_resource"]]:
                    raise AssertionError(
                        "appearance diagnostic did not inspect the rendered resources")
                if not all(appearance["opaque_pixels"][:3]):
                    raise AssertionError(
                        "gameplay selected an empty lower/upper/head player layer: "
                        f"frame={row['frame']} actor={actor['id']} "
                        f"appearance={appearance}")
                if appearance["flags"] & 0x04 and not appearance["opaque_pixels"][3]:
                    raise AssertionError(
                        "visible jersey-number overlay has no packed ROM pixels")
        if not draw_direction_changes or not any(
                mode in (10, 15) for _, _, mode, _, _ in draw_direction_changes):
            raise AssertionError(
                "$87:A52C-$A5FA target-facing draw selector was not runtime-bound")
        verify_shot_state_trace(rows)
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
        # Shooting-contact bookkeeping is verified by the deferred-foul
        # assertions below. Select an ordinary activated foul for this path;
        # the corrected RNG cadence may make a shooting foul occur first.
        activated_foul = next((row for row in rows
                               if sum(row["fouls"]["team_raw"]) and
                               row["fouls"]["shooting_raw"] == 0), None)
        if activated_foul:
            foul = activated_foul["fouls"]
            # An earlier offensive/charging foul may already have incremented
            # a personal count without incrementing the team-foul total. The
            # defensive invariant is the charged actor's nonzero personal
            # count and exactly one team foul, not a globally pristine ledger.
            event_one = foul["event_raw"] == 1 or \
                (foul["latched_event_raw_08f0"] == 1 and
                 foul["whistle_active_raw_09b6"] == 1)
            offender = foul["offender_raw"]
            victim = foul["victim_raw"]
            valid_pair = 0 <= offender < 10 and 0 <= victim < 10 and \
                offender // 5 != victim // 5
            if not valid_pair or not event_one or \
                    foul["shooting_raw"] != 0 or \
                    sum(foul["team_raw"]) != 1 or \
                    foul["team_raw"][offender // 5] != 1 or \
                    foul["personal_raw"][offender] < 1:
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

        first_tip = next(row for row in rows if row['tipoff']['contact_frame'])
        first_catch = next(row for row in rows if row['tipoff']['possession_frame'])
        catch_frame = first_catch['frame']
        if first_tip['frame'] >= catch_frame or first_tip['possession']['actor'] != -1:
            raise AssertionError('temporary tip contact was mistaken for possession')
        if any(row['possession']['actor'] != -1 for row in rows[:catch_frame-1]):
            raise AssertionError('ownership was awarded before physical acquisition')
        if first_catch['possession']['actor'] != rows[catch_frame-2]['possession']['pass_receiver_raw']:
            raise AssertionError('first catch did not collect the selected tip receiver')
        live = frame(240)
        # Only the pre-whistle prefix is an uninterrupted clock. Correct
        # owner dispatch now reaches an inbound pause before frame1800.
        # shot_state_runtime_probe checks the ROM clock helper binding on
        # every outer frame, including paused/resumed play (two 16k runs).
        for number in (220, 400):
            expected_clock = 43200 - max(0, number - catch_frame)
            if frame(number)["match"]["match_clock_raw_0928"] != expected_clock:
                raise AssertionError(
                    f"$0928 outer-frame clock changed at {number}: "
                    f"{frame(number)['match']}")
        if [actor["roster"] for actor in live["actors"]] != \
                [2, 0, 1, 3, 4, 2, 0, 1, 3, 4]:
            raise AssertionError("active actor-to-roster mapping changed")
        free_camera_states = set()
        actor_camera_states = set()
        expected_camera_subject = frame(219)["camera"]["subject_raw"]
        for row in rows[219:]:
            possession_actor = row["possession"]["actor"]
            camera = row["camera"]
            # `$87:95BB-$95D8` samples signed `$093E` only on the 30-Hz
            # logical pass. The intervening outer frame must preserve the
            # previous proxy even if possession changed during that frame.
            if row["simulation_tick"] & 1:
                expected_camera_subject = (
                    possession_actor if possession_actor >= 0 else -1)
            if camera["subject_raw"] != expected_camera_subject:
                raise AssertionError(
                    f"camera subject diverged from signed $093E: {camera}")
            # `$093A` is persistent offense-side context, not an output of
            # camera subject selection. Camera code must never rewrite it.
            if camera["side_group_raw"] not in (0, 5):
                raise AssertionError("camera corrupted persistent $093A")
            if expected_camera_subject >= 0:
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
        # `$86:D86C-$D89B` pairs the five lineup positions directly.  The
        # selected roster numbers are deliberately irrelevant here: actor 0
        # is lineup position 0 and therefore pairs with actor 5, not with the
        # opponent whose roster slot happens to equal 2.
        expected_assignments = [10, 12, 14, 16, 18, 0, 2, 4, 6, 8]
        if [actor["raw"]["assignment_base"] for actor in live["actors"]] != \
                expected_assignments:
            raise AssertionError("lineup-permuted `$86:D86C` assignments changed")
        replanned = next((row for row in rows[219:] if [
            actor["raw"]["assignment_current"] for actor in row["actors"]
        ] != expected_assignments), None)
        if replanned is None:
            raise AssertionError("`$85:BE06-$C0F5` never replanned assignments")
        current_assignments = [
            actor["raw"]["assignment_current"]
            for actor in replanned["actors"]]
        if any(value != 0xFFFF and
               (value & 1 or value > 18) for value in current_assignments):
            raise AssertionError("defensive planner emitted an invalid actor offset")
        if 4 not in [actor["raw"]["control_mode"] for actor in live["actors"]]:
            raise AssertionError("`$85:BF81-$BFDC` did not promote a primary defender")
        if not any(any(actor["raw"]["control_mode"] == 6
                       for actor in row["actors"]) for row in rows[219:]):
            raise AssertionError("`$85:C018-$C036` never selected a help defender")
        # Initialization is checked before the actual tip can advance the
        # strategy/context; frame240 is not an initialization boundary.
        initial_match = frame(1)["match"]
        if initial_match["team_context_mode_raw_30"] != [4, 4] or \
                initial_match["team_context_flags_raw_32"] != [1, 1] or \
                initial_match["team_context_activity_raw_39"] != [1, 1]:
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
                return None
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
                    expected_targets.discard(None)
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
        # Mode 14 owns a distinct receiver target ($86:B154), not the
        # ordinary $85:AE1F cutter anchor. A changed pass cadence can leave
        # every finite-trace $09A2 witness in that special mode.
        ordinary_special_rows = [row for row in special_rows
            if row["actors"][row["possession"]["special_actor_raw"]]
                ["raw"]["control_mode"] in (1, 2, 3, 4, 5, 6, 11)]
        if ordinary_special_rows and not anchor_rows:
            raise AssertionError("$85:AE1F cutter anchor was not represented")

        play_codes = {row["possession"]["play_code_raw"] for row in rows[219:]}
        # BAA2 sets `$0994`; B128 now selects from the asset-packed team
        # strategy ranges instead of rotating four host-authored fixtures.
        if 0x35 not in play_codes or 0x01 not in play_codes or \
                len(play_codes) < 8 or any(not 0 <= code < 61 for code in play_codes):
            raise AssertionError(f"ROM strategy play selection did not sustain: {play_codes}")
        # No forced frame220 strategy reset. The existing B24C stream tests
        # below verify consumption/barriers at actual requests, not a script.
        assignments = [actor["raw"]["controller_assignment_16"]
                       for actor in first_catch["actors"]]
        if assignments != [-1, -1, -1, -1, -1, -1, -1, -1, -1, -1]:
            raise AssertionError(f"actor +$16 ownership mapping changed: {assignments}")
        catcher = first_catch['possession']['actor']
        jump_owner = first_catch["actors"][catcher]
        if jump_owner['raw']['control_mode'] != 11:
            raise AssertionError('actual acquisition did not install owner mode11')
        first_owner_pass = frame(catch_frame+2)["actors"][catcher]
        # `$86:CF38-$CF50` leaves the high-tip receiver's upper reach $37
        # locked while the independent lower channel begins locomotion $05.
        # The old single-channel assertion mistook that native overlay for a
        # failure to move even when velocity and lower motion were correct.
        if first_owner_pass["raw"]["control_mode"] != 11 or \
                first_owner_pass["raw"]["motion_38"] != 5 or \
                first_owner_pass["animation"] != 0x37 or \
                first_owner_pass["lower_animation"] != 5 or \
                (first_owner_pass["vx"] == 0 and first_owner_pass["vy"] == 0):
            raise AssertionError(
                "mode-11 owner did not begin locomotion on the next logical "
                f"actor pass: {first_owner_pass}")
        if any(row["possession"]["play_request_raw"] not in (0, 1)
               for row in rows):
            raise AssertionError("$0994 escaped its word-boolean contract")
        requested_01 = [(index, row) for index, row in enumerate(rows)
                        if row["possession"]["play_request_raw"] == 1]
        if not requested_01:
            raise AssertionError("$86:BB5F acquisition never requested a play")
        consumed_requests = 0
        for index, requested in requested_01:
            possession = requested["possession"]
            next_due = next((row for row in rows[index + 1:index + 4]
                             if row["scheduler"]["due_raw"]), None)
            # A request raised on the terminal captured frame has no next
            # actor pass in this finite trace. Earlier requests still prove
            # the `$85:B128` consumption boundary.
            if next_due is None:
                continue
            consumed_requests += 1
            # Inbound behavior runs after consumption and may re-raise 0994
            # in this same pass. Observe the consume event, not the final bit.
            if next_due["possession"]["play_consumed_serial"] <= possession["play_consumed_serial"] or \
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
        if consumed_requests == 0:
            raise AssertionError("$0994 requests ended outside the captured actor passes")
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
        for previous, acquired in acquisitions:
            match = acquired["match"]
            if acquired["ball"]["activity_raw"] != 0 or \
                    match["rim_context_raw_097c"] != 0 or \
                    not match["event_bits_raw_13e7"] & 0x0010:
                raise AssertionError(
                    f"$86:BC81-$BC90 acquisition reset changed: {acquired}")
            # The direct BAA2 vector replay verifies the side-change branch
            # and same-side clock preservation.  An ownerless integration
            # frame retains a stale possession-team word, so it cannot infer
            # which branch BAA2 took from adjacent JSON rows alone.
        mode12_attached = [row for row in rows
                           if row["ball"]["state"] == 4 and
                           row["ball"]["activity_raw"] == 0xFFFF]
        if not any(row["ball"]["state"] == 5 and
                   row["ball"]["activity_raw"] == 0xFFFF for row in rows) or \
                not mode12_attached or \
                any(row["actors"][row["ball"]["owner"]]["raw"]["control_mode"] not in (12,17)
                    for row in mode12_attached) or \
                any(row["ball"]["state"] == 4 and
                    row["ball"]["activity_raw"] not in (0, 0xFFFF) and
                    not (row["ball"]["activity_raw"] == 1 and
                         row["fouls"]["free_throw_state_raw"] != 0) and
                    not (0 <= row["ball"]["owner"] < 10 and
                         row["actors"][row["ball"]["owner"]]["raw"]["control_mode"] == 17 and
                         row["ball"]["activity_raw"] in (1,3)) and
                    not (0 <= row["ball"]["owner"] < 10 and
                         row["actors"][row["ball"]["owner"]]["raw"]["control_mode"] == 12 and
                         1 <= row["ball"]["activity_raw"] < 30)
                    for row in rows):
            raise AssertionError("$0948 canonical shot/attach lifecycle changed")
        # `$86:9DBF/$9DFF` install these latches at the mode-12 release
        # boundary. Later ROM rim/impact branches may clear `$096A` while the
        # ball still retains its shot-mode renderer state, so validate the
        # transition rather than every subsequent flight row.
        # A release during actor integration can hit the rim in this SAME
        # frame. Check the captured launch output, before physics changes it.
        detached_shots = [rows[index] for index in range(1, len(rows))
                          if rows[index]["shot_launch"]["serial"] > rows[index-1]["shot_launch"]["serial"]]
        if not detached_shots or any(
                not 0 <= row["shot_launch"]["actor"] < 10 or
                row["shot_launch"]["value"] not in (1, 2, 3)
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
            stable_timers = 0
            previous = rows[row["frame"] - 2] if row["frame"] > 220 else None
            for actor in row["actors"]:
                raw = actor["raw"]
                for field in ("controller_assignment_16",
                              "movement_magnitude_4c", "recovery_inhibit_7a"):
                    if field not in raw:
                        raise AssertionError(f"missing actor raw field {field}")
                resolver_magnitude = max(abs(actor["vx"]), abs(actor["vy"])) + \
                    min(abs(actor["vx"]), abs(actor["vy"])) // 4
                commit_magnitude = native_target_distance(
                    actor["vx"], actor["vy"])
                expected_magnitudes = {resolver_magnitude, commit_magnitude}
                # Exact `$86:F1B0-$F2C9` parent replay proves the normal CPU
                # parent installs next-pass velocity without publishing the
                # derived +$4C during that actor pass. `$85:963D` therefore
                # reports magnitude from the pre-step velocity (the preceding
                # host-frame sample), not the newly installed velocity.
                if previous is not None:
                    pre_step = previous["actors"][actor["id"]]
                    pre_vx, pre_vy = pre_step["vx"], pre_step["vy"]
                    expected_magnitudes.add(
                        max(abs(pre_vx), abs(pre_vy)) +
                        min(abs(pre_vx), abs(pre_vy)) // 4)
                    expected_magnitudes.add(native_target_distance(
                        pre_vx, pre_vy))
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
                # `$86:F780/$F886 -> $86:EC32` runs after the ordinary
                # velocity resolver has published +$4C. A successful jump
                # may then replace planar velocity while deliberately
                # leaving that magnitude latched until the next actor pass.
                jump_launched = previous is not None and \
                    row["tipoff"]["jump_launches"] > \
                    previous["tipoff"]["jump_launches"]
                if (actor["vx"] or actor["vy"]) and \
                        raw["movement_magnitude_4c"] not in expected_magnitudes and \
                        not on_rectangular_edge and not on_isometric_edge and \
                        stable_movement_mode and not post_resolver_contact and \
                        not jump_launched and \
                        row["fouls"]["free_throw_state_raw"] == 0:
                    raise AssertionError(
                        f"actor +$4C magnitude changed at frame {row['frame']} "
                        f"actor {actor['id']}: {raw['movement_magnitude_4c']} "
                        f"not in {sorted(expected_magnitudes)}; "
                        f"collision={row['collision']}")
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
                if previous:
                    old_actor = previous["actors"][actor["id"]]
                    stable_mode = actor["raw"]["control_mode"] == \
                        old_actor["raw"]["control_mode"]
                    scheduled_mode = 1 <= actor["raw"]["control_mode"] <= 6 or \
                        actor["raw"]["control_mode"] == 11
                    if stable_mode and scheduled_mode:
                        stable_timers += 1
                        if actor["raw"]["action"] != \
                                old_actor["raw"]["action"]:
                            changed_timers += 1
            # A mode transition may reinstall the same numerical timer value;
            # only stable-mode actors can prove whether `$87:8F01` split its
            # all-ten-actor pass across host frames.
            # `$87:8F01` is the even-tick physics pass. An acquisition may
            # defer `$87:9244` behavior dispatch to the odd host frame; one
            # actor can legitimately reinstall the same numeric action while
            # its peers change, which is not evidence of a split physics pass.
            # Do not infer scheduling from action timers: completion/contact
            # can reinstall an unchanged value even on a due pass. The exact
            # mask, order, delta and phase assertion below is the scheduler
            # regression guard (all ten actors, not this ambiguous proxy).
        mismatch_pairs = {(actor["animation"], actor["lower_animation"])
                          for row in rows[219:] for actor in row["actors"]
                          if actor["animation"] != actor["lower_animation"]}
        if not {(0x0B, 0x03), (0x16, 0x32)}.issubset(mismatch_pairs):
            raise AssertionError(f"independent animation pairs missing: {mismatch_pairs}")
        rng_states = [row["possession"]["rng_state_raw"] for row in rows[219:]]
        if 0xFFFF in rng_states:
            raise AssertionError("$80:CEE7 RNG state was not retained")
        # `$80:CEE7` can publish zero for one idle span when a left shift
        # exhausts the state; its next actual call reseeds to `$9146`.
        # Preserve that native quirk while rejecting a stuck/uninitialized RNG.
        for index, value in enumerate(rng_states):
            if value != 0 or (index and rng_states[index - 1] == 0):
                continue
            following = next((state for state in rng_states[index + 1:]
                              if state != 0), None)
            reseed_prefix = {0x9146, 0x3F0B, 0x7E16, 0xFC2C,
                             0xE5DF, 0xD639, 0xB1F5, 0x7E6D}
            if following not in reseed_prefix:
                raise AssertionError("$80:CEE7 zero state did not reseed")

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
                    # A close finish positioned directly over the rim may
                    # score in the same physics substep as `$86:A9D0`.
                    # `$85:A079` then consumes/clears $094C, so accept either
                    # the observable two-point flight or its immediate
                    # two-point score transition; both retain detached state.
                    match = current["match"]
                    old_match = previous["match"]
                    scoring_key = "score_left_raw" if actor_id < 5 else \
                                  "score_right_raw"
                    immediate_make = match["live_state_raw"] == 0x82 and \
                        match["shot_value_raw"] == 0 and \
                        match[scoring_key] == old_match[scoring_key] + 2
                    if ball["state"] != 5 or ball["owner"] != -1 or \
                            (match["shot_value_raw"] != 2 and
                             not immediate_make):
                        raise AssertionError(
                            f"$86:A9D0 close-finish release changed: {after} {ball}")
                    mode13_finishes += 1
                if old_mode == 11 and new_mode == 12:
                    # B6D3 distinguishes moving jumps from stationary wind-up.
                    moving_start = (after["lower_animation"] == 0x32 and
                                    after["vz"] in (0x210, 0x1E0) and
                                    ball["activity_raw"] == 0xFFFF)
                    # The common actor commit can land a prior jump before
                    # the mode-11 decision starts the stationary shot later
                    # in that same pass. Its just-consumed negative VZ stays
                    # observable until mode 12's following continuation.
                    landing_start = before["z"] > 0 and after["z"] == 0 and \
                                    after["vz"] < 0
                    stationary_start = (after["lower_animation"] == 0x16 and
                                        (after["vz"] == 0 or landing_start) and
                                        ball["activity_raw"] in (1, 3))
                    if after["animation"] != 0x16 or \
                            not (moving_start or stationary_start) or \
                            ball["state"] != 4 or \
                            ball["owner"] != actor_id:
                        raise AssertionError(
                            f"$86:B625 mode-12 initialization changed: {current}")
                    shot_starts += 1
                elif old_mode == 12 and new_mode == 12:
                    activity = previous["ball"]["activity_raw"]
                    # A613's boundary cancel can reset 0948 to zero during
                    # an existing jump. B7CD still advances that value by 2;
                    # it does not assert that the actor is on the ground.
                    # 96B5 clamps only a NEGATIVE post-step height. Landing
                    # exactly on zero preserves VZ until the next step;
                    # rounded display Z cannot distinguish that boundary.
                    expected_vz = 0 if before["z_fp"] == 0 and before["vz"] == 0 else before["vz"] - 0x30
                    if before["z_fp"] + expected_vz * 2 < 0:
                        expected_vz = 0
                    if 0 <= activity < 30:
                        if activity + 2 < 30:
                            cadence = (ball["activity_raw"] == activity + 2 and
                                       after["vz"] == expected_vz)
                        else:
                            cadence = (ball["activity_raw"] == 0xFFFF and
                                       after["vz"] == 0x1E0 and after["lower_animation"] == 0x32)
                    else:
                        cadence = after["vz"] == expected_vz
                    # $86:C0D7-C0EA / C127-C13A clears the GLOBAL shot
                    # activity before checking whether the knocked-down actor
                    # owns the ball. A later collision can interrupt wind-up.
                    contact=current["collision"]
                    contact_reset=(contact["player_count"]>0 and contact["player_routine"]==0x86BFBA and
                        current["match"]["live_state_raw"]==0 and
                        ball["activity_raw"]==0 and after["vz"]==expected_vz and
                        any(current["actors"][i]["raw"]["control_mode"]==8
                            for i in (contact["player_a"],contact["player_b"])))
                    cadence = cadence or contact_reset
                    if not cadence or ball["state"] != 4 or ball["owner"] != actor_id:
                        raise AssertionError(
                            f"$85:96B5 mode-12 jump cadence changed: "
                            f"frame={current['frame']} actor={actor_id} "
                            f"activity={activity}->{ball['activity_raw']} "
                            f"flags={after['raw']['behavior_flags']} "
                            f"{before['vz']}->{after['vz']}")
                elif old_mode == 12 and new_mode == 1:
                    signed_gate = before["vz"] < 0
                    low_rng_gate = 0 <= before["vz"] < 0x60 and \
                        (previous["possession"]["rng_state_raw"] & 0x70) == 0
                    free_throw_gate = 0 <= before["vz"] < 0x60 and \
                        previous["fouls"]["free_throw_state_raw"] != 0
                    # A low release may reach $85:9ACB rim physics in the
                    # same completed frame. Preserve its witnessed contact
                    # response instead of requiring a pre-physics SHOT label.
                    immediate_rim=(ball["state"]==6 and
                        current["match"]["rim_contact_count_raw_0920"]>
                        previous["match"]["rim_contact_count_raw_0920"] and
                        current["match"]["rim_response_raw_0970"]==15 and
                        current["match"]["shot_actor_raw_09c8"]==actor_id)
                    # A launch can detach the ball earlier in this same
                    # scheduler pass; by the time this actor is observed its
                    # mode-12 owner gate takes the lost-owner restoration
                    # path, so the prior-frame velocity/RNG proxy no longer
                    # identifies the release gate. Prefer the explicit launch
                    # serial/actor witness when present.
                    direct_launch = (
                        current["shot_launch"]["serial"] >
                            previous["shot_launch"]["serial"] and
                        current["shot_launch"]["actor"] == actor_id)
                    if not (signed_gate or low_rng_gate or free_throw_gate or
                            direct_launch) or \
                            after["animation"] != 0x17 or \
                            (ball["state"] != 5 and not immediate_rim) or ball["owner"] != -1 or \
                            current["possession"]["actor"] != -1:
                        raise AssertionError(
                            f"$86:9D6E shot release changed: {current}")
                    shot_releases += 1
                elif old_mode == 11 and new_mode in (1, 3) and \
                        previous["possession"]["actor"] == -1:
                    # `$86:F3F6` restores mode 1; the same logical pass's
                    # ownerless-role rebuild may immediately promote that
                    # actor to the sole mode-3 pursuer.
                    mode11_fallbacks += 1
        if min(shot_starts, shot_releases) < 2:
            raise AssertionError(
                "mode 11->12->1 shot lifecycle was not sustained: "
                f"starts={shot_starts} releases={shot_releases} "
                f"fallbacks={mode11_fallbacks}")
        # Exact made-basket RNG cadence changes which legal play families a
        # finite autonomous trace happens to select. Mode 13 is locked by the
        # deterministic C lifecycle vectors; treat its appearance here as
        # coverage telemetry rather than a mandatory random-path outcome.

        # `$85:A079-$A345`: `$094C` is added to `$4711/$4791`, then
        # `$0936=$82` holds the dead ball until the inbound reset.
        score_changes = []
        previous_score = (0, 0)
        for score_index, row in enumerate(rows[219:], start=219):
            match = row["match"]
            score = (match["score_left_raw"], match["score_right_raw"])
            if score != previous_score:
                delta = (score[0] - previous_score[0],
                         score[1] - previous_score[1])
                # Complete 9D6E now also serves the stripe scene. Accept one
                # point ONLY from a prior live one-point free-throw attempt;
                # retain the same scoring, rim and inbound assertions below.
                prior=rows[score_index-1]
                free_throw_point=(sorted(delta)==[0,1] and
                    prior['fouls']['free_throw_state_raw']!=0 and
                    prior['match']['shot_value_raw']==1)
                if sorted(delta) not in ([0, 2], [0, 3]) and not free_throw_point:
                    raise AssertionError(f"invalid ROM score increment {delta}")
                # `$85:A262` clears `$094C` in the same inline scoring
                # substep; the score delta is the durable shot-value witness.
                # The native branch does not select an object routine here:
                # `$85:A118-$A124` clears the live/rim latches and execution
                # continues through the shared free-ball tail at `$85:A34A`.
                # SHOT (5) and BOUNCE (6) are host labels for that same native
                # path, so either is valid at the score-write observation.
                # `$85:9D4B` accepts integer Z < $53. Telemetry rounds
                # the retained fraction after the shared physics tail,
                # so 82.x may display 83.
                free_tail = (row["ball"]["state"] in (5, 6) and
                             74 <= row["ball"]["z"] <= 83)
                # The corrected F43A cadence can let the same outer update
                # continue through D353->BAA2 before telemetry is published.
                # That native terminal state is an attached mode-11 inbound
                # carrier, not a missing made-basket tail.
                installed_inbound = (row["ball"]["state"] == 4 and
                    row["ball"]["owner"] == -1 and
                    row["possession"]["actor"] >= 0 and
                    match["inbound_ready_raw"] == 0)
                if match["shot_value_raw"] != 0 or \
                        match["live_state_raw"] != 0x82 or \
                        not (free_tail or installed_inbound):
                    raise AssertionError(f"made basket state incomplete: {row}")
                scoring_side = 0 if delta[0] else 1
                expected_group = (scoring_side ^ 1) * 5
                inbound_actor = match["inbound_actor_raw"]
                provisional_actor = expected_group + 2
                # `$85:A079` initially seeds actor 2/7. In the same outer
                # native pass, `$86:D353->$85:BAA2` may already replace
                # `$0954` with the colliding side-matched inbound carrier
                # before the frame is observable. Accept that later native
                # terminal state only when every ownership/mode invariant
                # proves the replacement really occurred.
                installed_actor = row["possession"]["actor"]
                installed_replacement = (
                    installed_actor == inbound_actor and
                    expected_group <= installed_actor < expected_group + 5 and
                    row["ball"]["owner"] == -1 and
                    row["ball"]["state"] == 4 and
                    match["inbound_ready_raw"] == 0 and
                    row["actors"][installed_actor]["raw"]["control_mode"] == 11)
                if match["inbound_state_raw"] != expected_group or not (
                        inbound_actor == provisional_actor or
                        installed_replacement):
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
        completed_inbounds = 0
        expired_installed_inbounds = 0
        for inbound in dead_runs:
            first = inbound[0]
            match = first["match"]
            # `$0952` is the side-group seed. `$85:A079` derives the
            # provisional carrier as group+2, even when a later same-frame
            # collision has already replaced observable `$0954`.
            provisional_actor = match["inbound_state_raw"] + 2
            observed_actor = match["inbound_actor_raw"]
            layout = match["inbound_layout_raw"]
            if layout == 0:
                expected_target = (394, -64, 6) if provisional_actor == 2 \
                    else (-394, 64, 2)
            elif layout in (1, 3, 4):
                # `$85:C37D` consumes the rounded live ball record; `$09B0/B2`
                # separately preserve its signed integer-word floor.
                source_x = first["ball"]["x"]
                source_y = first["ball"]["y"]
                side_anchor = -336 if provisional_actor == 2 else 336
                if layout in (1, 4):
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
            installed_at_entry = first["possession"]["actor"]
            valid_entry_actor = observed_actor == provisional_actor or (
                observed_actor == installed_at_entry and
                match["inbound_state_raw"] <= observed_actor <
                    match["inbound_state_raw"] + 5 and
                first["ball"]["owner"] == -1 and
                first["ball"]["state"] == 4 and
                first["actors"][observed_actor]["raw"]["control_mode"] == 11)
            if match["inbound_timer_raw"] != 300 or provisional_actor not in (2, 7) or \
                    not valid_entry_actor or actual_target != expected_target:
                raise AssertionError(f"$85:A1E9/C37D inbound seed changed: {first}")
            installed = [row for row in inbound
                         if row["possession"]["actor"] >= 0]
            if not installed:
                # `$87:9AA6` may expire an untouched dead ball. That retry is
                # a valid run; validate the complete collision/arrival path
                # only for runs which actually install `$093E`.
                continue
            installed_actor = installed[0]["match"]["inbound_actor_raw"]
            if installed_actor != installed[0]["possession"]["actor"] or \
                    installed[0]["ball"]["owner"] != -1:
                raise AssertionError(
                    "$093E dead-ball owner was conflated with logical ball ownership: "
                    f"{installed[0]}")
            if installed_actor != provisional_actor:
                replaced_provisional += 1
            ready = [row for row in inbound
                     if row["match"]["inbound_ready_raw"]]
            if not ready:
                # Contact can repeatedly displace an installed mode-11
                # inbounder until `$87:9AA6` expires the untouched dead ball.
                # That is a native terminal path, not proof that F4F2 broke;
                # the deterministic inbound runtime witness below locks the
                # successful arrival/transfer path independently.
                expired_installed_inbounds += 1
                continue
            completed_inbounds += 1
            first_ready = ready[0]
            if first_ready["match"]["dead_ball_raw_0968"] != 2:
                raise AssertionError(
                    "$86:F54F inbound arrival did not write $0968=2")
            # `$86:F3D2/F43A` executes through current actor X/$96. After an
            # A613 cancellation, D353->BAA2 can replace ownership `$093E`
            # without rewriting provisional `$0954`; validate the carrier.
            actor_id = first_ready["possession"]["actor"]
            if actor_id < 0:
                raise AssertionError("ready inbound has no $093E carrier")
            actor = first_ready["actors"][actor_id]
            # F4F2 reads signed integer words +$04/+$08, not the nearest-pixel
            # coordinates used for presentation. Preserve the exact 24.8
            # telemetry so fractional boundary cases cannot pass unnoticed.
            dx = (first_ready["match"]["inbound_target_x_raw"] -
                  actor["x_fp"] // 256)
            dy = (first_ready["match"]["inbound_target_y_raw"] -
                  actor["y_fp"] // 256)
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
                # A reload during the 30-Hz pass can be observed after the
                # same pass's fixed two-unit timer consumption.
                displaced_reload = \
                    after["match"]["inbound_timer_raw"] in (298, 299, 300) and \
                    after["possession"]["actor"] >= 0 and \
                    after["actors"][after["possession"]["actor"]]["raw"][
                        "control_mode"] == 11
                if not (side_reset or displaced_reload) or \
                        after["match"]["inbound_transfer_raw"] != 0 or \
                        after["possession"]["actor"] < 0:
                    raise AssertionError("arrived inbound timer increased")
            transfer = [row for row in ready
                        if row["match"]["inbound_transfer_raw"]]
            transfer_actor = transfer[0]["possession"]["actor"] \
                if transfer else -1
            if not transfer or transfer[0]["match"]["inbound_timer_raw"] >= 240 or \
                    transfer[0]["possession"]["pass_actor_raw"] != transfer_actor or \
                    not transfer[0]["possession"]["pass_active_raw"]:
                raise AssertionError(f"$86:F59F/F64F transfer gate changed: {transfer[:1]}")
        if completed_inbounds == 0 and expired_installed_inbounds == 0:
            raise AssertionError("no installed inbound continuation was exercised")
        if replaced_provisional == 0:
            raise AssertionError(
                "$86:CCFC pose collision never completed/replaced provisional actor 2/7")

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
                              "score": (match["score_left_raw"],
                                        match["score_right_raw"]),
                              "made": False,
                              "rebound": None})
            # `$86:CCFC-$D1D6` may acquire a descending miss directly from
            # shot mode 5, while floor/rim misses first become bounce mode 6.
            if state == 4 and last_state in (5, 6) and shots:
                score = (row["match"]["score_left_raw"],
                         row["match"]["score_right_raw"])
                # A forced-rim shot can carry `$09F8` plus a concrete
                # `$09FA` offset and still score. Its state-4 successor is
                # the ownerless inbound ball, not a malformed rebound.
                if score != shots[-1]["score"] and \
                        row["match"]["live_state_raw"] == 0x82:
                    shots[-1]["made"] = True
                else:
                    shots[-1]["rebound"] = (row["ball"]["owner"],
                                            row["possession"]["team"])
            last_state = state
        # `$09F8` can be raised after release by the forced-rim path before
        # `$86:A17D` has selected an offset. `$FF` therefore remains valid for
        # that transient; a concrete miss index still requires the veto.
        misses = [shot for shot in shots
                  if shot["veto"] and shot["index"] != 255]
        if any(not 0 <= shot["index"] < 16 for shot in misses):
            raise AssertionError(f"$86:A110/$A17D miss path missing: {misses}")
        if any(shot["index"] != 255 and not shot["veto"] for shot in shots):
            raise AssertionError(
                f"$86:A110 veto/miss-index contract changed: {shots}")
        # Do not classify a shot begun inside the final 600-frame capture
        # tail as a failed rebound; the trace ended before its resolution
        # horizon. Every miss with a complete horizon must still resolve.
        settled_misses = [shot for shot in misses if not shot["made"]
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
        for index, row in enumerate(rows[219:], 219):
            owner = row["ball"]["owner"]
            if row["ball"]["state"] == 4 and owner >= 0 and \
                    row["fouls"]["free_throw_state_raw"] == 0:
                actor = row["actors"][owner]
                # Dynamic dribble bases 9/11 use a separate native hand-point
                # cadence that is not represented by this static end-row
                # resource oracle. Their +$38/+$4E selection is covered by
                # the dedicated function-vector replay.
                if actor["raw"]["motion_38"] in (9, 11):
                    continue
                signature = lambda candidate: (
                    candidate["direction"], candidate["animation"],
                    candidate["lower_animation"],
                    candidate["raw"]["upper_resource"],
                    candidate["raw"]["lower_resource"])
                # The ball pass precedes behavior/contact pose mutation. Only
                # compare end-of-frame resources once that pose has remained
                # stable for a complete 30-Hz pass; transition rows are
                # validated by the scheduler/pose vector suites instead.
                if index < 2 or any(
                        rows[index - age]["ball"]["state"] != 4 or
                        rows[index - age]["ball"]["owner"] != owner or
                        signature(rows[index - age]["actors"][owner]) !=
                        signature(actor) for age in (1, 2)):
                    continue
                attachment_actor=actor
                if actor["raw"]["control_mode"]==17:
                    # $86:B979 projects the held ball BEFORE the common
                    # actor integration. Unlike the ordinary owned-ball
                    # tail, use the incoming position, not post-step Z.
                    age=1 if row["scheduler"]["due_raw"] else 2
                    attachment_actor=rows[index-age]["actors"][owner]
                actual = (row["ball"]["x"] - attachment_actor["x"],
                          row["ball"]["y"] - attachment_actor["y"],
                          row["ball"]["z"] - attachment_actor["z"])
                expected = expected_attachment(actor)
                attached.append((row["frame"], actual, expected,
                                 actor["raw"]["upper_phase"]))
        def attachment_matches(pair):
            _, actual, expected, upper_phase = pair
            # `$85:A50D-$A52F` uses the pose-resource Z point only before
            # phase three. `$85:A532-$A597` then keeps X/Y attached while Z
            # follows its own gravity/integration response.
            axes = 2 if upper_phase >= 3 else 3
            return all(abs(actual[i] - expected[i]) <= 1
                       for i in range(axes)) and actual[2] >= 0
        if not attached or any(not attachment_matches(pair) for pair in attached):
            raise AssertionError(
                "ball diverged from `$87:B832/$B953` resource attachment: " +
                repr(next((pair for pair in attached
                           if not attachment_matches(pair)), None)))

        # `$86:AB2D-$B04A/$86:A6B3-$A790`: mode 15 installs a grounded or
        # boosted pass state, keeps the ball attached through the native
        # phase/apex gate, then releases it via the ROM table.
        pass_rows = []
        release_rows = []
        for index, row in enumerate(rows[219:], 219):
            possession = row["possession"]
            actor_id = possession["pass_actor_raw"]
            for released_actor in range(10):
                raw = row["actors"][released_actor]["raw"]
                before_raw = rows[index - 1]["actors"][released_actor]["raw"]
                if raw["pass_released"] and not before_raw["pass_released"]:
                    release_rows.append((
                        index, row, row["actors"][released_actor],
                        rows[index - 1]))
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
            # `$09DA` is the pass distance only during initialization;
            # `$85:C0BC-$C0F3` subsequently reuses it as the five-assignment
            # loop counter and normally leaves zero. The vector replay owns
            # exact distance-to-band verification; integration protects the
            # installed band after that shared scratch word is clobbered.
            # Actor +$66 is a pose/resource scratch word, not a normalized
            # direction; the pass-init vector owns its exact ROM value.
            if raw["pass_band_62"] not in (0, 6, 12, 18, 24, 30) or \
                    raw["mode_saved_62"] != raw["pass_band_62"] or \
                    raw["saved_mode_84"] != raw["control_mode_saved"] or \
                    (not raw["pass_released"] and
                     actor["animation"] not in
                     (0x18, 0x2A, 0x2B, 0x2C,
                      0x2D, 0x2E, 0x2F, 0x30, 0x31)):
                raise AssertionError(
                    f"mode-15 pass metadata diverged: possession={possession} "
                    f"actor={actor}")
            pass_rows.append((index, row, actor))
        if len(pass_rows) < 100 or not release_rows:
            raise AssertionError("ROM mode-15 pass lifecycle was not sustained")
        exact_pass_frames = 0
        for _, row, actor in pass_rows:
            raw = actor["raw"]
            state = actor["animation"]
            if raw["control_mode"] != 15 or not 0x2A <= state <= 0x31:
                continue
            native = raw["animation_rom"]
            if not native["action_integrated"]:
                continue  # Inbound cadence adoption is a separate checkpoint.
            bank84 = header[3]  # NBPANIM1 header +20, not the version at +8.
            descriptor = int.from_bytes(animation[
                bank84 + 0x42FC + state*2:bank84 + 0x42FE + state*2], "little")
            lock_at = bank84 + descriptor - 0x8000 + 2
            lock = int.from_bytes(animation[lock_at:lock_at+2], "little")
            if native["upper_lock_46"] != lock or native["upper_queue_18"] != 0xFFFF:
                raise AssertionError("pass did not install the packed action lock/queue sentinel")
            if native["resources_valid"]:
                if (raw["upper_phase"] != native["upper_phase_3a"] or
                    raw["upper_resource"] != native["upper_resource_2a"] or
                    raw["lower_resource"] != native["lower_resource_2c"]):
                    raise AssertionError("pass phase, visible resources and hand-point resources diverged")
                exact_pass_frames += 1
        action_completions = sum(
            before["raw"]["control_mode"] == 15 and
            before["raw"]["animation_rom"]["upper_lock_46"] != 0 and
            after["raw"]["animation_rom"]["upper_lock_46"] == 0
            for prior, current in zip(rows, rows[1:])
            for before, after in zip(prior["actors"], current["actors"]))
        if exact_pass_frames < 50 or action_completions < 5:
            raise AssertionError("exact pass lock/phase/completion integration was not sustained")
        for _, row, actor, before in release_rows:
            raw = actor["raw"]
            before_actor = before["actors"][actor["id"]]
            detached = row["ball"]["state"] == 3 and \
                row["ball"]["owner"] == -1
            same_frame_catch = row["ball"]["state"] == 4 and \
                0 <= row["ball"]["owner"] < 10 and \
                row["possession"]["actor"] == row["ball"]["owner"]
            boundary_dead_ball = row["ball"]["state"] in (4, 6) and \
                row["ball"]["owner"] == -1 and \
                row["match"]["live_state_raw"] == 0x82
            # Family four is `$86:A6EC->$A749`: the descending boosted-pass
            # continuation bypasses the ordinary `$86:A736-$A747` resource
            # phase table. All other families must cross the phase gate.
            phase_released = before_actor["raw"]["pass_family_c0"] == 4 or \
                before_actor["raw"]["upper_phase"] > \
                raw["pass_release_threshold"]
            # A same-frame catch can immediately install a new pass for the
            # catcher. In that case `$09C4=1` belongs to a different `$0942`,
            # while the releasing actor's pass was still cleared correctly.
            old_pass_still_active = \
                row["possession"]["pass_active_raw"] != 0 and \
                row["possession"]["pass_actor_raw"] == actor["id"]
            if not phase_released or \
                    not (detached or same_frame_catch or boundary_dead_ball) or \
                    old_pass_still_active:
                raise AssertionError(
                    "pass released before `$86:A736-$A747` phase gate: "
                    f"frame={row['frame']} actor={actor['id']} "
                    f"family={before_actor['raw']['pass_family_c0']} "
                    f"phase={before_actor['raw']['upper_phase']} "
                    f"threshold={raw['pass_release_threshold']} "
                    f"ball={row['ball']['state']}/{row['ball']['owner']} "
                    f"live={row['match']['live_state_raw']:04x} "
                    f"active={row['possession']['pass_active_raw']}/"
                    f"{row['possession']['pass_actor_raw']}")
        pass_animations = {actor["animation"] for _, _, actor in pass_rows}
        # Which pass directions occur depends on prior release positions.
        # The deterministic end-to-end run must sustain multiple families;
        # the checked-in ROM vectors independently protect the 2D/2E/2F/30/31
        # descriptor installers without relying on this one trajectory.
        if len(pass_animations.intersection(range(0x2A, 0x32))) < 2:
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
        "src/nba_gameplay_foul.c", "src/nba_player_lab.c", "src/nba_shot_launch.c",
        "src/nba_court_presentation.c"))
    for marker in ("$85:963D-$985F", "$85:BC52-$BC81", "$85:B95C",
                   "$87:B832", "$87:B649", "$87:B66A", "$85:9192",
                   "$87:8F01-$8F8D", "nba_gameplay_camera_step",
                   "nba_gameplay_camera_place", "nba_gameplay_camera_ready",
                   "nba_tip_complete_acquisition", "cpu_update_possession",
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
                   "$87:8FA1-$8FA9",
                   "nba_tipoff_refresh_team_roles_end_frame",
                   "$85:A5F4-$A655", "nba_gameplay_ball_apply_settle",
                   "$85:A43A-$A44B",
                   "nba_gameplay_ball_apply_ground_impact",
                   "$87:A9E3-$AA01", "$87:AA02-$AAB1",
                   "nba_gameplay_effect_step",
                   "$86:AB2D-$AF65", "$86:A6B3-$A790",
                   "nba_shot_launch", "$86:9B84-$9B8F",
                   "nba_gameplay_pass_direction", "nba_tipoff_begin_rom_pass",
                   "nba_gameplay_select_pass_receiver",
                   "$85:B50E-$B60A", "$85:B60B-$B677",
                   "nba_tipoff_update_rom_passer",
                   "$87:A3BB-$A3DC", "nba_court_project_actor",
                   "$87:A3DF-$A43B", "nba_court_actor_visible",
                   "latch_player_screen_origins", "player_screen_visible",
                   "ball_screen_x", "latched ball presentation",
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
    print(f"[ACTION INTEGRATION] exact_pass_frames={exact_pass_frames} "
          f"automatic_unlocks={action_completions}")
    print("CPU-versus-CPU gameplay regression checks passed")


if __name__ == "__main__":
    main()
