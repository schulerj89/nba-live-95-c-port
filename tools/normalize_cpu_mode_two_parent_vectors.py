"""Normalize repeated genuine `$86:F6CD-$F793` mode-two parent calls."""

import argparse
import hashlib
import json
from pathlib import Path

ROM_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
EXPECTED_STARTS_SHA256 = "3dcf5ee5a8c6d883acbaf57dd981962a08c51889b7ee52e75095aafb888a23b2"
SIZE = 0x10000
CASE_NAMES = (
    "prefix_both_negative_repair_hold",
    "prefix_receiver_named_skips_repair",
    "prefix_owner_named_skips_repair",
    "timer_21_positive_hold",
    "timer_8000_wraps_positive_hold",
    "timer_20_zero_opposite_half",
    "timer_1f_negative_same_half",
    "timer_8020_negative_due",
    "timer_ffff_negative_due",
    "recovery_inhibit_skips_role",
    "role_accepts_loose_pursuit",
    "role_rejects_then_targets",
    "context0_role0_e7dc",
    "context0_role8000_e96f",
    "context0_role8002_e96f",
    "context0_role8003_e7dc",
    "context1_same_sign_e7b3",
    "context1_opposite_sign_e7dc",
    "context1_opposite_role3_e96f",
    "context2_same_sign_e7b3",
    "context3_e6b7",
    "context4_same_sign_e7b3",
    "context4_opposite_e7dc",
    "cpu_context0_e7dc",
    "cpu_context0_moving_pair_e7dc",
    "cpu_pose_after_acceleration_integer_z",
    "cpu_pose_animation_after_acceleration_integer_z",
    "cpu_context1_same_e7b3",
    "cpu_context0_role8000_e96f",
    "cpu_context3_e6b7",
    "cached_pair_words_are_inputs",
    "noncanonical_context_anchor",
    "fractional_xy_integer_words",
    "fractional_z_integer_grounded",
    "human_controller_skips_jump",
    "controller4_natural_human_skips_jump",
    "controller7fff_positive_skips_jump",
    "controller8000_negative_calls_jump",
    "cpu_controller_calls_jump",
    "behavior_timer_preserved_on_hold",
    "context0_flags1_table",
    "context0_flags2_table",
    "context0_flags3_table",
)

GLOBAL_FIELDS = (
    ("scratch_raw_0046", 0x0046),
    ("scratch_raw_0047", 0x0047),
    ("formation_override_raw_005c", 0x005C),
    ("rng_raw_07f6", 0x07F6),
    ("ball_record_raw_0910", 0x0910),
    ("period_raw_0926", 0x0926),
    ("clock_raw_0928", 0x0928),
    ("live_raw_0936", 0x0936),
    ("offense_group_raw_093a", 0x093A),
    ("owner_raw_093e", 0x093E),
    ("receiver_raw_0946", 0x0946),
    ("activity_raw_0948", 0x0948),
    ("bounce_age_raw_094a", 0x094A),
    ("shot_value_raw_094c", 0x094C),
    ("inbound_group_raw_0952", 0x0952),
    ("inbound_actor_raw_0954", 0x0954),
    ("inbound_target_x_raw_0958", 0x0958),
    ("inbound_target_y_raw_095a", 0x095A),
    ("jump_gate_raw_0962", 0x0962),
    ("dead_ball_raw_0968", 0x0968),
    ("free_throw_raw_0978", 0x0978),
    ("jump_aux_raw_097c", 0x097C),
    ("play_code_raw_0996", 0x0996),
    ("play_step_raw_0998", 0x0998),
    ("play_mirror_raw_099c", 0x099C),
    ("special_actor_raw_09a2", 0x09A2),
    ("play_cycle_raw_09a4", 0x09A4),
    ("role_result_raw_09d8", 0x09D8),
    ("difficulty_raw_17af", 0x17AF),
    ("pose_selected_count_raw_1868", 0x1868),
    ("collision_actor_raw_492f", 0x492F),
    ("display_shot_value_raw_4939", 0x4939),
)
ACTOR_OFFSETS = (
    0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E, 0x10, 0x12,
    0x16, 0x18, 0x1A, 0x1C, 0x1E, 0x20, 0x22, 0x24, 0x26, 0x28,
    0x2A, 0x2C, 0x30, 0x32, 0x34, 0x36, 0x38, 0x3A, 0x3C, 0x3E,
    0x40, 0x42, 0x44, 0x46, 0x48, 0x4A, 0x4C, 0x4E, 0x50, 0x52,
    0x56, 0x58, 0x5A, 0x5C, 0x5E, 0x60, 0x62, 0x64, 0x66, 0x6C,
    0x6E, 0x72, 0x74, 0x76, 0x7A, 0x7E, 0x86, 0x88, 0x8A, 0x8C,
    0x8E, 0x92, 0xA2, 0xA8, 0xB0,
)
BALL_OFFSETS = (0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E, 0x10, 0x12)
CONTEXT_OFFSETS = (0x00, 0x08, 0x0A, 0x26, 0x30, 0x32, 0x39, 0x50)
LINEUP_ADDRESSES = tuple(0x46F9 + i * 2 for i in range(5)) + tuple(
    0x4779 + i * 2 for i in range(5))
