"""Replay `$87:AFA2-$B058` and its production Tipoff caller.

The appearance words remain in their existing native fixture. The jersey
fixture retains only captured inputs, byte count, exit PC, and output digest;
this verifier hashes bytes generated from the local asset pack.
"""

import argparse
import hashlib
import json
import struct
import subprocess
from pathlib import Path


def output_fields(stdout: str) -> dict[str, str]:
    fields = {}
    for line in stdout.splitlines():
        for name in ("APPEARANCE", "JERSEYS", "CACHE", "UPLOAD"):
            prefix = name + " "
            if line.startswith(prefix):
                if name in fields:
                    raise AssertionError(f"duplicate {name} output")
                fields[name] = line[len(prefix):].strip()
    if set(fields) != {"APPEARANCE", "JERSEYS", "CACHE", "UPLOAD"}:
        raise AssertionError(f"incomplete probe output: {sorted(fields)}")
    return fields


def active_jerseys_from_pack(pack_path: Path,
                             team_roster_pairs: list[int]) -> list[int]:
    raw = pack_path.read_bytes()
    if len(raw) < 16 or raw[:8] != b"NBA95PAK":
        raise AssertionError("invalid asset-pack header")
    _, count = struct.unpack_from("<II", raw, 8)
    roster_entry = None
    for index in range(count):
        entry_offset = 16 + index * 24
        if entry_offset + 24 > len(raw):
            raise AssertionError("truncated asset-pack directory")
        entry = struct.unpack_from("<IIIIII", raw, entry_offset)
        if entry[0] == 251:
            roster_entry = entry
            break
    if roster_entry is None:
        raise AssertionError("asset pack is missing player rosters")
    _, offset, size, width, height, flags = roster_entry
    if offset + size > len(raw) or (width, height, flags) != (29, 12, 64):
        raise AssertionError("invalid packed player-roster resource")
    roster = raw[offset:offset + size]
    if len(roster) < 24 or roster[:8] != b"NBPROST2" or \
            struct.unpack_from("<IIII", roster, 8) != (2, 29, 12, 64) or \
            len(roster) != 24 + 29 * 12 * 64:
        raise AssertionError("invalid packed player-roster schema")
    result = []
    for team, slot in zip(team_roster_pairs[::2], team_roster_pairs[1::2]):
        if team >= 29 or slot >= 12:
            raise AssertionError("native appearance input is outside roster")
        result.append(roster[24 + (team * 12 + slot) * 64 + 4])
    return result


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--appearance-vectors", type=Path, required=True)
    parser.add_argument("--jersey-vectors", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--pack", type=Path, required=True)
    args = parser.parse_args()

    appearance_rows = [row for row in
                       json.loads(args.appearance_vectors.read_text())
                       if row.get("entry_pc") == "87afa2"]
    if len(appearance_rows) != 2 or any(
            row.get("input") != appearance_rows[0]["input"] or
            row.get("expected") != appearance_rows[0]["expected"]
            for row in appearance_rows[1:]):
        raise AssertionError("expected two identical native AFA2 replays")
    appearance = appearance_rows[0]
    if appearance["input"][0] != 1 or len(appearance["input"]) != 21 or \
            len(appearance["expected"]) != 51:
        raise AssertionError("native AFA2 fixture shape changed")
    pairs = appearance["input"][1:]
    stdin = " ".join(f"{value:x}" for value in pairs) + "\n"

    jersey_rows = json.loads(args.jersey_vectors.read_text())
    if len(jersey_rows) != 1 or jersey_rows[0].get("exit_pc") != "87b354":
        raise AssertionError("native B059 child fixture shape changed")
    jersey = jersey_rows[0]
    if set(jersey) != {"input", "exit_pc", "expected_sha256", "byte_count"} or \
            len(jersey["input"]) != 10 or jersey["byte_count"] != 0x780 or \
            len(jersey["expected_sha256"]) != 64:
        raise AssertionError("native jersey fixture payload changed")
    if jersey["input"] != active_jerseys_from_pack(args.pack, pairs):
        raise AssertionError(
            "native jersey capture inputs do not match active pack records")

    expected_appearance = " ".join(
        [f"{appearance['expected'][0]:06x}"] +
        [f"{value:04x}" for value in appearance["expected"][1:]])
    expected = {
        "APPEARANCE": expected_appearance,
        "CACHE": "ff" * 0x14,
        "UPLOAD": "0c8080",
    }
    modes = ("--direct", "--caller")
    for mode in modes:
        result = subprocess.run(
            [str(args.probe.resolve()), str(args.pack.resolve()), mode],
            input=stdin, text=True, capture_output=True, check=True)
        actual = output_fields(result.stdout)
        for field in expected:
            if actual[field] != expected[field]:
                raise AssertionError(
                    f"{mode} {field} mismatch: expected "
                    f"{expected[field][:96]}, got {actual[field][:96]}")
        try:
            jersey_bytes = bytes.fromhex(actual["JERSEYS"])
        except ValueError as error:
            raise AssertionError(f"{mode} emitted invalid jersey hex") from error
        if len(jersey_bytes) != jersey["byte_count"]:
            raise AssertionError(
                f"{mode} emitted {len(jersey_bytes)} jersey bytes, expected "
                f"{jersey['byte_count']}")
        actual_sha256 = hashlib.sha256(jersey_bytes).hexdigest()
        if actual_sha256 != jersey["expected_sha256"]:
            raise AssertionError(
                f"{mode} jersey SHA-256 mismatch: expected "
                f"{jersey['expected_sha256']}, got {actual_sha256}")
    print("[PLAYER APPEARANCE PUBLICATION] PASS: two native parent entries, "
          "1,920 child bytes, cache/upload words, atomic rejection and "
          "production Tipoff caller; jersey sha256=" +
          jersey["expected_sha256"])


if __name__ == "__main__":
    main()
