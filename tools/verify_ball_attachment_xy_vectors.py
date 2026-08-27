"""Replay live `$87:B649-$B669` actor-relative ball attachment vectors."""

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
    rows = []
    expected = []
    for vector in vectors:
        raw = memory(vector["entry"])
        # `$87:B649` begins with `LDX $96`; the incoming CPU X is unrelated.
        actor = word(raw, 0x0096)
        rows.append(" ".join(f"{word(raw, actor + offset):04x}" for offset in
                             (0x2A, 0x2C, 0x28, 0x04, 0x08)))
        out = memory(vector["exit"])
        expected.append((word(out, 0x3EEF), word(out, 0x3EF3)))
    run = subprocess.run([args.probe, args.pack], input="\n".join(rows) + "\n",
                         text=True, capture_output=True, check=True)
    actual = [tuple(int(value, 16) for value in line.split())
              for line in run.stdout.splitlines() if len(line.split()) == 2]
    mismatches = [(i, want, got) for i, (want, got) in
                  enumerate(zip(expected, actual), 1) if want != got]
    if len(actual) != len(expected) or mismatches:
        for call, want, got in mismatches[:10]:
            print(f"call {call}: rom={want} port={got}")
        raise SystemExit(f"[BALL ATTACH XY] FAIL: vectors={len(vectors)} "
                         f"mismatches={len(mismatches)}")
    print(f"[BALL ATTACH XY] PASS: vectors={len(vectors)} mismatches=0")


if __name__ == "__main__":
    main()
