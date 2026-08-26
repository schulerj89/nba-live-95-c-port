"""Replay phase>=3 live attached-ball vertical-response vectors."""

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
    all_vectors = [json.loads(line) for line in Path(args.vectors).open()
                   if line.strip()]
    vectors = []
    for vector in all_vectors:
        entry = memory(vector["entry"])
        owner_record = word(entry, 0x0940)
        if word(entry, owner_record + 0x3A) >= 3:
            vectors.append(vector)
    rows, expected = [], []
    for vector in vectors:
        entry = memory(vector["entry"])
        exit_mem = memory(vector["exit"])
        rows.append([word(entry, address) for address in
                     (0x09F6, 0x0968, 0x00AA, 0x00AE, 0x00B0,
                      0x13E5, 0x13E7)])
        expected.append([word(exit_mem, address) for address in
                         (0x09F6, 0x0968, 0x00AA, 0x00AE, 0x00B0,
                          0x13E5, 0x13E7)])
    text = "\n".join(" ".join(f"{value:04x}" for value in row)
                     for row in rows) + "\n"
    run = subprocess.run([args.probe], input=text, text=True,
                         capture_output=True, check=True)
    actual = [[int(value, 16) for value in line.split()]
              for line in run.stdout.splitlines()]
    mismatches = [(index, want, got) for index, (want, got) in
                  enumerate(zip(expected, actual), 1) if want != got]
    if len(actual) != len(expected):
        raise AssertionError(
            f"probe returned {len(actual)} rows for {len(expected)} vectors")
    if mismatches:
        for mismatch in mismatches[:10]:
            print("call=%d rom=%s port=%s" % mismatch)
        raise SystemExit(
            f"[ATTACHED BALL] FAIL: vectors={len(vectors)} "
            f"mismatches={len(mismatches)}")
    print(f"[ATTACHED BALL] PASS: vectors={len(vectors)} mismatches=0")


if __name__ == "__main__":
    main()
