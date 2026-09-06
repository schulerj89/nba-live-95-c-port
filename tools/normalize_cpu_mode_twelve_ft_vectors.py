"""Normalize native `$87:9F11-$9F75` free-throw caller witnesses."""

import argparse
import hashlib
import json
from pathlib import Path

from normalize_cpu_mode_twelve_vectors import (
    ACTOR_NAMES, ACTOR_OFFSETS, BALL_NAMES, BALL_OFFSETS, ROM_SHA256,
    load_jsonl, memory, sha256, word,
)


GLOBAL_FIELDS = (
    ("shot_origin_x_raw_0900", 0x0900),
    ("shot_origin_y_raw_0902", 0x0902),
    ("controller_pointer_raw_090c", 0x090C),
    ("ball_record_raw_0910", 0x0910),
    ("roster_low_raw_0914", 0x0914),
    ("roster_bank_raw_0916", 0x0916),
    ("bounce_timer_raw_091c", 0x091C),
    ("bounce_raw_0920", 0x0920),
    ("previous_actor_x_raw_0922", 0x0922),
    ("timeout_raw_0930", 0x0930),
    ("live_raw_0936", 0x0936),
    ("offense_group_raw_093a", 0x093A),
    ("owner_raw_093e", 0x093E),
    ("activity_raw_0948", 0x0948),
    ("attempt_raw_094a", 0x094A),
    ("value_raw_094c", 0x094C),
    ("dead_raw_0966", 0x0966),
    ("height_raw_0968", 0x0968),
    ("initial_raw_096a", 0x096A),
    ("dead_raw_096c", 0x096C),
    ("resolution_raw_0972", 0x0972),
    ("free_throw_raw_0978", 0x0978),
    ("attempts_raw_097a", 0x097A),
    ("rim_raw_097c", 0x097C),
    ("aim_power_raw_0980", 0x0980),
    ("aim_accuracy_raw_0982", 0x0982),
    ("assistance_team_raw_09c0", 0x09C0),
    ("last_owner_raw_09c8", 0x09C8),
    ("attachment_raw_09f6", 0x09F6),
    ("inner_veto_raw_09f8", 0x09F8),
    ("assistance_raw_17bf", 0x17BF),
    ("shot_control_raw_17c3", 0x17C3),
    ("upload_low_raw_180b", 0x180B),
    ("upload_bank_raw_180c", 0x180C),
    ("display_value_raw_4939", 0x4939),
    ("display_shooter_raw_493b", 0x493B),
)
CONTEXT_NAMES = (
    "context0_team_raw_00", "context0_anchor_raw_0a",
    "context0_assist_raw_43", "context0_controller_raw_45",
    "context0_clock_raw_47",
)
OUTPUT_FIELDS = ("rng_raw_07f6", "slot") + tuple(name for name, _ in GLOBAL_FIELDS) + \
                ACTOR_NAMES + BALL_NAMES + CONTEXT_NAMES + ("clock_raw_0928",)
INPUT_FIELDS = OUTPUT_FIELDS
CASE_NAMES = (
    "prep_same_pass_hold", "state9_hold_attempts_two",
    "state9_release_attempts_two", "state9_release_final_attempt",
)


def projection(snapshot, include_rng):
    raw, covered = memory(snapshot)
    actor = word(raw, 0x0096, covered)
    if actor < 0x34EB or (actor - 0x34EB) % 0x100:
        raise ValueError("free-throw caller actor pointer is not an actor record")
    slot = (actor - 0x34EB) // 0x100
    result = ([word(raw, 0x07F6, covered)] if include_rng else []) + [slot]
    result.extend(word(raw, address, covered) for _, address in GLOBAL_FIELDS)
    result.extend(word(raw, actor + offset, covered) for offset in ACTOR_OFFSETS)
    result.extend(word(raw, 0x3EEB + offset, covered) for offset in BALL_OFFSETS)
    result.extend(word(raw, 0x46EB + offset, covered)
                  for offset in (0, 0x0A, 0x43, 0x45, 0x47))
    result.append(word(raw, 0x0928, covered))
    wanted = len(INPUT_FIELDS) if include_rng else len(OUTPUT_FIELDS)
    if len(result) != wanted:
        raise AssertionError("free-throw caller projection field count changed")
    return result


