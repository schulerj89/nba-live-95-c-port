"""Normalize genuine `$86:B769-$B978` calls into compact state witnesses."""

import argparse
import hashlib
import json
from pathlib import Path


ROM_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
SIZE = 0x10000
CASE_NAMES = (
    "lost_owner_same_group", "lost_owner_other_group", "pump_phase_three_wait",
    "pump_phase_four_acc_05ff_wait", "pump_phase_four_acc_0600_cancel",
    "delay_cpu_activity_one", "delay_human_held_0080",
    "delay_human_release_latches", "delay_human_free_throw_no_latch",
    "delay_activity_001b_to_001d", "activity_001c_crosses_jump",
    "activity_7fff_wraps_airborne", "negative_activity_ffff_airborne",
    "free_throw_cross_skips_jump", "sidestep_movement_nonzero",
    "sidestep_anchor_0119_reject_signed", "sidestep_anchor_0077_candidate",
    "sidestep_anchor_0078_reject", "sidestep_abs_x_56_rng_even_reject",
    "sidestep_abs_x_55_reject", "sidestep_abs_x_56_rng_odd_moves",
    "sidestep_abs_x_negative_56_rng_odd_moves", "grounded_human_latches",
    "grounded_cpu_returns", "turn_lower_05ff_preserves_4e",
    "turn_lower_0600_decrements_4e", "turn_lower_0600_increments_4e",
    "human_vz_fe80_releases", "human_vz_fe81_held_waits",
    "human_vz_fe81_released", "human_free_throw_releases",
    "cpu_vz_negative_releases", "cpu_vz_zero_rng_zero_releases",
    "cpu_vz_005f_rng_nonzero_waits", "cpu_vz_0060_waits",
    "cpu_free_throw_vz_005f_releases", "cpu_free_throw_vz_0060_waits",
)

