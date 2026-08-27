"""Replay unchanged-state live `$87:AAB2` calls through compiled C."""

import argparse
import json
import subprocess
from pathlib import Path


def memory(snapshot):
    out = {}
    for base_text, payload in snapshot["mem"].items():
        base = int(base_text, 16)
        out.update((base + i, value)
                   for i, value in enumerate(bytes.fromhex(payload)))
    return out


def word(mem, address):
    return mem.get(address, 0) | mem.get(address + 1, 0) << 8


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--probe", required=True)
    parser.add_argument("--pack", required=True)
    args = parser.parse_args()
    rows, expected, calls = [], [], []
    vectors = [json.loads(line) for line in
               Path(args.vectors).read_text().splitlines() if line.strip()]
    for vector in vectors:
        entry, end = memory(vector["entry"]), memory(vector["exit"])
        base = vector["entry"]["cpu"]["x"]
        # State/lock lifecycle branches are verified separately by B572.
        if (word(entry, base + 0x30) != word(end, base + 0x30) or
                word(entry, base + 0x32) != word(end, base + 0x32)):
            continue
        rows.append((word(entry, base + 0x30), word(entry, base + 0x32),
                     word(entry, base + 0x52), word(entry, base + 0x4A),
                     word(entry, base + 0xA8) != 0,
                     word(entry, base + 0x6C),
                     word(entry, base + 0x42), word(entry, base + 0x44),
                     word(entry, base + 0x3A), word(entry, base + 0x3C)))
        expected.append((word(end, base + 0x42), word(end, base + 0x44),
                         word(end, base + 0x3A), word(end, base + 0x3C),
                         word(end, base + 0x2A), word(end, base + 0x2C)))
        calls.append(vector["call"])
    stdin = "\n".join(" ".join(f"{int(value):x}" for value in row)
                      for row in rows) + "\n"
    run = subprocess.run([args.probe, args.pack], input=stdin, text=True,
                         capture_output=True, check=True)
    output = [line for line in run.stdout.splitlines()
              if line == "unsupported" or
              (len(line.split()) == 6 and
               all(len(value) == 4 for value in line.split()))]
    mismatches, checked, unsupported = [], 0, 0
    for call, want, line in zip(calls, expected, output):
        if line == "unsupported":
            unsupported += 1
            continue
        got = tuple(int(value, 16) for value in line.split())
        checked += 1
        if got != want:
            mismatches.append((call, want, got))
    if len(output) != len(rows):
        raise SystemExit(f"[ANIMATION CADENCE] FAIL: output={len(output)} "
                         f"input={len(rows)}")
    if mismatches:
        for item in mismatches[:12]:
            print(f"call {item[0]} want={item[1]} got={item[2]}")
        raise SystemExit(f"[ANIMATION CADENCE] FAIL: checked={checked} "
                         f"mismatches={len(mismatches)} unsupported={unsupported}")
    print(f"[ANIMATION CADENCE] PASS: captured={len(vectors)} "
          f"unchanged_state={len(rows)} checked={checked} mismatches=0 "
          f"separate_mode2={unsupported}")


if __name__ == "__main__":
    main()
