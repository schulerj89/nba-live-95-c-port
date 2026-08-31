"""Exact native Rules return: unchanged Simulation and changed row2/Custom.

Both natural native journeys press Start830. The C controller script waits212
idle frames (209 after three row/value presses) to match the entry BG2 phase,
then presses Start527. Compare all171 RGB frames830..1000/C527..697, the first
133 frames' complete mapped PPU states (including dispatch), and final configuration.
This does not assert identical earlier navigation timing or routine execution.
"""
import argparse
import csv
import json
import re
import struct
import subprocess
import tempfile
from pathlib import Path

from PIL import Image
from setup_transition_capture import (ROM_SHA256, digest, strict_json,
    validate_rules_capture, validate_rules_manifest, read_ppu_states, read_rgb_flags)
from test_setup_rules_reveal import (STATE_KEYS, canonical_manifest_digest,
                                     validate_ppu_state)
from test_setup_rules_settled import read_final_state

ROOT = Path(__file__).resolve().parents[1]
FIRST, TRACE_LAST, LAST, OFFSET = 830, 962, 1000, 303
DEFAULT_RULES = [45, 45] + [1] * 11


def validate(witness):
    if type(witness.get("schema")) is not int or witness["schema"] != 1 or \
            witness.get("mode") not in ("hold", "custom-row2"):
        raise ValueError("invalid Rules return witness schema/mode")
    custom = witness["mode"] == "custom-row2"
    manifest = validate_rules_manifest(witness.get("native_manifest", {}), include_return=True, include_dispatch=True)
    if canonical_manifest_digest(manifest) != witness.get("native_manifest_canonical_sha256"):
        raise ValueError("Rules return manifest digest mismatch")
    configuration = manifest.get("configuration", {})
    if custom:
        if configuration.get("hold_menu_without_value_edits") is not False or \
                type(configuration.get("target_row")) is not int or configuration["target_row"] != 2 or \
                type(configuration.get("target_rights")) is not int or configuration["target_rights"] != 1:
            raise ValueError("Custom witness requires the native row2/right1 journey")
    elif configuration.get("hold_menu_without_value_edits") is not True or \
            type(configuration.get("target_row", -1)) is not int or configuration.get("target_row", -1) != -1 or \
            type(configuration.get("target_rights", 0)) is not int or configuration.get("target_rights", 0) != 0:
        raise ValueError("unchanged witness requires a native no-edit menu hold")
    expected_rules = list(DEFAULT_RULES)
    if custom:
        expected_rules[2] = 0
    config = witness.get("native_committed", {})
    if set(config) != {"main", "rules", "source", "sha256"} or \
            config["main"] != [0, 2 if custom else 1, 0, 0] or config["rules"] != expected_rules or \
            any(type(value) is not int for value in config["main"] + config["rules"]) or \
            config["source"] != "wram_after_back.bin" or \
            not re.fullmatch(r"[0-9a-f]{64}", config["sha256"]) or \
            config["sha256"] != manifest["artifacts"]["files"]["wram_after_back.bin"]["sha256"]:
        raise ValueError("return witness lacks the native committed Rules/Style result")
    rows = witness.get("rows")
    if not isinstance(rows, list) or len(rows) != LAST - FIRST + 1:
        raise ValueError("return witness must contain all171 consecutive frames")
    for frame, row in zip(range(FIRST, LAST + 1), rows):
        if not isinstance(row, dict) or set(row) != {"native_frame", "port_step", "rgb_sha256", "state"} or \
                type(row.get("native_frame")) is not int or row["native_frame"] != frame or \
                type(row.get("port_step")) is not int or row["port_step"] != frame - OFFSET or \
                not re.fullmatch(r"[0-9a-f]{64}", row.get("rgb_sha256", "")):
            raise ValueError("invalid/duplicate/reordered return witness row")
        validate_ppu_state(row["state"])
    return witness


def native_witness(directory, mode):
    directory = Path(directory)
    manifest = validate_rules_capture(directory, include_return=True, include_dispatch=True)
    ram = (directory / "wram_after_back.bin").read_bytes()
    if len(ram) != 0x20000:
        raise ValueError("native return configuration requires full128KiB WRAM")
    states = read_ppu_states(directory / "return_transition_ppu_states.txt")
    dispatch = read_ppu_states(directory / "dispatch_ppu_states.txt")
    if list(dispatch) != [470, 830]:
        raise ValueError("native capture must retain both dispatch PPU states")
    states[830] = dispatch[830]
    flags = read_rgb_flags(directory / "rgb_state.csv")
    rows = []
    for frame in range(FIRST, LAST + 1):
        raw = (directory / f"close_step_{frame:03d}.rgb").read_bytes()
        if len(raw) != 256 * 239 * 3:
            raise ValueError("expected synchronous native256x239 RGB")
        state = dict(zip(STATE_KEYS, states[frame]))
        for layer in range(1, 4):
            state[f"bg{layer}map"] *= 2
            state[f"bg{layer}chr"] *= 2
        state["forced_blank"] = flags[f"close_step_{frame:03d}.png"]["forced_blank"]
        rows.append(dict(native_frame=frame, port_step=frame - OFFSET, state=state,
                         rgb_sha256=digest(raw[7 * 256 * 3:231 * 256 * 3])))
    return validate(dict(schema=1, mode=mode, native_capture=str(directory.resolve()),
        native_manifest=manifest, native_manifest_sha256=digest((directory / "manifest.json").read_bytes()),
        native_manifest_canonical_sha256=canonical_manifest_digest(manifest),
        native_committed=dict(source="wram_after_back.bin", sha256=digest(ram),
            main=list(struct.unpack_from("<4H", ram, 0x17AB)),
            rules=list(struct.unpack_from("<13H", ram, 0x17D1))), rows=rows))


