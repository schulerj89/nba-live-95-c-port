"""Replay dynamic actor pose writes from `$86:E593-$E5AA`."""

import argparse
import json
import subprocess
from collections import Counter
from pathlib import Path


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
    inputs, expected, poses = [], [], Counter()
    for vector in vectors:
        entry = memory(vector["entry"]["mem"])
        exit_mem = memory(vector["exit"]["mem"])
        actor = word(entry, 0x0096)
        inputs.append((word(entry, 0x0968), word(entry, actor + 0xAE)))
        pose = word(exit_mem, actor + 0x38)
        expected.append(pose)
        poses[pose] += 1
    stdin = "\n".join(f"{dead:04x} {latch:04x}"
                      for dead, latch in inputs) + "\n"
    result = subprocess.run([args.probe], input=stdin, capture_output=True,
                            text=True, check=True)
    produced = [int(value, 16) for value in result.stdout.split()]
    mismatches = [(i + 1, expected[i], produced[i])
                  for i in range(min(len(expected), len(produced)))
                  if expected[i] != produced[i]]
    print(f"[DRIBBLE POSE] {'PASS' if not mismatches else 'FAIL'}: "
          f"vectors={len(vectors)} mismatches={len(mismatches)} "
          f"poses={dict(poses)}")
    if len(produced) != len(expected) or mismatches or not poses[5] or not poses[12]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
