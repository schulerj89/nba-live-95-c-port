"""Replay `$85:B734-$B820` mode-11 shot policy through compiled C."""

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


def profile_byte(entry, rom, offset):
    address = (word(entry, 0x00E0) + offset) & 0xFFFF
    bank = entry[0x00E2]
    if bank in (0x7E, 0x7F):
        return entry[address]
    return rom[((bank & 0x7F) * 0x8000) + (address & 0x7FFF)]


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
    for vector in vectors:
        entry = memory(vector["entry"]["mem"])
        exit_mem = memory(vector["exit"]["mem"])
        actor = word(entry, 0x0096)
        inputs.append([
            word(entry, 0x0998), word(entry, 0x09A4), word(entry, 0x09D0),
            word(entry, 0x0968), word(entry, 0x17E1), word(entry, 0x17AF),
            word(entry, actor + 0x8A), word(entry, actor + 0x8C),
            profile_byte(entry, rom, 0x36), profile_byte(entry, rom, 0x37),
            profile_byte(entry, rom, 0x49), word(entry, 0x07F6)
        ])
        accepted = int(vector["exit_pc"].lower() == "85b820")
        expected.append([accepted, word(exit_mem, 0x07F6)])
        exits[vector["exit_pc"]] += 1
    stdin = "\n".join(" ".join(f"{value:04x}" for value in row)
                      for row in inputs) + "\n"
    result = subprocess.run([args.probe], input=stdin, capture_output=True,
                            text=True, check=True)
    produced = [[int(value, 16) for value in line.split()]
                for line in result.stdout.splitlines() if line.strip()]
    mismatches = [(i + 1, expected[i], produced[i])
                  for i in range(min(len(expected), len(produced)))
                  if expected[i] != produced[i]]
    print(f"[MODE-11 SHOT POLICY] {'PASS' if not mismatches else 'FAIL'}: "
          f"vectors={len(vectors)} mismatches={len(mismatches)} "
          f"exits={dict(exits)}")
    for mismatch in mismatches[:10]:
        print("  call=%d rom=%s port=%s" % mismatch)
    if len(produced) != len(expected) or mismatches or len(exits) != 2:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
