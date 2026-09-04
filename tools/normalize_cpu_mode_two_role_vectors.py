"""Retain controlled native `$86:F721-$F72D` role-flag witnesses."""

import argparse
import hashlib
import json
from pathlib import Path

from verify_normal_actor_parent_vectors import memory, projection, word


ROM_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
CASE_NAMES = ("role_clear_ownerless_record", "role_set_named_owner")


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
        raise ValueError("expected exactly two aligned mode-two witnesses")

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
        if number == 1 and ("86f72e" not in executed or
                            any(pc in executed for pc in
                                ("86f726", "86f72a", "86f72c"))):
            raise ValueError("role-clear case did not use the defense branch")
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
        if word(before, 0x0936) != 0x81 or word(before, 0x0946) != 0 or \
                word(before, 0x0978) != 1 or \
                word(before, 0x492F) != slot:
            raise ValueError(f"case {number} did not isolate the loose-ball child")
        if word(before, actor + 0x74) != case["assignment"] or \
                word(before, actor + 0x76) != case["assignment"]:
            raise ValueError(f"case {number} assignment inputs diverged")
        paired_slot = case["assignment"] >> 1
        paired = 0x34EB + paired_slot * 0x100
        if paired_slot != case["paired_slot"] or paired != case["paired"]:
            raise ValueError(f"case {number} paired actor identity changed")
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
        if word(before, 0x46EB + 0x0A) != 0xFEB0 or \
                word(before, 0x46EB + 0x30) != 4 or \
                word(before, 0x46EB + 0x32) != 1 or \
                word(before, 0x476B + 0x0A) != 0x0150:
            raise ValueError(f"case {number} team context was not normalized")
        if number == 1 and (word(after, actor + 0x56) != 148 or
                            word(after, actor + 0x58) != 0):
            raise ValueError("role-clear case did not refresh its target")
        if number == 2 and (word(after, actor + 0x56) !=
                            word(before, actor + 0x56) or
                            word(after, actor + 0x58) !=
                            word(before, actor + 0x58)):
            raise ValueError("accepted loose-ball gate changed the native target")

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
                "role_ownerless": word(before, 0x09D8),
                "owner": word(before, 0x093E),
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
    print("[CPU MODE TWO ROLE NORMALIZE] calls=2 entry=86f6cd exit=86f793")


if __name__ == "__main__":
    main()
