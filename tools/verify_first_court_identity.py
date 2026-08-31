"""Compare production initialization with a native team/actor identity witness.

This bounded projection does not claim scheduler, controls, full-state or
trajectory parity. The native journey is uninterrupted and controller-only;
the C side deliberately calls its production initializer directly.
"""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

ROM_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
KEYS = {"context_teams": 2, "anchor_x": 2, "actor_groups": 10,
        "active_roster": 10, "assignment_roles": 10, "appearance_variants": 10,
        "height_variants": 10, "active_stamina_ratings": 10}


def strict_json(text):
    def pairs(items):
        result = {}
        for key, value in items:
            if key in result:
                raise ValueError(f"duplicate JSON key: {key}")
            result[key] = value
        return result
    return json.loads(text, object_pairs_hook=pairs)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def projection(snapshot, rom):
    if len(snapshot) != 0x20000:
        raise ValueError("native snapshot must contain all128KiB")
    def word(address):
        return int.from_bytes(snapshot[address:address + 2], "little")
    ratings = []
    for i in range(10):
        # Original production pointer setup, not a C-derived team/roster lookup.
        pointer, bank = word(0x3449 + i * 4), word(0x344B + i * 4) & 255
        offset = (bank & 127) * 0x8000 + (pointer & 0x7FFF)
        if pointer < 0x8000 or offset + 0x35 >= len(rom):
            raise ValueError("native active roster pointer is outside verified ROM")
        ratings.append(rom[offset + 0x35])
    return dict(context_teams=[word(0x46EB), word(0x476B)],
                anchor_x=[word(0x46F5), word(0x4775)],
                actor_groups=[word(0x34EB + i * 256 + 0x6E) for i in range(10)],
                active_roster=[word((0x46F9 if i < 5 else 0x4779) + (i % 5) * 2) for i in range(10)],
                assignment_roles=[word(0x34EB + i * 256 + 0x92) for i in range(10)],
                appearance_variants=[word(0x34EB + i * 256 + 0x6C) for i in range(10)],
                height_variants=[word(0x34EB + i * 256 + 0xA8) for i in range(10)],
                active_stamina_ratings=ratings)


def validate_projection(row):
    if not isinstance(row, dict) or set(row) != set(KEYS):
        raise ValueError("identity projection has incorrect fields")
    for name, count in KEYS.items():
        values = row[name]
        if not isinstance(values, list) or len(values) != count or any(
                type(value) is not int or not 0 <= value <= 65535 for value in values):
            raise ValueError(f"invalid uint16 array: {name}")


def read_native(directory, rom):
    manifest = strict_json((directory / "manifest.json").read_text(encoding="utf-8-sig"))
    if manifest.get("state_injection") is not False or manifest.get("rom_patch") is not False:
        raise ValueError("identity witness must be a natural, unpatched capture")
    rom_bytes = rom.read_bytes()
    if digest(rom_bytes) != ROM_SHA256 or manifest["sources"]["rom"]["sha256"] != ROM_SHA256:
        raise ValueError("original ROM identity mismatch")
    for name in ("capture.lua", "first_court.wram", "ownership.jsonl", "capture_complete.txt"):
        raw = (directory / name).read_bytes()
        attested = manifest["artifacts"][name]
        if attested["bytes"] != len(raw) or attested["sha256"] != digest(raw):
            raise ValueError(f"native artifact identity mismatch: {name}")
    snapshot = (directory / "first_court.wram").read_bytes()
    expected = projection(snapshot, rom_bytes)
    def word(address):
        return int.from_bytes(snapshot[address:address + 2], "little")
    journey = [strict_json(line) for line in (directory / "ownership.jsonl").read_text().splitlines()]
    first = [row for row in journey if row["tag"] == "first_court"]
    if len(first) != 1 or first[0]["native_pc"] != 0x87A47A or not any(
            row["tag"] == "initialize.entry" and row["native_pc"] == 0x86E208 for row in journey):
        raise ValueError("native initializer/first-court boundaries were not observed")
    inputs = [word(0x16FB), word(0x16FD), word(0x17B1)]
    if not 0 <= inputs[0] < 29 or not 0 <= inputs[1] < 29 or not 0 <= inputs[2] < 4:
        raise ValueError("native UI initialization inputs are invalid")
    return expected, inputs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native", required=True, type=Path)
    parser.add_argument("--rom", required=True, type=Path)
    parser.add_argument("--probe", required=True, type=Path)
    parser.add_argument("--pack", required=True, type=Path)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    expected, inputs = read_native(args.native, args.rom)
    run = subprocess.run([str(args.probe.resolve()), str(args.pack.resolve())],
                         input=" ".join(map(str, inputs)) + "\n", text=True,
                         capture_output=True, timeout=60, check=True)
    rows = [strict_json(line.removeprefix("FIRST_COURT_IDENTITY "))
            for line in run.stdout.splitlines() if line.startswith("FIRST_COURT_IDENTITY ")]
    if len(rows) != 1:
        raise ValueError("C probe did not emit exactly one identity projection")
    actual = rows[0]
    validate_projection(expected)
    validate_projection(actual)
    differences = [dict(field=f"{key}[{i}]", native=wanted, port=got)
                   for key in KEYS for i, (wanted, got) in enumerate(zip(expected[key], actual[key]))
                   if wanted != got]
    report = dict(scope="production initializer identity projection only", ui_inputs=inputs,
                  native=expected, port=actual, differences=differences,
                  native_manifest_sha256=digest((args.native / "manifest.json").read_bytes()),
                  native_snapshot_sha256=digest((args.native / "first_court.wram").read_bytes()),
                  probe_sha256=digest(args.probe.read_bytes()), pack_sha256=digest(args.pack.read_bytes()))
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))
    if differences:
        raise SystemExit("FAIL: native first-court identity differs; no C expectation was rebased")
    print("PASS: native first-court identity projection; full initialization/controls/trajectory parity NOT claimed")


if __name__ == "__main__":
    main()
