"""Replay captured `$87:8F13-$8F61` facing easing through compiled C."""

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
    args = parser.parse_args()
    vectors = [json.loads(line) for line in
               Path(args.vectors).read_text().splitlines() if line.strip()]
    rows, expected = [], []
    for vector in vectors:
        entry, end = memory(vector["entry"]), memory(vector["exit"])
        # Entry is before `$87:8F13` executes its LDX <$96; CPU X still
        # belongs to the preceding actor, so resolve the dynamic record.
        base = word(entry, 0x0096)
        rows.append((word(entry, base + 0x4E), word(entry, base + 0x46),
                     word(entry, base + 0x52), word(entry, base + 0xBE)))
        expected.append((word(end, base + 0x52) & 0xFF,
                         word(end, base + 0xBE) & 0xFF))
    stdin = "\n".join(" ".join(f"{value:x}" for value in row)
                      for row in rows) + "\n"
    run = subprocess.run([args.probe], input=stdin, text=True,
                         capture_output=True, check=True)
    output = [tuple(int(value, 16) for value in line.split())
              for line in run.stdout.splitlines() if line.strip()]
    mismatches = [(i + 1, want, got) for i, (want, got) in
                  enumerate(zip(expected, output)) if want != got]
    if len(output) != len(expected) or mismatches:
        for item in mismatches[:12]:
            print(f"call {item[0]} want={item[1]} got={item[2]}")
        raise SystemExit(f"[FACING EASE] FAIL: vectors={len(vectors)} "
                         f"mismatches={len(mismatches)}")
    print(f"[FACING EASE] PASS: vectors={len(vectors)} mismatches=0")


if __name__ == "__main__":
    main()
