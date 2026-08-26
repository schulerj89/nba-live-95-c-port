"""Replay `$86:F0FD-$F1AF` loose-ball pursuit gates through compiled C."""

import argparse, json, subprocess
from collections import Counter
from pathlib import Path

ACTOR_BASE, ACTOR_STRIDE = 0x34EB, 0x100


def memory(snapshot):
    return {int(base, 16) + i: value for base, payload in snapshot.items()
            for i, value in enumerate(bytes.fromhex(payload))}


def word(mem, address): return mem[address] | (mem[address + 1] << 8)


def main():
    parser = argparse.ArgumentParser(); parser.add_argument("--vectors", required=True); parser.add_argument("--probe", required=True); args = parser.parse_args()
    vectors = [json.loads(line) for line in Path(args.vectors).read_text().splitlines()]
    inputs, expected, exits = [], [], Counter()
    for vector in vectors:
        entry = memory(vector["entry"]["mem"]); actor = word(entry, 0x0096); actor_id = (actor - ACTOR_BASE) // ACTOR_STRIDE
        inputs.append([word(entry, 0x0936), word(entry, 0x0948), word(entry, 0x094A), word(entry, 0x0978), word(entry, 0x0996), word(entry, 0x492F), actor_id, word(entry, actor + 0x5E), word(entry, actor + 0x6E), word(entry, 0x093A), word(entry, 0x0952)])
        expected.append(1 if vector["exit_pc"].lower() == "86f190" else 0); exits[vector["exit_pc"]] += 1
    stdin = "\n".join(" ".join(f"{v:04x}" for v in row) for row in inputs) + "\n"
    result = subprocess.run([args.probe], input=stdin, capture_output=True, text=True, check=True)
    produced = [int(line, 16) for line in result.stdout.splitlines()]
    mismatches = [(i + 1, expected[i], produced[i]) for i in range(min(len(expected), len(produced))) if expected[i] != produced[i]]
    print(f"[LOOSE PURSUIT GATE] {'PASS' if not mismatches else 'FAIL'}: vectors={len(vectors)} mismatches={len(mismatches)} exits={dict(exits)}")
    for row in mismatches[:10]: print("  call=%d rom=%s port=%s" % row)
    if len(produced) != len(expected) or mismatches or len(exits) != 2: raise SystemExit(1)


if __name__ == "__main__": main()
