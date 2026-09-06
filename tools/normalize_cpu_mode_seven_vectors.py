"""Normalize genuine `$86:994C-$99C3` calls into compact state witnesses."""

import argparse
import hashlib
import json
from pathlib import Path


ROM_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
SIZE = 0x4B00
CASE_NAMES = (
    "live_state_restore",
    "high_byte_live_state_restore",
    "integer_height_hold",
    "negative_integer_height_hold",
    "timer_underflow_restore",
    "timer_8002_restores",
    "timer_8001_wraps_positive",
    "fractional_height_stationary",
    "stationary_human",
    "moving_human_first",
    "moving_human_y_only",
    "moving_human_repeat",
    "moving_human_boost_preserved",
    "cpu_origin_damps",
    "cpu_origin_damps_to_zero",
    "cpu_off_origin_steers",
    "stationary_matching_state",
    "stationary_negative_upper_lock",
    "restore_boost_remaps_state_three",
)
FIELDS = (
    "live_state_raw_0936", "camera_side_group_raw_093a",
    "possession_actor_raw_093e", "slot", "right_team_raw_46eb",
    "left_team_raw_476b", "roster_slot", "scratch_raw_0046",
    "x_subpixel_raw_02", "x_integer_raw_04", "y_subpixel_raw_06",
    "y_integer_raw_08", "z_subpixel_raw_0a", "z_integer_raw_0c",
    "velocity_x_raw_0e", "velocity_y_raw_10", "velocity_z_raw_12",
    "controller_raw_16", "upper_queue_cursor_raw_18",
    "lower_queue_cursor_raw_1a", "upper_queue_0_raw_1c",
    "upper_queue_1_raw_1e", "upper_queue_2_raw_20",
    "lower_queue_0_raw_22", "lower_queue_1_raw_24",
    "lower_queue_2_raw_26", "status_raw_28", "upper_state_raw_30",
    "lower_state_raw_32", "base_state_raw_38", "upper_phase_raw_3a",
    "lower_phase_raw_3c", "upper_accumulator_raw_42",
    "lower_accumulator_raw_44", "upper_lock_raw_46",
    "lower_lock_raw_48", "movement_direction_raw_4e",
    "requested_direction_raw_50", "display_direction_raw_52",
    "control_mode_raw_5e", "timer_raw_60", "behavior_timer_raw_64",
    "direction_latch_raw_66", "team_group_raw_6e", "boost_raw_72",
    "behavior_flags_raw_7e", "free_throw_half_raw_a8",
    "upper_phase_target_raw_b0",
    "upper_resource_raw_2a", "lower_resource_raw_2c",
)


def load_jsonl(path):
    return [json.loads(line) for line in Path(path).read_text().splitlines()
            if line.strip()]


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def memory(snapshot):
    result = bytearray(SIZE)
    for base, payload in snapshot["mem"].items():
        address = int(base, 16)
        data = bytes.fromhex(payload)
        result[address:address + len(data)] = data
    return result


def word(raw, address):
    return raw[address] | raw[address + 1] << 8


