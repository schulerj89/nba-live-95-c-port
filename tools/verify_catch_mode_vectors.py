"""Replay dynamic `$96/$9E` catch-mode vectors through compiled C."""

import argparse
import json
import subprocess
from collections import Counter
from pathlib import Path


def memory(snapshot):
    out = {}
    for base_text, payload in snapshot.items():
        base = int(base_text, 16)
        out.update((base + i, value)
                   for i, value in enumerate(bytes.fromhex(payload)))
    return out


def word(mem, address):
    return mem[address] | (mem[address + 1] << 8)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--probe", required=True)
    args = parser.parse_args()
    vectors = [json.loads(line) for line in
               Path(args.vectors).read_text().splitlines() if line.strip()]
    inputs, expected, branches = [], [], Counter()
    for vector in vectors:
        entry = memory(vector["entry"]["mem"])
        exit_mem = memory(vector["exit"]["mem"])
        actor, context = word(entry, 0x0096), word(entry, 0x009E)
        mode = word(entry, actor + 0x5E)
        branches["preserve14" if mode == 14 else "install11"] += 1
        inputs.append([
            word(entry, 0x0928), word(entry, context + 0x47), mode,
            word(entry, actor + 0x60), word(entry, actor + 0x7E)
        ])
        expected.append([
            word(exit_mem, context + 0x47), word(exit_mem, actor + 0x5E),
            word(exit_mem, actor + 0x60), word(exit_mem, actor + 0x7E)
        ])
    stdin = "\n".join(" ".join(f"{value:04x}" for value in row)
                      for row in inputs) + "\n"
    result = subprocess.run([args.probe], input=stdin, capture_output=True,
                            text=True, check=True)
    produced = [[int(value, 16) for value in line.split()]
                for line in result.stdout.splitlines() if line.strip()]
    mismatches = [(i + 1, expected[i], produced[i])
                  for i in range(min(len(expected), len(produced)))
                  if expected[i] != produced[i]]
    if len(produced) != len(expected):
        raise SystemExit(f"[CATCH MODE] FAIL: rows={len(produced)}/"
                         f"{len(expected)}")
    print(f"[CATCH MODE] {'PASS' if not mismatches else 'FAIL'}: "
          f"vectors={len(vectors)} mismatches={len(mismatches)} "
          f"branches={dict(branches)}")
    for call, rom, port in mismatches[:10]:
        print(f"  call={call} rom={rom} port={port}")
    if mismatches or not branches["install11"] or not branches["preserve14"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
