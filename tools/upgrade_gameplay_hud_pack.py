"""Add verified original HUD resources to an existing pack without replacing it.

Every existing payload and its dimensions/flags remain identical. The pack's
directory offsets change because one entry is appended. Native input must pass
the same provenance checks used by the full ROM extractor; no screenshots are
packed. Output and receipt paths must be new.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from build_gameplay_hud_lifecycle_assets import build


def sha(data):
    return hashlib.sha256(data).hexdigest()


def unpack(raw):
    if len(raw) < 16 or raw[:8] != b"NBA95PAK":
        raise ValueError("invalid asset pack header")
    version, count = struct.unpack_from("<II", raw, 8)
    if version != 31 or not count or count > (len(raw) - 16) // 24:
        raise ValueError("invalid asset pack version/directory")
    cursor = 16 + count * 24
    records, seen = [], set()
    for index in range(count):
        asset_id, offset, size, width, height, flags = struct.unpack_from(
            "<6I", raw, 16 + index * 24)
        if asset_id in seen or offset != cursor or size > len(raw) - cursor:
            raise ValueError("duplicate, overlapping or truncated asset entry")
        seen.add(asset_id)
        records.append((asset_id, width, height, flags, raw[cursor:cursor + size]))
        cursor += size
    if cursor != len(raw):
        raise ValueError("asset pack has unclaimed trailing data")
    return records


def append_hud(raw, hud):
    records = unpack(raw)
    if any(record[0] == 286 for record in records):
        raise ValueError("base pack already contains HUD resource286; use a fresh full extraction")
    updated = records + [(286, 0, 0, 0, hud)]
    cursor = 16 + len(updated) * 24
    directory = bytearray(b"NBA95PAK" + struct.pack("<II", 31, len(updated)))
    payloads = bytearray()
    for asset_id, width, height, flags, payload in updated:
        directory += struct.pack("<6I", asset_id, cursor, len(payload), width, height, flags)
        payloads += payload
        cursor += len(payload)
    result = bytes(directory + payloads)
    if unpack(result) != updated:
        raise ValueError("pack serialization changed payload or metadata")
    return result, records


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("base-pack", "rom", "native", "output", "manifest"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    paths = [args.base_pack.resolve(), args.rom.resolve(),
             args.output.resolve(), args.manifest.resolve()]
    if len(set(paths)) != len(paths):
        raise ValueError("input, output and receipt paths must be distinct")
    if args.output.exists() or args.manifest.exists():
        raise ValueError("output and receipt must be new; existing files are never replaced")
    raw = args.base_pack.read_bytes()
    unpack(raw)
    rom = args.rom.read_bytes()
    if len(rom) % 1024 == 512:
        rom = rom[512:]
    hud, provenance = build(rom, args.native)
    result, records = append_hud(raw, hud)
    receipt = {
        "schema": 1,
        "base_pack": str(args.base_pack.resolve()),
        "base_sha256": sha(raw),
        "output_sha256": sha(result),
        "preserved_assets": [
            {"id": key, "width": width, "height": height, "flags": flags,
             "bytes": len(payload), "sha256": sha(payload)}
            for key, width, height, flags, payload in records],
        "new_asset": {"id": 286, "bytes": len(hud), "sha256": sha(hud)},
        "hud_provenance": provenance,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.manifest.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("xb") as stream:
        stream.write(result)
    with args.manifest.open("x", encoding="utf-8") as stream:
        json.dump(receipt, stream, indent=2)
        stream.write("\n")
    print(f"Preserved {len(records)} assets; added resource286; SHA256 {sha(result)}")


if __name__ == "__main__":
    main()
