"""Normalize genuine `$86:D7B8-$D85D` Mesen calls into a compact fixture."""

import argparse
import hashlib
import json
from pathlib import Path


REFERENCE_ROM_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def exact_slice(data: bytes, base: int, address: int, size: int) -> bytes:
    offset = address - base
    if offset < 0 or offset + size > len(data):
        raise ValueError(f"short capture window for ${address:04X}+{size}")
    return data[offset:offset + size]


def words(data: bytes, base: int, address: int, count: int) -> list[int]:
    raw = exact_slice(data, base, address, count * 2)
    return [int.from_bytes(raw[i * 2:i * 2 + 2], "little")
            for i in range(count)]


def longs(data: bytes, base: int, address: int, count: int) -> list[int]:
    raw = exact_slice(data, base, address, count * 4)
    return [int.from_bytes(raw[i * 4:i * 4 + 4], "little")
            for i in range(count)]


def normalize(row: dict) -> dict:
    if row.get("entry_pc") != "86d7b8" or row.get("exit_pc") != "86d85d":
        raise ValueError("unexpected D7B8 entry/exit")
    entry = row["entry"]
    exit_state = row["exit"]
    source = bytes.fromhex(entry["mem"]["3435"])
    result = bytes.fromhex(exit_state["mem"]["3435"])
    stack = bytes.fromhex(entry["mem"]["1f00"])
    sp = entry["cpu"]["sp"]
    stack_offset = sp - 0x1F00 + 1
    caller_return = int.from_bytes(
        exact_slice(stack, 0x1F00, 0x1F00 + stack_offset, 3), "little")
    return {
        "call": row["call"],
        "entry_frame": row["entry_frame"],
        "caller_return": f"{caller_return:06x}",
        "entry_cpu": entry["cpu"],
        "exit_cpu": exit_state["cpu"],
        "lineup": [
            words(bytes.fromhex(entry["mem"]["46f9"]), 0x46F9, 0x46F9, 5),
            words(bytes.fromhex(entry["mem"]["4779"]), 0x4779, 0x4779, 5),
        ],
        "roster_address": [
            longs(source, 0x3435, 0x3471, 12),
            longs(source, 0x3435, 0x34A1, 12),
        ],
        "active_roster_address": longs(result, 0x3435, 0x3449, 10),
        "statistics_address": words(result, 0x3435, 0x3435, 10),
    }


def signature(case: dict) -> tuple:
    return (case["caller_return"], case["lineup"], case["roster_address"],
            case["active_roster_address"], case["statistics_address"])


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--capture", type=Path, action="append", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    captures = []
    normalized_runs = []
    for path in args.capture:
        metadata = path.with_name("active_player_graphics_map.meta.json")
        completion = path.with_name("capture_complete.txt")
        if not metadata.is_file() or not completion.is_file():
            raise ValueError(f"{path}: retained metadata/completion evidence missing")
        rows = [json.loads(line) for line in path.read_text().splitlines()]
        if len(rows) != 2:
            raise ValueError(f"{path}: expected exactly two natural calls")
        cases = [normalize(row) for row in rows]
        if [case["caller_return"] for case in cases] != ["87afa1", "86d8a7"]:
            raise ValueError(f"{path}: unexpected production caller order")
        captures.append({
            "raw_vectors": path.as_posix(),
            "raw_vectors_sha256": sha256(path),
            "metadata": metadata.as_posix(),
            "metadata_sha256": sha256(metadata),
            "completion": completion.as_posix(),
            "completion_sha256": sha256(completion),
            "calls": len(rows),
        })
        normalized_runs.append(cases)
    expected = [signature(case) for case in normalized_runs[0]]
    for cases in normalized_runs[1:]:
        if [signature(case) for case in cases] != expected:
            raise ValueError("repeated native capture changed represented behavior")
    fixture = {
        "schema": "nba95-player-graphics-map-v2",
        "routine": "$86:D7B8-$D85D",
        "capture_identity": {
            "capture_time_manifest": False,
            "claim": (
                "Retained raw-vector, metadata, and completion-file integrity "
                "only. Capture-time ROM, emulator, and script hashes were not "
                "recorded and are not claimed."
            ),
            "reference_rom_sha256": REFERENCE_ROM_SHA256,
        },
        "capture": captures,
        "natural_callers": ["$87:AF9E", "$86:D8A4"],
        "cases": normalized_runs[0],
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(fixture, indent=2) + "\n")


if __name__ == "__main__":
    main()
