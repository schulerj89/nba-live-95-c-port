"""Replay request-zero `$85:B24C-$B353` calls through compiled C."""

import argparse
import json
import subprocess
from pathlib import Path

WRAM_SIZE = 0x4B00


def memory(snapshot):
    raw = bytearray(WRAM_SIZE)
    for base_text, payload in snapshot["mem"].items():
        base = int(base_text, 16)
        data = bytes.fromhex(payload)
        raw[base:base + len(data)] = data
    return raw


def word(raw, address):
    return raw[address] | raw[address + 1] << 8


def row(raw):
    values = [word(raw, address) for address in
              (0x0994, 0x0996, 0x0998, 0x099A, 0x099C, 0x099E,
               0x09A2, 0x09A4, 0x09AA, 0x09AC, 0x09AE, 0x09D0,
               0x07F6, 0x0936)]
    values.extend(word(raw, 0x3569 + i * 0x100) for i in range(10))
    return values


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--probe", required=True)
    parser.add_argument("--pack", required=True)
    args = parser.parse_args()
    vectors = [json.loads(line) for line in
               Path(args.vectors).read_text().splitlines() if line.strip()]
    retained = [v for v in vectors if word(memory(v["entry"]), 0x0994) == 0]
    entries = [memory(v["entry"]) for v in retained]
    expected = []
    for vector in retained:
        raw = memory(vector["exit"])
        values = row(raw)
        # The trace's shared-exit hook is at `$85:B353`, immediately before
        # `STA $099A`.  Complete that final store from live DP `$AA` so the
        # snapshot represents the callable routine's output.
        if vector.get("exit_pc", "").lower() == "85b353":
            values[3] = word(raw, 0x00AA)
        expected.append(values)
    run = subprocess.run([args.probe, args.pack], input=b"".join(entries),
                         capture_output=True, check=True)
    output = [line for line in run.stdout.decode().splitlines()
              if len(line.split()) == 24]
    actual = [[int(value, 16) for value in line.split()] for line in output]
    mismatches = []
    for index, (want, got) in enumerate(zip(expected, actual), 1):
        diff = [(field, a, b) for field, (a, b) in
                enumerate(zip(want, got)) if a != b]
        if diff:
            mismatches.append((index, diff[:8]))
    if len(actual) != len(expected) or mismatches:
        for call, diff in mismatches[:12]:
            print(f"call {call}: {diff}")
        raise SystemExit(f"[PLAY CONTROL] FAIL: vectors={len(retained)} "
                         f"mismatches={len(mismatches)}")
    print(f"[PLAY CONTROL] PASS: vectors={len(retained)} mismatches=0")


if __name__ == "__main__":
    main()
