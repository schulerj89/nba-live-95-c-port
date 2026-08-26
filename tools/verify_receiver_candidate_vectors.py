"""Replay `$85:B60B-$B677` receiver gates through compiled C."""

import argparse
import json
import subprocess
from collections import Counter
from pathlib import Path


ACTOR_BASE = 0x34EB
ACTOR_STRIDE = 0x100
ACTOR_COUNT = 10


def memory(snapshot):
    return {int(base, 16) + i: value
            for base, payload in snapshot.items()
            for i, value in enumerate(bytes.fromhex(payload))}


def word(mem, address):
    return mem[address] | (mem[address + 1] << 8)


def actor_id(pointer):
    delta = pointer - ACTOR_BASE
    if delta < 0 or delta % ACTOR_STRIDE or delta // ACTOR_STRIDE >= ACTOR_COUNT:
        raise ValueError(f"invalid actor pointer ${pointer:04X}")
    return delta // ACTOR_STRIDE


def actor_fields(mem, actor):
    base = ACTOR_BASE + actor * ACTOR_STRIDE
    return [word(mem, base + 0x04), word(mem, base + 0x08),
            word(mem, base + 0x5E), word(mem, base + 0x86),
            word(mem, base + 0x8A)]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--probe", required=True)
    args = parser.parse_args()
    vectors = [json.loads(line) for line in
               Path(args.vectors).read_text().splitlines() if line.strip()]
    inputs, expected = [], []
    exits = Counter()
    candidates = Counter()
    for vector in vectors:
        entry = memory(vector["entry"]["mem"])
        passer = actor_id(word(entry, 0x0096))
        candidate = vector["entry"]["cpu"]["a"] & 0xFFFF
        candidate_fields = (actor_fields(entry, candidate)
                            if candidate < ACTOR_COUNT else [0] * 5)
        inputs.append([passer, candidate] + actor_fields(entry, passer) +
                      candidate_fields)
        # Native carry clear accepts; both rejecting returns set carry.
        expected.append(0 if vector["exit"]["cpu"]["ps"] & 1 else 1)
        exits[vector["exit_pc"]] += 1
        candidates[candidate] += 1
    stdin = "\n".join(" ".join(f"{value:04x}" for value in row)
                      for row in inputs) + "\n"
    result = subprocess.run([args.probe], input=stdin, capture_output=True,
                            text=True, check=True)
    produced = [int(line, 16) for line in result.stdout.splitlines()
                if line.strip()]
    mismatches = [(i + 1, inputs[i], expected[i], produced[i])
                  for i in range(min(len(expected), len(produced)))
                  if expected[i] != produced[i]]
    print(f"[RECEIVER CANDIDATE] {'PASS' if not mismatches else 'FAIL'}: "
          f"vectors={len(vectors)} mismatches={len(mismatches)} "
          f"exits={dict(exits)} candidates={dict(candidates)}")
    for mismatch in mismatches[:10]:
        print("  call=%d input=%s rom=%s port=%s" % mismatch)
    if len(produced) != len(expected) or mismatches or len(exits) != 3:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
