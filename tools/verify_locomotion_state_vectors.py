"""Replay captured `$87:B572` state selection through compiled C."""

import argparse
import json
import subprocess
from pathlib import Path


def memory(snapshot):
    raw = bytearray(0x4B00)
    for base_text, payload in snapshot["mem"].items():
        base = int(base_text, 16)
        data = bytes.fromhex(payload)
        raw[base:base + len(data)] = data
    return raw


def word(raw, address):
    return raw[address] | raw[address + 1] << 8


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--probe", required=True)
    args = parser.parse_args()
    vectors = [json.loads(line) for line in
               Path(args.vectors).read_text().splitlines() if line.strip()]
    rows, expected = [], []
    for vector in vectors:
        entry, end = memory(vector["entry"]), memory(vector["exit"])
        base = word(entry, 0x0096)
        velocity_or = word(entry, base + 0x0E) | word(entry, base + 0x10)
        rows.append((word(entry, base + 0x38), velocity_or < 5,
                     word(entry, base + 0x72) != 0,
                     base == word(entry, 0x0940),
                     word(entry, base + 0x0C) != 0))
        expected.append(word(end, base + 0x38))
    stdin = "\n".join(" ".join(f"{int(value):x}" for value in row)
                      for row in rows) + "\n"
    run = subprocess.run([args.probe], input=stdin, text=True,
                         capture_output=True, check=True)
    actual = [int(value, 16) for value in run.stdout.split()]
    mismatches = [(index + 1, want, got) for index, (want, got) in
                  enumerate(zip(expected, actual)) if want != got]
    if len(actual) != len(expected) or mismatches:
        for item in mismatches[:12]:
            print(f"call {item[0]} want={item[1]:04x} got={item[2]:04x}")
        raise SystemExit(f"[LOCOMOTION STATE] FAIL: vectors={len(vectors)} "
                         f"mismatches={len(mismatches)}")
    print(f"[LOCOMOTION STATE] PASS: vectors={len(vectors)} mismatches=0")


if __name__ == "__main__":
    main()
