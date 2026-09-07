"""Replay `$86:D7B8-$D85D` and its production graphics/substitution callers."""

import argparse
import hashlib
import json
import struct
import subprocess
from pathlib import Path


SCHEMA = "nba95-player-graphics-map-v2"
ROUTINE = "$86:D7B8-$D85D"
REFERENCE_ROM_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
CAPTURE_SHA256 = "a2b416b7dd264dfbbd84e77e78218137e8c5c2ccdfa656462a3ca19511d90562"
METADATA_SHA256 = "99f70492632996e40d689b0e127f1259a6b7c4980423b13f8dc91bbdea2aafb4"
COMPLETION_SHA256 = "343447fbd6936f0ed3240c23777f97fe9b00e95829e00bb276cc3d8240171ea2"
CAPTURE_IDENTITY = {
    "capture_time_manifest": False,
    "claim": (
        "Retained raw-vector, metadata, and completion-file integrity only. "
        "Capture-time ROM, emulator, and script hashes were not recorded and "
        "are not claimed."
    ),
    "reference_rom_sha256": REFERENCE_ROM_SHA256,
}
CASE_KEYS = {
    "call", "entry_frame", "caller_return", "entry_cpu", "exit_cpu",
    "lineup", "roster_address", "active_roster_address", "statistics_address",
}


def run(command: list[str], stdin: str | None = None) -> subprocess.CompletedProcess:
    result = subprocess.run(command, input=stdin, text=True, capture_output=True)
    if result.returncode:
        raise AssertionError(
            f"failed ({result.returncode}): {' '.join(command)}\n"
            f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}")
    return result


def parse_probe(stdout: str) -> tuple[list[int], list[int]]:
    lines = stdout.splitlines()
    active = next((line for line in lines if line.startswith("ACTIVE ")), None)
    stats = next((line for line in lines if line.startswith("STATS ")), None)
    if active is None or stats is None:
        raise AssertionError(f"incomplete probe output: {stdout!r}")
    return ([int(value, 16) for value in active.split()[1:]],
            [int(value, 16) for value in stats.split()[1:]])


def compare(case: dict, actual: tuple[list[int], list[int]]) -> None:
    if actual[0] != case["active_roster_address"]:
        raise AssertionError(f"call {case['call']}: active roster addresses differ")
    if actual[1] != case["statistics_address"]:
        raise AssertionError(f"call {case['call']}: statistics addresses differ")


def roster_resource(pack: Path) -> bytes:
    raw = pack.read_bytes()
    if len(raw) < 16 or raw[:8] != b"NBA95PAK":
        raise AssertionError("invalid asset pack")
    _, count = struct.unpack_from("<II", raw, 8)
    for index in range(count):
        entry = struct.unpack_from("<IIIIII", raw, 16 + index * 24)
        if entry[0] == 251:
            _, offset, size, width, height, flags = entry
            if offset + size > len(raw) or (width, height, flags) != (29, 12, 64):
                break
            data = raw[offset:offset + size]
            if data[:8] == b"NBPROST2" and len(data) == 24 + 29 * 12 * 64:
                return data
            break
    raise AssertionError("invalid player roster resource")


def pack_addresses(roster: bytes, team: int) -> list[int]:
    return [struct.unpack_from("<I", roster, 24 + (team * 12 + slot) * 64)[0]
            for slot in range(12)]


def validate_retained_capture(capture: dict, root: Path) -> None:
    for path_key, hash_key in (
            ("raw_vectors", "raw_vectors_sha256"),
            ("metadata", "metadata_sha256"),
            ("completion", "completion_sha256")):
        retained = root / capture[path_key]
        if retained.is_file() and \
                hashlib.sha256(retained.read_bytes()).hexdigest() != capture[hash_key]:
            raise AssertionError(f"retained capture changed: {retained}")


