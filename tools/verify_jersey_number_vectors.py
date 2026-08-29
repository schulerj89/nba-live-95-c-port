"""Replay `$87:B05B-$B354`'s six number tiles for all ten active actors."""

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


def bcd_to_binary(value):
    if value == 0xff:
        return value
    return ((value >> 4) & 0xf) * 10 + (value & 0xf)


def convert(number_vector, bcd_vectors):
    ordered = sorted(bcd_vectors, key=lambda row: row["entry"]["cpu"]["a"])
    if [row["entry"]["cpu"]["a"] for row in ordered] != list(range(10)):
        raise ValueError("expected one B357 witness for each active actor")
    jerseys = [bcd_to_binary(row["exit"]["cpu"]["a"] & 0xffff)
               for row in ordered]
    after = memory(number_vector, "exit")
    return {"input": jerseys,
            "expected": after[0x8690:0x8e10].hex(),
            "exit_pc": number_vector["exit_pc"]}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--bcd-vectors")
    parser.add_argument("--probe", required=True)
    parser.add_argument("--pack", required=True)
    parser.add_argument("--normalized", action="store_true")
    parser.add_argument("--write-fixture")
    args = parser.parse_args()
    if args.normalized:
        rows = json.loads(Path(args.vectors).read_text())
    else:
        if not args.bcd_vectors:
            parser.error("raw capture requires --bcd-vectors")
        numbers = [json.loads(line) for line in
                   Path(args.vectors).read_text().splitlines() if line]
        bcd = [json.loads(line) for line in
               Path(args.bcd_vectors).read_text().splitlines() if line]
        rows = [convert(row, bcd) for row in numbers]
    run = subprocess.run([args.probe, args.pack], input="".join(
        " ".join(f"{value:x}" for value in row["input"]) + "\n"
        for row in rows), text=True, capture_output=True, check=True)
    outputs = [line.strip() for line in run.stdout.splitlines()
               if len(line.strip()) == 0x780 * 2]
    bad = [(row["expected"], got) for row, got in zip(rows, outputs)
           if row["expected"] != got]
    print(f"[JERSEY NUMBER] calls={len(rows)} bytes={len(rows) * 0x780} "
          f"mismatches={len(bad)}")
    if len(outputs) != len(rows) or bad:
        if bad:
            expected, got = bad[0]
            first = next((i for i, pair in enumerate(zip(expected, got))
                          if pair[0] != pair[1]), -1)
            print(f"first hex-digit mismatch={first}")
        raise SystemExit(1)
    if args.write_fixture and not args.normalized:
        Path(args.write_fixture).write_text(json.dumps(rows, indent=2) + "\n")


if __name__ == "__main__":
    main()
