"""Replay `$85:A82C-$AB16` acceleration/cap calls through compiled C."""

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
    parser.add_argument("--rom", required=True)
    args = parser.parse_args()
    rom = Path(args.rom).read_bytes()
    vectors = [json.loads(line) for line in
               Path(args.vectors).read_text().splitlines() if line.strip()]
    inputs, expected = [], []
    branches = Counter()
    for vector in vectors:
        entry = memory(vector["entry"]["mem"])
        exit_mem = memory(vector["exit"]["mem"])
        actor = word(entry, 0x0096)
        profile_record = word(entry, 0x00E0)
        profile_bank = entry[0x00E2]
        profile_address = (profile_record + 0x42) & 0xFFFF
        if profile_bank in (0x7E, 0x7F):
            profile = entry[profile_address]
        else:
            profile_offset = ((profile_bank & 0x7F) * 0x8000 +
                              (profile_address & 0x7FFF))
            profile = rom[profile_offset]
        blocked = (word(entry, actor + 0x0C) != 0 or
                   word(entry, 0x0936) == 0x81)
        boost = word(entry, actor + 0x72)
        direction = word(entry, 0x00AA)
        inputs.append([
            word(entry, actor + 0x0E), word(entry, actor + 0x10), boost,
            direction, profile, word(entry, 0x00C6),
            int(blocked), word(entry, 0x093E)
        ])
        expected.append([
            word(exit_mem, actor + 0x0E), word(exit_mem, actor + 0x10),
            word(exit_mem, actor + 0x72)
        ])
        branches[("blocked" if blocked else
                  "damp" if direction >= 8 else
                  "boost" if boost else "accelerate")] += 1
    stdin = "\n".join(" ".join(f"{value:04x}" for value in row)
                      for row in inputs) + "\n"
    result = subprocess.run([args.probe], input=stdin, capture_output=True,
                            text=True, check=True)
    produced = [[int(value, 16) for value in line.split()]
                for line in result.stdout.splitlines() if line.strip()]
    mismatches = [(i + 1, inputs[i], expected[i], produced[i])
                  for i in range(min(len(expected), len(produced)))
                  if expected[i] != produced[i]]
    print(f"[VELOCITY STEP] {'PASS' if not mismatches else 'FAIL'}: "
          f"vectors={len(vectors)} mismatches={len(mismatches)} "
          f"branches={dict(branches)}")
    for mismatch in mismatches[:10]:
        print("  call=%d input=%s rom=%s port=%s" % mismatch)
    if len(produced) != len(expected) or mismatches:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
