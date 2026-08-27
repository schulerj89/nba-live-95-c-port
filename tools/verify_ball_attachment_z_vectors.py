"""Replay live `$87:B66A-$B67B` actor-relative ball Z vectors."""

import argparse
import json
import subprocess
from pathlib import Path


def memory(snapshot):
    raw = bytearray(0x4000)
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
    parser.add_argument("--pack", required=True)
    args = parser.parse_args()
    vectors = [json.loads(line) for line in
               Path(args.vectors).read_text().splitlines() if line.strip()]
    rows, expected = [], []
    for vector in vectors:
        raw = memory(vector["entry"])
        actor = word(raw, 0x0096)
        rows.append(" ".join(f"{word(raw, actor + offset):04x}" for offset in
                             (0x2A, 0x2C, 0x28, 0x0C)))
        expected.append(word(memory(vector["exit"]), 0x3EF7))
    run = subprocess.run([args.probe, args.pack], input="\n".join(rows) + "\n",
                         text=True, capture_output=True, check=True)
    actual = [int(line, 16) for line in run.stdout.splitlines()
              if len(line.split()) == 1]
    mismatches = [(i, want, got) for i, (want, got) in
                  enumerate(zip(expected, actual), 1) if want != got]
    if len(actual) != len(expected) or mismatches:
        for call, want, got in mismatches[:10]:
            print(f"call {call}: rom={want:04x} port={got:04x}")
        raise SystemExit(f"[BALL ATTACH Z] FAIL: vectors={len(vectors)} "
                         f"mismatches={len(mismatches)}")
    print(f"[BALL ATTACH Z] PASS: vectors={len(vectors)} mismatches=0")


if __name__ == "__main__":
    main()
