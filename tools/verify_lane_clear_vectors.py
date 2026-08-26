"""Replay `$85:F5E4-$F727` lane-obstruction calls through compiled C."""

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


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--probe", required=True)
    args = parser.parse_args()
    vectors = [json.loads(line) for line in
               Path(args.vectors).read_text().splitlines() if line.strip()]
    inputs, expected = [], []
    outcomes = Counter()
    subjects = Counter()
    anchors = Counter()
    for vector in vectors:
        entry = memory(vector["entry"]["mem"])
        exit_mem = memory(vector["exit"]["mem"])
        subject = actor_id(word(entry, 0x0096))
        context = word(entry, 0x009E)
        anchor = word(entry, context + 0x0A)
        row = [subject, anchor]
        for actor in range(ACTOR_COUNT):
            base = ACTOR_BASE + actor * ACTOR_STRIDE
            row += [word(entry, base + 0x04), word(entry, base + 0x08),
                    word(entry, base + 0x6E)]
        inputs.append(row)
        blocked = word(exit_mem, 0x00AA)
        expected.append(blocked)
        outcomes[blocked] += 1
        subjects[subject] += 1
        anchors[anchor] += 1
    stdin = "\n".join(" ".join(f"{value:04x}" for value in row)
                      for row in inputs) + "\n"
    result = subprocess.run([args.probe], input=stdin, capture_output=True,
                            text=True, check=True)
    produced = [int(line, 16) for line in result.stdout.splitlines()
                if line.strip()]
    mismatches = [(i + 1, inputs[i], expected[i], produced[i])
                  for i in range(min(len(expected), len(produced)))
                  if expected[i] != produced[i]]
    print(f"[LANE CLEAR] {'PASS' if not mismatches else 'FAIL'}: "
          f"vectors={len(vectors)} mismatches={len(mismatches)} "
          f"outcomes={dict(outcomes)} subjects={dict(subjects)} "
          f"anchors={dict(anchors)}")
    for mismatch in mismatches[:10]:
        print("  call=%d rom=%s port=%s input=%s" %
              (mismatch[0], mismatch[2], mismatch[3], mismatch[1]))
    if len(produced) != len(expected) or mismatches or len(outcomes) != 2:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
