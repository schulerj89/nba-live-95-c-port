"""Normalize controlled native `$86:C493-$C4FD` bookkeeping calls."""
import argparse
import json
from pathlib import Path


def memory(snapshot):
    result = {}
    for base, payload in snapshot.items():
        start = int(base, 16)
        result.update((start + i, value)
                      for i, value in enumerate(bytes.fromhex(payload)))
    return result


def word(mem, address):
    return mem[address] | mem[address + 1] << 8


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", action="append", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    calls = []
    seen = set()
    stat_records = (0x47EB, 0x482B, 0x486B, 0x48AB, 0x48EB)
    for vector_path in args.vectors:
        for line in Path(vector_path).read_text().splitlines():
            row = json.loads(line)
            before = memory(row["entry"]["mem"])
            after = memory(row["exit"]["mem"])
            actor = row["entry"]["cpu"]["a"] & 0xFFFF
            if actor >= 10:
                raise ValueError(f"invalid native actor {actor}")
            actor_record = 0x34EB + actor * 0x100
            player_record = word(before, 0x3435 + actor * 2)
            team_record = word(before, actor_record + 0x70)
            team = 0 if team_record == 0x46EB else 1
            assignment_word = word(before, actor_record + 0x16)
            assignment = (assignment_word - 0x10000
                          if assignment_word & 0x8000 else assignment_word)
            stat_before = (word(before, stat_records[assignment] + 0x26)
                           if 0 <= assignment < len(stat_records) else 0)
            stat_after = (word(after, stat_records[assignment] + 0x26)
                          if 0 <= assignment < len(stat_records) else 0)
            inputs = [actor, team, assignment_word,
                      word(before, player_record + 0x14),
                      word(before, team_record + 0x54), stat_before,
                      word(before, 0x17DF), word(before, 0x09CA),
                      word(before, 0x0A08)]
            expected = [word(after, player_record + 0x14),
                        word(after, team_record + 0x54), stat_after,
                        word(after, 0x09CA), word(after, 0x0A08)]
            signature = tuple(inputs + expected)
            if signature in seen:
                continue
            seen.add(signature)
            calls.append({"call": len(calls) + 1, "input": inputs,
                          "expected": expected})
    Path(args.output).write_text(json.dumps({
        "routine": "$86:C493-$C4FD foul/stat bookkeeping",
        "provenance": "controlled WRAM at genuine nested native entry; no ROM/PC/stack/flags patching",
        "calls": calls,
    }, separators=(",", ":")) + "\n")
    print(f"normalized {len(calls)} foul-bookkeeping cases")


if __name__ == "__main__":
    main()
