"""Rebuild the court catalogs and append the missing standard ROM map.

All unrelated payloads stay byte-identical. Inputs are the ROM and existing
raw PPU captures; rendered screenshots are never packed.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from extract_assets import build_gameplay_home_court_catalog, load_verified_rom
from upgrade_gameplay_hud_pack import unpack


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("base-pack", "rom", "capture-root", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    out = args.output.resolve()
    if out.exists():
        raise ValueError("output must be new")
    raw = args.base_pack.read_bytes()
    records = unpack(raw)
    assets = {item[0]: item for item in records}
    if 288 in assets or len(records) != 265:
        raise ValueError("expected the 265-resource preview pack")
    rom = load_verified_rom(args.rom)
    captures = args.capture_root.resolve()
    original = captures / "camera-source-20260823"
    home = captures / "player_intro_portraits_verified_20260823"
    inputs = [original / ("tipoff_0140_" + kind + ".bin") for kind in ("vram", "cgram")]
    for team in range(29):
        inputs.append(home / f"team_{team:02d}" / "slot_0_vram.bin")
    states = assets[284][4]
    offset = 24 + 18 * 0x10200
    rebuilt = build_gameplay_home_court_catalog(
        rom, inputs[0].read_bytes(), inputs[1].read_bytes(), str(home), assets[263][4],
        states[offset:offset + 0x10000], states[offset + 0x10000:offset + 0x10200])
    replacements = dict(zip((272, 273, 284), rebuilt))
    for ident, stride in ((272, 256 * 224 * 4), (273, 1184 * 416 * 4), (284, 0x10200)):
        start, end = 24 + 18 * stride, 24 + 19 * stride
        if replacements[ident][start:end] != assets[ident][4][start:end]:
            raise AssertionError(f"Orlando baseline changed in asset {ident}")
    updated = [(ident, w, h, flags, replacements.get(ident, payload))
               for ident, w, h, flags, payload in records]
    # The map includes its six-byte header and exactly 148*52 words.
    updated.append((288, 148, 52, 0xA0BC26,
                    rom[0x103C26:0x103C26 + 6 + 148 * 52 * 2]))
    cursor = 16 + len(updated) * 24
    directory = bytearray(b"NBA95PAK" + struct.pack("<II", 31, len(updated)))
    payloads = bytearray()
    for ident, w, h, flags, payload in updated:
        directory.extend(struct.pack("<6I", ident, cursor, len(payload), w, h, flags))
        payloads.extend(payload)
        cursor += len(payload)
    result = bytes(directory + payloads)
    if unpack(result) != updated:
        raise AssertionError("pack serialization changed payloads")
    changed = [ident for ident, w, h, flags, payload in updated
               if ident not in assets or assets[ident] != (ident, w, h, flags, payload)]
    if changed != [272, 273, 284, 288]:
        raise AssertionError(f"unexpected changed resources: {changed}")
    receipt = {
        "rom_sha256": sha(rom), "base_sha256": sha(raw), "output_sha256": sha(result),
        "changed_assets": changed, "preserved_assets": len(records) - 3,
        "inputs": {str(path): sha(path.read_bytes()) for path in inputs},
        "sources": {str(path): sha(path.read_bytes()) for path in
                    (Path(__file__), Path(__file__).with_name("extract_assets.py"))}}
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("xb") as stream:
        stream.write(result)
    out.with_suffix(".receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps({"output": str(out), "sha256": sha(result), "changed_assets": changed}))


if __name__ == "__main__":
    main()
