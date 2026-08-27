"""Replay live gameplay-effect start or step vectors through production C."""

import argparse
import json
import subprocess
from pathlib import Path


def memory(snapshot):
    raw = bytearray(0x4100)
    for base_text, payload in snapshot["mem"].items():
        base = int(base_text, 16)
        data = bytes.fromhex(payload)
        raw[base:base + len(data)] = data
    return raw


def word(raw, address):
    return raw[address] | raw[address + 1] << 8


STATE = (0x4015, 0x401B, 0x3F33, 0x4025, 0x402D)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--probe", required=True)
    parser.add_argument("--mode", choices=("start", "step"), required=True)
    args = parser.parse_args()
    vectors = [json.loads(line) for line in
               Path(args.vectors).read_text().splitlines() if line.strip()]
    rows, expected = [], []
    for vector in vectors:
        raw = memory(vector["entry"])
        state = [word(raw, address) for address in STATE]
        if args.mode == "start":
            values = [int(vector["entry"]["cpu"]["a"]) & 0xFFFF] + state
        else:
            values = state + [word(raw, address) for address in
                              (0x3FF3, 0x3EF3, 0x3EF7, 0x3EFD, 0x00C6)]
        rows.append(" ".join(f"{value:04x}" for value in values))
        out = memory(vector["exit"])
        expected.append(tuple(word(out, address) for address in STATE))
    run = subprocess.run([args.probe, args.mode], input="\n".join(rows) + "\n",
                         text=True, capture_output=True, check=True)
    actual = [tuple(int(value, 16) for value in line.split())
              for line in run.stdout.splitlines() if len(line.split()) == 5]
    mismatches = [(i, want, got) for i, (want, got) in
                  enumerate(zip(expected, actual), 1) if want != got]
    if len(actual) != len(expected) or mismatches:
        for call, want, got in mismatches[:10]:
            print(f"call {call}: rom={want} port={got}")
        raise SystemExit(f"[EFFECT {args.mode.upper()}] FAIL: "
                         f"vectors={len(vectors)} mismatches={len(mismatches)}")
    print(f"[EFFECT {args.mode.upper()}] PASS: vectors={len(vectors)} "
          "mismatches=0")


if __name__ == "__main__":
    main()
