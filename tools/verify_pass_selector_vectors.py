"""Replay `$85:B50E-$B5FE` pass selection through compiled C."""

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
    exits, selected = Counter(), Counter()
    for vector in vectors:
        entry = memory(vector["entry"]["mem"])
        context = word(entry, 0x009E)
        # Context +$0A is the signed attacking-basket X anchor. Positive is
        # the ROM's rightward offense; negative mirrors the X comparisons.
        attack_right = word(entry, context + 0x0A) < 0x8000
        row = [actor_id(word(entry, 0x0096)), word(entry, 0x09A2),
               word(entry, 0x09AA), word(entry, 0x09AC),
               word(entry, 0x09AE), int(attack_right)]
        for actor in range(ACTOR_COUNT):
            row.extend(actor_fields(entry, actor))
        inputs.append(row)
        result = (word(memory(vector["exit"]["mem"]), 0x00AA)
                  if vector["exit_pc"].lower() == "85b600" else 0xFFFF)
        expected.append(result)
        exits[vector["exit_pc"]] += 1
        selected[result] += 1
    stdin = "\n".join(" ".join(f"{value:04x}" for value in row)
                      for row in inputs) + "\n"
    result = subprocess.run([args.probe], input=stdin, capture_output=True,
                            text=True, check=True)
    produced = [int(line, 16) for line in result.stdout.splitlines()
                if line.strip()]
    mismatches = [(i + 1, expected[i], produced[i])
                  for i in range(min(len(expected), len(produced)))
                  if expected[i] != produced[i]]
    print(f"[PASS SELECTOR] {'PASS' if not mismatches else 'FAIL'}: "
          f"vectors={len(vectors)} mismatches={len(mismatches)} "
          f"exits={dict(exits)} selected={dict(selected)}")
    for mismatch in mismatches[:10]:
        print("  call=%d rom=%04x port=%04x" % mismatch)
    if len(produced) != len(expected) or mismatches or len(exits) != 2:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
