"""Replay `$85:B4B9-$B50D` cutter cadence through compiled C."""

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


def weighted_distance(dx, dy):
    ax, ay = abs(dx), abs(dy)
    return max(ax, ay) + (min(ax, ay) >> 2)


def lane_clear(mem, subject, basket_x):
    subject_base = ACTOR_BASE + subject * ACTOR_STRIDE
    sx, sy = signed(word(mem, subject_base + 4)), signed(word(mem, subject_base + 8))
    low_x, high_x = min(sx, basket_x), max(sx, basket_x)
    low_y, high_y = sy - 40, sy + 40
    group = (subject // 5) * 5
    for actor in range(ACTOR_COUNT):
        if actor == subject or (actor // 5) * 5 == group:
            continue
        base = ACTOR_BASE + actor * ACTOR_STRIDE
        x, y = signed(word(mem, base + 4)), signed(word(mem, base + 8))
        if low_x <= x < high_x and low_y <= y < high_y:
            return False
    return True


def signed(value):
    return value - 0x10000 if value & 0x8000 else value


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--probe", required=True)
    args = parser.parse_args()
    vectors = [json.loads(line) for line in
               Path(args.vectors).read_text().splitlines() if line.strip()]
    inputs, expected = [], []
    branches = Counter()
    for vector in vectors:
        entry = memory(vector["entry"]["mem"])
        exit_mem = memory(vector["exit"]["mem"])
        actor = actor_id(vector["entry"]["cpu"]["x"] & 0xFFFF)
        base = ACTOR_BASE + actor * ACTOR_STRIDE
        owner = signed(word(entry, 0x093E))
        try:
            if owner < 0 or owner >= ACTOR_COUNT:
                raise ValueError(f"invalid possession actor {owner}")
            owner_base = ACTOR_BASE + owner * ACTOR_STRIDE
            distance = weighted_distance(
                signed(word(entry, owner_base + 4)) - signed(word(entry, base + 4)),
                signed(word(entry, owner_base + 8)) - signed(word(entry, base + 8)))
            possession = True
        except ValueError:
            possession, distance = False, 0xFFFF
        context = word(entry, 0x009E)
        clear = lane_clear(entry, actor, signed(word(entry, context + 0x0A)))
        inputs.append([word(entry, base + 0x64), word(entry, base + 0x5E),
                       word(entry, 0x09A4), int(possession), int(clear),
                       distance, actor, word(entry, 0x09A2)])
        expected.append([word(exit_mem, base + 0x64), word(exit_mem, 0x09A2)])
        branches["reload" if signed(word(entry, base + 0x64) - 2 & 0xFFFF) < 0
                 else "decrement"] += 1
    stdin = "\n".join(" ".join(f"{value:04x}" for value in row)
                      for row in inputs) + "\n"
    result = subprocess.run([args.probe], input=stdin, capture_output=True,
                            text=True, check=True)
    produced = [[int(value, 16) for value in line.split()]
                for line in result.stdout.splitlines() if line.strip()]
    mismatches = [(i + 1, expected[i], produced[i])
                  for i in range(min(len(expected), len(produced)))
                  if expected[i] != produced[i]]
    print(f"[SPECIAL ACTOR] {'PASS' if not mismatches else 'FAIL'}: "
          f"vectors={len(vectors)} mismatches={len(mismatches)} "
          f"branches={dict(branches)}")
    for mismatch in mismatches[:10]:
        print("  call=%d rom=%s port=%s" % mismatch)
    if len(produced) != len(expected) or mismatches or len(branches) != 2:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