GLOBAL_FIELDS = (
    ("rng_raw_07f6", 0x07F6), ("shot_origin_x_raw_0900", 0x0900),
    ("shot_origin_y_raw_0902", 0x0902), ("controller_pointer_raw_090c", 0x090C),
    ("ball_record_raw_0910", 0x0910), ("bounce_timer_raw_091c", 0x091C),
    ("bounce_raw_0920", 0x0920), ("previous_actor_x_raw_0922", 0x0922),
    ("period_raw_0926", 0x0926), ("clock_raw_0928", 0x0928),
    ("timeout_raw_0930", 0x0930), ("live_raw_0936", 0x0936),
    ("offense_group_raw_093a", 0x093A), ("owner_raw_093e", 0x093E),
    ("activity_raw_0948", 0x0948), ("attempt_raw_094a", 0x094A),
    ("value_raw_094c", 0x094C), ("dead_raw_0966", 0x0966),
    ("height_raw_0968", 0x0968), ("initial_raw_096a", 0x096A),
    ("dead_raw_096c", 0x096C), ("free_throw_raw_0978", 0x0978),
    ("attempts_raw_097a", 0x097A), ("aim_power_raw_0980", 0x0980),
    ("aim_accuracy_raw_0982", 0x0982), ("assistance_team_raw_09c0", 0x09C0),
    ("last_owner_raw_09c8", 0x09C8), ("attachment_raw_09f6", 0x09F6),
    ("inner_veto_raw_09f8", 0x09F8), ("difficulty_raw_17af", 0x17AF),
    ("assistance_raw_17bf", 0x17BF), ("shot_control_raw_17c3", 0x17C3),
    ("display_shooter_raw_493b", 0x493B), ("display_value_raw_4939", 0x4939),
)
ACTOR_OFFSETS = (
    0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E, 0x10, 0x12, 0x16,
    0x18, 0x1A, 0x1C, 0x1E, 0x20, 0x22, 0x24, 0x26, 0x28, 0x2A,
    0x2C, 0x30, 0x32, 0x38, 0x3A, 0x3C, 0x42, 0x44, 0x46, 0x48,
    0x4A, 0x4C, 0x4E, 0x50, 0x52, 0x5A, 0x5E, 0x60, 0x64, 0x6C,
    0x6E, 0x72, 0x7E, 0x88, 0x8A, 0x8C, 0xA8, 0xB0, 0xB2,
)
ACTOR_NAMES = (
    "actor_x_fraction_raw_02", "actor_x_integer_raw_04",
    "actor_y_fraction_raw_06", "actor_y_integer_raw_08",
    "actor_z_fraction_raw_0a", "actor_z_integer_raw_0c",
    "actor_vx_raw_0e", "actor_vy_raw_10", "actor_vz_raw_12",
    "actor_controller_raw_16", "actor_upper_cursor_raw_18",
    "actor_lower_cursor_raw_1a", "actor_upper_queue_0_raw_1c",
    "actor_upper_queue_1_raw_1e", "actor_upper_queue_2_raw_20",
    "actor_lower_queue_0_raw_22", "actor_lower_queue_1_raw_24",
    "actor_lower_queue_2_raw_26", "actor_status_raw_28",
    "actor_upper_resource_raw_2a", "actor_lower_resource_raw_2c",
    "actor_upper_state_raw_30", "actor_lower_state_raw_32",
    "actor_base_state_raw_38", "actor_upper_phase_raw_3a",
    "actor_lower_phase_raw_3c", "actor_upper_accumulator_raw_42",
    "actor_lower_accumulator_raw_44", "actor_upper_lock_raw_46",
    "actor_lower_lock_raw_48", "actor_speed_raw_4a",
    "actor_movement_raw_4c", "actor_facing_raw_4e",
    "actor_requested_raw_50", "actor_displayed_raw_52",
    "actor_contact_raw_5a", "actor_mode_raw_5e", "actor_timer_raw_60",
    "actor_behavior_timer_raw_64", "actor_variant_raw_6c",
    "actor_group_raw_6e", "actor_boost_raw_72", "actor_flags_raw_7e",
    "actor_anchor_direction_raw_88", "actor_defense_raw_8a",
    "actor_anchor_distance_raw_8c", "actor_free_throw_half_raw_a8",
    "actor_upper_target_raw_b0", "actor_modifier_raw_b2",
)
BALL_OFFSETS = (0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E, 0x10, 0x12)
BALL_NAMES = (
    "ball_x_fraction_raw_02", "ball_x_integer_raw_04",
    "ball_y_fraction_raw_06", "ball_y_integer_raw_08",
    "ball_z_fraction_raw_0a", "ball_z_integer_raw_0c",
    "ball_vx_raw_0e", "ball_vy_raw_10", "ball_vz_raw_12",
)
CONTEXT_FIELDS = (
    "context0_team_raw_00", "context0_anchor_raw_0a", "context0_assist_raw_43",
    "context0_controller_raw_45", "context0_clock_raw_47",
    "context1_team_raw_00", "context1_anchor_raw_0a", "context1_assist_raw_43",
    "context1_controller_raw_45", "context1_clock_raw_47",
)
CONTROLLER_FIELDS = tuple(f"controller_{n}_held_raw_08" for n in range(5))
PLAYER_FIELDS = tuple(f"player_shot_stat_{n}_raw_{n * 2:02x}" for n in range(5)) + \
                ("player_stamina_raw_18",)
CONTROLLER_STAT_FIELDS = tuple(
    f"controller_0_shot_stat_{n}_raw_{0x12 + n * 2:02x}" for n in range(5))
ROSTER_OUTPUT_FIELDS = ("roster_low_raw_0914", "roster_bank_raw_0916")
FIELDS = ("slot",) + tuple(name for name, _ in GLOBAL_FIELDS) + ACTOR_NAMES + \
         BALL_NAMES + CONTEXT_FIELDS + CONTROLLER_FIELDS + \
         PLAYER_FIELDS + CONTROLLER_STAT_FIELDS + ROSTER_OUTPUT_FIELDS + \
         ("scratch_raw_0046", "scratch_raw_0047")


