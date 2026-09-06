"""Normalize genuine `$86:F0B7-$F0FC` calls into compact state witnesses."""

import argparse
import hashlib
import json
from pathlib import Path


ROM_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
SIZE = 0x20000
CASE_NAMES = (
    "inhibit_restores", "inhibit_high_restores_mode_six_alt_lower",
    "timer_zero_restores",
    "timer_one_restores", "timer_8002_restores", "timer_8001_steers",
    "target_x_steers", "target_y_steers", "target_equal_damps",
    "final_zero_accelerates", "final_nine_accelerates",
    "final_direction_damps", "final_boosted", "final_live_blocked",
    "final_airborne", "restore_locked_animation",
)
FIELDS = (
    "live_state_raw_0936", "possession_actor_raw_093e", "slot",
    "right_team_raw_46eb", "left_team_raw_476b", "roster_slot",
    "scratch_raw_0046", "scratch_raw_0047",
    "x_subpixel_raw_02", "x_integer_raw_04", "y_subpixel_raw_06",
    "y_integer_raw_08", "z_subpixel_raw_0a", "z_integer_raw_0c",
    "velocity_x_raw_0e", "velocity_y_raw_10", "velocity_z_raw_12",
    "upper_queue_cursor_raw_18", "lower_queue_cursor_raw_1a",
    "upper_queue_0_raw_1c", "upper_queue_1_raw_1e",
    "upper_queue_2_raw_20", "lower_queue_0_raw_22",
    "lower_queue_1_raw_24", "lower_queue_2_raw_26", "status_raw_28",
    "upper_state_raw_30", "lower_state_raw_32", "base_state_raw_38",
    "upper_phase_raw_3a", "lower_phase_raw_3c",
    "upper_accumulator_raw_42", "lower_accumulator_raw_44",
    "upper_lock_raw_46", "lower_lock_raw_48", "movement_direction_raw_4e",
    "requested_direction_raw_50", "display_direction_raw_52",
    "target_x_raw_56", "target_y_raw_58", "control_mode_raw_5e",
    "timer_raw_60", "saved_mode_raw_62", "behavior_timer_raw_64",
    "boost_raw_72", "behavior_flags_raw_7e", "recovery_inhibit_raw_7a",
    "velocity_direction_raw_a2", "free_throw_half_raw_a8",
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
    offsets = (
        0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E, 0x10, 0x12,
        0x18, 0x1A, 0x1C, 0x1E, 0x20, 0x22, 0x24, 0x26, 0x28,
        0x30, 0x32, 0x38, 0x3A, 0x3C, 0x42, 0x44, 0x46, 0x48,
        0x4E, 0x50, 0x52, 0x56, 0x58, 0x5E, 0x60, 0x62, 0x64,
        0x72, 0x7E, 0x7A, 0xA2, 0xA8, 0xB0, 0x2A, 0x2C,
    )
    result = [
        word(raw, 0x0936), word(raw, 0x093E), slot,
        word(raw, 0x46EB), word(raw, 0x476B),
        word(raw, roster_base + (slot % 5) * 2),
        word(raw, 0x0046), word(raw, 0x0047),
    ]
    result.extend(word(raw, actor + offset) for offset in offsets)
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
        raise ValueError("expected sixteen aligned mode-nine witnesses")

    restore_names = {
        "inhibit_restores", "inhibit_high_restores_mode_six_alt_lower",
        "timer_zero_restores",
        "timer_one_restores", "timer_8002_restores",
        "restore_locked_animation",
    }
    target_names = {
        "timer_8001_steers", "target_x_steers", "target_y_steers",
        "target_equal_damps",
    }
    retained = []
    for number, (vector, case, path, name) in enumerate(
            zip(vectors, cases, paths, CASE_NAMES), 1):
        if case["case"] != number or path["case"] != number or \
                case["name"] != name or path["name"] != name:
            raise ValueError(f"case {number} is not aligned")
        expected_exit = "86f0f2" if name in restore_names else \
            "86f0dc" if name in target_names else "86f0fc"
        if vector["entry_frame"] != vector["exit_frame"] or \
                vector["entry_pc"] != "86f0b7" or \
                vector["exit_pc"] != expected_exit or \
                path["exit_pc"] != expected_exit:
            raise ValueError(f"case {number} has an invalid native boundary")
        executed = path["executed"]
        if not executed or executed[0] != "86f0b7" or executed[-1] != expected_exit:
            raise ValueError(f"case {number} has an incomplete PC path")
        expected_children = {
            "animation_87b3bd": 1 if name in restore_names else 0,
            "steering_85b3aa": 1 if name in target_names else 0,
            "velocity_85a82c": 0 if name in restore_names else 1,
        }
        actual_children = {
            "animation_87b3bd": path["b3bd_calls"],
            "steering_85b3aa": path["b3aa_calls"],
            "velocity_85a82c": path["a82c_calls"],
        }
        if actual_children != expected_children:
            raise ValueError(f"case {number} has unexpected child calls")

        before = memory(vector["entry"])
        after = memory(vector["exit"])
        inputs = projection(before)
        outputs = projection(after)
        slot = inputs[FIELDS.index("slot")]
        actor = 0x34EB + slot * 0x100
        if word(before, 0x0096) != actor or word(before, 0x00C6) != 2 or \
                word(before, actor + 0x5E) != 9:
            raise ValueError(f"case {number} lacks a genuine mode-nine entry")
        expected_mode = 6 if name == \
            "inhibit_high_restores_mode_six_alt_lower" else 4
        if name in restore_names and not (
                outputs[FIELDS.index("control_mode_raw_5e")] == expected_mode and
                outputs[FIELDS.index("timer_raw_60")] == 0 and
                outputs[FIELDS.index("boost_raw_72")] == 0):
            raise ValueError(f"case {number} did not restore saved mode four")
        if name == "timer_8001_steers" and \
                outputs[FIELDS.index("timer_raw_60")] != 0x7FFF:
            raise ValueError("$8001 timer boundary did not wrap positive")
        if name in {"final_live_blocked", "final_airborne"} and not (
                outputs[FIELDS.index("velocity_x_raw_0e")] == inputs[FIELDS.index("velocity_x_raw_0e")] and
                outputs[FIELDS.index("velocity_y_raw_10")] == inputs[FIELDS.index("velocity_y_raw_10")] and
                outputs[FIELDS.index("boost_raw_72")] == 14):
            raise ValueError(f"case {number} did not preserve blocked velocity and count boost")

        retained.append({
            "name": name, "entry_pc": vector["entry_pc"],
            "exit_pc": vector["exit_pc"], "executed": executed,
            "child_calls": expected_children, "input": inputs,
            "expected": outputs,
        })

    document = {
        "schema": 1,
        "routine": "$86:F0B7-$F0FC mode-nine timed target override",
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
    print("[CPU MODE NINE NORMALIZE] calls=16 entry=86f0b7 exits=86f0dc/86f0f2/86f0fc")


if __name__ == "__main__":
    main()
