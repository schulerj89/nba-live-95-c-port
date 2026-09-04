"""Retain controlled native `$86:F721-$F78A` mode-two witnesses."""

import argparse
import hashlib
import json
from pathlib import Path

from verify_normal_actor_parent_vectors import memory, projection, word


ROM_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
CASE_NAMES = ("role_clear_ownerless_record", "role_set_named_owner",
              "role_clear_uses_base_assignment",
              "boosted_cpu_calls_jump_reach")


def load_jsonl(path):
    return [json.loads(line) for line in Path(path).read_text().splitlines()
            if line.strip()]


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--vectors", type=Path, required=True)
    parser.add_argument("--cases", type=Path, required=True)
    parser.add_argument("--paths", type=Path, required=True)
    parser.add_argument("--ghidra", type=Path, required=True)
    parser.add_argument("--recomp", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    vectors = load_jsonl(args.vectors)
    cases = load_jsonl(args.cases)
    paths = load_jsonl(args.paths)
    if not (len(vectors) == len(cases) == len(paths) == len(CASE_NAMES)):
        raise ValueError("expected exactly four aligned mode-two witnesses")

    retained = []
    for number, (vector, case, path, name) in enumerate(
            zip(vectors, cases, paths, CASE_NAMES), 1):
        if case["case"] != number or path["case"] != number:
            raise ValueError(f"case {number} is not aligned")
        if case["name"] != name:
            raise ValueError(f"case {number} should be {name}")
        if vector["entry_frame"] != vector["exit_frame"]:
            raise ValueError(f"case {number} crossed an emulator frame")
        if vector["entry_pc"] != "86f6cd" or vector["exit_pc"] != "86f793":
            raise ValueError(f"case {number} has an unexpected boundary")
        executed = path["executed"]
        if not executed or executed[0] != "86f6cd" or executed[-1] != "86f793":
            raise ValueError(f"case {number} has an incomplete PC path")
        if number in (1, 3, 4) and ("86f72e" not in executed or
                            any(pc in executed for pc in
                                ("86f726", "86f72a", "86f72c"))):
            raise ValueError("role-clear case did not use the defense branch")
        if number == 3 and not all(pc in executed for pc in
                                   ("86f72e", "86f730", "86f733",
                                    "86f737", "86f739")):
            raise ValueError("base-assignment case missed the selector path")
        if number == 4 and (not all(pc in executed for pc in
                                    ("86f780", "86f782", "86f785", "86f787")) or
                            path.get("ec32_calls") != 1):
            raise ValueError("boosted CPU case did not call jump/reach")
        if number != 4 and path.get("ec32_calls") != 0:
            raise ValueError(f"case {number} unexpectedly called jump/reach")
        if number == 2 and (not all(pc in executed for pc in
                                    ("86f721", "86f724", "86f726",
                                     "86f72a", "86f72c", "86f780")) or
                            "86f72e" in executed):
            raise ValueError("role-set case did not use the loose-ball gate")

        before = memory(vector["entry"])
        after = memory(vector["exit"])
        slot = word(before, 0x00C2)
        actor = 0x34EB + slot * 0x100
        if slot != case["slot"] or actor != case["actor"]:
            raise ValueError(f"case {number} actor identity changed")
        if word(before, actor + 0x5E) != 2 or word(after, actor + 0x5E) != 2:
            raise ValueError(f"case {number} did not preserve mode two")
        if word(before, actor + 0x60) != 0x20 or \
                word(before, actor + 0x7A) != 0:
            raise ValueError(f"case {number} did not enter an uninhibited decision")
        if word(before, 0x09D8) != case["role_ownerless"]:
            raise ValueError(f"case {number} role flag changed before entry")
        if word(before, 0x093E) != case["owner"]:
            raise ValueError(f"case {number} owner record changed before entry")
        if word(before, 0x0936) != 0x81 or \
                word(before, 0x0946) != case["receiver"] or \
                word(before, 0x0978) != 1 or \
                word(before, 0x492F) != slot:
            raise ValueError(f"case {number} did not isolate the loose-ball child")
        if word(before, actor + 0x74) != case["assignment_base"] or \
                word(before, actor + 0x76) != case["assignment_current"]:
            raise ValueError(f"case {number} assignment inputs changed")
        if (number == 3) != (case["assignment_base"] !=
                             case["assignment_current"]):
            raise ValueError(f"case {number} has the wrong assignment relation")
        paired_slot = case["assignment_base"] >> 1
        paired = 0x34EB + paired_slot * 0x100
        if paired_slot != case["paired_slot"] or paired != case["paired"]:
            raise ValueError(f"case {number} paired actor identity changed")
        current_slot = case["assignment_current"] >> 1
        current_paired = 0x34EB + current_slot * 0x100
        if current_slot != case["current_slot"] or \
                current_paired != case["current_paired"]:
            raise ValueError(f"case {number} current actor identity changed")
        if any(word(before, actor + offset) != 0 for offset in
               (0x02, 0x04, 0x06, 0x08)) or \
                word(before, paired + 0x02) != 0 or \
                word(before, paired + 0x04) != 100 or \
                word(before, paired + 0x06) != 0 or \
                word(before, paired + 0x08) != 0 or \
                word(before, paired + 0x0E) != 0 or \
                word(before, paired + 0x10) != 0:
            raise ValueError(f"case {number} pair geometry was not isolated")
        if word(before, actor + 0x86) != 2 or \
                word(before, actor + 0x8A) != 100 or \
                word(before, paired + 0x86) != 6 or \
                word(before, paired + 0x88) != 4 or \
                word(before, paired + 0x8A) != 100 or \
                word(before, paired + 0x8C) != 236 or \
                word(before, paired + 0x92) != 0:
            raise ValueError(f"case {number} pair cache was not normalized")
        if number == 3 and (current_slot == paired_slot or
                            word(before, current_paired + 0x04) != 0xFF9C or
                            word(before, current_paired + 0x08) != 0):
            raise ValueError("base-assignment case did not oppose its targets")
        if word(before, 0x46EB + 0x0A) != 0xFEB0 or \
                word(before, 0x46EB + 0x30) != 4 or \
                word(before, 0x46EB + 0x32) != 1 or \
                word(before, 0x476B + 0x0A) != 0x0150:
            raise ValueError(f"case {number} team context was not normalized")
        if number in (1, 3, 4) and (word(after, actor + 0x56) != 148 or
                            word(after, actor + 0x58) != 0):
            raise ValueError("role-clear case did not refresh its target")
        if number == 2 and (word(after, actor + 0x56) !=
                            word(before, actor + 0x56) or
                            word(after, actor + 0x58) !=
                            word(before, actor + 0x58)):
            raise ValueError("accepted loose-ball gate changed the native target")
        if word(before, actor + 0x16) != case["controller"] or \
                word(before, actor + 0x72) != case["boost"]:
            raise ValueError(f"case {number} controller/boost inputs changed")
        if number == 4:
            if word(before, 0x0910) != 0x3EEB or \
                    word(before, 0x0948) != 0 or \
                    word(before, 0x3EF7) != 100 or \
                    word(before, 0x3EFD) != 0xFF9C or \
                    word(before, actor + 0x8E) != 10:
                raise ValueError("boosted CPU jump inputs changed")
            if word(after, actor + 0x72) != 14 or \
                    word(after, actor + 0x30) != 0x32 or \
                    word(after, actor + 0x32) != 0x32:
                raise ValueError("boosted CPU jump result changed")

        retained.append({
            "name": name,
            "entry_pc": vector["entry_pc"],
            "exit_pc": vector["exit_pc"],
            "executed": executed,
            "input": before.hex(),
            "expected": projection(after),
            "observed": {
                "slot": slot,
                "paired_slot": paired_slot,
                "current_slot": current_slot,
                "assignment_base": word(before, actor + 0x74),
                "assignment_current": word(before, actor + 0x76),
                "role_ownerless": word(before, 0x09D8),
                "owner": word(before, 0x093E),
                "receiver": word(before, 0x0946),
                "controller": word(before, actor + 0x16),
                "boost": [word(before, actor + 0x72),
                          word(after, actor + 0x72)],
                "ec32_calls": path.get("ec32_calls"),
                "target_x": [word(before, actor + 0x56),
                             word(after, actor + 0x56)],
                "target_y": [word(before, actor + 0x58),
                             word(after, actor + 0x58)],
                "timer": [word(before, actor + 0x60),
                          word(after, actor + 0x60)],
                "velocity_x": [word(before, actor + 0x0E),
                               word(after, actor + 0x0E)],
                "velocity_y": [word(before, actor + 0x10),
                               word(after, actor + 0x10)],
                "velocity_z": [word(before, actor + 0x12),
                               word(after, actor + 0x12)],
                "upper_state": [word(before, actor + 0x30),
                                word(after, actor + 0x30)],
                "lower_state": [word(before, actor + 0x32),
                                word(after, actor + 0x32)],
            },
        })

    document = {
        "schema": 1,
        "rom_sha256": ROM_SHA256,
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
    print("[CPU MODE TWO PARENT NORMALIZE] calls=4 entry=86f6cd exit=86f793")


if __name__ == "__main__":
    main()
