"""Replay live `$86:E7DC-$E82E` calls through the production target core."""

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
    rows, expected = [], []
    modes = set()
    for vector in vectors:
        entry = memory(vector["entry"])
        exit_mem = memory(vector["exit"])
        subject = word(entry, 0x0096)
        paired = word(entry, 0x009A)
        context = word(entry, 0x009E)
        mode = word(entry, subject + 0x5E) & 0xFF
        modes.add(mode)
        rows.append([
            mode, word(entry, subject + 4), word(entry, subject + 8),
            word(entry, subject + 0x86), word(entry, subject + 0x8A),
            word(entry, paired + 4), word(entry, paired + 8),
            word(entry, paired + 0x0E), word(entry, paired + 0x10),
            word(entry, paired + 0x88), word(entry, paired + 0x8C),
            word(entry, paired + 0x92), word(entry, context + 0x0A),
            word(entry, context + 0x30), word(entry, context + 0x32), 0,
        ])
        expected.append([word(exit_mem, subject + 0x56),
                         word(exit_mem, subject + 0x58)])
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
            f"[DEFENSE CLOSE TARGET] FAIL: vectors={len(vectors)} "
            f"mismatches={len(mismatches)} modes={sorted(modes)}")
    print(f"[DEFENSE CLOSE TARGET] PASS: vectors={len(vectors)} "
          f"mismatches=0 modes={sorted(modes)}")


if __name__ == "__main__":
    main()
