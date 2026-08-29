"""Replay live `$86:AB73-$AF4D` calls through the C pass initializer."""

import argparse
import json
import subprocess
from collections import Counter
from pathlib import Path


WRAM_SIZE = 0x4B00
ACTOR_BASE = 0x34EB
ACTOR_STRIDE = 0x100


def memory(snapshot):
    raw = bytearray(WRAM_SIZE)
    for base, payload in snapshot["mem"].items():
        start = int(base, 16)
        data = bytes.fromhex(payload)
        raw[start:start + len(data)] = data
    return raw


def word(raw, address):
    return raw[address] | raw[address + 1] << 8


def actor_values(raw, actor):
    base = ACTOR_BASE + actor * ACTOR_STRIDE
    return [word(raw, base + offset) for offset in (
        0x5E, 0x60, 0x62, 0x66, 0xC0, 0x72, 0x7E,
        0x0E, 0x10, 0x12, 0x4C, 0x30, 0x32, 0x3A)]


def expected_row(vector):
    entry = memory(vector["entry"])
    raw = memory(vector["exit"])
    passer = word(entry, 0x00C2)
    receiver = word(entry, 0x00AA)
    return [1, word(raw, 0x0936), word(raw, 0x0942), word(raw, 0x0946),
            word(raw, 0x09C4), word(raw, 0x09B8), word(raw, 0x09DA),
            *actor_values(raw, passer), *actor_values(raw, receiver)]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--probe", required=True)
    parser.add_argument("--pack", required=True)
    args = parser.parse_args()
    path = Path(args.vectors)
    normalized = path.suffix == ".json"
    if normalized:
        vectors = json.loads(path.read_text())["calls"]
        entry_images = [bytes.fromhex(vector["input"]) for vector in vectors]
        expected = [vector["expected"] for vector in vectors]
    else:
        vectors = [json.loads(line) for line in
                   path.read_text().splitlines() if line.strip()]
        entry_images = [memory(vector["entry"]) for vector in vectors]
        expected = [expected_row(vector) for vector in vectors]
    run = subprocess.run([args.probe, args.pack], input=b"".join(entry_images),
                         capture_output=True, check=True)
    actual = [[int(item, 16) for item in line.split()]
              for line in run.stdout.decode().splitlines()
              if line and not line.startswith("[")]
    mismatches = []
    # Passer +$66 (row 10) is pose-resource state written by the called
    # `$87:B47A/$B649` resolver, not by the AB73-AF4D initializer itself.
    compared = set(range(len(expected[0]))) - {10}
    for index, (want, got) in enumerate(zip(expected, actual), 1):
        differences = [(slot, a, b) for slot, (a, b) in
                       enumerate(zip(want, got))
                       if slot in compared and a != b]
        if differences:
            mismatches.append((index, differences[:12]))
    if len(actual) != len(expected):
        raise AssertionError(
            f"probe returned {len(actual)} rows for {len(expected)} vectors; "
            f"stderr={run.stderr.decode(errors='replace')}")
    exits = Counter(vector["exit_pc"] for vector in vectors)
    if mismatches:
        for call, differences in mismatches[:10]:
            print(f"call {call}: {differences}")
        raise SystemExit(f"[PASS INIT] FAIL: vectors={len(vectors)} "
                         f"mismatches={len(mismatches)} exits={dict(exits)}")
    print(f"[PASS INIT] PASS: vectors={len(vectors)} mismatches=0 "
          f"exits={dict(exits)}")


if __name__ == "__main__":
    main()
