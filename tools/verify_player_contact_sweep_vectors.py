"""Replay player/player outputs from live `$86:D652-$D6EE` sweeps."""

import argparse
import json
import subprocess
from pathlib import Path

WRAM_SIZE = 0x4B00
ACTOR_BASE = 0x34EB
ACTOR_STRIDE = 0x100
ACTOR_OFFSETS = (0x0E, 0x10, 0x12, 0x28, 0x30, 0x32, 0x3A,
                 0x56, 0x5A, 0x5E, 0x60, 0x66, 0x72, 0x7A, 0x7E,
                 0x4C, 0x4E)


def memory(snapshot):
    raw = bytearray(WRAM_SIZE)
    for base, payload in snapshot["mem"].items():
        start = int(base, 16)
        data = bytes.fromhex(payload)
        raw[start:start + len(data)] = data
    return raw


def word(raw, address):
    return raw[address] | raw[address + 1] << 8


def row(raw):
    values = [word(raw, address) for address in
              (0x07F6, 0x0936, 0x093E, 0x0946, 0x13E7,
               0x0964, 0x0978, 0x09BC, 0x09B6)]
    for actor in range(10):
        base = ACTOR_BASE + actor * ACTOR_STRIDE
        values.extend(word(raw, base + offset) for offset in ACTOR_OFFSETS)
    return values


def native_ball_or_event_changed(entry, exit_mem):
    ranges = ((0x3EEB, 0x3EFF),)
    if any(entry[start:end + 1] != exit_mem[start:end + 1]
           for start, end in ranges):
        return True
    return any(word(entry, address) != word(exit_mem, address) for address in
               (0x0936, 0x093E, 0x0940, 0x0942, 0x0946, 0x0948,
                0x0964, 0x0978, 0x097A, 0x09BC, 0x13E7))


def native_nested_animation_only(entry, exit_mem):
    changed = []
    for actor in range(10):
        base = ACTOR_BASE + actor * ACTOR_STRIDE
        for offset in ACTOR_OFFSETS:
            if word(entry, base + offset) != word(exit_mem, base + offset):
                changed.append(offset)
    return bool(changed) and all(offset in (0x30, 0x32, 0x3A)
                                 for offset in changed)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--probe", required=True)
    parser.add_argument("--pack", required=True)
    args = parser.parse_args()
    all_vectors = [json.loads(line) for line in Path(args.vectors).open()
                   if line.strip()]
    vectors = []
    for vector in all_vectors:
        entry, exit_mem = memory(vector["entry"]), memory(vector["exit"])
        if (not native_ball_or_event_changed(entry, exit_mem) and
                not native_nested_animation_only(entry, exit_mem)):
            vectors.append(vector)
    images = [memory(vector["entry"]) for vector in vectors]
    expected = [row(memory(vector["exit"])) for vector in vectors]
    run = subprocess.run([args.probe, args.pack], input=b"".join(images),
                         capture_output=True, check=True)
    actual = [[int(value, 16) for value in line.split()]
              for line in run.stdout.decode().splitlines()
              if line and not line.startswith("[")]
    mismatches = []
    for index, (want, got) in enumerate(zip(expected, actual), 1):
        differences = [(field, a, b) for field, (a, b) in
                       enumerate(zip(want, got)) if a != b]
        if differences:
            mismatches.append((index, differences[:12]))
    if len(actual) != len(expected):
        raise AssertionError(
            f"probe returned {len(actual)} rows for {len(expected)} vectors; "
            f"stderr={run.stderr.decode(errors='replace')}")
    changed = sum(row(memory(v["entry"])) != row(memory(v["exit"]))
                  for v in vectors)
    if mismatches:
        for call, differences in mismatches[:12]:
            print(f"call {call}: {differences}")
        raise SystemExit(
            f"[PLAYER CONTACT SWEEP] FAIL: vectors={len(vectors)} "
            f"changed={changed} mismatches={len(mismatches)}")
    print(f"[PLAYER CONTACT SWEEP] PASS: vectors={len(vectors)} "
          f"changed={changed} mismatches=0")


if __name__ == "__main__":
    main()
