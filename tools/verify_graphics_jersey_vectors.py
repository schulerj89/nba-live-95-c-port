"""Replay native jersey cache/appender witnesses through production C."""
from __future__ import annotations

import argparse
import copy
import hashlib
import json
import subprocess
from pathlib import Path

from normalize_graphics_jersey_vectors import validate_boundaries

ROM_SHA = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
MESEN_SHA = "d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b"
LISTING_SHA = "add14f13c6ac7787369299f6a055b47cad471839e6be262f7e6d7fb6d2cb9f03"
RUNS_SHA = "cb68f23485e15e5e9e17c92b0f3e56526888bb3ae7d90c12a73ff10ab12fc59f"
STARTS_SHA = "445402a4c117bb514e360bd5588501c46147b73739f4b796659af5ea3c36b3ec"
FNV_OFFSET, FNV_PRIME = 14695981039346656037, 1099511628211
WRAM_SIZE = 0x20000
EXPECTED_OPERATIONS = {
    "natural_direction_0_miss": [0x5A, 2, 0, 0x01E0, 0x6000, 0x080, 0xFFFF],
    "natural_direction_0_hit": [0x5A, 2, 0, 0x01E0, 0x6000, 0x0F0, 0x86F0],
    "natural_direction_2_miss": [0x5A, 10, 2, 0x01E0, 0x6000, 0x040, 0xFFFF],
    "natural_direction_2_hit": [0x5A, 10, 2, 0x01E0, 0x6000, 0x0F0, 0x8710],
    "natural_direction_3_miss": [0x5A, 16, 3, 0x01E0, 0x6000, 0x1D8, 0xFFFF],
    "natural_direction_3_hit": [0x5A, 16, 3, 0x01E0, 0x6000, 0x0F0, 0x8690],
    "natural_direction_4_miss": [0x5A, 4, 4, 0x01E0, 0x6000, 0x008, 0xFFFF],
    "natural_direction_4_hit": [0x5A, 4, 4, 0x01E0, 0x6000, 0x0F0, 0x86D0],
    "natural_direction_6_miss": [0x5A, 0, 6, 0x01E0, 0x6000, 0x060, 0xFFFF],
    "natural_direction_6_hit": [0x5A, 0, 6, 0x01E0, 0x6000, 0x0F0, 0x8730],
    "natural_direction_7_miss": [0x5A, 6, 7, 0x01E0, 0x6000, 0x0B0, 0xFFFF],
    "natural_direction_7_hit": [0x5A, 6, 7, 0x01E0, 0x6000, 0x0F0, 0x86B0],
    "natural_tail_rollover": [0x5A, 14, 4, 0x01E0, 0x6000, 0x1F8, 0x86F0],
    "controlled_arithmetic_wrap": [0x5A, 18, 7, 0xFFFF, 0xFFF0, 0x1F8, 0],
    "controlled_hit_high_index": [0x5A, 0, 0, 0x8000, 0x1234, 0x1F8, 0x86F0],
    "controlled_adc_carry": [0x5A, 0, 0, 0x1000, 0x1234, 0, 0],
}


def parse_row(fields: list[str]) -> dict:
    if len(fields) != 28:
        raise AssertionError(f"probe row has {len(fields)} fields")
    return {
        "ok": int(fields[0]),
        "result": [int(value, 16) for value in fields[1:4]] + [int(fields[4])],
        "full_hash": fields[5], "changed_hash": fields[6],
        "changed_count": int(fields[7]),
        "words": [int(value, 16) for value in fields[8:14]],
        "record": [int(value, 16) for value in fields[14:22]],
        "sentinels": [int(value, 16) for value in fields[22:28]],
    }


def run_probe(probe: Path, pack: Path, operations: list[list[int]]) -> list[dict]:
    payload = "".join(" ".join(f"{value:x}" for value in row) + "\n"
                      for row in operations)
    result = subprocess.run([str(probe.resolve()), str(pack.resolve())],
                            input=payload, text=True, capture_output=True,
                            check=True)
    rows = [line for line in result.stdout.splitlines()
            if line.strip() and line.split()[0] in ("0", "1")]
    return [parse_row(line.split()) for line in rows]


def set_word(memory: bytearray, address: int, value: int) -> None:
    memory[address] = value & 0xFF
    memory[(address + 1) & 0xFFFF] = value >> 8


def fnv(data) -> str:
    digest = FNV_OFFSET
    for value in data:
        digest = ((digest ^ value) * FNV_PRIME) & 0xFFFFFFFFFFFFFFFF
    return f"{digest:016x}"


