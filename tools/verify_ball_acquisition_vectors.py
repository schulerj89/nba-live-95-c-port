"""Replay live `$86:D25A-$D3C5` catch/acquisition continuations."""

import argparse
import json
import subprocess
from pathlib import Path

WRAM_SIZE = 0x4B00
ACTOR_BASE = 0x34EB
ACTOR_STRIDE = 0x100
ACTOR_OFFSETS = (0x0E, 0x10, 0x12, 0x4C, 0x5E, 0x60, 0xAA,
                 0x64, 0x72, 0x74, 0x7A, 0x7E, 0xAE)


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
    values = [word(raw, address) for address in (
        0x07F6, 0x0936, 0x093E, 0x093A, 0x0942, 0x0944, 0x0946,
        0x09C4, 0x09DA, 0x092C, 0x0930, 0x09D6, 0x0994, 0x0952,
        0x0954, 0x092E, 0x09BA, 0x09B8, 0x0948, 0x094C, 0x09C8,
        0x0962, 0x096A, 0x097C, 0x13E7,
        0x3EEF, 0x3EF3, 0x3EF7, 0x3EF9, 0x3EFB, 0x3EFD,
        0x472A, 0x472C, 0x472E, 0x4730,
        0x47AA, 0x47AC, 0x47AE, 0x47B0)]
    for actor in range(10):
        base = ACTOR_BASE + actor * ACTOR_STRIDE
        values.extend(word(raw, base + offset) for offset in ACTOR_OFFSETS)
    return values


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--probe", required=True)
    parser.add_argument("--pack", required=True)
    parser.add_argument("--continuation", action="store_true")
    args = parser.parse_args()
    vectors = [json.loads(line) for line in Path(args.vectors).open()
               if line.strip()]
    if args.continuation:
        # `$81` branches at D3C6 into the separately owned jump-ball setup
        # instead of returning through the captured D3C5 boundary.
        vectors = [vector for vector in vectors
                   if word(memory(vector["entry"]), 0x0936) != 0x81]
    images = [memory(vector["entry"]) for vector in vectors]
    expected = [row(memory(vector["exit"])) for vector in vectors]
    command = [args.probe, args.pack]
    if args.continuation:
        command.append("continuation")
    run = subprocess.run(command, input=b"".join(images),
                         capture_output=True, check=True)
    actual = [[int(value, 16) for value in line.split()]
              for line in run.stdout.decode().splitlines()
              if line and not line.startswith("[")]
    if len(actual) != len(expected):
        raise AssertionError(
            f"probe returned {len(actual)} rows for {len(expected)} vectors; "
            f"stderr={run.stderr.decode(errors='replace')}")
    mismatches = []
    for index, (want, got) in enumerate(zip(expected, actual), 1):
        # `$86:BBB5/$BBE8/$BC66` dispatches separately owned stat/effect
        # callbacks. Two captured rebound calls consume RNG inside those
        # nested callbacks; the acquisition core itself never reads `$07F6`.
        owned_fields = ({1, 4, 5, 6, 12, 17} if args.continuation
                        else set(range(len(want))) - {0})
        differences = [(field, a, b) for field, (a, b) in
                       enumerate(zip(want, got))
                       if a != b and field in owned_fields]
        if differences:
            mismatches.append((index, differences[:16]))
    if mismatches:
        for call, differences in mismatches[:12]:
            print(f"call {call}: {differences}")
        raise SystemExit(
            f"[BALL ACQUISITION] FAIL: vectors={len(vectors)} "
            f"mismatches={len(mismatches)}")
    pointer_address = 0x009A if args.continuation else 0x0096
    catchers = sorted({
        (word(memory(vector["entry"]), pointer_address) - ACTOR_BASE) // ACTOR_STRIDE
        for vector in vectors
    })
    nested_rng = sum(row(memory(vector["entry"]))[0] !=
                     row(memory(vector["exit"]))[0] for vector in vectors)
    label = "BALL ACQUISITION CONTINUATION" if args.continuation else \
        "BALL ACQUISITION"
    print(f"[{label}] PASS: vectors={len(vectors)} "
          f"catchers={catchers} nested_stat_rng={nested_rng} mismatches=0")


if __name__ == "__main__":
    main()
