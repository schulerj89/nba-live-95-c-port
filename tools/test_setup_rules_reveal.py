"""Exact native framebuffer/state gates for Rules reveal and complete opening.

The witness covers native frames546..616, mapped once from real A-confirm
dispatch to C steps243..313. It does not claim outgoing or return parity,
changed values, repeated visits, or execution equivalence of packed PPU traces.
The separate --whole-open contract compares native470..616, including dispatch,
after a fixed717
frame headless input-idle alignment derived from the incoming BG2 phase.
"""
import argparse
import csv
import hashlib
import json
import re
import struct
import subprocess
import tempfile
from pathlib import Path

from PIL import Image
from setup_transition_capture import (validate_rules_capture, validate_rules_manifest,
                                      read_ppu_states, read_rgb_flags)

ROOT = Path(__file__).resolve().parents[1]
FIRST, LAST, OFFSET = 546, 616, 303
# Whole-open alignment is fixed before comparison: native pre-open BG2v258
# minus C19, multiplied by the observed3-frame/pixel cadence =717 input-idle
# frames. This is a headless controller-script wait, never a production fade.
CONTRACTS = {"reveal": (546, 616, 303, 0), "open": (470, 616, -414, 717)}
# Preserve the historical native fixture and its original C mapping. The real
# held/release CLI now performs eight configuration taps and four cursor taps.
# With no idle, A dispatch is C187 instead of167 (shift20). For whole-open,
# configuration occurs before the long idle; four release frames replace four
# idle frames, so713 retains the independently fixed A884/BG2v258 boundary.
CONFIGURED_REVEAL_SHIFT = 20
CONFIGURED_OPEN_DELAY = 713
ROM_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
STATE_KEYS = ("brightness", "main", "sub", "bg1h", "bg1v", "bg1map", "bg1chr",
              "bg1wide", "bg1tall", "bg2h", "bg2v", "bg2map", "bg2chr",
              "bg2wide", "bg2tall", "bg3h", "bg3v", "bg3map", "bg3chr",
              "bg3wide", "bg3tall")


def digest(data):
    return hashlib.sha256(data).hexdigest()


def strict_json(text):
    def pairs(items):
        result = {}
        for key, value in items:
            if key in result:
                raise ValueError(f"duplicate JSON key: {key}")
            result[key] = value
        return result
    return json.loads(text, object_pairs_hook=pairs)


def canonical_manifest_digest(manifest):
    return digest(json.dumps(manifest, sort_keys=True, separators=(",", ":")).encode())


def validate_ppu_state(state):
    if not isinstance(state, dict) or set(state) != set(STATE_KEYS) | {"forced_blank"} or \
            any(type(value) is not int for value in state.values()):
        raise ValueError("witness must retain the complete mapped PPU state")
    if state["forced_blank"] not in (0, 1) or not 0 <= state["brightness"] <= 15:
        raise ValueError("invalid native INIDISP witness")
    if any(not 0 <= state[key] <= 31 for key in ("main", "sub")):
        raise ValueError("invalid native layer-enable witness")
    for layer in range(1, 4):
        if any(not 0 <= state[f"bg{layer}{axis}"] <= 1023 for axis in ("h", "v")) or \
                any(not 0 <= state[f"bg{layer}{kind}"] <= 65534 or
                    state[f"bg{layer}{kind}"] % 2 for kind in ("map", "chr")) or \
                any(state[f"bg{layer}{kind}"] not in (0, 1) for kind in ("wide", "tall")):
            raise ValueError("invalid native PPU layer witness")