def validate_fixture(fixture: dict, pack: Path, root: Path) -> None:
    if set(fixture) != {
            "schema", "routine", "capture_identity", "capture", "natural_callers",
            "cases"} or fixture["schema"] != SCHEMA or \
            fixture["routine"] != ROUTINE or \
            fixture["capture_identity"] != CAPTURE_IDENTITY:
        raise AssertionError("fixture identity changed")
    expected_capture_paths = [
        ".analysis/captures/active-player-graphics-map-native-repeat-20260906",
        ".analysis/captures/active-player-graphics-map-native-repeat2-20260906",
    ]
    captures = fixture["capture"]
    if len(captures) != 2 or \
            fixture["natural_callers"] != ["$87:AF9E", "$86:D8A4"]:
        raise AssertionError("repeated-capture provenance changed")
    for capture, directory in zip(captures, expected_capture_paths):
        if capture != {
                "raw_vectors": f"{directory}/active_player_graphics_map.vectors.jsonl",
                "raw_vectors_sha256": CAPTURE_SHA256,
                "metadata": f"{directory}/active_player_graphics_map.meta.json",
                "metadata_sha256": METADATA_SHA256,
                "completion": f"{directory}/capture_complete.txt",
                "completion_sha256": COMPLETION_SHA256,
                "calls": 2}:
            raise AssertionError("retained capture manifest changed")
        validate_retained_capture(capture, root)
    cases = fixture["cases"]
    if len(cases) != 2 or [case.get("caller_return") for case in cases] != [
            "87afa1", "86d8a7"]:
        raise AssertionError("native caller order changed")
    for index, case in enumerate(cases, 1):
        if set(case) != CASE_KEYS or case["call"] != index or \
                case["entry_frame"] not in (2929, 2931):
            raise AssertionError("native case shape changed")
        entry, exit_state = case["entry_cpu"], case["exit_cpu"]
        if entry.get("pc") != 0xD7B8 or exit_state.get("pc") != 0xD85D or \
                entry.get("d") != 0 or entry.get("dbr") != 0x7E or \
                exit_state.get("dbr") != 0x7E or entry.get("sp") != exit_state.get("sp"):
            raise AssertionError("native entry/exit CPU contract changed")
        if len(case["lineup"]) != 2 or any(len(side) != 5 for side in case["lineup"]) or \
                len(case["roster_address"]) != 2 or \
                any(len(side) != 12 for side in case["roster_address"]) or \
                len(case["active_roster_address"]) != 10 or \
                len(case["statistics_address"]) != 10:
            raise AssertionError("native table dimensions changed")
        expected_active = []
        expected_stats = []
        for side in range(2):
            for roster in case["lineup"][side]:
                if roster >= 12:
                    raise AssertionError("native roster index is outside its table")
                expected_active.append(case["roster_address"][side][roster])
                expected_stats.append(0x40EB + 0x40 * (roster + side * 12))
        if case["active_roster_address"] != expected_active or \
                case["statistics_address"] != expected_stats:
            raise AssertionError("fixture output is inconsistent with native mapping")
    if cases[0]["lineup"] != cases[1]["lineup"] or \
            cases[0]["roster_address"] != cases[1]["roster_address"] or \
            cases[0]["active_roster_address"] != cases[1]["active_roster_address"] or \
            cases[0]["statistics_address"] != cases[1]["statistics_address"]:
        raise AssertionError("the two natural callers disagree")
    roster = roster_resource(pack)
    if cases[0]["roster_address"] != [pack_addresses(roster, 18),
                                      pack_addresses(roster, 28)]:
        raise AssertionError("native source tables do not match pack teams 18/28")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--pack", type=Path, required=True)
    args = parser.parse_args()
    fixture = json.loads(args.vectors.read_text())
    validate_fixture(fixture, args.pack, args.vectors.resolve().parents[2])
    run([str(args.probe), "selftest"])
    for case in fixture["cases"]:
        values = [*case["lineup"][0], *case["lineup"][1],
                  *case["roster_address"][0], *case["roster_address"][1]]
        result = run([str(args.probe), "replay"],
                     " ".join(f"{value:x}" for value in values) + "\n")
        compare(case, parse_probe(result.stdout))
    mutated = json.loads(json.dumps(fixture["cases"][0]))
    mutated["active_roster_address"][0] ^= 1
    try:
        values = [*mutated["lineup"][0], *mutated["lineup"][1],
                  *mutated["roster_address"][0], *mutated["roster_address"][1]]
        result = run([str(args.probe), "replay"],
                     " ".join(f"{value:x}" for value in values) + "\n")
        compare(mutated, parse_probe(result.stdout))
    except AssertionError:
        pass
    else:
        raise AssertionError("mutated native expectation was not rejected")
    caller = run([str(args.probe), "caller", str(args.pack)])
    if "CALLER init=2 substitution=" not in caller.stdout:
        raise AssertionError("production init/substitution caller did not run")
    print("player graphics map: 2 native calls, asset-free cases, pack tables, "
          "and init/substitution callers passed")


if __name__ == "__main__":
    main()
