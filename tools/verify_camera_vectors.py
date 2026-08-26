"""Replay `$85:9192-$93F4` camera calls through compiled C."""

import argparse
import json
import subprocess
from collections import Counter
from pathlib import Path


def memory(snapshot):
    return {int(base, 16) + i: value
            for base, payload in snapshot.items()
            for i, value in enumerate(bytes.fromhex(payload))}


def word(mem, address):
    return mem[address] | (mem[address + 1] << 8)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--probe", required=True)
    args = parser.parse_args()
    vectors = [json.loads(line) for line in
               Path(args.vectors).read_text().splitlines() if line.strip()]
    inputs, expected = [], []
    paths = Counter()
    for vector in vectors:
        entry = memory(vector["entry"]["mem"])
        exit_mem = memory(vector["exit"]["mem"])
        height_path = word(entry, 0x0936) == 1
        inputs.append([
            word(entry, 0x085C), word(entry, 0x0860),
            word(entry, 0x085E), word(entry, 0x0862),
            word(entry, 0x4A56), word(entry, 0x4A58),
            word(entry, 0x4A5A), word(entry, 0x4A5C),
            word(entry, 0x3EF7), word(entry, 0x093A), int(height_path)
        ])
        expected.append([word(exit_mem, address) for address in
                         (0x085C, 0x0860, 0x085E, 0x0862,
                          0x088C, 0x088E, 0x0890, 0x0892)])
        paths["height" if height_path else "normal"] += 1
    stdin = "\n".join(" ".join(f"{value:04x}" for value in row)
                      for row in inputs) + "\n"
    result = subprocess.run([args.probe], input=stdin, capture_output=True,
                            text=True, check=True)
    produced = [[int(value, 16) for value in line.split()]
                for line in result.stdout.splitlines() if line.strip()]
    mismatches = [(i + 1, inputs[i], expected[i], produced[i])
                  for i in range(min(len(expected), len(produced)))
                  if expected[i] != produced[i]]
    print(f"[CAMERA] {'PASS' if not mismatches else 'FAIL'}: "
          f"vectors={len(vectors)} mismatches={len(mismatches)} "
          f"paths={dict(paths)}")
    for mismatch in mismatches[:10]:
        print("  call=%d input=%s rom=%s port=%s" % mismatch)
    if len(produced) != len(expected) or mismatches:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
