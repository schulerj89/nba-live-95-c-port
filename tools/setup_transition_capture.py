"""Strict provenance and raw-state readers for synchronous Setup captures."""
import csv
import hashlib
import json
import re
import struct
from pathlib import Path

ROM_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"


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


REQUIRED_ARTIFACTS = ["rgb_state.csv", "raster_registers.csv", "exec_trace.txt", "capture_log.txt",
                      "capture_complete.txt", "wram_before_open.bin", "wram_open.bin"] + [
    f"open_transition_{name}" for name in (
        "vram.bin", "cgram.bin", "vram_writes.txt", "cgram_writes.txt", "ppu_states.txt")]


RETURN_ARTIFACTS = ["wram_after_back.bin"] + [f"return_transition_{name}" for name in (
    "vram.bin", "cgram.bin", "vram_writes.txt", "cgram_writes.txt", "ppu_states.txt")]


def validate_rules_manifest(manifest, include_return=False, include_dispatch=False):
    if type(manifest.get("schema")) is not int or manifest["schema"] != 2 or manifest.get("menu") != "rules" or \
            manifest.get("kind") != "natural-input frontend journey":
        raise ValueError("Rules capture requires a schema2 natural-input manifest")
    if manifest.get("sources", {}).get("rom", {}).get("sha256") != ROM_SHA256:
        raise ValueError("Rules capture does not identify the verified USA ROM")
    if manifest.get("isolation", {}).get("settings", {}).get("Snes", {}).get("DisableFrameSkipping") is not True:
        raise ValueError("Rules capture must disable native PPU frame skipping")
    result = manifest.get("result", {})
    if type(result.get("exit_code")) is not int or result["exit_code"] != 0 or result.get("native_handler") != "81D318" or \
            result.get("transition_main_values") != [0, 1, 0, 0] or \
            result.get("committed_after_open") != [0, 1, 0, 0]:
        raise ValueError("Rules capture did not reach the matching native menu/configuration")
    for name in ("transition_main_values", "committed_after_open"):
        if any(type(value) is not int for value in result[name]):
            raise ValueError("native configuration fields must be integers")
    artifacts = manifest.get("artifacts", {})
    if type(artifacts.get("schema")) is not int or artifacts["schema"] != 1:
        raise ValueError("Rules capture lacks raw artifact digest attestation")
    for name in REQUIRED_ARTIFACTS + (RETURN_ARTIFACTS if include_return else []) + \
            (["dispatch_ppu_states.txt"] if include_dispatch else []):
        expected = artifacts.get("files", {}).get(name, {})
        if type(expected.get("bytes")) is not int or expected["bytes"] <= 0 or \
                not re.fullmatch(r"[0-9a-f]{64}", expected.get("sha256", "")):
            raise ValueError(f"missing/invalid raw artifact attestation: {name}")
    for name in ("rom", "mesen", "capture", "portable_settings"):
        source = manifest.get("sources", {}).get(name, {})
        if not isinstance(source.get("path"), str) or not source["path"] or \
                not re.fullmatch(r"[0-9a-f]{64}", source.get("sha256", "")):
            raise ValueError(f"missing/invalid native source identity: {name}")
    return manifest


def validate_rules_capture(directory, include_return=False, include_dispatch=False):
    directory = Path(directory)
    manifest = validate_rules_manifest(strict_json(
        (directory / "manifest.json").read_text(encoding="utf-8-sig")), include_return, include_dispatch)
    for name in REQUIRED_ARTIFACTS + (RETURN_ARTIFACTS if include_return else []) + \
            (["dispatch_ppu_states.txt"] if include_dispatch else []):
        raw = (directory / name).read_bytes()
        expected = manifest["artifacts"]["files"][name]
        if expected.get("bytes") != len(raw) or expected.get("sha256") != digest(raw):
            raise ValueError(f"native artifact hash/size mismatch: {name}")
    for name in ("rom", "mesen", "capture", "portable_settings"):
        source = manifest.get("sources", {}).get(name, {})
        path = Path(source.get("path", ""))
        if not path.is_file() or digest(path.read_bytes()) != source.get("sha256"):
            raise ValueError(f"native source hash mismatch: {name}")
    ram = (directory / "wram_before_open.bin").read_bytes()
    if len(ram) != 0x20000 or list(struct.unpack_from("<4H", ram, 0x16FB)) != [0, 1, 0, 0]:
        raise ValueError("native pre-open WRAM does not contain the matching UI settings")
    return manifest


def read_ppu_states(path):
    states = {}
    previous = -1
    for raw in Path(path).read_text(encoding="ascii").splitlines():
        fields = list(map(int, raw.split()))
        if len(fields) != 22 or fields[0] <= previous:
            raise ValueError("native PPU states must have exactly22 fields and unique ascending frames")
        frame, values = fields[0], fields[1:]
        if not 0 <= values[0] <= 15 or any(not 0 <= x <= 31 for x in values[1:3]):
            raise ValueError("invalid native PPU screen state")
        for base in (3, 9, 15):
            h, v, tilemap, chars, wide, tall = values[base:base + 6]
            if not 0 <= h <= 1023 or not 0 <= v <= 1023 or \
                    not 0 <= tilemap <= 32767 or not 0 <= chars <= 32767 or \
                    wide not in (0, 1) or tall not in (0, 1):
                raise ValueError("invalid native PPU layer state")
        states[frame] = values
        previous = frame
    return states


def read_rgb_flags(path):
    flags = {}
    with Path(path).open(newline="", encoding="ascii") as source:
        reader = csv.DictReader(source)
        if reader.fieldnames != ["name", "forced_blank", "brightness", "main", "sub"]:
            raise ValueError("unexpected synchronous RGB-state header")
        for row in reader:
            if None in row or any(value is None for value in row.values()) or row["name"] in flags:
                raise ValueError("duplicate/malformed synchronous RGB-state row")
            values = [int(row[key]) for key in reader.fieldnames[1:]]
            if values[0] not in (0, 1) or not 0 <= values[1] <= 15 or \
                    any(not 0 <= value <= 31 for value in values[2:]):
                raise ValueError("invalid synchronous RGB-state values")
            flags[row["name"]] = dict(zip(reader.fieldnames[1:], values))
    return flags