FIELDS = (
    ("slot", "paired_slot")
    + tuple(name for name, _ in GLOBAL_FIELDS)
    + tuple(f"actor_raw_{offset:02x}" for offset in ACTOR_OFFSETS)
    + tuple(f"paired_raw_{offset:02x}" for offset in ACTOR_OFFSETS)
    + tuple(f"ball_raw_{offset:02x}" for offset in BALL_OFFSETS)
    + tuple(f"context{side}_raw_{offset:02x}" for side in range(2)
            for offset in CONTEXT_OFFSETS)
    + tuple(f"lineup_raw_{address:04x}" for address in LINEUP_ADDRESSES)
)
EXPECTED_MUTATED_FIELDS = {
    "scratch_raw_0046", "scratch_raw_0047",
    "actor_raw_0e", "actor_raw_10", "actor_raw_38", "actor_raw_4e",
    "actor_raw_50", "actor_raw_56", "actor_raw_58", "actor_raw_60",
    "actor_raw_72", "actor_raw_30", "actor_raw_32", "actor_raw_3c",
}


def load_jsonl(path: Path) -> list[dict]:
    return [json.loads(line) for line in path.read_text().splitlines() if line.strip()]


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def memory(snapshot: dict) -> tuple[bytearray, bytearray]:
    raw, covered = bytearray(SIZE), bytearray(SIZE)
    for base, payload in snapshot["mem"].items():
        address = int(base, 16)
        data = bytes.fromhex(payload)
        raw[address:address + len(data)] = data
        covered[address:address + len(data)] = b"\1" * len(data)
    return raw, covered


def word(pair: tuple[bytearray, bytearray], address: int) -> int:
    raw, covered = pair
    if not (covered[address] and covered[address + 1]):
        raise ValueError(f"projection address {address:04x} was not captured")
    return raw[address] | raw[address + 1] << 8


def projection(pair: tuple[bytearray, bytearray]) -> list[int]:
    slot = word(pair, 0xC2)
    actor = 0x34EB + slot * 0x100
    paired_slot = word(pair, actor + 0x74) >> 1
    paired = 0x34EB + paired_slot * 0x100
    values = [slot, paired_slot]
    values.extend(word(pair, address) for _, address in GLOBAL_FIELDS)
    values.extend(word(pair, actor + offset) for offset in ACTOR_OFFSETS)
    values.extend(word(pair, paired + offset) for offset in ACTOR_OFFSETS)
    values.extend(word(pair, 0x3EEB + offset) for offset in BALL_OFFSETS)
    for context in (0x46EB, 0x476B):
        values.extend(word(pair, context + offset) for offset in CONTEXT_OFFSETS)
    values.extend(word(pair, address) for address in LINEUP_ADDRESSES)
    assert len(values) == len(FIELDS)
    return values


