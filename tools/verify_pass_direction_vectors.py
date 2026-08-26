"""Replay `$85:F3C3-$F472` pass-direction calls through compiled C."""

import argparse, json, subprocess
from collections import Counter
from pathlib import Path


def memory(snapshot):
    return {int(base, 16) + i: value for base, payload in snapshot.items()
            for i, value in enumerate(bytes.fromhex(payload))}


def word(mem, address): return mem[address] | (mem[address + 1] << 8)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--probe", required=True)
    args = parser.parse_args()
    vectors = [json.loads(line) for line in Path(args.vectors).read_text().splitlines()]
    inputs, expected, directions = [], [], Counter()
    for vector in vectors:
        entry, exit_mem = memory(vector["entry"]["mem"]), memory(vector["exit"]["mem"])
        inputs.append([word(entry, 0x00AA), word(entry, 0x00AE)])
        expected.append([word(exit_mem, 0x00AA), word(exit_mem, 0x00B2)])
        directions[word(exit_mem, 0x00B2)] += 1
    stdin = "\n".join("%04x %04x" % tuple(row) for row in inputs) + "\n"
    result = subprocess.run([args.probe], input=stdin, capture_output=True, text=True, check=True)
    produced = [[int(v, 16) for v in line.split()] for line in result.stdout.splitlines()]
    mismatches = [(i + 1, expected[i], produced[i]) for i in range(min(len(expected), len(produced))) if expected[i] != produced[i]]
    print(f"[PASS DIRECTION] {'PASS' if not mismatches else 'FAIL'}: vectors={len(vectors)} mismatches={len(mismatches)} directions={dict(directions)}")
    for row in mismatches[:10]: print("  call=%d rom=%s port=%s" % row)
    if len(produced) != len(expected) or mismatches: raise SystemExit(1)


if __name__ == "__main__": main()
