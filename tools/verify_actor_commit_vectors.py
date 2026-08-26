"""Replay live moving `$85:96B5-$9961` actor commits through compiled C."""

import argparse
import json
import subprocess
from collections import Counter
from pathlib import Path

WRAM_SIZE = 0x4B00


def memory(snapshot):
    raw = bytearray(WRAM_SIZE)
    for base, payload in snapshot["mem"].items():
        start = int(base, 16)
        data = bytes.fromhex(payload)
        raw[start:start + len(data)] = data
    return raw


def word(raw, address):
    return raw[address] | raw[address + 1] << 8


def row(raw, base):
    return [word(raw, base + offset) for offset in (
        0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C,
        0x0E, 0x10, 0x12, 0x4A, 0x4C, 0x4E, 0xA2,
        0x94, 0x96, 0x98, 0x9A, 0xA0)]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--probe", required=True)
    args = parser.parse_args()
    vectors = [json.loads(line) for line in
               Path(args.vectors).read_text().splitlines() if line.strip()]
    entries = [memory(vector["entry"]) for vector in vectors]
    expected = []
    for vector in vectors:
        entry = memory(vector["entry"])
        end = memory(vector["exit"])
        expected.append(row(end, word(entry, 0x0096)))
    run = subprocess.run([args.probe], input=b"".join(entries),
                         capture_output=True, check=True)
    actual = [[int(item, 16) for item in line.split()]
              for line in run.stdout.decode().splitlines() if line]
    mismatches = []
    for index, (want, got) in enumerate(zip(expected, actual), 1):
        differences = [(field, a, b) for field, (a, b) in
                       enumerate(zip(want, got)) if a != b]
        if differences:
            mismatches.append((index, differences[:8]))
    if len(actual) != len(expected):
        raise AssertionError(
            f"probe returned {len(actual)} rows for {len(expected)} vectors")
    exits = Counter(vector["exit_pc"] for vector in vectors)
    if mismatches:
        for call, differences in mismatches[:12]:
            print(f"call {call}: {differences}")
        raise SystemExit(f"[ACTOR COMMIT] FAIL: vectors={len(vectors)} "
                         f"mismatches={len(mismatches)} exits={dict(exits)}")
    print(f"[ACTOR COMMIT] PASS: vectors={len(vectors)} mismatches=0 "
          f"exits={dict(exits)}")


if __name__ == "__main__":
    main()
