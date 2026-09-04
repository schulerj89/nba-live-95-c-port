"""Retain the six controlled native `$86:F2CA` witnesses for replay."""

import argparse
import hashlib
import json
from pathlib import Path

from verify_normal_actor_parent_vectors import memory, projection, word


ROM_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
CASE_NAMES = (
    "opposite_group_bypass",
    "timer_hold",
    "decision_due_cpu",
    "decision_due_human",
    "decision_due_inhibited",
    "state_82_hold",
    "ownerless_rejected",
    "state_82_center_pursuit",
)


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
        raise ValueError("expected exactly eight aligned mode-five witnesses")

    retained = []
    for number, (vector, case, path, name) in enumerate(
            zip(vectors, cases, paths, CASE_NAMES), 1):
        if case["case"] != number or path["case"] != number:
            raise ValueError(f"case {number} is not aligned")
        if case["name"] != name:
            raise ValueError(f"case {number} should be {name}")
        if vector["entry_frame"] != vector["exit_frame"]:
            raise ValueError(f"case {number} crossed an emulator frame")
        expected_exit = "86f345" if name == "state_82_center_pursuit" else "86f34e"
        if vector["entry_pc"] != "86f2ca" or vector["exit_pc"] != expected_exit:
            raise ValueError(f"case {number} has an unexpected boundary")
        executed = path["executed"]
        if not executed or executed[0] != "86f2ca" or executed[-1] != expected_exit:
            raise ValueError(f"case {number} has an incomplete PC path")
        if name == "ownerless_rejected" and (
                "86f335" not in executed or "86f33d" in executed):
            raise ValueError("rejected pursuit did not return to the normal decision")
        if name == "state_82_center_pursuit" and not all(
                pc in executed for pc in ("86f335", "86f33d", "86f345")):
            raise ValueError("accepted pursuit did not use the dedicated return")

        before = memory(vector["entry"])
        after = memory(vector["exit"])
        slot = word(before, 0x00C2)
        actor = 0x34EB + slot * 0x100
        if slot != case["slot"] or actor != case["actor"]:
            raise ValueError(f"case {number} actor identity changed")
        if word(before, actor + 0x5E) != 5 or word(after, actor + 0x5E) != 5:
            raise ValueError(f"case {number} did not preserve mode five")

        retained.append({
            "name": name,
            "entry_pc": vector["entry_pc"],
            "exit_pc": vector["exit_pc"],
            "executed": executed,
            "input": before.hex(),
            "expected": projection(after),
            "observed": {
                "slot": slot,
                "mode": [word(before, actor + 0x5E), word(after, actor + 0x5E)],
                "timer": [word(before, actor + 0x60), word(after, actor + 0x60)],
                "behavior": [word(before, actor + 0x64), word(after, actor + 0x64)],
                "flags": [word(before, actor + 0x7E), word(after, actor + 0x7E)],
                "facing": [word(before, actor + 0x4E), word(after, actor + 0x4E)],
                "ownerless": word(before, 0x09D8),
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
    print("[CPU MODE FIVE NORMALIZE] calls=8 entry=86f2ca exits=86f345/86f34e")


if __name__ == "__main__":
    main()
