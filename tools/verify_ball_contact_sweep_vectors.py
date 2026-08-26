"""Replay ball/event outputs from live `$86:D652` collision sweeps."""

import argparse
import json
import subprocess
from pathlib import Path

from verify_player_contact_sweep_vectors import (
    memory, native_ball_or_event_changed, word,
)


def row(raw):
    return [word(raw, address) for address in (
        0x07F6, 0x0936, 0x093E, 0x0946, 0x13E7,
        0x0964, 0x0978, 0x09BC, 0x09B6, 0x0948,
        0x094C, 0x09C8, 0x096A, 0x097C, 0x09C4,
    )]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--probe", required=True)
    parser.add_argument("--pack", required=True)
    args = parser.parse_args()
    vectors = [json.loads(line) for line in Path(args.vectors).open()
               if line.strip()]
    vectors = [vector for vector in vectors
               if native_ball_or_event_changed(
                   memory(vector["entry"]), memory(vector["exit"]))]
    # `$81` continues into the separately owned jump-ball swap at D3C6.
    vectors = [vector for vector in vectors
               if word(memory(vector["entry"]), 0x0936) != 0x81]
    # Event-bit/RNG-only changes belong to nested animation/effect dispatch,
    # not the actor/ball classifier represented by this replay.
    vectors = [vector for vector in vectors
               if any(a != b for index, (a, b) in enumerate(zip(
                   row(memory(vector["entry"])),
                   row(memory(vector["exit"])))) if index not in (0, 4))]
    images = [memory(vector["entry"]) for vector in vectors]
    expected = [row(memory(vector["exit"])) for vector in vectors]
    run = subprocess.run([args.probe, args.pack, "ball"],
                         input=b"".join(images), capture_output=True,
                         check=True)
    actual = [[int(value, 16) for value in line.split()]
              for line in run.stdout.decode().splitlines()
              if line and not line.startswith("[")]
    mismatches = []
    for index, (want, got) in enumerate(zip(expected, actual), 1):
        acquisition = want[2] != row(memory(vectors[index - 1]["entry"]))[2]
        differences = [(field, a, b) for field, (a, b) in
                       enumerate(zip(want, got))
                       if a != b and not (field == 0 and acquisition)]
        if differences:
            mismatches.append((index, differences))
    if len(actual) != len(expected):
        raise AssertionError(f"probe rows={len(actual)}/{len(expected)}")
    if mismatches:
        for call, differences in mismatches[:15]:
            print(f"call {call}: {differences[:12]}")
        raise SystemExit(f"[BALL CONTACT SWEEP] FAIL: vectors={len(vectors)} "
                         f"mismatches={len(mismatches)}")
    print(f"[BALL CONTACT SWEEP] PASS: vectors={len(vectors)} mismatches=0")


if __name__ == "__main__":
    main()