def checked_pair(primary_path, repeat_path):
    if primary_path.read_bytes() != repeat_path.read_bytes():
        raise ValueError(f"free-throw repeat differs: {primary_path.name}")
    return load_jsonl(primary_path)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("prep", "prep-repeat", "state9", "state9-repeat",
                 "final", "final-repeat"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    prep = checked_pair(args.prep, args.prep_repeat)
    state9 = checked_pair(args.state9, args.state9_repeat)
    final = checked_pair(args.final, args.final_repeat)
    if len(prep) != 1 or len(state9) != 2 or len(final) != 2:
        raise ValueError("free-throw caller capture counts changed")
    selected = (prep[0], state9[0], state9[1], final[1])
    expected_entry = ("879f11", "879f3e", "879f3e", "879f3e")
    calls = []
    for number, (vector, name, entry_pc) in enumerate(
            zip(selected, CASE_NAMES, expected_entry), 1):
        entry_cpu = vector["entry"]["cpu"]
        exit_cpu = vector["exit"]["cpu"]
        if vector.get("entry_pc") != entry_pc or \
                vector.get("exit_pc") != "87a017" or \
                entry_cpu.get("sp") != 8185 or exit_cpu.get("sp") != 8185:
            raise ValueError(f"case {number} has an invalid caller boundary")
        before = projection(vector["entry"], True)
        after = projection(vector["exit"], True)
        if before[1] != 0 or word(memory(vector["entry"])[0], 0x3435, memory(vector["entry"])[1]) != 0x416B:
            raise ValueError(f"case {number} has an unexpected shooter record")
        calls.append({
            "call": number,
            "name": name,
            "entry_pc": entry_pc,
            "exit_pc": "87a017",
            "entry_frame": vector["entry_frame"],
            "exit_frame": vector["exit_frame"],
            "cycle_count": exit_cpu["cycleCount"] - entry_cpu["cycleCount"],
            "stack_pointer": entry_cpu["sp"],
            "input": before,
            "expected": after,
        })
    document = {
        "schema": 1,
        "routine": "$87:9F11-$9F75 free-throw mode-twelve caller",
        "rom_sha256": ROM_SHA256,
        "provenance": ("repeated genuine-entry Mesen captures at $87:9F11 and "
            "$87:9F3E with matching entry/exit stack; the two-frame release "
            "includes native $85:95DB graphics/DMA work, whose volatile/NMI "
            "state is excluded from this caller-owned projection"),
        "scope": ("caller-owned gameplay words only; player record $416B was "
            "outside the captured ranges, so player statistics/stamina are "
            "excluded; parent witnesses cover the child mutation contract "
            "for captured player $44EB, while exact $416B values remain outside"),
        "input_fields": list(INPUT_FIELDS),
        "output_fields": list(OUTPUT_FIELDS),
        "source": {
            "prep_sha256": sha256(args.prep),
            "prep_repeat_sha256": sha256(args.prep_repeat),
            "state9_sha256": sha256(args.state9),
            "state9_repeat_sha256": sha256(args.state9_repeat),
            "final_sha256": sha256(args.final),
            "final_repeat_sha256": sha256(args.final_repeat),
        },
        "calls": calls,
    }
    document["calls_sha256"] = hashlib.sha256(json.dumps(
        calls, separators=(",", ":"), ensure_ascii=True).encode()).hexdigest()
    args.output.write_text(json.dumps(document, separators=(",", ":")) + "\n",
                           encoding="utf-8")
    print(f"[CPU MODE TWELVE FT NORMALIZE] calls=4 "
          f"inputs={len(INPUT_FIELDS)} outputs={len(OUTPUT_FIELDS)}")


if __name__ == "__main__":
    main()
