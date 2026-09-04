"""Replay `$85:B3C9-$85:B401` direct arrival/velocity calls through compiled C."""

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
    return mem.get(address, 0) | (mem.get(address + 1, 0) << 8)


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
    exits = Counter()
    directions = Counter()
    for vector in vectors:
        entry = memory(vector["entry"]["mem"])
        exit_mem = memory(vector["exit"]["mem"])
        actor = word(entry, 0x0096)
        profile_record = word(entry, 0x00E0)
        profile_bank = entry.get(0x00E2, 0)
        profile_address = (profile_record + 0x42) & 0xFFFF
        if profile_bank in (0x7E, 0x7F):
            profile = entry.get(profile_address, 0)
        else:
            profile_offset = ((profile_bank & 0x7F) * 0x8000 +
                              (profile_address & 0x7FFF))
            profile = rom[profile_offset]
        blocked = (word(entry, actor + 0x0C) != 0 or
                   word(entry, 0x0936) == 0x81)
        inputs.append([
            word(entry, actor + 0x04), word(entry, actor + 0x08),
            word(entry, actor + 0x0E), word(entry, actor + 0x10),
            word(entry, 0x00AA), word(entry, 0x00AE),
            word(entry, 0x00B6), word(entry, actor + 0x72), profile,
            word(entry, 0x00C6), int(blocked), word(entry, 0x093E)
        ])
        arrived = 1 if vector["exit"]["cpu"]["ps"] & 1 else 0
        expected.append([
            arrived, word(exit_mem, 0x00B6),
            word(exit_mem, actor + 0x0E), word(exit_mem, actor + 0x10),
            word(exit_mem, actor + 0x72)
        ])
        exits[vector["exit_pc"]] += 1
        directions[word(exit_mem, 0x00B6)] += 1
    stdin = "\n".join(" ".join(f"{value:04x}" for value in row)
                      for row in inputs) + "\n"
    result = subprocess.run([args.probe], input=stdin, capture_output=True,
                            text=True, check=True)
    produced = [[int(value, 16) for value in line.split()]
                for line in result.stdout.splitlines() if line.strip()]
    mismatches = [(i + 1, inputs[i], expected[i], produced[i])
                  for i in range(min(len(expected), len(produced)))
                  if expected[i] != produced[i]]
    print(f"[DIRECT ARRIVAL STEP] "
          f"{'PASS' if not mismatches else 'FAIL'}: vectors={len(vectors)} "
          f"mismatches={len(mismatches)} exits={dict(exits)} "
          f"directions={dict(directions)}")
    for mismatch in mismatches[:10]:
        print("  call=%d input=%s rom=%s port=%s" % mismatch)
    if len(produced) != len(expected) or mismatches or len(exits) != 2:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
