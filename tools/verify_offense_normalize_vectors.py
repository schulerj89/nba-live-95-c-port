"""Replay `$85:AF5C-$B128` offense normalization through compiled C."""

import argparse
import json
import subprocess
from pathlib import Path

WRAM_SIZE = 0x4B00


def memory(snapshot):
    raw = bytearray(WRAM_SIZE)
    for base_text, payload in snapshot["mem"].items():
        base = int(base_text, 16)
        data = bytes.fromhex(payload)
        raw[base:base + len(data)] = data
    return raw


def word(raw, address):
    return raw[address] | raw[address + 1] << 8


def result_row(raw, offense):
    row = [word(raw, address) for address in
           (0x0918, 0x091A, 0x09D4, 0x09D8, 0x09DE)]
    for i in range(5):
        base = 0x34EB + (offense * 5 + i) * 0x100
        row.extend((word(raw, base + 0x5E), word(raw, base + 0x88),
                    word(raw, base + 0x8C)))
    return row


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--probe", required=True)
    args = parser.parse_args()
    vectors = [json.loads(line) for line in
               Path(args.vectors).read_text().splitlines() if line.strip()]
    entries, expected = [], []
    for vector in vectors:
        entry, end = memory(vector["entry"]), memory(vector["exit"])
        side = word(entry, 0x0952) if word(entry, 0x0936) == 0x82 \
            else word(entry, 0x093A)
        offense = int(side != 0)
        entries.append(entry)
        expected.append(result_row(end, offense))
    run = subprocess.run([args.probe], input=b"".join(entries),
                         capture_output=True, check=True)
    actual = [[int(value, 16) for value in line.split()]
              for line in run.stdout.decode().splitlines() if line]
    mismatches = []
    for index, (want, got) in enumerate(zip(expected, actual), 1):
        diff = [(field, a, b) for field, (a, b) in
                enumerate(zip(want, got)) if a != b]
        if diff:
            mismatches.append((index, diff[:8]))
    if len(actual) != len(expected) or mismatches:
        for call, diff in mismatches[:12]:
            print(f"call {call}: {diff}")
        raise SystemExit(f"[OFFENSE NORMALIZE] FAIL: vectors={len(vectors)} "
                         f"mismatches={len(mismatches)}")
    print(f"[OFFENSE NORMALIZE] PASS: vectors={len(vectors)} mismatches=0")


if __name__ == "__main__":
    main()
