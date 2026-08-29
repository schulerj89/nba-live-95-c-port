"""Replay live `$86:A6B3-$A78F` mode-15 core calls through compiled C."""

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


def actor_values(raw, slot):
    base = ACTOR_BASE + slot * ACTOR_STRIDE
    return [word(raw, base + offset) for offset in (
        0x5E, 0x60, 0x64, 0x28, 0x7E, 0xC0, 0x3A,
        0x30, 0x32, 0x0E, 0x10, 0x12)]


def expected_row(vector):
    entry = memory(vector["entry"])
    raw = memory(vector["exit"])
    slot = (word(entry, 0x0096) - ACTOR_BASE) // ACTOR_STRIDE
    # Native owner/ball routine are represented by host labels in the last
    # three columns; the raw state changes above remain the primary oracle.
    owner = word(raw, 0x093E)
    return [1, word(raw, 0x0936), word(raw, 0x0942), word(raw, 0x0944),
            word(raw, 0x0946), word(raw, 0x09C4), word(raw, 0x09B8),
            word(raw, 0x3EEF), word(raw, 0x3EF3), word(raw, 0x3EF7),
            word(raw, 0x3EFB), word(raw, 0x3EFD), word(raw, 0x3EFF),
            owner, None, owner, *actor_values(raw, slot)]


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
        entries = [bytes.fromhex(vector["input"]) for vector in vectors]
        expected = [vector["expected"] for vector in vectors]
    else:
        vectors = [json.loads(line) for line in
                   path.read_text().splitlines() if line.strip()]
        entries = [memory(vector["entry"]) for vector in vectors]
        expected = [expected_row(vector) for vector in vectors]
    run = subprocess.run([args.probe, args.pack], input=b"".join(entries),
                         capture_output=True, check=True)
    actual = [[int(item, 16) for item in line.split()]
              for line in run.stdout.decode().splitlines()
              if line and not line.startswith("[")]
    mismatches = []
    # Ball attachment/detachment is delegated to `$87:B649`/`$86:99C4`,
    # outside this ledger range. Compare only globals and actor words written
    # by the A6B3-A78F core; +$7E is likewise changed by B649's attachment.
    compared = set(range(0, 7)) | {16, 17, 18, 19, 21, 22, 23, 24}
    for index, (want, got) in enumerate(zip(expected, actual), 1):
        differences = [(slot, a, b) for slot, (a, b) in
                       enumerate(zip(want, got))
                       if slot in compared and a is not None and a != b]
        if differences:
            mismatches.append((index, differences[:12]))
    if len(actual) != len(expected):
        raise AssertionError(
            f"probe returned {len(actual)} rows for {len(expected)} vectors")
    exits = Counter(vector["exit_pc"] for vector in vectors)
    if mismatches:
        for call, differences in mismatches[:10]:
            print(f"call {call}: {differences}")
        raise SystemExit(f"[PASS RELEASE] FAIL: vectors={len(vectors)} "
                         f"mismatches={len(mismatches)} exits={dict(exits)}")
    print(f"[PASS RELEASE] PASS: vectors={len(vectors)} mismatches=0 "
          f"exits={dict(exits)}")


if __name__ == "__main__":
    main()
