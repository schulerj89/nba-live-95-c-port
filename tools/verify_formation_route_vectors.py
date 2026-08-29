"""Replay `$85:AD6B-$AF5B` calls through the production C formation route."""

import argparse
import json
import subprocess
from collections import Counter
from pathlib import Path

SIZE = 0x4B00


def memory(snapshot):
    raw = bytearray(SIZE)
    for base, payload in snapshot["mem"].items():
        start = int(base, 16)
        data = bytes.fromhex(payload)
        raw[start:start + len(data)] = data
    return raw


def word(raw, at):
    return raw[at] | raw[at + 1] << 8


def projected(raw, ran):
    values = [int(ran)]
    for actor in range(10):
        base = 0x34EB + actor * 0x100
        values.extend((word(raw, base + 0x56), word(raw, base + 0x58),
                       word(raw, base + 0x7E), word(raw, base + 0x0E),
                       word(raw, base + 0x10), word(raw, base + 0x72)))
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
    else:
        vectors = [json.loads(line) for line in path.open() if line.strip()]
    images, expected = [], []
    exits = Counter()
    for vector in vectors:
        if normalized:
            images.append(bytes.fromhex(vector["input"]))
            expected.append(vector["expected"])
            exits[vector["exit"]] += 1
            continue
        before = memory(vector["entry"])
        after = memory(vector["exit"])
        images.append(before)
        ran = vector["exit_pc"] != "85ad77"
        expected.append(projected(after, ran))
        exits[vector["exit_pc"]] += 1
    run = subprocess.run([args.probe, args.pack], input=b"".join(images),
                         capture_output=True, check=True)
    actual = [[int(value, 16) for value in line.split()]
              for line in run.stdout.decode().splitlines()
              if line and not line.startswith("[")]
    bad = [(index, want, got) for index, (want, got) in
           enumerate(zip(expected, actual), 1) if want != got]
    print(f"[FORMATION ROUTE] {'PASS' if not bad and len(actual) == len(expected) else 'FAIL'}: "
          f"calls={len(expected)} exits={dict(exits)} mismatches={len(bad)}")
    for index, want, got in bad[:12]:
        differences = [(i, a, b) for i, (a, b) in enumerate(zip(want, got))
                       if a != b]
        print(index, differences[:12])
    if bad or len(actual) != len(expected):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
