"""Normalize genuine `$86:C6AD-$C758` calls into compact state witnesses."""

import argparse
import hashlib
import json
from pathlib import Path


ROM_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
SIZE = 0x20000
CASE_NAMES = (
    "selector_negative", "vertical_nonzero", "airborne_integer",
    "fractional_z_lands", "bounce_signed_halves", "settle_negative_marker",
    "timer_two_holds", "timer_one_owner_restores",
    "timer_zero_group_match_restores",
    "timer_8002_group_mismatch_restores", "timer_8001_wraps_positive",
    "phase_nine_clears", "phase_ten_sets_10", "phase_nineteen_sets_10",
    "phase_twenty_sets_08", "phase_twentynine_sets_08",
    "phase_thirty_sets_10", "phase_thirtynine_sets_10",
    "phase_forty_clears",
)
FIELDS = (
    "scratch_raw_0046", "scratch_raw_0047", "offense_group_raw_093a",
    "owner_raw_093e", "rim_raw_13e7", "slot", "x_fraction_raw_02",
    "x_integer_raw_04", "y_fraction_raw_06", "y_integer_raw_08",
    "z_fraction_raw_0a", "z_integer_raw_0c", "velocity_x_raw_0e",
    "velocity_y_raw_10", "velocity_z_raw_12", "status_raw_28",
    "selector_raw_56", "contact_inhibit_raw_5a",
    "recovery_inhibit_raw_7a", "control_mode_raw_5e",
    "contact_timer_raw_60", "reaction_timer_projection_raw_60",
    "behavior_timer_raw_64", "landing_marker_raw_66", "team_group_raw_6e",
    "behavior_flags_raw_7e", "alternate_lower_raw_a8",
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
    timer = word(raw, actor + 0x60)
    return [
        word(raw, 0x0046), word(raw, 0x0047), word(raw, 0x093A),
        word(raw, 0x093E), word(raw, 0x13E7), slot,
        word(raw, actor + 0x02), word(raw, actor + 0x04),
        word(raw, actor + 0x06), word(raw, actor + 0x08),
        word(raw, actor + 0x0A), word(raw, actor + 0x0C),
        word(raw, actor + 0x0E), word(raw, actor + 0x10),
        word(raw, actor + 0x12), word(raw, actor + 0x28),
        word(raw, actor + 0x56), word(raw, actor + 0x5A),
        word(raw, actor + 0x7A), word(raw, actor + 0x5E), timer, timer,
        word(raw, actor + 0x64), word(raw, actor + 0x66),
        word(raw, actor + 0x6E), word(raw, actor + 0x7E),
        word(raw, actor + 0xA8),
    ]


def instruction_starts(path):
    starts = set()
    for line in Path(path).read_text(encoding="utf-8").splitlines():
        fields = line.split("\t", 2)
        if not fields or len(fields[0]) != 6:
            continue
        try:
            address = int(fields[0], 16)
        except ValueError:
            continue
        if 0x86C6AD <= address <= 0x86C758:
            starts.add(fields[0].lower())
    return starts


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
        raise ValueError("expected nineteen aligned mode-eight witnesses")

    restore_names = {
        "timer_one_owner_restores", "timer_zero_group_match_restores",
        "timer_8002_group_mismatch_restores",
    }
    retained = []
    reached_union = set()
    for number, (vector, case, path, name) in enumerate(
            zip(vectors, cases, paths, CASE_NAMES), 1):
        if case["case"] != number or path["case"] != number or \
                case["name"] != name or path["name"] != name:
            raise ValueError(f"case {number} is not aligned")
        if vector["entry_frame"] != vector["exit_frame"] or \
                vector["entry_pc"] != "86c6ad" or \
                vector["exit_pc"] != "86c758" or \
                path["exit_pc"] != "86c758":
            raise ValueError(f"case {number} has an invalid native boundary")
        executed = path["executed"]
        if not executed or executed[0] != "86c6ad" or \
                executed[-1] != "86c758":
            raise ValueError(f"case {number} has an incomplete PC path")
        reached_union.update(executed)
        expected_children = {
            "restore_869846": 1 if name in restore_names else 0,
        }
        if path["restore_calls"] != expected_children["restore_869846"]:
            raise ValueError(f"case {number} has unexpected child calls")

        before = memory(vector["entry"])
        after = memory(vector["exit"])
        inputs = projection(before)
        outputs = projection(after)
        slot = inputs[FIELDS.index("slot")]
        actor = 0x34EB + slot * 0x100
        if word(before, 0x0096) != actor or word(before, 0x00C6) != 2 or \
                word(before, actor + 0x5E) != 8:
            raise ValueError(f"case {number} lacks a genuine mode-eight entry")
        allowed = {0x13E7, 0x13E8}
        for offset in (0x0E, 0x10, 0x12, 0x28, 0x5A, 0x5E,
                       0x60, 0x64, 0x66, 0x7E):
            allowed.update((actor + offset, actor + offset + 1))
        changed = {address for address in range(SIZE)
                   if before[address] != after[address]}
        if not changed <= allowed:
            extra = ",".join(f"{address:04x}" for address in sorted(changed - allowed))
            raise ValueError(f"case {number} has unexpected writes: {extra}")

        if name == "timer_one_owner_restores" and \
                outputs[FIELDS.index("control_mode_raw_5e")] != 11:
            raise ValueError("owner expiry did not select mode eleven")
        if name == "timer_zero_group_match_restores" and \
                outputs[FIELDS.index("control_mode_raw_5e")] != 1:
            raise ValueError("actor-group match did not restore mode one")
        if name == "timer_8002_group_mismatch_restores" and \
                outputs[FIELDS.index("control_mode_raw_5e")] != 2:
            raise ValueError("actor-group mismatch did not restore mode two")
        if name in restore_names and not (
                outputs[FIELDS.index("contact_timer_raw_60")] == 0 and
                outputs[FIELDS.index("contact_inhibit_raw_5a")] == 0 and
                outputs[FIELDS.index("behavior_flags_raw_7e")] == 0 and
                outputs[FIELDS.index("status_raw_28")] == 0 and
                outputs[FIELDS.index("behavior_timer_raw_64")] == 0x2F):
            raise ValueError(f"case {number} did not complete native restore")

        retained.append({
            "name": name, "entry_pc": vector["entry_pc"],
            "exit_pc": vector["exit_pc"], "executed": executed,
            "child_calls": expected_children, "input": inputs,
            "expected": outputs,
        })

    expected_starts = instruction_starts(args.ghidra)
    if len(expected_starts) != 67 or reached_union != expected_starts:
        missing = sorted(expected_starts - reached_union)
        extra = sorted(reached_union - expected_starts)
        raise ValueError(f"instruction coverage mismatch missing={missing} extra={extra}")

    document = {
        "schema": 1,
        "routine": "$86:C6AD-$C758 mode-eight knockdown recovery",
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
    print("[CPU MODE EIGHT NORMALIZE] calls=19 starts=67 entry=86c6ad exit=86c758")


if __name__ == "__main__":
    main()
