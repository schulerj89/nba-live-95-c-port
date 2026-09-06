"""Normalize genuine `$86:A5B0-$A628` calls into compact state witnesses."""

import argparse
import hashlib
import json
from pathlib import Path


ROM_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
SIZE = 0x20000
CASE_NAMES = (
    "invalid_live_82_preserves", "invalid_live_81_clears",
    "aux_four_below_holds", "aux_five_equal_holds",
    "aux_six_actor_positive",
    "aux_thirteen_actor_positive_normalizes",
    "aux_thirteen_actor_negative_preserves",
    "aux_fourteen_actor_zero_normalizes",
    "aux_fifteen_actor_negative_preserves",
    "aux_eight_controller_positive_normalizes",
    "aux_eight_controller_negative_preserves",
    "cmp_8004_positive_controller_normalizes",
    "cmp_8005_negative_skips_table",
    "timer_one_expires_path_zero_offense",
    "timer_zero_expires_path_nonzero_defense",
    "timer_8002_expires", "timer_8001_wraps_positive",
    "valid_expiry_always_clears_live_82",
)
FIELDS = (
    "live_state_raw_0936", "offense_group_raw_093a",
    "possession_actor_raw_093e", "pass_actor_raw_0942",
    "pass_aux_raw_0944", "pass_receiver_raw_0946",
    "ball_activity_raw_0948", "attempt_latch_raw_094a",
    "inbound_transfer_raw_09b8", "pass_active_raw_09c4",
    "rule_raw_17d5", "slot", "scratch_raw_0046", "scratch_raw_0047",
    "controller_0_held_raw_08", "controller_1_held_raw_08",
    "controller_2_held_raw_08", "controller_3_held_raw_08",
    "controller_4_held_raw_08",
    "actor_0_y_subpixel_raw_06", "actor_0_y_integer_raw_08",
    "actor_1_y_subpixel_raw_06", "actor_1_y_integer_raw_08",
    "actor_2_y_subpixel_raw_06", "actor_2_y_integer_raw_08",
    "x_subpixel_raw_02", "x_integer_raw_04",
    "y_subpixel_raw_06", "y_integer_raw_08",
    "z_subpixel_raw_0a", "z_integer_raw_0c",
    "velocity_x_raw_0e", "velocity_y_raw_10", "velocity_z_raw_12",
    "upper_queue_cursor_raw_18", "lower_queue_cursor_raw_1a",
    "upper_queue_0_raw_1c", "upper_queue_1_raw_1e",
    "upper_queue_2_raw_20", "lower_queue_0_raw_22",
    "lower_queue_1_raw_24", "lower_queue_2_raw_26", "status_raw_28",
    "upper_resource_raw_2a", "lower_resource_raw_2c",
    "upper_state_raw_30", "lower_state_raw_32", "base_state_raw_38",
    "upper_phase_raw_3a", "lower_phase_raw_3c",
    "upper_accumulator_raw_42", "lower_accumulator_raw_44",
    "upper_lock_raw_46", "lower_lock_raw_48",
    "control_mode_raw_5e", "timer_raw_60", "behavior_timer_raw_64",
    "actor_group_raw_6e", "behavior_flags_raw_7e",
    "ball_x_subpixel_raw_02", "ball_x_integer_raw_04",
    "ball_y_subpixel_raw_06", "ball_y_integer_raw_08",
    "ball_z_subpixel_raw_0a", "ball_z_integer_raw_0c",
    "ball_velocity_x_raw_0e", "ball_velocity_y_raw_10",
    "ball_velocity_z_raw_12",
)
MUTABLE_FIELDS = {
    "live_state_raw_0936", "pass_actor_raw_0942", "pass_aux_raw_0944",
    "pass_receiver_raw_0946", "ball_activity_raw_0948",
    "attempt_latch_raw_094a", "inbound_transfer_raw_09b8",
    "status_raw_28", "control_mode_raw_5e", "timer_raw_60",
    "behavior_timer_raw_64", "behavior_flags_raw_7e",
}


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
    actor_offsets = (
        0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E, 0x10, 0x12,
        0x18, 0x1A, 0x1C, 0x1E, 0x20, 0x22, 0x24, 0x26, 0x28,
        0x2A, 0x2C, 0x30, 0x32, 0x38, 0x3A, 0x3C, 0x42, 0x44,
        0x46, 0x48, 0x5E, 0x60, 0x64, 0x6E, 0x7E,
    )
    ball = 0x3EEB
    result = [
        word(raw, address) for address in (
            0x0936, 0x093A, 0x093E, 0x0942, 0x0944, 0x0946,
            0x0948, 0x094A, 0x09B8, 0x09C4, 0x17D5,
        )
    ]
    result.extend((slot, word(raw, 0x0046), word(raw, 0x0047)))
    result.extend(word(raw, 0x47EB + pad * 0x40 + 0x08)
                  for pad in range(5))
    for lookup_actor in range(3):
        base = 0x34EB + lookup_actor * 0x100
        result.extend((word(raw, base + 0x06), word(raw, base + 0x08)))
    result.extend(word(raw, actor + offset) for offset in actor_offsets)
    result.extend(word(raw, ball + offset) for offset in
                  (0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E, 0x10, 0x12))
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
        raise ValueError("expected eighteen aligned mode-ten witnesses")

    restoring = {
        "invalid_live_82_preserves", "invalid_live_81_clears",
        "timer_one_expires_path_zero_offense",
        "timer_zero_expires_path_nonzero_defense", "timer_8002_expires",
        "valid_expiry_always_clears_live_82",
    }
    retained = []
    for number, (vector, case, path, name) in enumerate(
            zip(vectors, cases, paths, CASE_NAMES), 1):
        if case["case"] != number or path["case"] != number or \
                case["name"] != name or path["name"] != name:
            raise ValueError(f"case {number} is not aligned")
        if vector["entry_frame"] != vector["exit_frame"] or \
                vector["entry_pc"] != "86a5b0" or \
                vector["exit_pc"] != "86a628" or \
                path["exit_pc"] != "86a628":
            raise ValueError(f"case {number} has an invalid native boundary")
        executed = path["executed"]
        if not executed or executed[0] != "86a5b0" or \
                executed[-1] != "86a628":
            raise ValueError(f"case {number} has an incomplete PC path")
        expected_children = {"restore_869846": 1 if name in restoring else 0}
        if path["restore_calls"] != expected_children["restore_869846"]:
            raise ValueError(f"case {number} has unexpected child calls")

        before = memory(vector["entry"])
        after = memory(vector["exit"])
        inputs = projection(before)
        outputs = projection(after)
        unexpected = [
            field for field, initial, final in zip(FIELDS, inputs, outputs)
            if field not in MUTABLE_FIELDS and initial != final
        ]
        if unexpected:
            raise ValueError(
                f"case {number} changed fields outside the native write set: "
                f"{unexpected}")
        slot = inputs[FIELDS.index("slot")]
        actor = 0x34EB + slot * 0x100
        if word(before, 0x0096) != actor or word(before, 0x00C6) != 2 or \
                word(before, actor + 0x5E) != 10:
            raise ValueError(f"case {number} lacks a genuine mode-ten entry")
        if name in restoring:
            expected_mode = 1 if inputs[FIELDS.index("actor_group_raw_6e")] == \
                inputs[FIELDS.index("offense_group_raw_093a")] else 2
            if outputs[FIELDS.index("control_mode_raw_5e")] != expected_mode or \
                    outputs[FIELDS.index("timer_raw_60")] != 0 or \
                    outputs[FIELDS.index("behavior_timer_raw_64")] != 0x2F or \
                    outputs[FIELDS.index("behavior_flags_raw_7e")] != 0 or \
                    outputs[FIELDS.index("status_raw_28")] != 0:
                raise ValueError(f"case {number} did not restore actor state")
        if name == "invalid_live_82_preserves" and \
                outputs[FIELDS.index("live_state_raw_0936")] != 0x0082:
            raise ValueError("invalid receiver did not preserve exact live state 82")
        if name == "valid_expiry_always_clears_live_82" and \
                outputs[FIELDS.index("live_state_raw_0936")] != 0:
            raise ValueError("valid expiry did not clear exact live state 82")
        if name == "timer_8001_wraps_positive" and \
                outputs[FIELDS.index("timer_raw_60")] != 0x7FFF:
            raise ValueError("$8001 timer boundary did not wrap positive")

        retained.append({
            "name": name, "entry_pc": vector["entry_pc"],
            "exit_pc": vector["exit_pc"], "executed": executed,
            "child_calls": expected_children, "input": inputs,
            "expected": outputs,
        })

    document = {
        "schema": 1,
        "routine": "$86:A5B0-$A628 mode-ten receiver",
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
    print("[CPU MODE TEN NORMALIZE] calls=18 entry=86a5b0 exit=86a628")


if __name__ == "__main__":
    main()
