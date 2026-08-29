"""Create a bounded permanent `$85:AD6B-$AF5B` witness corpus."""

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


def projected(raw, ran):
    values = [int(ran)]
    for actor in range(10):
        base = 0x34EB + actor * 0x100
        values.extend((word(raw, base + 0x56), word(raw, base + 0x58),
                       word(raw, base + 0x7E), word(raw, base + 0x0E),
                       word(raw, base + 0x10), word(raw, base + 0x72)))
    return values


def select_spread(vectors, count):
    if len(vectors) <= count:
        return vectors
    return [vectors[(i * (len(vectors) - 1)) // (count - 1)]
            for i in range(count)]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--natural", required=True)
    parser.add_argument("--controlled", nargs="+", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    sources = [("natural", args.natural, 24)]
    sources.extend((Path(path).parent.name.rsplit("-", 1)[-1], path, 8)
                   for path in args.controlled)
    calls = []
    for label, path, count in sources:
        vectors = [json.loads(line) for line in Path(path).open()
                   if line.strip()]
        for vector in select_spread(vectors, count):
            before = memory(vector["entry"])
            after = memory(vector["exit"])
            ran = vector["exit_pc"] != "85ad77"
            calls.append({"source": label, "exit": vector["exit_pc"],
                          "input": before.hex(),
                          "expected": projected(after, ran)})
    Path(args.output).write_text(json.dumps(
        {"schema": "nba95-formation-route-v1", "calls": calls},
        separators=(",", ":")) + "\n")
    print(f"[FORMATION NORMALIZE] wrote {len(calls)} calls to {args.output}")


if __name__ == "__main__":
    main()
