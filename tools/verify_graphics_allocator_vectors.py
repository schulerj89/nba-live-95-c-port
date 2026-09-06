"""Replay native graphics allocator witnesses through production C."""
from __future__ import annotations

import argparse
import copy
import hashlib
import json
import subprocess
from pathlib import Path

from normalize_graphics_allocator_vectors import validate_boundaries

ROM_SHA = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
MESEN_SHA = "d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b"
INVENTORY_SHA = "a286699964a69801b08cc5e54fc54a6902fa9cd64337b8830dcd44b957003a51"
RUNS_SHA = "7d78c468e738fa9e97c20ddb5ff20cd8d46857ab3578e5935194f9253530bbe9"
EXPECTED_NAMES = {
    "natural_boot_clamp_zero", "natural_boot_clamp_01a0",
    "natural_court_caller", "controlled_y01bf", "controlled_y01c0",
    "controlled_y01e1", "controlled_y0201", "controlled_yffe1",
    "natural_ac89_empty", "natural_ac89_nonempty", "natural_ac0d_first",
}
EXPECTED_OPERATIONS = {
    "natural_boot_clamp_zero": [0, 0x5A, 0, 0, 0, 0, 0],
    "natural_boot_clamp_01a0": [0, 0x5A, 0x6000, 0, 0x01A0, 0, 0],
    "natural_court_caller": [0, 0x5A, 0x6000, 0, 0x01E0, 0, 0],
    "controlled_y01bf": [0, 0x5A, 0x1234, 0x5678, 0x01BF, 0, 0],
    "controlled_y01c0": [0, 0x5A, 0x6000, 0, 0x01C0, 0, 0],
    "controlled_y01e1": [0, 0x5A, 0x6000, 0, 0x01E1, 0, 0],
    "controlled_y0201": [0, 0x5A, 0x6000, 0, 0x0201, 0, 0],
    "controlled_yffe1": [0, 0x5A, 0xABCD, 0x1357, 0xFFE1, 0, 0],
    "natural_ac89_empty": [2, 0x5A, 0x2044, 0x2000, 0x2220, 0x2000, 0x2220],
    "natural_ac89_nonempty": [2, 0x5A, 0x2220, 0x2420, 0x2000, 0x2220, 0x2000],
    "natural_ac0d_first": [1, 0x5A, 0, 0, 0, 0, 0],
}


def parse_row(fields: list[str]) -> dict:
    if len(fields) != 24:
        raise AssertionError(f"probe row has {len(fields)} fields")
    return {
        "ok": int(fields[0]), "full_hash": fields[1],
        "changed_hash": fields[2], "changed_count": int(fields[3]),
        "fill_hash": fields[4], "cache_hash": fields[5],
        "table_hash": fields[6],
        "words": [int(value, 16) for value in fields[7:18]],
        "bytes": [int(value, 16) for value in fields[18:24]],
    }


def projected_expected(call: dict) -> dict:
    return {key: value for key, value in call["expected"].items()
            if not key.startswith("native_")}


