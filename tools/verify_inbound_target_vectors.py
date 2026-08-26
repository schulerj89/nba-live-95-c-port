"""Replay live `$85:C37D-$C5C0` inbound target construction."""

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
    rows, expected, layouts = [], [], set()
    for vector in vectors:
        entry = memory(vector["entry"])
        exit_mem = memory(vector["exit"])
        side = word(entry, 0x0952)
        context = 0x46EB if side == 0 else 0x476B
        layout = word(entry, 0x0956)
        layouts.add(layout if layout < 0x8000 else layout - 0x10000)
        rows.append([
            layout, word(entry, 0x09B0), word(entry, 0x09B2),
            word(entry, context + 0x0A), word(entry, 0x3EEF),
            word(entry, 0x07F6),
        ])
        requested = 1 if word(exit_mem, 0x0994) != word(entry, 0x0994) else 0
        expected.append([
            word(exit_mem, 0x0958), word(exit_mem, 0x095A),
            word(exit_mem, 0x095C), word(exit_mem, 0x0996), requested,
            word(exit_mem, 0x07F6),
        ])
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
            f"[INBOUND TARGET] FAIL: vectors={len(vectors)} "
            f"mismatches={len(mismatches)} layouts={sorted(layouts)}")
    print(f"[INBOUND TARGET] PASS: vectors={len(vectors)} "
          f"mismatches=0 layouts={sorted(layouts)}")


if __name__ == "__main__":
    main()
