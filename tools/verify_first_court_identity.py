"""Compare production initialization with a native team/actor identity witness.

This bounded projection does not claim scheduler, controls, full-state or
trajectory parity. The native journey is uninterrupted and controller-only;
the C side deliberately calls its production initializer directly.
"""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path

ROM_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
MESEN_SHA256 = "d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b"
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


def uint(value, maximum=65535):
    return type(value) is int and 0 <= value <= maximum


def sha256(value):
    return isinstance(value, str) and re.fullmatch(r"[0-9a-f]{64}", value) is not None


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
    result = dict(context_teams=[word(0x46EB), word(0x476B)],
                anchor_x=[word(0x46F5), word(0x4775)],
                actor_groups=[word(0x34EB + i * 256 + 0x6E) for i in range(10)],
                active_roster=[word((0x46F9 if i < 5 else 0x4779) + (i % 5) * 2) for i in range(10)],
                assignment_roles=[word(0x34EB + i * 256 + 0x92) for i in range(10)],
                appearance_variants=[word(0x34EB + i * 256 + 0x6C) for i in range(10)],
                height_variants=[word(0x34EB + i * 256 + 0xA8) for i in range(10)],
                active_stamina_ratings=ratings)
    validate_projection(result)
    return result


def validate_projection(row):
    if not isinstance(row, dict) or set(row) != set(KEYS):
        raise ValueError("identity projection has incorrect fields")
    for name, count in KEYS.items():
        values = row[name]
        if not isinstance(values, list) or len(values) != count or any(
                not uint(value) for value in values):
            raise ValueError(f"invalid uint16 array: {name}")
    domains = {"context_teams": range(29), "actor_groups": (0, 5),
               "active_roster": range(12), "assignment_roles": range(5),
               "active_stamina_ratings": range(256)}
    for name, domain in domains.items():
        if any(value not in domain for value in row[name]):
            raise ValueError(f"invalid native domain: {name}")
    for start in (0, 5):
        if set(row["assignment_roles"][start:start + 5]) != set(range(5)):
            raise ValueError("each team must publish all five distinct assignment ranks")


def validate_manifest(manifest):
    if not isinstance(manifest, dict) or type(manifest.get("schema")) is not int or manifest["schema"] != 1:
        raise ValueError("unsupported native manifest schema")
    if manifest.get("kind") != "natural controller-only Player Setup to gameplay journey":
        raise ValueError("unexpected native capture kind")
    if manifest.get("state_injection") is not False or manifest.get("rom_patch") is not False:
        raise ValueError("identity witness must be a natural, unpatched capture")
    if not uint(manifest.get("selection"), 2):
        raise ValueError("invalid native controller selection")
    if "alternate_teams" in manifest and type(manifest["alternate_teams"]) is not bool:
        raise ValueError("invalid native team variant")
    if "pause_after_frames" in manifest and (type(manifest["pause_after_frames"]) is not int or
                                             not -1 <= manifest["pause_after_frames"] <= 1800):
        raise ValueError("invalid native pause schedule")
    sources = manifest.get("sources")
    if not isinstance(sources, dict) or set(sources) != {"rom", "mesen", "capture", "settings", "runner"}:
        raise ValueError("missing or unexpected native source identity")
    for source in sources.values():
        if not isinstance(source, dict) or set(source) != {"path", "sha256"} or not sha256(source["sha256"]) or not isinstance(source["path"], str) or not source["path"]:
            raise ValueError("invalid native source identity")
    if sources["rom"]["sha256"] != ROM_SHA256 or sources["mesen"]["sha256"] != MESEN_SHA256:
        raise ValueError("unsupported ROM or Mesen identity")
    isolation = manifest.get("isolation", {})
    if isolation.get("initial_save_files") != []:
        raise ValueError("native capture must start without save files")
    settings = isolation.get("settings", {})
    expected = {"Preferences": {"SingleInstance": False, "AutoLoadPatches": False,
                                 "OverrideSaveDataFolder": True},
                "Snes": {"EnableRandomPowerOnState": False, "RamPowerOnState": "AllZeros",
                         "Port1": {"Type": "SnesController"}, "Port2": {"Type": "None"}}}
    for group, fields in expected.items():
        for name, value in fields.items():
            got = settings.get(group, {}).get(name)
            if type(got) is not type(value) or got != value:
                raise ValueError(f"unsupported initialization setting: {group}.{name}")
    result = manifest.get("result", {})
    if type(result.get("exit_code")) is not int or result["exit_code"] != 0 or not isinstance(result.get("summary"), str):
        raise ValueError("native capture did not complete successfully")
    artifacts = manifest.get("artifacts")
    if not isinstance(artifacts, dict) or not artifacts:
        raise ValueError("native capture has no artifact attestations")
    for name, attestation in artifacts.items():
        if not isinstance(name, str) or not name or "/" in name or "\\" in name or name in (".", ".."):
            raise ValueError("invalid native artifact name")
        if not isinstance(attestation, dict) or set(attestation) != {"bytes", "sha256"} or not uint(attestation["bytes"], 2**31 - 1) or not sha256(attestation["sha256"]):
            raise ValueError("invalid native artifact attestation")


