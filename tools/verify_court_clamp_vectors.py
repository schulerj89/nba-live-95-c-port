"""Replay `$85:A692-$A755` court-clamp states through compiled C."""

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
        entry, exit_mem = memory(vector["entry"]["mem"]), memory(vector["exit"]["mem"]); actor = word(entry, 0x0096)
        # At `$A692`, DP $AA/$AC already hold integrated integer/fraction X;
        # the actor record still holds the pre-step X until `$A6B8`.
        inputs.append([word(entry, 0x00AC), word(entry, 0x00AA), word(entry, actor + 6), word(entry, actor + 8), word(entry, actor + 0x0E), word(entry, actor + 0x10)])
        expected.append([word(exit_mem, actor + offset) for offset in (2, 4, 6, 8, 0x0E, 0x10)])
    stdin = "\n".join(" ".join(f"{v:04x}" for v in row) for row in inputs) + "\n"
    result = subprocess.run([args.probe], input=stdin, capture_output=True, text=True, check=True)
    produced = [[int(v, 16) for v in line.split()] for line in result.stdout.splitlines()]
    mismatches = [(i + 1, inputs[i], expected[i], produced[i]) for i in range(min(len(expected), len(produced))) if expected[i] != produced[i]]
    print(f"[COURT CLAMP] {'PASS' if not mismatches else 'FAIL'}: vectors={len(vectors)} mismatches={len(mismatches)}")
    for row in mismatches[:10]: print("  call=%d input=%s rom=%s port=%s" % row)
    if len(produced) != len(expected) or mismatches: raise SystemExit(1)


if __name__ == "__main__": main()
