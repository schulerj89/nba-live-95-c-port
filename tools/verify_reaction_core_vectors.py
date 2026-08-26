"""Replay `$85:B971-$B9D1` reaction-threshold calls through compiled C."""

import argparse, json, subprocess
from pathlib import Path


def memory(snapshot):
    return {int(base, 16) + i: value for base, payload in snapshot.items()
            for i, value in enumerate(bytes.fromhex(payload))}


def word(mem, address): return mem[address] | (mem[address + 1] << 8)


def main():
    parser = argparse.ArgumentParser(); parser.add_argument("--vectors", required=True); parser.add_argument("--probe", required=True); args = parser.parse_args()
    vectors = [json.loads(line) for line in Path(args.vectors).read_text().splitlines()]
    inputs, expected = [], []
    for vector in vectors:
        entry, exit_mem = memory(vector["entry"]["mem"]), memory(vector["exit"]["mem"])
        actor = vector["entry"]["cpu"]["x"] & 0xFFFF
        inputs.append([word(entry, actor + 4), word(entry, actor + 8), word(entry, 0x3EEF), word(entry, 0x3EF3), word(entry, 0x07F6)])
        expected.append([word(exit_mem, 0x00AA), word(exit_mem, 0x07F6)])
    stdin = "\n".join(" ".join(f"{v:04x}" for v in row) for row in inputs) + "\n"
    result = subprocess.run([args.probe], input=stdin, capture_output=True, text=True, check=True)
    produced = [[int(v, 16) for v in line.split()] for line in result.stdout.splitlines()]
    mismatches = [(i + 1, expected[i], produced[i]) for i in range(min(len(expected), len(produced))) if expected[i] != produced[i]]
    print(f"[REACTION CORE] {'PASS' if not mismatches else 'FAIL'}: vectors={len(vectors)} mismatches={len(mismatches)} rng_transitions={len({tuple(row) for row in expected})}")
    for row in mismatches[:10]: print("  call=%d rom=%s port=%s" % row)
    if len(produced) != len(expected) or mismatches: raise SystemExit(1)


if __name__ == "__main__": main()
