"""Replay live `$86:A1BD-$A292` base shot-launch vectors."""

import argparse
import json
import subprocess
from pathlib import Path


def memory(snapshot):
    raw = bytearray(0x4B00)
    for base, payload in snapshot["mem"].items():
        start = int(base, 16)
        data = bytes.fromhex(payload)
        raw[start:start + len(data)] = data
    return raw


def word(raw, address):
    return raw[address] | raw[address + 1] << 8


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--probe", required=True)
    args = parser.parse_args()
    vectors = [json.loads(line) for line in Path(args.vectors).open()
               if line.strip()]
    inputs, expected = [], []
    for vector in vectors:
        before, after = memory(vector["entry"]), memory(vector["exit"])
        inputs.append([word(before, address) for address in
                       (0x00B6, 0x00B8, 0x00BA, 0x00BC,
                        0x3EF5, 0x3EF7)])
        # The exec hook at `$A292` fires immediately before `STA $3EFD`;
        # DP `$AA` already holds the computed vertical velocity.
        expected.append([word(after, 0x3EF9), word(after, 0x3EFB),
                         word(after, 0x00AA)])
    stdin = "\n".join(" ".join(f"{value:04x}" for value in row)
                      for row in inputs) + "\n"
    run = subprocess.run([args.probe], input=stdin, text=True,
                         capture_output=True, check=True)
    actual = [[int(value, 16) for value in line.split()]
              for line in run.stdout.splitlines() if line.strip()]
    mismatches = [(i + 1, want, got) for i, (want, got) in
                  enumerate(zip(expected, actual)) if want != got]
    if len(actual) != len(expected):
        raise AssertionError(f"probe rows={len(actual)}/{len(expected)}")
    if mismatches:
        for item in mismatches[:10]:
            print(f"call {item[0]}: rom={item[1]} port={item[2]}")
        raise SystemExit(f"[SHOT LAUNCH] FAIL: vectors={len(vectors)} "
                         f"mismatches={len(mismatches)}")
    print(f"[SHOT LAUNCH] PASS: vectors={len(vectors)} mismatches=0")


if __name__ == "__main__":
    main()
