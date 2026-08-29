"""Replay native `$85:B678-$B8CA` parent calls through production C."""

import argparse
import json
import subprocess
from collections import Counter
from pathlib import Path

SIZE = 0x4B00


def memory(snapshot):
    raw = bytearray(SIZE)
    for base, payload in snapshot["mem"].items():
        start = int(base, 16); data = bytes.fromhex(payload)
        raw[start:start + len(data)] = data
    return raw


def word(raw, at):
    return raw[at] | raw[at + 1] << 8


def projection(vector, before, after):
    slot = word(before, 0xC2)
    subject = 0x34EB + slot * 0x100
    if vector["exit_pc"] == "85b88c":
        outcome = 0
    else:
        outcome = 2 if word(after, subject + 0x5E) == 12 else 1
    values = [outcome, word(after, 0x07F6), word(after, 0x0936),
              word(after, 0x094C), word(after, 0x09C8)]
    for actor in range(10):
        base = 0x34EB + actor * 0x100
        values.extend(word(after, base + offset) for offset in
                      (0x56, 0x58, 0x7E, 0x0E, 0x10, 0x12,
                       0x5E, 0x30, 0x32))
    return values


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--probe", required=True)
    parser.add_argument("--pack", required=True)
    args = parser.parse_args()
    path = Path(args.vectors)
    normalized = path.suffix == ".json"
    if normalized:
        vectors = json.loads(path.read_text())["calls"]
        interrupted = 0
    else:
        captured = [json.loads(line) for line in path.open() if line.strip()]
        # Do not charge NMI-side writes to a gameplay routine suspended across
        # a frame boundary. The shared ROM RNG at $07F6 can advance there.
        vectors = [vector for vector in captured
                   if vector["entry_frame"] == vector["exit_frame"]]
        interrupted = len(captured) - len(vectors)
    images, expected, exits = [], [], Counter()
    for vector in vectors:
        if normalized:
            images.append(bytes.fromhex(vector["input"]))
            expected.append(vector["expected"])
            exits[vector["exit"]] += 1
            continue
        before, after = memory(vector["entry"]), memory(vector["exit"])
        images.append(before); expected.append(projection(vector, before, after))
        exits[vector["exit_pc"]] += 1
    run = subprocess.run([args.probe, args.pack], input=b"".join(images),
                         capture_output=True, check=True)
    actual = [[int(value, 16) for value in line.split()]
              for line in run.stdout.decode().splitlines()
              if line and not line.startswith("[")]
    bad = [(i, want, got) for i, (want, got) in
           enumerate(zip(expected, actual), 1) if want != got]
    print(f"[MODE11 PARENT] {'PASS' if not bad and len(actual)==len(expected) else 'FAIL'}: "
          f"calls={len(expected)} exits={dict(exits)} interrupted={interrupted} "
          f"mismatches={len(bad)}")
    for index, want, got in bad[:12]:
        differences = [(field, a, b) for field, (a, b) in
                       enumerate(zip(want, got)) if a != b]
        print(index, differences[:15])
    if bad or len(actual) != len(expected): raise SystemExit(1)


if __name__ == "__main__":
    main()
