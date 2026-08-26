"""Replay `$86:E4A7-$E4C4` gate outcomes through compiled C."""

import argparse
import json
import subprocess
from collections import Counter
from pathlib import Path


OUTCOME = {"86e4ae": 0, "86e4c4": 1, "86e4c7": 2}


def memory(snapshot):
    out = {}
    for base_text, payload in snapshot.items():
        base = int(base_text, 16)
        out.update((base + i, value)
                   for i, value in enumerate(bytes.fromhex(payload)))
    return out


def word(mem, address):
    return mem[address] | (mem[address + 1] << 8)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--probe", required=True)
    args = parser.parse_args()
    vectors = [json.loads(line) for line in
               Path(args.vectors).read_text().splitlines() if line.strip()]
    inputs, expected, outcomes = [], [], Counter()
    for vector in vectors:
        entry = memory(vector["entry"]["mem"])
        actor = word(entry, 0x0096)
        inputs.append([
            word(entry, actor + 0x0C), word(entry, 0x0978),
            word(entry, 0x0936), word(entry, actor + 0x4C)
        ])
        expected.append(OUTCOME[vector["exit_pc"]])
        outcomes[vector["exit_pc"]] += 1
    stdin = "\n".join(" ".join(f"{value:04x}" for value in row)
                      for row in inputs) + "\n"
    result = subprocess.run([args.probe], input=stdin, capture_output=True,
                            text=True, check=True)
    produced = [int(value, 16) for value in result.stdout.split()]
    mismatches = [(i + 1, expected[i], produced[i])
                  for i in range(min(len(expected), len(produced)))
                  if expected[i] != produced[i]]
    print(f"[DRIBBLE GATE] {'PASS' if not mismatches else 'FAIL'}: "
          f"vectors={len(vectors)} mismatches={len(mismatches)} "
          f"outcomes={dict(outcomes)}")
    if len(produced) != len(expected) or mismatches or len(outcomes) != 3:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