def read_probe_output(stdout):
    rows = [strict_json(line.removeprefix("FIRST_COURT_IDENTITY "))
            for line in stdout.splitlines() if line.startswith("FIRST_COURT_IDENTITY ")]
    if len(rows) != 1:
        raise ValueError("C probe did not emit exactly one identity projection")
    validate_projection(rows[0])
    return rows[0]


def compare_projections(expected, actual):
    validate_projection(expected)
    validate_projection(actual)
    return [dict(field=f"{key}[{i}]", native=wanted, port=got)
            for key in KEYS for i, (wanted, got) in enumerate(zip(expected[key], actual[key]))
            if wanted != got]


def read_native(directory, rom):
    manifest = strict_json((directory / "manifest.json").read_text(encoding="utf-8-sig"))
    validate_manifest(manifest)
    rom_bytes = rom.read_bytes()
    if digest(rom_bytes) != ROM_SHA256 or manifest["sources"]["rom"]["sha256"] != ROM_SHA256:
        raise ValueError("original ROM identity mismatch")
    for name in ("capture.lua", "first_court.wram", "ownership.jsonl", "capture_complete.txt"):
        if name not in manifest["artifacts"]:
            raise ValueError(f"missing native artifact identity: {name}")
    for name in manifest["artifacts"]:
        raw = (directory / name).read_bytes()
        attested = manifest["artifacts"][name]
        if attested["bytes"] != len(raw) or attested["sha256"] != digest(raw):
            raise ValueError(f"native artifact identity mismatch: {name}")
    for name in ("mesen", "capture", "settings"):
        source = manifest["sources"][name]
        raw = Path(source["path"]).read_bytes()
        if digest(raw) != source["sha256"]:
            raise ValueError(f"native source identity mismatch: {name}")
    if manifest["sources"]["capture"]["sha256"] != manifest["artifacts"]["capture.lua"]["sha256"]:
        raise ValueError("executed script and captured script differ")
    settings = strict_json(Path(manifest["sources"]["settings"]["path"]).read_text(encoding="utf-8-sig"))
    if settings != manifest["isolation"]["settings"]:
        raise ValueError("recorded launch settings differ from executed settings")
    if (directory / "capture_complete.txt").read_text() != manifest["result"]["summary"]:
        raise ValueError("native completion summary differs")
    snapshot = (directory / "first_court.wram").read_bytes()
    expected = projection(snapshot, rom_bytes)
    def word(address):
        return int.from_bytes(snapshot[address:address + 2], "little")
    journey = [strict_json(line) for line in (directory / "ownership.jsonl").read_text().splitlines()]
    first = [row for row in journey if row["tag"] == "first_court"]
    if len(first) != 1 or first[0]["native_pc"] != 0x87A47A or not any(
            row["tag"] == "initialize.entry" and row["native_pc"] == 0x86E208 for row in journey):
        raise ValueError("native initializer/first-court boundaries were not observed")
    first = first[0]
    if first["court_frame"] != 0 or type(first["court_frame"]) is not int or first["selections"] != [word(0x166d + i * 2) for i in range(5)]:
        raise ValueError("first-court event and native WRAM disagree")
    if first["selections"][0] != manifest["selection"] or len(first["actors"]) != 10 or any(
            actor["slot"] != i or actor["group"] != expected["actor_groups"][i]
            for i, actor in enumerate(first["actors"])):
        raise ValueError("first-court actor/controller identity is inconsistent")
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
    actual = read_probe_output(run.stdout)
    differences = compare_projections(expected, actual)
    report = dict(scope="production initializer identity projection only", ui_inputs=inputs,
                  native=expected, port=actual, differences=differences,
                  native_manifest_sha256=digest((args.native / "manifest.json").read_bytes()),
                  native_snapshot_sha256=digest((args.native / "first_court.wram").read_bytes()),
                  probe_sha256=digest(args.probe.read_bytes()), pack_sha256=digest(args.pack.read_bytes()),
                  caveats=["C production initializer called directly; native snapshot came through normal menus",
                           "64 identity words only; scheduler, physics, RNG, input routing and HUD excluded",
                           "historical runner source hash retained in manifest; first capture did not preserve its runner copy",
                           "private Mesen/settings/save folder attested; historical capture did not log Lua home"])
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))
    if differences:
        raise SystemExit("FAIL: native first-court identity differs; no C expectation was rebased")
    print("PASS: native first-court identity projection; full initialization/controls/trajectory parity NOT claimed")


if __name__ == "__main__":
    main()
