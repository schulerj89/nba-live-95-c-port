"""Nonblocking native regression: repeated Rules entry currently FAILS.

Natural journey: row2 ON->OFF, return with Custom, reenter, OFF->ON, return.
Fixed native-C offset303 follows the accepted first return. An explicit132
frame idle between C visits allows the native spaced navigation to reach
second A1100/C797. No offset search or production delay is allowed.
This gate is intentionally not in build.ps1 until the causal defects are fixed.
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
from test_setup_rules_reveal import STATE_KEYS, canonical_manifest_digest, validate_ppu_state

ROOT = Path(__file__).resolve().parents[1]
FRAMES = list(range(1101, 1247)) + list(range(1461, 1631))
SNAPSHOTS = ("before_open", "open", "before_return", "after_return")
RAW_FILES = [f"wram_repeat_{name}.bin" for name in SNAPSHOTS] + [
    f"repeat_{prefix}_transition_{suffix}" for prefix in ("open", "return") for suffix in
    ("vram.bin", "cgram.bin", "vram_writes.txt", "cgram_writes.txt", "ppu_states.txt")]
RULES_ON = [45, 45] + [1] * 11
RULES_OFF = RULES_ON[:2] + [0] + RULES_ON[3:]
EXPECTED_SNAPSHOTS = {
    "before_open": dict(working=[0, 2, 0, 0], main=[0, 2, 0, 0], rules=RULES_OFF, row=4),
    "open": dict(working=RULES_OFF, main=[0, 2, 0, 0], rules=RULES_OFF, row=0),
    "before_return": dict(working=RULES_ON, main=[0, 2, 0, 0], rules=RULES_OFF, row=2),
    "after_return": dict(working=RULES_ON, main=[0, 2, 0, 0], rules=RULES_ON, row=2),
}


def validate(witness):
    if type(witness.get("schema")) is not int or witness["schema"] != 1 or \
            witness.get("contract") != "repeat-row2-fixed303":
        raise ValueError("invalid reentry witness contract")
    manifest = validate_rules_manifest(witness.get("native_manifest", {}), include_return=True)
    if canonical_manifest_digest(manifest) != witness.get("native_manifest_canonical_sha256"):
        raise ValueError("reentry manifest digest mismatch")
    config = manifest.get("configuration", {})
    if config.get("repeat_visit") is not True or type(config.get("target_row")) is not int or \
            config["target_row"] != 2 or type(config.get("target_rights")) is not int or config["target_rights"] != 1:
        raise ValueError("native reentry input profile differs")
    for name in RAW_FILES:
        entry = manifest.get("artifacts", {}).get("files", {}).get(name, {})
        if type(entry.get("bytes")) is not int or entry["bytes"] <= 0 or \
                not re.fullmatch(r"[0-9a-f]{64}", entry.get("sha256", "")):
            raise ValueError(f"missing reentry source attestation: {name}")
    snapshots = witness.get("snapshots")
    if not isinstance(snapshots, dict) or set(snapshots) != set(SNAPSHOTS):
        raise ValueError("missing reentry configuration snapshots")
    for name, expected in EXPECTED_SNAPSHOTS.items():
        actual = snapshots[name]
        if actual != expected or type(actual.get("row")) is not int or \
                any(type(value) is not int for key in ("working", "main", "rules") for value in actual[key]):
            raise ValueError("native reentry configuration/commit boundaries differ")
    rows = witness.get("rows")
    if not isinstance(rows, list) or len(rows) != len(FRAMES):
        raise ValueError("reentry witness must contain both complete frame ranges")
    for frame, row in zip(FRAMES, rows):
        if not isinstance(row, dict) or set(row) != {"native_frame", "port_step", "rgb_sha256", "state"} or \
                type(row.get("native_frame")) is not int or row["native_frame"] != frame or \
                type(row.get("port_step")) is not int or row["port_step"] != frame - 303 or \
                not re.fullmatch(r"[0-9a-f]{64}", row.get("rgb_sha256", "")):
            raise ValueError("invalid/duplicate/reordered reentry witness row")
        validate_ppu_state(row["state"])
    return witness


def native_witness(directory):
    directory = Path(directory)
    manifest = validate_rules_capture(directory, include_return=True)
    for name in RAW_FILES:
        raw = (directory / name).read_bytes()
        entry = manifest["artifacts"]["files"][name]
        if len(raw) != entry["bytes"] or digest(raw) != entry["sha256"]:
            raise ValueError(f"reentry raw source changed: {name}")
    snapshots = {}
    for name in SNAPSHOTS:
        ram = (directory / f"wram_repeat_{name}.bin").read_bytes()
        if len(ram) != 0x20000:
            raise ValueError("reentry WRAM must contain full128KiB")
        snapshots[name] = dict(working=list(struct.unpack_from("<4H" if name == "before_open" else "<13H", ram, 0x16FB)),
            main=list(struct.unpack_from("<4H", ram, 0x17AB)), rules=list(struct.unpack_from("<13H", ram, 0x17D1)),
            row=struct.unpack_from("<H", ram, 0x1693)[0])
    states = read_ppu_states(directory / "repeat_open_transition_ppu_states.txt")
    states.update(read_ppu_states(directory / "repeat_return_transition_ppu_states.txt"))
    flags = read_rgb_flags(directory / "rgb_state.csv")
    rows = []
    for frame in FRAMES:
        prefix = "repeat_open" if frame < 1460 else "repeat_close"
        stem = f"{prefix}_step_{frame:04d}"
        raw = (directory / f"{stem}.rgb").read_bytes()
        if len(raw) != 256 * 239 * 3:
            raise ValueError("expected synchronous native256x239 RGB")
        state = dict(zip(STATE_KEYS, states[frame]))
        for layer in range(1, 4):
            state[f"bg{layer}map"] *= 2
            state[f"bg{layer}chr"] *= 2
        state["forced_blank"] = flags[f"{stem}.png"]["forced_blank"]
        rows.append(dict(native_frame=frame, port_step=frame - 303, state=state,
                         rgb_sha256=digest(raw[7 * 256 * 3:231 * 256 * 3])))
    return validate(dict(schema=1, contract="repeat-row2-fixed303", native_capture=str(directory.resolve()),
        native_manifest=manifest, native_manifest_canonical_sha256=canonical_manifest_digest(manifest),
        snapshots=snapshots, rows=rows))


def check(witness, exe, rom, pack):
    from test_setup_rules_reveal import CONFIGURED_REVEAL_SHIFT
    shift = CONFIGURED_REVEAL_SHIFT
    raw = Path(rom).read_bytes()
    if len(raw) % 1024 == 512:
        raw = raw[512:]
    if digest(raw) != ROM_SHA256:
        raise ValueError("port/native ROM identity mismatch")
    failures = []
    with tempfile.TemporaryDirectory(prefix="nba95-rules-reentry-") as temp:
        path = Path(temp)
        trace = path / "trace.csv"
        subprocess.run([str(Path(exe).resolve()), "--headless", "--setup-only", "--setup-menu", "rules",
            "--setup-simulation-three-minute",
            # Four cursor release frames replace four between-visit idle frames;
            # three row/value releases replace three before-Start idle frames.
            "--setup-menu-visits", "2", "--setup-menu-revisit-delay", "128", "--setup-menu-row", "2",
            "--setup-menu-right", "1", "--setup-menu-confirm", "--setup-menu-confirm-delay", "206",
            "--rom", str(Path(rom).resolve()), "--assets", str(Path(pack).resolve()), "--frames", str(1327 + shift),
            "--dump-sequence-from", str(798 + shift), "--dump-sequence-dir", temp, "--setup-transition-trace", str(trace)],
            check=True, capture_output=True, text=True, timeout=30)
        with trace.open(newline="", encoding="ascii") as stream:
            reader = csv.DictReader(stream)
            if reader.fieldnames is None or len(reader.fieldnames) != len(set(reader.fieldnames)):
                raise ValueError("invalid C reentry trace header")
            states = {}
            order = []
            for row in reader:
                if None in row or any(value is None for value in row.values()):
                    raise ValueError("missing/trailing C trace field")
                step = int(row["step"])
                validate_ppu_state({key: int(row[key]) for key in (*STATE_KEYS, "forced_blank")})
                order.append(step)
                states[step] = row
            expected_order = list(range(167, 314)) + list(range(527, 660)) + list(range(797, 944)) + list(range(1157, 1290))
            if order != [step + shift for step in expected_order]:
                raise ValueError("C reentry trace lacks the four exact complete ranges")
        for row in witness["rows"]:
            frame, step = row["native_frame"], row["port_step"] + shift
            rgb = Image.open(path / f"frame_{step:04d}.bmp").convert("RGB").tobytes()
            if digest(rgb) != row["rgb_sha256"]:
                failures.append(f"native{frame}/C{step}: RGB")
            if frame <= 1246 or frame <= 1592:
                for key, value in row["state"].items():
                    if int(states[step][key]) != value:
                        failures.append(f"native{frame}/C{step}: {key}")
    if failures:
        raise AssertionError(f"FAIL: {len(failures)} reentry RGB/state mismatches; first {failures[0]}")
    print("PASS:316 native reentry RGB frames and278 mapped PPU states")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native", type=Path)
    parser.add_argument("--fixture", type=Path, default=ROOT / "tests/fixtures/setup-rules-reentry-native.json")
    parser.add_argument("--write-fixture", type=Path)
    parser.add_argument("--replace-fixture", action="store_true")
    parser.add_argument("--exe", type=Path)
    parser.add_argument("--rom", type=Path)
    parser.add_argument("--pack", type=Path)
    args = parser.parse_args()
    witness = native_witness(args.native) if args.native else validate(strict_json(args.fixture.read_text()))
    if args.write_fixture:
        if not args.native or (args.write_fixture.exists() and not args.replace_fixture):
            parser.error("native evidence and explicit fixture replacement are required")
        args.write_fixture.write_text(json.dumps(witness, indent=2) + "\n")
    if args.exe is None:
        if args.write_fixture:
            return
        parser.error("--exe, --rom and --pack are required")
    if args.rom is None or args.pack is None:
        parser.error("--exe, --rom and --pack are required")
    check(witness, args.exe, args.rom, args.pack)


if __name__ == "__main__":
    main()
