"""Normalize dynamic `$96/$9E` catch records and replay them through C."""

import argparse
import json
import subprocess
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


def fields(mem, actor, context):
    return [
        word(mem, actor + 0x0E), word(mem, actor + 0x10),
        word(mem, actor + 0x4C), word(mem, actor + 0xAE),
        word(mem, 0x1866), word(mem, 0x0968), word(mem, 0x096A),
        word(mem, context + 0x3F), word(mem, context + 0x41),
        word(mem, context + 0x43), word(mem, context + 0x45),
        word(mem, 0x09A2), word(mem, 0x09A6), word(mem, 0x09AA),
        word(mem, 0x09AC), word(mem, 0x09AE), word(mem, 0x093E),
        word(mem, 0x0910), word(mem, 0x0912),
    ]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--probe", required=True)
    args = parser.parse_args()
    vectors = [json.loads(line) for line in
               Path(args.vectors).read_text().splitlines() if line.strip()]
    inputs, expected = [], []
    for vector in vectors:
        entry = memory(vector["entry"]["mem"])
        exit_mem = memory(vector["exit"]["mem"])
        actor, context = word(entry, 0x0096), word(entry, 0x009E)
        inputs.append([word(entry, actor), word(entry, actor + 0x16)] +
                      fields(entry, actor, context))
        expected.append(fields(exit_mem, actor, context))
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
        raise SystemExit(f"[CATCH VECTORS] FAIL: rows={len(produced)}/"
                         f"{len(expected)}")
    print(f"[CATCH VECTORS] {'PASS' if not mismatches else 'FAIL'}: "
          f"vectors={len(vectors)} mismatches={len(mismatches)}")
    for call, rom, port in mismatches[:10]:
        print(f"  call={call} rom={rom} port={port}")
    if mismatches:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
