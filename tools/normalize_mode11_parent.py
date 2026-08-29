"""Create a bounded permanent `$85:B678-$B8CA` differential corpus."""

import argparse
import json
from pathlib import Path

SIZE = 0x4B00


def memory(snapshot):
    raw = bytearray(SIZE)
    for base, payload in snapshot["mem"].items():
        start = int(base, 16)
        data = bytes.fromhex(payload)
        raw[start:start + len(data)] = data
    return raw


def word(raw, at):
    return raw[at] | raw[at + 1] << 8


def projection(vector, after):
    slot = word(after, 0xC2)
    subject = 0x34EB + slot * 0x100
    outcome = (0 if vector["exit_pc"] == "85b88c" else
               (2 if word(after, subject + 0x5E) == 12 else 1))
    values = [outcome, word(after, 0x07F6), word(after, 0x0936),
              word(after, 0x094C), word(after, 0x09C8)]
    for actor in range(10):
        base = 0x34EB + actor * 0x100
        values.extend(word(after, base + offset) for offset in
                      (0x56, 0x58, 0x7E, 0x0E, 0x10, 0x12,
                       0x5E, 0x30, 0x32))
    return values


def spread(rows, count):
    if len(rows) <= count:
        return rows
    return [rows[(i * (len(rows) - 1)) // (count - 1)]
            for i in range(count)]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", action="append", nargs=3,
                        metavar=("LABEL", "PATH", "COUNT"), required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    calls = []
    for label, source, count in args.source:
        captured = [json.loads(line) for line in Path(source).open()
                    if line.strip()]
        rows = [row for row in captured
                if row["entry_frame"] == row["exit_frame"]]
        for row in spread(rows, int(count)):
            before, after = memory(row["entry"]), memory(row["exit"])
            calls.append({"source": label, "exit": row["exit_pc"],
                          "input": before.hex(),
                          "expected": projection(row, after)})
    Path(args.output).write_text(json.dumps(
        {"schema": "nba95-mode11-parent-v1", "calls": calls},
        separators=(",", ":")) + "\n")
    print(f"[MODE11 NORMALIZE] wrote {len(calls)} calls to {args.output}")


if __name__ == "__main__":
    main()