def run_probe(probe: Path, operations: list[list[int]]) -> list[dict]:
    payload = "".join(" ".join(f"{value:x}" for value in row) + "\n"
                      for row in operations)
    result = subprocess.run([str(probe.resolve())], input=payload, text=True,
                            capture_output=True, check=True)
    return [parse_row(line.split()) for line in result.stdout.splitlines()
            if line.strip()]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--vectors", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    args = parser.parse_args()
    fixture = json.loads(args.vectors.read_text())
    calls = fixture["calls"]
    sources = fixture.get("sources", {})
    runs_hash = hashlib.sha256(json.dumps(
        sources.get("runs"), sort_keys=True, separators=(",", ":")).encode()).hexdigest()
    if (fixture.get("schema") != 1 or fixture.get("routine") != "80ab7e-80ac0c" or
            fixture.get("children") != ["80ac0d-80ac1a", "80ac89-80acc1"] or
            sources.get("rom_sha256") != ROM_SHA or
            sources.get("mesen_sha256") != MESEN_SHA or
            sources.get("inventory_sha256") != INVENTORY_SHA or runs_hash != RUNS_SHA or
            fixture.get("coverage") != {"parent_starts": 61, "clear_starts": 7,
                                        "swap_starts": 24, "natural_ac89_calls": 49} or
            {call["name"] for call in calls} != EXPECTED_NAMES or len(calls) != 11):
        raise ValueError("graphics allocator fixture provenance or matrix changed")
    for call in calls:
        if (call.get("operation") != EXPECTED_OPERATIONS[call["name"]] or
                len(call["expected"].get("words", [])) != 11 or
                len(call["expected"].get("bytes", [])) != 6):
            raise ValueError(f"native operation matrix changed: {call['name']}")
        if call["entry"]["pc"] not in ("80ab7e", "80ac0d", "80ac89"):
            raise ValueError(f"invalid native boundary: {call['name']}")
        if call["name"].startswith("controlled_") and \
                call.get("controlled_registers") != ["a", "x", "y"]:
            raise ValueError(f"controlled seed provenance changed: {call['name']}")
        if call["expected"]["native_write_events"] < \
                call["expected"]["native_unique_written_bytes"]:
            raise ValueError(f"invalid native write footprint: {call['name']}")
        if call["kind"] == "parent" and call["operation"][2:5] != \
                [call["entry"][key] for key in ("a", "x", "y")]:
            raise ValueError(f"parent entry projection changed: {call['name']}")
        if call["kind"] == "ac89":
            native_inputs = [call["entry"][key] for key in
                             ("05f3", "05f5", "05df", "05e1", "05e3")]
            if call["operation"][2:] != native_inputs:
                raise ValueError(f"AC89 entry projection changed: {call['name']}")

    boundary_contract = fixture.get("natural_boundary_contract", [])
    validate_boundaries(boundary_contract, False)
    mutated = copy.deepcopy(boundary_contract)
    parent_entry = next(index for index, row in enumerate(mutated)
                        if row["tag"] == "parent.entry")
    parent_exit = next(index for index, row in enumerate(mutated)
                       if row["tag"] == "parent.exit")
    mutated[parent_entry], mutated[parent_exit] = mutated[parent_exit], mutated[parent_entry]
    try:
        validate_boundaries(mutated, False)
    except ValueError:
        pass
    else:
        raise AssertionError("boundary-order mutation was accepted")

    actual = run_probe(args.probe, [call["operation"] for call in calls])
    if len(actual) != len(calls):
        raise AssertionError("native replay row count")
    failures = []
    for call, row in zip(calls, actual):
        expected = projected_expected(call)
        if row != expected:
            failures.append((call["name"], expected, row))

    # Source-derived host safety cases are separate from captured reachability:
    # aligned near-wrap must terminate, an aligned alias must re-read `$05F5`
    # and `$05F3`, and invalid views/misaligned live fills must not mutate WRAM.
    safety_operations = [
        [2, 0x5A, 0xFFFC, 0xFFFF, 0x2420, 0x2000, 0x2220],
        [2, 0x5A, 0x05F4, 0x0600, 0x2420, 0x2000, 0x2220],
        [3, 0x5A, 0xFFFD, 0xFFFF, 0x2420, 0x2000, 0x2220],
        [4, 0x5A, 0x6000, 0, 0x01E0, 0, 0],
        [5, 0x5A, 0x6000, 0, 0x01E0, 0, 0],
    ]
    safety = run_probe(args.probe, safety_operations)
    if len(safety) != len(safety_operations):
        raise AssertionError("source safety row count")
    safety_contracts = [
        {"ok": 1, "changed_count": 11,
         "words": [0xFFFC, 0x2220, 0x2000, 0x5A5A, 0x5A5A, 0x5A5A,
                   0x5A5A, 0x5A5A, 0xFFFC, 0x2420, 0x5A5A],
         "bytes": [1, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A]},
        {"ok": 1, "changed_count": 128,
         "words": [0x00F4, 0x2220, 0x2000, 0x5A5A, 0x5A5A, 0x5A5A,
                   0x5A5A, 0x5A5A, 0x00F4, 0x2420, 0x5AE1],
         "bytes": [1, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A]},
        {"ok": 0, "changed_count": 0,
         "words": [0x2420, 0x2000, 0x2220, 0x5A5A, 0x5A5A, 0x5A5A,
                   0x5A5A, 0x5A5A, 0xFFFD, 0xFFFF, 0x5A5A],
         "bytes": [0x5A] * 6},
        {"ok": 0, "changed_count": 0, "words": [0x5A5A] * 11,
         "bytes": [0x5A] * 6},
        {"ok": 0, "changed_count": 0, "words": [0x5A5A] * 11,
         "bytes": [0x5A] * 6},
    ]
    for index, (row, expected) in enumerate(zip(safety, safety_contracts)):
        actual_contract = {key: row[key] for key in expected}
        if actual_contract != expected:
            failures.append((f"source_safety_{index}", expected, actual_contract))

    print(f"[GRAPHICS ALLOCATOR] {'PASS' if not failures else 'FAIL'}: "
          f"native_calls={len(calls)} safety_cases={len(safety)} mismatches={len(failures)}")
    for failure in failures:
        print(failure)
    if failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
