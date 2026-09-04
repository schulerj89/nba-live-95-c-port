"""Retain controlled native `$86:F6EF-$F703` signed-half witnesses."""

import argparse
import hashlib
import json
from pathlib import Path

from verify_normal_actor_parent_vectors import memory, projection, word


ROM_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
CASE_NAMES = ("negative_subpixel_opposite_half", "zero_same_half")


def load_jsonl(path):
    return [json.loads(line) for line in Path(path).read_text().splitlines()
            if line.strip()]


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def require_path(executed, required, forbidden, name):
    missing = sorted(set(required) - set(executed))
    unexpected = sorted(set(forbidden) & set(executed))
    if missing or unexpected:
        raise ValueError(
            f"{name} branch path differs: missing={missing} unexpected={unexpected}")


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
    common = {
        "86f6cd", "86f6d0", "86f6db", "86f6dd", "86f6e0", "86f6e1",
        "86f6e3", "86f6e5", "86f6ef", "86f6f1", "86f6f3", "86f6f6",
        "86f6f9", "86f703", "86f705", "86f706", "86f709", "86f70b",
        "86f70e", "86f710", "86f713", "86f714", "86f716", "86f719",
        "86f71c", "86f71e", "86f780", "86f782", "86f785", "86f78b",
        "86f78d", "86f790", "86f793",
    }
    same_half_only = {"86f6fb", "86f6fd", "86f6fe", "86f701"}
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
        if number == 1:
            require_path(executed, common, same_half_only, name)
        else:
            require_path(executed, common | same_half_only, set(), name)

        before = memory(vector["entry"])
        after = memory(vector["exit"])
        slot = word(before, 0x00C2)
        actor = 0x34EB + slot * 0x100
        context = 0x46EB if slot < 5 else 0x476B
        if slot != case["slot"] or actor != case["actor"] or \
                context != case["context"]:
            raise ValueError(f"case {number} actor/context identity changed")
        if word(before, actor + 0x5E) != 2 or word(after, actor + 0x5E) != 2:
            raise ValueError(f"case {number} did not preserve mode two")
        if word(before, actor + 0x02) != case["fraction"] or \
                word(before, actor + 0x04) != case["integer"]:
            raise ValueError(f"case {number} actor fixed-point X changed")
        if word(before, context + 0x0A) != case["anchor"]:
            raise ValueError(f"case {number} side anchor changed")
        if word(before, actor + 0x60) != 0x20:
            raise ValueError(f"case {number} did not enter the reload branch")
        if word(before, actor + 0x7A) != 1 or \
                word(before, actor + 0x16) != 0:
            raise ValueError(f"case {number} did not isolate child decisions")
        if word(after, actor + 0x60) != case["expected_timer"]:
            raise ValueError(f"case {number} native timer result changed")
        if word(after, actor + 0x4E) != 6:
            raise ValueError(f"case {number} parent direction copy changed")

        retained.append({
            "name": name,
            "entry_pc": vector["entry_pc"],
            "exit_pc": vector["exit_pc"],
            "executed": executed,
            "input": before.hex(),
            "expected": projection(after),
            "observed": {
                "slot": slot,
                "actor_x_integer": word(before, actor + 0x04),
                "actor_x_fraction": word(before, actor + 0x02),
                "side_anchor": word(before, context + 0x0A),
                "profile_40": case["profile_40"],
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
    print("[CPU MODE TWO HALF NORMALIZE] calls=2 entry=86f6cd exit=86f793")


if __name__ == "__main__":
    main()
