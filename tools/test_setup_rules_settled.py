"""Independent native Rules steady continuation or state-aligned UI snapshot.

Hold mode compares every native617..753/C314..450 RGB frame. UI variants
compare only settled native753/C450, after equivalent settings/navigation;
their distinct input timing does not establish whole-journey equivalence.
"""
import argparse
import json
import re
import struct
import subprocess
import tempfile
from pathlib import Path

from PIL import Image

from setup_transition_capture import (ROM_SHA256, digest, strict_json,
    validate_rules_capture, validate_rules_manifest, read_ppu_states)
from test_setup_rules_reveal import canonical_manifest_digest

ROOT = Path(__file__).resolve().parents[1]


def validate(witness):
    if type(witness.get("schema")) is not int or witness["schema"] != 1 or \
            witness.get("mode") not in ("hold", "ui-snapshot"):
        raise ValueError("invalid settled witness schema/mode")
    menu_row, menu_rights = witness.get("menu_row"), witness.get("menu_rights")
    if type(menu_row) is not int or not 0 <= menu_row <= 12 or \
            type(menu_rights) is not int or not 0 <= menu_rights <= 2:
        raise ValueError("invalid settled witness controls")
    manifest = validate_rules_manifest(witness.get("native_manifest", {}))
    if canonical_manifest_digest(manifest) != witness.get("native_manifest_canonical_sha256"):
        raise ValueError("settled manifest digest mismatch")
    expected = list(range(617, 754)) if witness["mode"] == "hold" else [753]
    configuration = manifest.get("configuration", {})
    if witness["mode"] == "hold":
        if configuration.get("hold_menu_without_value_edits") is not True or (menu_row, menu_rights) != (0, 0) or \
                configuration.get("target_row", -1) != -1 or configuration.get("target_rights", 0) != 0:
            raise ValueError("continuation witness requires a native no-input menu hold")
    else:
        if type(configuration.get("target_row")) is not int or type(configuration.get("target_rights")) is not int or \
                configuration["target_row"] != menu_row or configuration["target_rights"] != menu_rights:
            raise ValueError("UI witness controls do not match native capture")
        before, after = witness.get("native_values_before"), witness.get("native_values_after")
        if not isinstance(before, list) or len(before) != 13 or not isinstance(after, list) or len(after) != 13 or \
                any(type(value) is not int for value in before + after):
            raise ValueError("UI witness requires complete native before/after working rules")
        if any(not 0 <= value <= (45 if index < 2 else 1)
               for values in (before, after) for index, value in enumerate(values)):
            raise ValueError("native working rule value outside its ROM range")
        expected_values = list(before)
        expected_values[menu_row] = min(45, before[menu_row] + menu_rights) if menu_row < 2 else \
            (before[menu_row] + menu_rights) % 2
        if after != expected_values:
            raise ValueError("UI witness requested right presses did not change the native value as expected")
    rows = witness.get("rows")
    if not isinstance(rows, list) or len(rows) != len(expected):
        raise ValueError("incomplete settled witness")
    for frame, row in zip(expected, rows):
        if not isinstance(row, dict) or set(row) != {"native_frame", "port_step", "bg2v", "rgb_sha256"} or \
                type(row.get("native_frame")) is not int or type(row.get("port_step")) is not int or \
                row["native_frame"] != frame or row["port_step"] != frame - 303 or \
                not re.fullmatch(r"[0-9a-f]{64}", row.get("rgb_sha256", "")) or \
                type(row.get("bg2v")) is not int or not 0 <= row["bg2v"] <= 1023:
            raise ValueError("invalid/duplicate/reordered settled witness row")
    return witness


def native_witness(directory, row, rights):
    directory = Path(directory)
    manifest = validate_rules_capture(directory)
    mode = "ui-snapshot" if row >= 0 else "hold"
    values = {}
    if row >= 0:
        ram = (directory / "wram_state753.bin").read_bytes()
        entry = manifest.get("artifacts", {}).get("files", {}).get("wram_state753.bin", {})
        if len(ram) != 0x20000 or entry.get("sha256") != digest(ram) or \
                struct.unpack_from("<H", ram, 0x1693)[0] != row:
            raise ValueError("native settled row/WRAM attestation mismatch")
        values = dict(native_values_before=list(struct.unpack_from("<13H",
            (directory / "wram_open.bin").read_bytes(), 0x16FB)),
            native_values_after=list(struct.unpack_from("<13H", ram, 0x16FB)))
    states = read_ppu_states(directory / "open_transition_ppu_states.txt")
    rows = []
    for frame in (range(617, 754) if mode == "hold" else [753]):
        raw = (directory / f"open_step_{frame:03d}.rgb").read_bytes()
        if len(raw) != 256 * 239 * 3:
            raise ValueError("expected native256x239 synchronous RGB")
        rows.append(dict(native_frame=frame, port_step=frame - 303,
            bg2v=states[frame][10], rgb_sha256=digest(raw[7 * 256 * 3:231 * 256 * 3])))
    return validate(dict(schema=1, mode=mode, menu_row=max(0, row), menu_rights=rights,
        native_capture=str(directory.resolve()), native_manifest=manifest,
        native_manifest_canonical_sha256=canonical_manifest_digest(manifest), rows=rows, **values))