def read_trace(path):
    states = {}
    with Path(path).open(newline="", encoding="ascii") as stream:
        reader = csv.DictReader(stream)
        if reader.fieldnames is None or len(reader.fieldnames) != len(set(reader.fieldnames)) or \
                not set(STATE_KEYS).union({"step", "forced_blank"}).issubset(reader.fieldnames):
            raise ValueError("invalid return C telemetry header")
        actual = []
        for row in reader:
            if None in row or any(value is None for value in row.values()):
                raise ValueError("missing/trailing return telemetry field")
            step = int(row["step"])
            validate_ppu_state({key: int(row[key]) for key in (*STATE_KEYS, "forced_blank")})
            actual.append(step)
            states[step] = row
        if actual != list(range(167, 314)) + list(range(527, 660)):
            raise ValueError("return telemetry must contain both complete ordered transition ranges")
    return states


def check(witness, exe, rom, pack):
    raw = Path(rom).read_bytes()
    if len(raw) % 1024 == 512:
        raw = raw[512:]
    if digest(raw) != ROM_SHA256:
        raise ValueError("C input ROM differs from the native witness ROM")
    custom = witness["mode"] == "custom-row2"
    with tempfile.TemporaryDirectory(prefix="nba95-rules-return-") as temp:
        directory = Path(temp)
        trace = directory / "trace.csv"
        command = [str(Path(exe).resolve()), "--headless", "--setup-only", "--setup-menu", "rules",
            "--setup-menu-confirm", "--setup-menu-row", "2" if custom else "0",
            "--setup-menu-right", "1" if custom else "0", "--setup-menu-confirm-delay", "209" if custom else "212",
            "--rom", str(Path(rom).resolve()), "--assets", str(Path(pack).resolve()),
            "--frames", "697", "--dump-sequence-from", "527", "--dump-sequence-dir", temp,
            "--setup-transition-trace", str(trace), "--debug-state"]
        result = subprocess.run(command, check=True, capture_output=True, text=True, timeout=30)
        states = read_trace(trace)
        for row in witness["rows"]:
            step = row["port_step"]
            rgb = Image.open(directory / f"frame_{step:04d}.bmp").convert("RGB").tobytes()
            if digest(rgb) != row["rgb_sha256"]:
                raise AssertionError(f"native{row['native_frame']}/C{step}: exact return RGB mismatch")
            if row["native_frame"] <= TRACE_LAST:
                for key, expected in row["state"].items():
                    if int(states[step][key]) != expected:
                        raise AssertionError(f"native{row['native_frame']}/C{step}: {key} mismatch")
        fields, ppu = read_final_state(result.stdout)
        if fields.get("page") != "0" or fields.get("main_row") != "0" or \
                fields.get("transition") != "0/132" or fields.get("blank") != "0" or fields.get("gfx") != "1" or \
                fields.get("main") != "/".join(map(str, witness["native_committed"]["main"])) or \
                fields.get("rules") != "/".join(map(str, witness["native_committed"]["rules"])) or \
                fields.get("working") != ("0" if custom else "45") or \
                fields.get("committed") != ("0" if custom else "45") or \
                ppu != (15, 512, 0, witness["rows"][-1]["state"]["bg2v"], 0):
            raise AssertionError("returned parent cursor/configuration/PPU state differs from native")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native", type=Path)
    parser.add_argument("--mode", choices=("hold", "custom-row2"), default="hold")
    parser.add_argument("--fixture", type=Path)
    parser.add_argument("--write-fixture", type=Path)
    parser.add_argument("--replace-fixture", action="store_true")
    parser.add_argument("--exe", type=Path)
    parser.add_argument("--rom", type=Path)
    parser.add_argument("--pack", type=Path)
    args = parser.parse_args()
    fixture = args.fixture or ROOT / f"tests/fixtures/setup-rules-return-{args.mode}-native.json"
    witness = native_witness(args.native, args.mode) if args.native else validate(strict_json(fixture.read_text()))
    if args.write_fixture:
        if not args.native or (args.write_fixture.exists() and not args.replace_fixture):
            parser.error("native evidence and explicit replacement authorization are required")
        args.write_fixture.write_text(json.dumps(witness, indent=2) + "\n")
    if args.exe is None:
        if args.write_fixture:
            print("Wrote native-only return witness; no C verification requested")
            return
        parser.error("--exe, --rom and --pack are required")
    if args.rom is None or args.pack is None:
        parser.error("--exe, --rom and --pack are required")
    check(witness, args.exe, args.rom, args.pack)
    print(f"PASS: Rules {witness['mode']} return171 native RGB frames,133 complete PPU states, final cursor/configuration")


if __name__ == "__main__":
    main()
