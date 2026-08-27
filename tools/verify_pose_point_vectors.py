"""Replay live `$87:B832-$B952` pose-point vectors through compiled C."""

import argparse
import json
import subprocess
from collections import Counter
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
    vectors = [json.loads(line) for line in
               Path(args.vectors).read_text().splitlines() if line.strip()]
    rows, expected = [], []
    selectors, exits = Counter(), Counter()
    for vector in vectors:
        entry, end = memory(vector["entry"]), memory(vector["exit"])
        base = vector["entry"]["cpu"]["x"]
        selector = word(entry, 0x0000)
        rows.append((word(entry, base + 0x2A), word(entry, base + 0x2C),
                     word(entry, base + 0x28), selector))
        expected.append((word(end, 0x0000), word(end, 0x0002),
                         word(end, 0x0004)))
        selectors[selector] += 1
        exits[vector["exit_pc"]] += 1
    stdin = "\n".join(" ".join(f"{value:x}" for value in row)
                      for row in rows) + "\n"
    run = subprocess.run([args.probe, args.pack], input=stdin, text=True,
                         capture_output=True, check=True)
    output = [line for line in run.stdout.splitlines()
              if line == "unsupported" or len(line.split()) == 3]
    actual = [None if line == "unsupported" else
              tuple(int(value, 16) for value in line.split())
              for line in output]
    mismatches = [(i + 1, want, got) for i, (want, got) in
                  enumerate(zip(expected, actual)) if want != got]
    if len(actual) != len(expected) or mismatches:
        for item in mismatches[:12]:
            print(f"call {item[0]} want={item[1]} got={item[2]}")
        raise SystemExit(f"[POSE POINT] FAIL: vectors={len(vectors)} "
                         f"mismatches={len(mismatches)}")
    print(f"[POSE POINT] PASS: vectors={len(vectors)} mismatches=0 "
          f"selectors={dict(selectors)} exits={dict(exits)}")


if __name__ == "__main__":
    main()