def read_final_state(stdout):
    setups = [line for line in stdout.splitlines() if line.startswith("[SETUP TEST] ")]
    ppus = [line for line in stdout.splitlines() if line.startswith("[DEBUG STATE] PPU ")]
    if len(setups) != 1 or len(ppus) != 1:
        raise ValueError("expected exactly one final Setup and PPU state")
    fields = {}
    for pair in setups[0][len("[SETUP TEST] "):].split():
        key, value = pair.split("=", 1)
        if key in fields:
            raise ValueError("duplicate final Setup state field")
        fields[key] = value
    ppu = re.fullmatch(r"\[DEBUG STATE\] PPU B:(\d+) X1:(-?\d+) X2:(-?\d+) Y2:(-?\d+) Y3:(-?\d+)", ppus[0])
    if not ppu:
        raise ValueError("malformed final PPU state")
    return fields, tuple(map(int, ppu.groups()))


def check(witness, exe, rom, pack):
    # Input-only configuration/tap migration; original witness mapping retained.
    from test_setup_rules_reveal import CONFIGURED_REVEAL_SHIFT
    shift = CONFIGURED_REVEAL_SHIFT
    raw = Path(rom).read_bytes()
    if len(raw) % 1024 == 512:
        raw = raw[512:]
    if digest(raw) != ROM_SHA256:
        raise ValueError("port ROM identity differs from verified native ROM")
    with tempfile.TemporaryDirectory(prefix="nba95-rules-settled-") as temp:
        first = witness["rows"][0]["port_step"] + shift
        command = [str(Path(exe).resolve()), "--headless", "--setup-only", "--setup-menu", "rules",
            "--setup-simulation-three-minute",
            "--setup-menu-row", str(witness["menu_row"]), "--setup-menu-right", str(witness["menu_rights"]),
            "--rom", str(Path(rom).resolve()), "--assets", str(Path(pack).resolve()),
            "--frames", str(450 + shift), "--dump-sequence-from", str(first), "--dump-sequence-dir", temp,
            "--debug-state"]
        result = subprocess.run(command, check=True, capture_output=True, text=True, timeout=30)
        for row in witness["rows"]:
            rgb = Image.open(Path(temp) / f"frame_{row['port_step'] + shift:04d}.bmp").convert("RGB").tobytes()
            if digest(rgb) != row["rgb_sha256"]:
                raise AssertionError(f"native{row['native_frame']}/C{row['port_step'] + shift}: exact RGB mismatch")
        fields, ppu = read_final_state(result.stdout)
        if fields.get("page") != "1" or fields.get("menu_row") != str(witness["menu_row"]) or \
                fields.get("transition") != "0/146" or fields.get("blank") != "0" or fields.get("gfx") != "1" or \
                ppu[0] != 15 or ppu[3] != witness["rows"][-1]["bg2v"]:
            raise AssertionError("settled runtime page/row/phase differs from native state")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native", type=Path)
    parser.add_argument("--menu-row", type=int, default=-1)
    parser.add_argument("--menu-rights", type=int, default=0)
    parser.add_argument("--fixture", type=Path,
                        default=ROOT / "tests/fixtures/setup-rules-settled-native.json")
    parser.add_argument("--write-fixture", type=Path)
    parser.add_argument("--replace-fixture", action="store_true")
    parser.add_argument("--exe", type=Path)
    parser.add_argument("--rom", type=Path)
    parser.add_argument("--pack", type=Path)
    args = parser.parse_args()
    witness = native_witness(args.native, args.menu_row, args.menu_rights) if args.native else \
        validate(strict_json(args.fixture.read_text()))
    if args.write_fixture:
        if not args.native or (args.write_fixture.exists() and not args.replace_fixture):
            parser.error("native evidence and explicit --replace-fixture required to replace a fixture")
        args.write_fixture.write_text(json.dumps(witness, indent=2) + "\n")
    if args.exe is None:
        if args.write_fixture:
            print("Wrote native-only settled witness; no C verification requested")
            return
        parser.error("--exe, --rom and --pack are required for verification")
    if args.rom is None or args.pack is None:
        parser.error("--exe, --rom and --pack are required for verification")
    check(witness, args.exe, args.rom, args.pack)
    print(f"PASS: {len(witness['rows'])} native Rules {witness['mode']} RGB frames, final page/row/BG2 phase")


if __name__ == "__main__":
    main()
