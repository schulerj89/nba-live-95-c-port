"""Replay live `$85:BC07-$C0F5` calls through the production C planner."""

import argparse
import json
import subprocess
from collections import Counter
from pathlib import Path


WRAM_SIZE = 0x4B00
ACTOR_BASE = 0x34EB
ACTOR_STRIDE = 0x100


def memory(snapshot):
    raw = bytearray(WRAM_SIZE)
    for base, payload in snapshot["mem"].items():
        start = int(base, 16)
        data = bytes.fromhex(payload)
        raw[start:start + len(data)] = data
    return raw


def word(raw, address):
    return raw[address] | raw[address + 1] << 8


def expected_row(raw):
    row = [word(raw, address) for address in
           (0x09D6, 0x09D8, 0x09DA, 0x07F6)]
    for actor in range(10):
        base = ACTOR_BASE + actor * ACTOR_STRIDE
        row.extend(word(raw, base + offset) for offset in (
            0x5E, 0x84, 0x60, 0x64, 0x72, 0x74, 0x86, 0x88,
            0x8A, 0x8C, 0x8E, 0x92, 0x7E))
    return row


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--probe", required=True)
    args = parser.parse_args()
    vectors = [json.loads(line) for line in
               Path(args.vectors).read_text().splitlines() if line.strip()]
    entry_images = [memory(vector["entry"]) for vector in vectors]
    expected = [expected_row(memory(vector["exit"])) for vector in vectors]
    run = subprocess.run([args.probe], input=b"".join(entry_images),
                         capture_output=True, check=True)
    actual = [[int(item, 16) for item in line.split()]
              for line in run.stdout.decode().splitlines()]
    mismatches = []
    for index, (want, got) in enumerate(zip(expected, actual), 1):
        if want != got:
            differences = [(slot, a, b) for slot, (a, b) in
                           enumerate(zip(want, got)) if a != b]
            mismatches.append((index, differences[:12]))
    if len(actual) != len(expected):
        raise AssertionError(
            f"probe returned {len(actual)} rows for {len(expected)} vectors")
    exits = Counter(vector["exit_pc"] for vector in vectors)
    if mismatches:
        for call, differences in mismatches[:10]:
            print(f"call {call}: {differences}")
        raise SystemExit(
            f"[DEFENSE REFRESH] FAIL: vectors={len(vectors)} "
            f"mismatches={len(mismatches)} exits={dict(exits)}")
    print(f"[DEFENSE REFRESH] PASS: vectors={len(vectors)} "
          f"mismatches=0 exits={dict(exits)}")


if __name__ == "__main__":
    main()