def seeded_memory(operation: list[int]) -> bytearray:
    _, fill, be, c2, extent, vram, tail, cache = operation
    memory = bytearray([fill]) * WRAM_SIZE
    set_word(memory, 0x00BE, be)
    set_word(memory, 0x00C2, c2)
    set_word(memory, 0x05EF, extent)
    set_word(memory, 0x05EB, vram)
    set_word(memory, 0x0037, tail)
    if be < 20 and not be & 1:
        set_word(memory, 0x8E10 + be, cache)
    return memory


def changed_address_hash(before: bytearray, after: bytearray) -> tuple[str, int]:
    encoded = bytearray()
    count = 0
    for address, (left, right) in enumerate(zip(before, after)):
        if left == right:
            continue
        count += 1
        encoded.extend(address.to_bytes(3, "little"))
    return fnv(encoded), count


def initial_state(operation: list[int]) -> dict:
    _, _, be, _, extent, vram, tail, cache = operation
    memory = seeded_memory(operation)
    cache_word = cache if be < 20 and not be & 1 else 0xA5A5
    record = 0x0100 + tail
    return {
        "ok": 0, "result": [0xA5A5, 0xA5A5, 0xA5A5, 1],
        "full_hash": fnv(memory), "changed_hash": f"{FNV_OFFSET:016x}",
        "changed_count": 0,
        "words": [memory[0] | memory[1] << 8,
                  memory[4] | memory[5] << 8, tail, vram, extent, cache_word],
        "record": [memory[(record + index) & 0xFFFF] for index in range(8)],
        "sentinels": [memory[address] for address in
                      (0x35, 0x39, 0x12C, 0x566, 0x8E24, 0x1FFFF)],
    }