def projection(raw):
    slot = word(raw, 0x00C2)
    actor = 0x34EB + slot * 0x100
    roster_base = 0x46F9 if slot < 5 else 0x4779
    actor_offsets = (
        0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E, 0x10, 0x12,
        0x16, 0x18, 0x1A, 0x1C, 0x1E, 0x20, 0x22, 0x24, 0x26,
        0x28, 0x30, 0x32, 0x38, 0x3A, 0x3C, 0x42, 0x44, 0x46,
        0x48, 0x4E, 0x50, 0x52, 0x5E, 0x60, 0x64, 0x66, 0x6E,
        0x72, 0x7E, 0xA8, 0xB0,
        0x2A, 0x2C,
    )
    result = [
        word(raw, 0x0936), word(raw, 0x093A), word(raw, 0x093E), slot,
        word(raw, 0x46EB), word(raw, 0x476B),
        word(raw, roster_base + (slot % 5) * 2), word(raw, 0x0046),
    ]
    result.extend(word(raw, actor + offset) for offset in actor_offsets)
    if len(result) != len(FIELDS):
        raise AssertionError("projection field count changed")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--vectors", type=Path, required=True)
    parser.add_argument("--cases", type=Path, required=True)
    parser.add_argument("--paths", type=Path, required=True)
    parser.add_argument("--repeat-vectors", type=Path, required=True)
    parser.add_argument("--repeat-cases", type=Path, required=True)
    parser.add_argument("--repeat-paths", type=Path, required=True)
    parser.add_argument("--ghidra", type=Path, required=True)
    parser.add_argument("--recomp", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    for primary, repeat in ((args.vectors, args.repeat_vectors),
                            (args.cases, args.repeat_cases),
                            (args.paths, args.repeat_paths)):
        if primary.read_bytes() != repeat.read_bytes():
            raise ValueError(f"repeat capture differs: {primary.name}")

    vectors = load_jsonl(args.vectors)
    cases = load_jsonl(args.cases)
    paths = load_jsonl(args.paths)
    if not (len(vectors) == len(cases) == len(paths) == len(CASE_NAMES)):
        raise ValueError("expected nineteen aligned mode-seven witnesses")

    retained = []
    restore_names = {
        "live_state_restore", "high_byte_live_state_restore",
        "timer_underflow_restore", "timer_8002_restores",
        "restore_boost_remaps_state_three",
    }
    cpu_names = {
        "cpu_origin_damps", "cpu_origin_damps_to_zero",
        "cpu_off_origin_steers",
    }
    for number, (vector, case, path, name) in enumerate(
            zip(vectors, cases, paths, CASE_NAMES), 1):
        if case["case"] != number or path["case"] != number or \
                case["name"] != name or path["name"] != name:
            raise ValueError(f"case {number} is not aligned")
        if vector["entry_frame"] != vector["exit_frame"]:
            raise ValueError(f"case {number} crossed an emulator frame")
        if vector["entry_pc"] != "86994c":
            raise ValueError(f"case {number} has an unexpected entry")
        expected_exit = "869961" if name in restore_names else "8699c3"
        if vector["exit_pc"] != expected_exit or path["exit_pc"] != expected_exit:
            raise ValueError(f"case {number} has an unexpected exit")
        executed = path["executed"]
        if not executed or executed[0] != "86994c" or executed[-1] != expected_exit:
            raise ValueError(f"case {number} has an incomplete PC path")
        expected_b3bd = 0 if name in {
            "integer_height_hold", "negative_integer_height_hold"
        } else 1
        expected_restore = 1 if name in restore_names else 0
        expected_move = 1 if name in cpu_names else 0
        if path["b3bd_calls"] != expected_b3bd or \
                path["restore_calls"] != expected_restore or \
                path["b3aa_calls"] != expected_move or \
                path["a82c_calls"] != expected_move:
            raise ValueError(f"case {number} has unexpected child calls")

        before = memory(vector["entry"])
        after = memory(vector["exit"])
        inputs = projection(before)
        outputs = projection(after)
        slot = inputs[FIELDS.index("slot")]
        actor = 0x34EB + slot * 0x100
        if word(before, actor + 0x5E) != 7:
            raise ValueError(f"case {number} did not enter mode seven")
        if word(before, 0x00C6) != 2:
            raise ValueError(f"case {number} did not use native delta two")
        if word(before, actor + 0x16) not in (0, 0xFFFF):
            raise ValueError(f"case {number} exceeds the supported controller domain")
        if name in {"integer_height_hold", "negative_integer_height_hold"} and \
                inputs != outputs:
            raise ValueError("integer-height early return changed represented state")
        if name == "fractional_height_stationary" and \
                outputs[FIELDS.index("timer_raw_60")] != 0:
            raise ValueError("fractional-Z actor did not run the grounded path")
        if name == "timer_8001_wraps_positive" and \
                outputs[FIELDS.index("timer_raw_60")] != 0x7FFF:
            raise ValueError("$8001 timer boundary did not wrap positive")
        if name == "moving_human_repeat" and not (
                outputs[FIELDS.index("movement_direction_raw_4e")] == 5 and
                outputs[FIELDS.index("requested_direction_raw_50")] == 5 and
                outputs[FIELDS.index("direction_latch_raw_66")] == 1):
            raise ValueError("nonzero direction latch did not flip both directions")
        if name == "cpu_origin_damps" and not (
                outputs[FIELDS.index("velocity_x_raw_0e")] == 206 and
                outputs[FIELDS.index("velocity_y_raw_10")] == 0xFF32):
            raise ValueError("CPU origin damping result changed")
        if name == "cpu_origin_damps_to_zero" and not (
                outputs[FIELDS.index("velocity_x_raw_0e")] == 0 and
                outputs[FIELDS.index("velocity_y_raw_10")] == 0 and
                outputs[FIELDS.index("boost_raw_72")] == 0 and
                outputs[FIELDS.index("upper_state_raw_30")] == 0x10):
            raise ValueError("CPU damping did not select the stationary branch")
        if name == "cpu_off_origin_steers" and \
                outputs[FIELDS.index("velocity_x_raw_0e")] != 0xFF9F:
            raise ValueError("CPU off-origin steering result changed")

        retained.append({
            "name": name,
            "entry_pc": vector["entry_pc"],
            "exit_pc": vector["exit_pc"],
            "executed": executed,
            "child_calls": {
                "animation_87b3bd": path["b3bd_calls"],
                "restore_869846": path["restore_calls"],
                "steering_85b3aa": path["b3aa_calls"],
                "velocity_85a82c": path["a82c_calls"],
            },
            "input": inputs,
            "expected": outputs,
        })

    document = {
        "schema": 1,
        "routine": "$86:994C-$99C3 mode-seven dead-ball hold",
        "rom_sha256": ROM_SHA256,
        "provenance": (
            "two byte-identical controlled genuine-entry Mesen captures; "
            "only documented WRAM inputs changed before entry snapshot; "
            "no PC, stack, ROM, RNG, or child result patching"
        ),
        "fields": list(FIELDS),
        "source": {
            "vectors_sha256": sha256(args.vectors),
            "cases_sha256": sha256(args.cases),
            "paths_sha256": sha256(args.paths),
            "ghidra_sha256": sha256(args.ghidra),
            "recomp_sha256": sha256(args.recomp),
        },
        "calls": retained,
    }
    args.output.write_text(
        json.dumps(document, separators=(",", ":")) + "\n", encoding="utf-8")
    print("[CPU MODE SEVEN NORMALIZE] calls=19 entry=86994c exits=869961/8699c3")


if __name__ == "__main__":
    main()