def load_jsonl(path):
    return [json.loads(line) for line in Path(path).read_text().splitlines() if line.strip()]


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def memory(snapshot):
    raw = bytearray(SIZE)
    covered = bytearray(SIZE)
    for base, payload in snapshot["mem"].items():
        address = int(base, 16)
        data = bytes.fromhex(payload)
        raw[address:address + len(data)] = data
        covered[address:address + len(data)] = b"\x01" * len(data)
    return raw, covered


def word(raw, address, covered=None):
    if covered is not None and not (covered[address] and covered[address + 1]):
        raise ValueError(f"projection address {address:04x} was not captured")
    return raw[address] | raw[address + 1] << 8


def projection(memory_pair):
    raw, covered = memory_pair
    slot = word(raw, 0x00C2, covered)
    actor = 0x34EB + slot * 0x100
    result = [slot]
    result.extend(word(raw, address, covered) for _, address in GLOBAL_FIELDS)
    result.extend(word(raw, actor + offset, covered) for offset in ACTOR_OFFSETS)
    result.extend(word(raw, 0x3EEB + offset, covered) for offset in BALL_OFFSETS)
    for context in (0x46EB, 0x476B):
        result.extend(word(raw, context + offset, covered) for offset in (0, 0x0A, 0x43, 0x45, 0x47))
    result.extend(word(raw, 0x47EB + pad * 0x40 + 8, covered) for pad in range(5))
    actor_index = word(raw, actor, covered)
    player = word(raw, 0x3435 + actor_index * 2, covered)
    result.extend(word(raw, player + offset, covered)
                  for offset in (0, 2, 4, 6, 8, 0x18))
    result.extend(word(raw, 0x47EB + offset, covered)
                  for offset in (0x12, 0x14, 0x16, 0x18, 0x1A))
    result.extend((word(raw, 0x0914, covered), word(raw, 0x0916, covered)))
    result.extend((word(raw, 0x0046, covered), word(raw, 0x0047, covered)))
    if len(result) != len(FIELDS):
        raise AssertionError("projection field count changed")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("vectors", "cases", "paths", "repeat-vectors", "repeat-cases", "repeat-paths"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--ghidra-functions", type=Path, required=True)
    parser.add_argument("--ghidra-instructions", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    for primary, repeat in ((args.cases, args.repeat_cases),
                            (args.paths, args.repeat_paths)):
        if primary.read_bytes() != repeat.read_bytes():
            raise ValueError(f"repeat capture differs: {primary.name}")
    vectors, repeat_vectors, cases, paths = map(load_jsonl, (args.vectors, args.repeat_vectors, args.cases, args.paths))
    if not (len(vectors) == len(repeat_vectors) == len(cases) == len(paths) == len(CASE_NAMES)):
        raise ValueError("expected 37 aligned mode-twelve witnesses")
    calls = []
    executed_union = set()
    exits = set()
    aggregate_children = {name: 0 for name in (
        "attach_87b832", "direction_85f02d", "cancel_lower_87b555",
        "animation_87b4db", "restore_869846", "cancel_upper_87b538",
        "launch_869d6e")}
    for number, (vector, case, path, name) in enumerate(zip(vectors, cases, paths, CASE_NAMES), 1):
        if vector.get("call") != number or case["case"] != number or \
                path["case"] != number or case["name"] != name or \
                path["name"] != name:
            raise ValueError(f"case {number} is not aligned")
        if vector["entry_frame"] != vector["exit_frame"] or vector["entry_pc"] != "86b769" or \
                vector["exit_pc"] != path["exit_pc"]:
            raise ValueError(f"case {number} has an invalid native boundary")
        executed = path["executed"]
        if not executed or executed[0] != "86b769" or executed[-1] != path["exit_pc"]:
            raise ValueError(f"case {number} has an incomplete PC path")
        before, after = memory(vector["entry"]), memory(vector["exit"])
        repeat_before, repeat_after = memory(repeat_vectors[number - 1]["entry"]), memory(repeat_vectors[number - 1]["exit"])
        repeat_vector = repeat_vectors[number - 1]
        if repeat_vector["call"] != number or repeat_vector["entry_frame"] != repeat_vector["exit_frame"] or \
                repeat_vector["entry_pc"] != "86b769" or repeat_vector["exit_pc"] != path["exit_pc"]:
            raise ValueError(f"case {number} repeat has an invalid native boundary")
        if projection(before) != projection(repeat_before) or projection(after) != projection(repeat_after):
            raise ValueError(f"case {number} relevant projection differs across repeats")
        actor = 0x34EB + projection(before)[0] * 0x100
        if word(before[0], 0x0096, before[1]) != actor or word(before[0], 0x00C6, before[1]) != 2 or word(before[0], actor + 0x5E, before[1]) != 12:
            raise ValueError(f"case {number} lacks a genuine mode-twelve entry")
        actor_index = word(before[0], actor, before[1])
        player = word(before[0], 0x3435 + actor_index * 2, before[1])
        if actor_index != 9 or player != 0x44EB:
            raise ValueError(f"case {number} has an unexpected selected player record")
        controller = word(before[0], actor + 0x16, before[1])
        if controller < 5 and word(before[0], 0x090C, before[1]) != 0x47EB + controller * 0x40:
            raise ValueError(f"case {number} violates the captured human controller invariant")
        executed_union.update(executed)
        exits.add(path["exit_pc"])
        for child, count in path["child_calls"].items():
            aggregate_children[child] += count
        calls.append({
            "call": number, "name": name,
            "entry_pc": vector["entry_pc"], "exit_pc": vector["exit_pc"],
            "executed": executed, "child_calls": path["child_calls"],
            "input": projection(before), "expected": projection(after),
        })
    if len(executed_union) != 213 or exits != {"86b790", "86b866", "86b86b", "86b88f", "86b8c8", "86b8c9", "86b978"}:
        raise ValueError("mode-twelve owned-start or exit coverage changed")
    if aggregate_children != {"attach_87b832": 32, "direction_85f02d": 11,
            "cancel_lower_87b555": 11, "animation_87b4db": 10,
            "restore_869846": 2, "cancel_upper_87b538": 1,
            "launch_869d6e": 6}:
        raise ValueError(f"aggregate child calls changed: {aggregate_children}")
    document = {
        "schema": 1, "routine": "$86:B769-$B978 mode-twelve shooter",
        "rom_sha256": ROM_SHA256,
        "provenance": "two controlled genuine-entry Mesen captures with identical relevant projections; documented WRAM inputs only; no PC, stack, ROM, RNG, or child-result patching; unrelated volatile bytes in the widened player-record window are excluded",
        "fields": list(FIELDS), "owned_starts_sha256": hashlib.sha256("\n".join(sorted(executed_union)).encode()).hexdigest(),
        "aggregate_child_calls": aggregate_children,
        "source": {"vectors_sha256": sha256(args.vectors),
                   "repeat_vectors_sha256": sha256(args.repeat_vectors),
                   "cases_sha256": sha256(args.cases),
                   "paths_sha256": sha256(args.paths),
                   "ghidra_functions_sha256": sha256(args.ghidra_functions),
                   "ghidra_instructions_sha256": sha256(args.ghidra_instructions)},
        "calls": calls,
    }
    args.output.write_text(json.dumps(document, separators=(",", ":")) + "\n", encoding="utf-8")
    print(f"[CPU MODE TWELVE NORMALIZE] calls=37 fields={len(FIELDS)} starts=213 exits=7")


if __name__ == "__main__":
    main()
