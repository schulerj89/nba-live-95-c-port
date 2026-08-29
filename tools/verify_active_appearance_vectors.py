"""Replay the observable `$86:D85E-$DA17` active-appearance record build."""

import argparse
import json
from pathlib import Path
import subprocess


def memory(vector, side):
    result = bytearray(0x10000)
    for base, encoded in vector[side]["mem"].items():
        raw = bytes.fromhex(encoded)
        start = int(base, 16)
        result[start:start + len(raw)] = raw
    return result


def word(raw, address):
    return raw[address] | raw[address + 1] << 8


def lorom_offset(address):
    return ((address >> 16) & 0x7f) * 0x8000 + (address & 0x7fff)


def convert(vector, rom):
    before, after = memory(vector, "entry"), memory(vector, "exit")
    lineup = list(before[0x159a:0x15a4])
    appearance_a, appearance_b, variants = [], [], []
    for actor in range(10):
        base = 0x34eb + actor * 0x100
        record_index = word(before, base)
        pointer_at = 0x3449 + record_index * 4
        pointer = (before[pointer_at] | before[pointer_at + 1] << 8 |
                   before[pointer_at + 2] << 16)
        offset = lorom_offset(pointer)
        variants.append(rom[offset + 0x08])
        selector = lineup[actor] & 0x7f
        appearance_index = 5 + selector if actor < 5 else selector
        pointer_at = 0x3449 + appearance_index * 4
        pointer = (before[pointer_at] | before[pointer_at + 1] << 8 |
                   before[pointer_at + 2] << 16)
        offset = lorom_offset(pointer)
        appearance_a.append(rom[offset + 0x36])
        appearance_b.append(rom[offset + 0x37])
    expected = []
    for field in (0x76, 0x78, 0x6c, 0x80):
        expected.extend(word(after, 0x34eb + actor * 0x100 + field)
                        for actor in range(10))
    expected.extend(word(after, 0x09da + i * 2) for i in range(10))
    return {"input": lineup + appearance_a + appearance_b + variants,
            "expected": expected, "exit_pc": vector["exit_pc"]}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--probe", required=True)
    parser.add_argument("--rom")
    parser.add_argument("--normalized", action="store_true")
    parser.add_argument("--write-fixture")
    args = parser.parse_args()
    if args.normalized:
        rows = json.loads(Path(args.vectors).read_text())
    else:
        if not args.rom:
            parser.error("raw captures require --rom")
        rom = Path(args.rom).read_bytes()
        rows = [convert(json.loads(line), rom) for line in
                Path(args.vectors).read_text().splitlines() if line]
    run = subprocess.run([args.probe], input="".join(
        " ".join(f"{value & 0xffff:x}" for value in row["input"]) + "\n"
        for row in rows), text=True, capture_output=True, check=True)
    outputs = [[int(value, 16) for value in line.split()]
               for line in run.stdout.splitlines() if line]
    bad = [(row["expected"], got) for row, got in zip(rows, outputs)
           if row["expected"] != got]
    print(f"[ACTIVE APPEARANCE] calls={len(rows)} mismatches={len(bad)}")
    for failure in bad[:3]:
        print(failure)
    if len(outputs) != len(rows) or bad:
        raise SystemExit(1)
    if args.write_fixture and not args.normalized:
        Path(args.write_fixture).write_text(json.dumps(rows, indent=2) + "\n")


if __name__ == "__main__":
    main()