def validate_witness(witness):
    if type(witness.get("schema")) is not int or witness["schema"] != 1:
        raise ValueError("unsupported reveal witness schema")
    contract = witness.get("contract", "reveal")
    if contract not in CONTRACTS:
        raise ValueError("unknown fixed Rules comparison contract")
    first, last, offset, _ = CONTRACTS[contract]
    manifest = witness.get("native_manifest")
    if not isinstance(manifest, dict) or \
            canonical_manifest_digest(manifest) != witness.get("native_manifest_canonical_sha256"):
        raise ValueError("native manifest content/digest mismatch")
    validate_rules_manifest(manifest, include_dispatch=contract == "open")
    if manifest.get("sources", {}).get("rom", {}).get("sha256") != ROM_SHA256:
        raise ValueError("native capture is not the verified USA ROM")
    if manifest.get("isolation", {}).get("settings", {}).get("Snes", {}).get("DisableFrameSkipping") is not True:
        raise ValueError("native capture did not disable PPU frame skipping")
    rows = witness.get("rows")
    if not isinstance(rows, list) or len(rows) != last - first + 1:
        raise ValueError("Rules witness must contain every consecutive contract frame")
    required = set(STATE_KEYS) | {"forced_blank"}
    for frame, row in zip(range(first, last + 1), rows):
        if not isinstance(row, dict) or set(row) != {"native_frame", "port_step", "rgb_sha256", "state"} or \
                type(row.get("native_frame")) is not int or type(row.get("port_step")) is not int or \
                row.get("native_frame") != frame or \
                row.get("port_step") != frame - offset:
            raise ValueError("reveal witness frame order/offset is invalid")
        if not re.fullmatch(r"[0-9a-f]{64}", row.get("rgb_sha256", "")):
            raise ValueError("reveal witness has a missing/invalid native RGB hash")
        validate_ppu_state(row.get("state"))
    return witness


def native_witness(directory, contract="reveal"):
    """Read a fresh, provenance-bearing capture; never derive goldens from C."""
    directory = Path(directory)
    first, last, offset, _ = CONTRACTS[contract]
    if not (directory / "capture_complete.txt").is_file():
        raise ValueError("incomplete native capture")
    manifest_bytes = (directory / "manifest.json").read_bytes()
    manifest = validate_rules_capture(directory, include_dispatch=contract == "open")
    for name in ("rom", "mesen", "capture", "portable_settings"):
        source = manifest.get("sources", {}).get(name, {})
        path = Path(source.get("path", ""))
        if not path.is_file() or digest(path.read_bytes()) != source.get("sha256"):
            raise ValueError(f"native source {name} does not match its captured hash")
    ram = (directory / "wram_before_open.bin").read_bytes()
    if list(struct.unpack_from("<4H", ram, 0x16FB)) != [0, 1, 0, 0]:
        raise ValueError("native input journey must select Exhibition/Simulation/Rookie/3 Minutes")
    states = {}
    raw_states = read_ppu_states(directory / "open_transition_ppu_states.txt")
    if contract == "open":
        dispatch = read_ppu_states(directory / "dispatch_ppu_states.txt")
        if list(dispatch) != [470, 830]:
            raise ValueError("native capture must record both dispatch PPU states")
        raw_states[470] = dispatch[470]
    for frame, fields in raw_states.items():
        state = dict(zip(STATE_KEYS, fields))
        for layer in range(1, 4):
            state[f"bg{layer}map"] *= 2
            state[f"bg{layer}chr"] *= 2
        states[frame] = state
    flags = read_rgb_flags(directory / "rgb_state.csv")
    rows = []
    for frame in range(first, last + 1):
        name = f"open_step_{frame:03d}"
        raw = (directory / f"{name}.rgb").read_bytes()
        if len(raw) != 256 * 239 * 3:
            raise ValueError("expected synchronous Mesen256x239 RGB buffer")
        # Mesen's SNES GetPpuFrame returns239 rows with seven leading rows;
        # the game's224 active lines are fixed here, never fitted per frame.
        visible = raw[7 * 256 * 3:231 * 256 * 3]
        state = states[frame]
        state["forced_blank"] = int(flags[f"{name}.png"]["forced_blank"])
        rows.append(dict(native_frame=frame, port_step=frame - offset,
                         rgb_sha256=digest(visible), state=state))
    return validate_witness(dict(schema=1, contract=contract,
                scope="Rules complete open" if contract == "open" else "Rules incoming build/entrance/reveal only",
                exclusions=(["outgoing prefix"] if contract == "reveal" else []) + ["Rules return", "reentry", "altered values",
                            "whole routine/caller equivalence", "OAM resource generation"],
                native_manifest=manifest, native_manifest_sha256=digest(
                    manifest_bytes), native_manifest_canonical_sha256=canonical_manifest_digest(manifest),
                native_capture=str(directory.resolve()), rows=rows))


