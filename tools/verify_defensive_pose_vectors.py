"""Replay `$86:E39A-$E3CA` / `$86:E3E1-$E4A6` native pose calls."""

import argparse
from collections import Counter
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


def signed(value):
    return value - 0x10000 if value & 0x8000 else value


def convert(vector):
    before, after = memory(vector, "entry"), memory(vector, "exit")
    kind = 0 if vector["entry_pc"] == "86e39a" else 1
    actor = word(before, 0x96)
    paired = word(before, 0x9a)
    context = word(before, 0x9e)
    if not (0x34eb <= actor < 0x3eeb and context + 0x0b < 0x10000):
        raise ValueError("unexpected defensive-pose pointer")
    # The caller's rare setup path uses a valid non-player work record at
    # $E01F. The capture deliberately includes $E000-$E100 so replay the
    # pointer exactly instead of replacing it with the active actor. Selector
    # calls, however, always use another player record.
    if kind == 0 and not 0x34eb <= paired < 0x3eeb:
        raise ValueError("selector missing paired actor")
    if paired + 0x8d >= 0x10000:
        raise ValueError("defensive-pose pair pointer outside WRAM")
    packed_facing = word(before, actor + 0x4e)
    values = [
        kind, word(before, actor + 0x0c), word(before, 0x0978),
        word(before, 0x0936), word(before, 0x093e), word(before, 0x0946),
        word(before, context + 0x0a), word(before, actor + 0x04),
        word(before, actor + 0x5e), word(before, actor + 0x4c),
        word(before, paired + 0x4c), word(before, actor + 0x8a),
        word(before, actor + 0x86), word(before, actor + 0x8c),
        word(before, paired + 0x8c), word(before, actor + 0x0e),
        word(before, actor + 0x10), word(before, actor + 0x30),
        word(before, actor + 0x38),
        (word(before, actor + 0x50) << 8) | (packed_facing & 0xff),
        word(before, 0x1868),
    ]
    install = vector["exit_pc"] in ("86e485", "86e497")
    install_state = 8 if vector["exit_pc"] == "86e485" else \
        10 if vector["exit_pc"] == "86e497" else 0
    expected = [
        word(after, actor + 0x38) & 0xff,
        word(after, actor + 0x4e) & 0xff,
        word(after, actor + 0x50) & 0xff,
        word(after, 0x1868), word(after, 0x00aa) & 0xff,
        int(install), install_state,
    ]
    return {"kind": kind, "exit_pc": vector["exit_pc"],
            "input": values, "expected": expected}


def load_rows(path, normalized):
    text = Path(path).read_text()
    if normalized:
        return json.loads(text)
    return [convert(json.loads(line)) for line in text.splitlines() if line]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--selector")
    parser.add_argument("--caller")
    parser.add_argument("--vectors")
    parser.add_argument("--probe", required=True)
    parser.add_argument("--normalized", action="store_true")
    parser.add_argument("--write-fixture")
    args = parser.parse_args()
    if args.vectors:
        rows = load_rows(args.vectors, args.normalized)
    elif args.selector and args.caller:
        rows = load_rows(args.selector, args.normalized) + \
            load_rows(args.caller, args.normalized)
    else:
        parser.error("provide --vectors or both --selector and --caller")
    if not rows:
        raise SystemExit("empty defensive-pose replay")
    run = subprocess.run([args.probe], input="".join(
        " ".join(f"{value & 0xffff:x}" for value in row["input"]) + "\n"
        for row in rows), text=True, capture_output=True, check=True)
    outputs = [[int(value, 16) for value in line.split()]
               for line in run.stdout.splitlines() if line]
    if len(outputs) != len(rows):
        raise SystemExit("missing defensive-pose probe outputs")
    bad = []
    for row, got in zip(rows, outputs):
        expected = row["expected"]
        # B37C owns the installed base state and is independently replayed.
        if row["kind"] == 0:
            compare = range(7)
        else:
            compare = [1, 2, 3, 5, 6]
            if not expected[5]:
                compare.append(0)
        if any(got[i] != expected[i] for i in compare):
            bad.append((row["exit_pc"], expected, got))
    for failure in bad[:10]:
        print(failure)
    exits = Counter(row["exit_pc"] for row in rows)
    print(f"[DEFENSIVE POSE] calls={len(rows)} exits={dict(exits)} mismatches={len(bad)}")
    if bad:
        raise SystemExit(1)
    if args.write_fixture and not args.normalized:
        # Preserve every rare exit, then a deterministic spread of common
        # exits. Raw captures remain local because they contain full WRAM.
        selected = []
        groups = {}
        for row in rows:
            groups.setdefault((row["kind"], row["exit_pc"]), []).append(row)
        for group in groups.values():
            stride = max(1, len(group) // 32)
            selected.extend(group[::stride][:32])
        Path(args.write_fixture).write_text(json.dumps(selected, indent=2) + "\n")


if __name__ == "__main__":
    main()
