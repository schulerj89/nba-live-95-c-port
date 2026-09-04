"""Retain controlled native `$86:F6CD` prefix witnesses for replay."""

import argparse
import hashlib
import json
from pathlib import Path

from verify_normal_actor_parent_vectors import memory, projection, word


ROM_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
CASE_NAMES = ("owner_and_receiver_unset", "named_owner", "named_receiver")


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
        raise ValueError("expected exactly three aligned mode-two witnesses")

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
        if number == 1 and "86f6d7" not in executed:
            raise ValueError("owner/receiver-clear case skipped the repair call")
        if number == 2 and ("86f6d2" not in executed or "86f6d7" in executed):
            raise ValueError("named-owner case took the wrong prefix branch")
        if number == 3 and ("86f6d2" in executed or "86f6d7" in executed):
            raise ValueError("named-receiver case took the wrong prefix branch")

        before = memory(vector["entry"])
        after = memory(vector["exit"])
        slot = word(before, 0x00C2)
        actor = 0x34EB + slot * 0x100
        if slot != case["slot"] or actor != case["actor"]:
            raise ValueError(f"case {number} actor identity changed")
        if word(before, actor + 0x5E) != 2 or word(after, actor + 0x5E) != 2:
            raise ValueError(f"case {number} did not preserve mode two")
        if word(before, 0x093E) != case["owner"]:
            raise ValueError(f"case {number} owner record changed before entry")
        if word(before, 0x0946) != case["receiver"]:
            raise ValueError(f"case {number} receiver record changed before entry")
        if word(before, actor + 0x30) != case["animation"]:
            raise ValueError(f"case {number} animation input changed")
        if word(before, actor + 0x38) != case["base"]:
            raise ValueError(f"case {number} base input changed")
        if word(after, actor + 0x38) != case["expected_base"]:
            raise ValueError(f"case {number} base result changed")
        if word(before, actor + 0x60) != 0x40 or word(after, actor + 0x60) != 0x20:
            raise ValueError(f"case {number} did not take the timer-hold path")

        retained.append({
            "name": name,
            "entry_pc": vector["entry_pc"],
            "exit_pc": vector["exit_pc"],
            "executed": executed,
            "input": before.hex(),
            "expected": projection(after),
            "observed": {
                "slot": slot,
                "owner": word(before, 0x093E),
                "receiver": word(before, 0x0946),
                "animation": word(before, actor + 0x30),
                "base": [word(before, actor + 0x38),
                         word(after, actor + 0x38)],
                "timer": [word(before, actor + 0x60),
                          word(after, actor + 0x60)],
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
    print("[CPU MODE TWO PREFIX NORMALIZE] calls=3 entry=86f6cd exit=86f793")


if __name__ == "__main__":
    main()