def listed_starts(path: Path) -> set[str]:
    result = set()
    for line in path.read_text().splitlines():
        address = line.split("\t", 1)[0].lower()
        if "86f6cd" <= address <= "86f793":
            result.add(address)
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("vectors", "cases", "paths", "repeat-vectors",
                 "repeat-cases", "repeat-paths"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--capture-provenance", type=Path, required=True)
    parser.add_argument("--repeat-capture-provenance", type=Path, required=True)
    parser.add_argument("--ghidra-functions", type=Path, required=True)
    parser.add_argument("--ghidra-instructions", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if sha256(args.rom) != ROM_SHA256:
        raise ValueError("unexpected ROM identity")
    if (args.cases.read_bytes() != args.repeat_cases.read_bytes() or
            args.paths.read_bytes() != args.repeat_paths.read_bytes()):
        raise ValueError("repeat case/path capture differs")
    if args.capture_provenance.read_bytes() != args.repeat_capture_provenance.read_bytes():
        raise ValueError("repeat capture provenance differs")
    capture_provenance = json.loads(args.capture_provenance.read_text())
    expected_capture_keys = {
        "rom_sha256", "capture_lua_sha256", "capture_runner_sha256",
        "vector_recorder_sha256", "entry", "routine_entry", "exit",
        "restoration",
    }
    if (set(capture_provenance) != expected_capture_keys or
            capture_provenance["rom_sha256"] != ROM_SHA256 or
            capture_provenance["entry"] != "879C21" or
            capture_provenance["routine_entry"] != "86F6CD" or
            capture_provenance["exit"] != "86F793" or
            capture_provenance["restoration"] !=
            "native exit snapshot precedes post-vector WRAM restoration; RNG is excluded from all writes/restoration"):
        raise ValueError("capture provenance identity changed")
    for key, filename in (
            ("capture_lua_sha256", "mesen_cpu_mode_two_parent.lua"),
            ("capture_runner_sha256", "capture_cpu_mode_two_parent.ps1"),
            ("vector_recorder_sha256", "mesen_func_vectors.lua")):
        if (sha256(args.capture_provenance.parent / filename) !=
                capture_provenance[key] or
                sha256(args.repeat_capture_provenance.parent / filename) !=
                capture_provenance[key]):
            raise ValueError(f"capture source snapshot changed: {filename}")
    vectors, repeat, cases, paths = map(load_jsonl, (
        args.vectors, args.repeat_vectors, args.cases, args.paths))
    if not all(len(rows) == len(CASE_NAMES)
               for rows in (vectors, repeat, cases, paths)):
        raise ValueError("expected 43 aligned witnesses")

    calls, union, exits, aggregate, mutated_fields = [], set(), set(), {}, set()
    for number, (vector, repeated, case, path, name) in enumerate(
            zip(vectors, repeat, cases, paths, CASE_NAMES), 1):
        identity = (number, number, number, name, name)
        if (vector["call"], case["case"], path["case"],
                case["name"], path["name"]) != identity:
            raise ValueError(f"case {number} alignment changed")
        if (vector["entry_pc"] != "86f6cd" or
                vector["exit_pc"] != "86f793" or
                vector["entry_frame"] != vector["exit_frame"] or
                repeated["call"] != number or
                repeated["entry_pc"] != "86f6cd" or
                repeated["exit_pc"] != "86f793" or
                repeated["entry_frame"] != repeated["exit_frame"]):
            raise ValueError(f"case {number} invalid native boundary")
        before, after, repeated_before, repeated_after = map(memory, (
            vector["entry"], vector["exit"], repeated["entry"], repeated["exit"]))
        before_values, after_values = projection(before), projection(after)
        if (before_values != projection(repeated_before) or
                after_values != projection(repeated_after)):
            raise ValueError(f"case {number} relevant projection differs")
        slot, paired_slot = before_values[:2]
        actor, paired = 0x34EB + slot * 0x100, 0x34EB + paired_slot * 0x100
        actor_id = word(before, actor)
        paired_id = word(before, paired)
        if (case["slot"] != slot or case["paired_slot"] != paired_slot or
                case["actor"] != actor or case["paired"] != paired or
                word(before, 0x96) != actor or word(before, 0xC6) != 2 or
                word(before, 0xC8) != 0x20 or word(before, actor + 0x5E) != 2):
            raise ValueError(f"case {number} is not a genuine mode-two entry")
        if (actor_id != slot or paired_id != paired_slot or
                word(after, actor) != actor_id or word(after, paired) != paired_id or
                word(repeated_before, actor) != actor_id or
                word(repeated_before, paired) != paired_id or
                word(repeated_after, actor) != actor_id or
                word(repeated_after, paired) != paired_id):
            raise ValueError(f"case {number} actor identity changed")
        if slot >= 10 or paired_slot >= 10 or slot // 5 == paired_slot // 5:
            raise ValueError(f"case {number} invalid opposing assignment")
        if word(before, 0x0926) != 0 or word(before, 0x17AF) != 0 or any(
                word(before, context + 0x08) != 0 for context in (0x46EB, 0x476B)):
            raise ValueError(f"case {number} fixed capture domain changed")
        executed = path["executed"]
        if (not executed or executed[0] != "86f6cd" or
                executed[-1] != "86f793"):
            raise ValueError(f"case {number} incomplete PC path")
        union.update(executed)
        exits.add(path["exit_pc"])
        for child, count in path["child_calls"].items():
            aggregate[child] = aggregate.get(child, 0) + count
        mutated_fields.update(field for field, entry, exit_value in
                              zip(FIELDS, before_values, after_values)
                              if entry != exit_value)
        calls.append({"call": number, "name": name,
                      "entry_pc": vector["entry_pc"],
                      "exit_pc": vector["exit_pc"],
                      "native_actor_id": actor_id,
                      "native_paired_id": paired_id,
                      "executed": executed,
                      "child_calls": path["child_calls"],
                      "input": before_values, "expected": after_values})

    expected_aggregate = {
        "repair_86e3cb": 1, "loose_86f0fd": 2,
        "target_mode3_86e6b7": 2, "target_weak_86e7b3": 5,
        "target_normal_86e7dc": 20, "target_role_86e96f": 4,
        "jump_86ec32": 10, "pose_86e3e1": 30,
        "steer_85b3aa": 1, "accelerate_85a82c": 10,
        "finalize_86e5ab": 0,
        "approach_static_85b3c9": 8, "approach_moving_85b402": 1,
        "pose_animation_87b37c": 1,
    }
    digest = hashlib.sha256("\n".join(sorted(union)).encode()).hexdigest()
    if (union != listed_starts(args.ghidra_instructions) or len(union) != 81 or
            digest != EXPECTED_STARTS_SHA256):
        raise ValueError(f"parent owned-start census changed: count={len(union)} "
                         f"digest={digest} missing={sorted(listed_starts(args.ghidra_instructions) - union)} "
                         f"extra={sorted(union - listed_starts(args.ghidra_instructions))}")
    if exits != {"86f793"}:
        raise ValueError(f"parent exit contract changed: {exits}")
    if aggregate != expected_aggregate:
        raise ValueError(f"parent child contract changed: {aggregate}")
    if mutated_fields != EXPECTED_MUTATED_FIELDS:
        raise ValueError(f"native mutated-field contract changed: {sorted(mutated_fields)}")

    provenance = (
        "two controlled genuine `$87:9C21` dispatcher-entry Mesen captures "
        "with identical represented projections; documented WRAM inputs only; "
        "CPU state, stack, CPU flags, ROM and RNG were never written; no child "
        "result was written during the observed parent; the native exit was "
        "recorded before restoring original WRAM controls and state")
    domain = (
        "represented parent entry/exit state plus direct gameplay child outputs; "
        "both native team contexts and assigned opponents are represented across "
        "slots 1-4 and 6-9; natural controller records 0, 4 and $FFFF are covered, "
        "while $7FFF/$8000 are explicit signed-word boundary witnesses rather than "
        "retail controller IDs; these captures hold offense-group $093A at $FFFF, "
        "and the replay adapter supports canonical 0, 5 and $FFFF while mapping the "
        "host byte sentinel back to native $FFFF; paired "
        "+$92 covers 0, 3, $8000, $8002 and $8003; "
        "period $0926, difficulty $17AF and both context +$08 fractions are fixed "
        "zero; actor/paired +$00 identities are provenance metadata rather than "
        "replayed outputs; `$092E` has no storage in NbaTipoff and is excluded "
        "because none of these live-state-two paths writes it; lineup words and pack-backed "
        "decision/shot profiles establish roster inputs without embedding ROM data; "
        "DP address/arithmetic scratch and unmodeled temporary globals are excluded")
    document = {
        "schema": 1,
        "routine": "$86:F6CD-$F793 mode-two parent",
        "rom_sha256": ROM_SHA256,
        "provenance": provenance,
        "domain": domain,
        "fields": list(FIELDS),
        "owned_starts_sha256": digest,
        "aggregate_child_calls": aggregate,
        "source": {
            "vectors_sha256": sha256(args.vectors),
            "repeat_vectors_sha256": sha256(args.repeat_vectors),
            "cases_sha256": sha256(args.cases),
            "paths_sha256": sha256(args.paths),
            "capture_provenance_sha256": sha256(args.capture_provenance),
            "capture_lua_sha256": capture_provenance["capture_lua_sha256"],
            "capture_runner_sha256": capture_provenance["capture_runner_sha256"],
            "vector_recorder_sha256": capture_provenance["vector_recorder_sha256"],
            "ghidra_functions_sha256": sha256(args.ghidra_functions),
            "ghidra_instructions_sha256": sha256(args.ghidra_instructions),
        },
        "calls": calls,
    }
    args.output.write_text(json.dumps(document, separators=(",", ":")) + "\n",
                           encoding="utf-8")
    print(f"[CPU MODE TWO PARENT NORMALIZE] calls={len(calls)} "
          f"fields={len(FIELDS)} starts={len(union)} exit=86f793")


if __name__ == "__main__":
    main()
