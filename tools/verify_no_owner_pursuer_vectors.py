"""Replay live `$85:B0A8-$B128` no-owner pursuit scans."""

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
    inputs, expected = [], []
    for vector in vectors:
        before, after = memory(vector["entry"]), memory(vector["exit"])
        base = word(before, 0x0096)
        row = [word(before, 0x0918), word(before, 0x091A)]
        for i in range(5):
            actor = base + i * 0x100
            row += [word(before, actor + 4), word(before, actor + 8),
                    word(before, actor + 0x5E)]
        inputs.append(row)
        selected = word(after, 0x009A)
        selected = 0xFFFF if selected == 0 else (selected - base) // 0x100
        expected.append([selected] + [word(after, base + i * 0x100 + 0x5E)
                                     for i in range(5)])
    stdin = "\n".join(" ".join(f"{value:04x}" for value in row)
                      for row in inputs) + "\n"
    run = subprocess.run([args.probe], input=stdin, text=True,
                         capture_output=True, check=True)
    actual = [[int(value, 16) for value in line.split()]
              for line in run.stdout.splitlines() if line.strip()]
    mismatches = [(i + 1, want, got) for i, (want, got) in
                  enumerate(zip(expected, actual)) if want != got]
    if len(actual) != len(expected):
        raise AssertionError(f"probe rows={len(actual)}/{len(expected)}")
    if mismatches:
        for item in mismatches[:10]:
            print(f"call {item[0]}: rom={item[1]} port={item[2]}")
        raise SystemExit(f"[NO OWNER PURSUER] FAIL: vectors={len(vectors)} "
                         f"mismatches={len(mismatches)}")
    selected = sum(row[0] != 0xFFFF for row in expected)
    print(f"[NO OWNER PURSUER] PASS: vectors={len(vectors)} "
          f"selected={selected} mismatches=0")


if __name__ == "__main__":
    main()
