"""Replay live `$87:A60D-$A6B2` resource-derived contact heights."""

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
        actor = int(vector["entry"]["cpu"]["x"]) & 0xFFFF
        rows.append(f"{word(raw, actor + 0x2A):04x} "
                    f"{word(raw, actor + 0x2C):04x}")
        out = memory(vector["exit"])
        expected.append(word(out, actor + 0xAA))
    run = subprocess.run([args.probe, args.pack], input="\n".join(rows) + "\n",
                         text=True, capture_output=True, check=True)
    actual = [int(line, 16) for line in run.stdout.splitlines()
              if len(line.split()) == 1]
    mismatches = [(i, want, got) for i, (want, got) in
                  enumerate(zip(expected, actual), 1) if want != got]
    if len(actual) != len(expected) or mismatches:
        for call, want, got in mismatches[:10]:
            print(f"call {call}: rom={want:04x} port={got:04x}")
        raise SystemExit(f"[CONTACT HEIGHT] FAIL: vectors={len(vectors)} "
                         f"mismatches={len(mismatches)}")
    print(f"[CONTACT HEIGHT] PASS: vectors={len(vectors)} mismatches=0")


if __name__ == "__main__":
    main()