def invalid_tail_cache_hit_state(operation: list[int]) -> dict:
    before = seeded_memory(operation)
    memory = bytearray(before)
    _, _, be, _, extent, vram, tail, cache = operation
    destination_index = ((be >> 1) + extent) & 0xFFFF
    set_word(memory, 0x0004, destination_index)
    changed_hash, changed_count = changed_address_hash(before, memory)
    record = 0x0100 + tail
    return {
        "ok": 1, "result": [destination_index, 0, 0, 0],
        "full_hash": fnv(memory), "changed_hash": changed_hash,
        "changed_count": changed_count,
        "words": [memory[0] | memory[1] << 8,
                  destination_index, tail, vram, extent, cache],
        "record": [memory[(record + index) & 0xFFFF] for index in range(8)],
        "sentinels": [memory[address] for address in
                      (0x35, 0x39, 0x12C, 0x566, 0x8E24, 0x1FFFF)],
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--vectors", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--pack", type=Path, required=True)
    parser.add_argument("--legacy-pack", type=Path)
    args = parser.parse_args()

    fixture = json.loads(args.vectors.read_text())
    sources = fixture.get("sources", {})
    runs_sha = hashlib.sha256(json.dumps(
        sources.get("runs"), sort_keys=True, separators=(",", ":")).encode()).hexdigest()
    starts_sha = hashlib.sha256(json.dumps(
        fixture.get("owned_start_addresses"), separators=(",", ":")).encode()).hexdigest()
    calls = fixture.get("calls", [])
    if (fixture.get("schema") != 1 or fixture.get("routine") != "80ad2b-80ad88" or
            fixture.get("projection") !=
            "ordered native functional WRAM writes over byte 0x5a" or
            sources.get("rom_sha256") != ROM_SHA or
            sources.get("mesen_sha256") != MESEN_SHA or
            sources.get("listing_sha256") != LISTING_SHA or
            runs_sha != RUNS_SHA or starts_sha != STARTS_SHA or
            fixture.get("coverage") != {
                "owned_starts": 49, "natural_calls": 1288,
                "natural_hits": 1244, "natural_misses": 44,
                "natural_valid_sources": 6, "natural_rollovers": 1,
                "interrupt_stack_sequences": 2} or
            len(calls) != len(EXPECTED_OPERATIONS) or
            {call.get("name") for call in calls} != set(EXPECTED_OPERATIONS)):
        raise ValueError("jersey fixture provenance or coverage changed")

    source_by_direction = {0: 0x86F0, 2: 0x8710, 3: 0x8690,
                           4: 0x86D0, 6: 0x8730, 7: 0x86B0}
    for call in calls:
        operation = EXPECTED_OPERATIONS[call["name"]]
        entry = call.get("entry", {})
        expected = call.get("expected", {})
        if (call.get("operation") != operation or
                entry.get("be") != operation[1] or
                entry.get("c2") != operation[2] or
                entry.get("extent") != operation[3] or
                entry.get("vram_base") != operation[4] or
                entry.get("tail") != operation[5] or
                entry.get("cache") != operation[6] or
                entry.get("source") != source_by_direction[operation[2]] or
                len(expected.get("result", [])) != 4 or
                len(expected.get("words", [])) != 6 or
                len(expected.get("record", [])) != 8 or
                len(expected.get("sentinels", [])) != 6 or
                expected.get("native_write_events") not in (2, 20) or
                expected.get("native_unique_written_bytes") not in (2, 16)):
            raise ValueError(f"native case matrix changed: {call.get('name')}")

    boundaries = fixture.get("natural_boundary_contract", [])
    validate_boundaries(boundaries, False)
    mutated = copy.deepcopy(boundaries)
    mutated[0], mutated[1] = mutated[1], mutated[0]
    try:
        validate_boundaries(mutated, False)
    except ValueError:
        pass
    else:
        raise AssertionError("boundary-order mutation was accepted")

    native_operations = [[0] + call["operation"] for call in calls]
    actual = run_probe(args.probe, args.pack, native_operations)
    if len(actual) != len(calls):
        raise AssertionError("native replay row count")
    failures = []
    for call, row in zip(calls, actual):
        expected = {key: value for key, value in call["expected"].items()
                    if not key.startswith("native_")}
        if row != expected:
            failures.append((call["name"], expected, row))

    # Host safety is separate from captured reachability. Every rejected case
    # is compared with its own seeded state, including invalid actor offsets.
    safety = [
        [0, 0x5A, 1, 0, 0x1E0, 0x6000, 0, 0xFFFF],
        [0, 0x5A, 20, 0, 0x1E0, 0x6000, 0, 0xFFFF],
        [0, 0x5A, 0, 1, 0x1E0, 0x6000, 0, 0xFFFF],
        [0, 0x5A, 0, 5, 0x1E0, 0x6000, 0, 0xFFFF],
        [0, 0x5A, 0, 8, 0x1E0, 0x6000, 0, 0xFFFF],
        [0, 0x5A, 0, 0, 0x1E0, 0x6000, 1, 0xFFFF],
        [0, 0x5A, 0, 0, 0x1E0, 0x6000, 0x1F9, 0xFFFF],
        [1, 0x5A, 0, 0, 0x1E0, 0x6000, 0, 0xFFFF],
        [2, 0x5A, 0, 0, 0x1E0, 0x6000, 0, 0xFFFF],
        [3, 0x5A, 0, 0, 0x1E0, 0x6000, 0, 0xFFFF],
        [4, 0x5A, 0, 0, 0x1E0, 0x6000, 0, 0xFFFF],
        [5, 0x5A, 0, 0, 0x1E0, 0x6000, 0, 0xFFFF],
        [6, 0x5A, 0, 0, 0x1E0, 0x6000, 0, 0xFFFF],
        [7, 0x5A, 0, 0, 0x1E0, 0x6000, 0, 0xFFFF],
    ]
    rejected = run_probe(args.probe, args.pack, safety)
    if len(rejected) != len(safety):
        raise AssertionError("safety replay row count")
    for index, (operation, row) in enumerate(zip(safety, rejected)):
        expected = initial_state(operation)
        if row != expected:
            failures.append((f"safety_{index}", expected, row))

    # A hit never reads or advances `$0037`; its only native WRAM store is the
    # destination index in DP `$04`, even if the dormant tail is misaligned.
    hit_with_invalid_tail = [0, 0x5A, 0, 0, 0x01E0,
                             0x6000, 1, 0x86F0]
    hit_rows = run_probe(args.probe, args.pack, [hit_with_invalid_tail])
    if len(hit_rows) != 1:
        raise AssertionError("cache-hit source case row count")
    expected_hit = invalid_tail_cache_hit_state(hit_with_invalid_tail)
    if hit_rows[0] != expected_hit:
        failures.append(("cache_hit_does_not_read_tail", expected_hit, hit_rows[0]))

    if args.legacy_pack:
        legacy_operation = [0] + calls[0]["operation"]
        legacy_rows = run_probe(args.probe, args.legacy_pack, [legacy_operation])
        if len(legacy_rows) != 1:
            raise AssertionError("legacy-pack row count")
        expected_legacy = initial_state(legacy_operation)
        if legacy_rows[0] != expected_legacy:
            failures.append(("legacy_pack_without_a99e", expected_legacy,
                             legacy_rows[0]))

    print(f"[GRAPHICS JERSEY] {'PASS' if not failures else 'FAIL'}: "
          f"native_calls={len(calls)} safety_cases={len(safety) + 1} "
          f"legacy_pack={1 if args.legacy_pack else 0} "
          f"mismatches={len(failures)}")
    for failure in failures:
        print(failure)
    if failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
