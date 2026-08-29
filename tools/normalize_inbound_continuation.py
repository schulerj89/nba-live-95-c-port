"""Reduce raw Mesen `$86:F43A` snapshots to durable motion witnesses."""

import argparse
import json
from pathlib import Path


def memory(snapshot):
    result = {}
    for base_text, payload in snapshot.items():
        base = int(base_text, 16)
        result.update((base + index, value)
                      for index, value in enumerate(bytes.fromhex(payload)))
    return result


def word(data, address):
    return data[address] | data[address + 1] << 8


def rom_byte(rom, bank, address):
    offset = ((bank & 0x7F) * 0x8000) + (address & 0x7FFF)
    return rom[offset]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    rom = Path(args.rom).read_bytes()
    rows = [json.loads(line) for line in Path(args.vectors).read_text().splitlines()
            if line.strip()]
    calls = []
    for row in rows:
        entry = memory(row["entry"]["mem"])
        exit_mem = memory(row["exit"]["mem"])
        actor = word(entry, 0x0096)
        dx = (word(entry, 0x0958) - word(entry, actor + 0x04)) & 0xFFFF
        dy = (word(entry, 0x095A) - word(entry, actor + 0x08)) & 0xFFFF
        dx = dx - 0x10000 if dx & 0x8000 else dx
        dy = dy - 0x10000 if dy & 0x8000 else dy
        arrived = -9 <= dx < 9 and -9 <= dy < 9
        # F654->F439 is the non-arrival continuation. Arrival waits also reach
        # F439, so classify them by the exact F4F2 signed box rather than by a
        # timer value that the surrounding 60-Hz clock can concurrently alter.
        if row["exit_pc"].lower() != "86f439" or arrived:
            continue
        profile_address = (word(entry, 0x00E0) + 0x42) & 0xFFFF
        profile_bank = entry[0x00E2]
        if profile_bank in (0x7E, 0x7F):
            profile = entry[profile_address]
        else:
            profile = rom_byte(rom, profile_bank, profile_address)
        inputs = [
            word(entry, actor + 0x04), word(entry, actor + 0x08),
            word(entry, 0x0958), word(entry, 0x095A),
            word(entry, actor + 0x0E), word(entry, actor + 0x10),
            word(entry, actor + 0x72), profile, word(entry, 0x00C6),
            int(word(entry, actor + 0x0C) != 0 or word(entry, 0x0936) == 0x81),
            word(entry, 0x093E),
        ]
        expected = [word(exit_mem, actor + 0x0E),
                    word(exit_mem, actor + 0x10),
                    word(exit_mem, actor + 0x72)]
        calls.append({"call": row["call"], "frame": row["entry_frame"],
                      "actor": word(entry, actor), "input": inputs,
                      "expected": expected})
    payload = {
        "routine": "$86:F43A-$F4E5 -> $85:A82C",
        "provenance": "natural ROM execution in Mesen; no PC/ROM patching",
        "raw_calls": len(rows), "motion_calls": len(calls), "calls": calls,
    }
    Path(args.output).write_text(json.dumps(payload, indent=2) + "\n")
    print(f"normalized {len(calls)} motion calls from {len(rows)} F43A calls")


if __name__ == "__main__":
    main()