def read_port_trace(path, contract="reveal", port_shift=0):
    _, last, offset, delay = CONTRACTS[contract]
    states = {}
    with Path(path).open(newline="", encoding="ascii") as source:
        reader = csv.DictReader(source)
        fields = reader.fieldnames
        if fields is None or len(fields) != len(set(fields)) or \
                not set(STATE_KEYS).union({"step", "forced_blank"}).issubset(fields):
            raise ValueError("port transition trace has invalid/duplicate field names")
        previous = None
        for row in reader:
            if None in row or any(value is None for value in row.values()):
                raise ValueError("port transition trace has missing/trailing fields")
            step = int(row["step"])
            if previous is not None and step != previous + 1:
                raise ValueError("port transition trace skipped, duplicated or reordered a step")
            states[step] = row
            previous = step
        if list(states) != list(range(167 + delay + port_shift, last - offset + port_shift + 1)):
            raise ValueError("port transition trace does not contain the fixed contract's complete range")
    return states


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native", type=Path, help="fresh independent capture to compare directly")
    parser.add_argument("--whole-open", action="store_true",
                        help="fixed717-frame input-idle alignment; native470..616 full opening including dispatch")
    parser.add_argument("--write-fixture", type=Path,
                        help="write only native-derived witness, never C-derived expected output")
    parser.add_argument("--replace-fixture", action="store_true",
                        help="explicitly authorize replacing an existing native witness")
    parser.add_argument("--fixture", type=Path)
    parser.add_argument("--exe", type=Path)
    parser.add_argument("--rom", type=Path)
    parser.add_argument("--pack", type=Path)
    args = parser.parse_args()
    requested_contract = "open" if args.whole_open else "reveal"
    fixture = args.fixture or ROOT / f"tests/fixtures/setup-rules-{'open' if args.whole_open else 'reveal'}-native.json"
    witness = native_witness(args.native, requested_contract) if args.native else validate_witness(
        strict_json(fixture.read_text()))
    contract = witness.get("contract", "reveal")
    if args.whole_open and contract != "open":
        parser.error("--whole-open requires the complete open witness")
    first, last, offset, delay = CONTRACTS[contract]
    port_shift = CONFIGURED_REVEAL_SHIFT if contract == "reveal" else 0
    input_delay = CONFIGURED_OPEN_DELAY if contract == "open" else 0
    if args.write_fixture:
        if not args.native:
            parser.error("--write-fixture requires --native; C cannot author its oracle")
        if args.write_fixture.exists() and not args.replace_fixture:
            parser.error("existing native fixture is preserved; use --replace-fixture after evidence review")
        args.write_fixture.write_text(json.dumps(witness, indent=2) + "\n")
    if args.exe is None:
        if args.write_fixture:
            return
        parser.error("--exe, --rom and --pack are required for verification")
    if args.rom is None or args.pack is None:
        parser.error("--exe, --rom and --pack are required for verification")
    rom_bytes = args.rom.read_bytes()
    if len(rom_bytes) % 1024 == 512:
        rom_bytes = rom_bytes[512:]
    if digest(rom_bytes) != ROM_SHA256:
        raise ValueError("port input ROM does not match the verified USA ROM")
    failures = []
    with tempfile.TemporaryDirectory(prefix="nba95-rules-reveal-") as temp:
        directory = Path(temp)
        trace = directory / "transition.csv"
        subprocess.run([str(args.exe.resolve()), "--headless", "--rom", str(args.rom.resolve()),
            "--assets", str(args.pack.resolve()), "--setup-only", "--setup-menu", "rules",
            "--setup-simulation-three-minute", "--setup-menu-delay", str(input_delay),
            "--frames", str(last - offset + port_shift), "--dump-sequence-from", str(first - offset + port_shift),
            "--dump-sequence-dir", temp, "--setup-transition-trace", str(trace)],
            check=True, capture_output=True, text=True, timeout=30)
        states = read_port_trace(trace, contract, port_shift)
        for row in witness["rows"]:
            step = row["port_step"] + port_shift
            rgb = Image.open(directory / f"frame_{step:04d}.bmp").convert("RGB").tobytes()
            if digest(rgb) != row["rgb_sha256"]:
                failures.append(f"native{row['native_frame']}/C{step}: RGB mismatch")
            for field, expected in row["state"].items():
                actual = int(states[step][field])
                if actual != expected:
                    failures.append(f"native{row['native_frame']}/C{step}: {field}={actual}, native={expected}")
        if failures:
            raise AssertionError(f"{len(failures)} exact mismatch(es); first: {failures[0]}")
    print(f"PASS: {len(witness['rows'])} consecutive native Rules {contract} frames; exact RGB and mapped PPU state")


if __name__ == "__main__":
    main()
