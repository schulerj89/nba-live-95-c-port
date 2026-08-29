"""Integrity and native-byte gates for Bank $80 gameplay resource publication."""

import argparse
import hashlib
import struct
from pathlib import Path


EXPECTED_SHA256 = {
    256: "64f0c37bc0b7dde634b1f5366044d74a2fa5369d9208cea91e63061183618de6",
    281: "29448386c66d1dfd0ffab649e0a3b15631db0007ce82160e8797c508052451c9",
    282: "4502fac6951baa8727a4c9e0c851608db07845ffd827221d7cfad9ce5b488834",
    284: "cdbf0dfc99162d3703d0a2532444a8fe89bc307aa0ae42a0b550c398f82de459",
}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pack", required=True)
    parser.add_argument("--native-dir")
    args = parser.parse_args()
    raw = Path(args.pack).read_bytes()
    assert raw[:8] == b"NBA95PAK"
    assert struct.unpack_from("<I", raw, 8)[0] == 31
    count = struct.unpack_from("<I", raw, 12)[0]
    assert 0 < count <= 285
    directory_end = 16 + count * 24
    entries = {}
    occupied = []
    for index in range(count):
        item = struct.unpack_from("<6I", raw, 16 + index * 24)
        asset_id, offset, size, _, _, _ = item
        assert asset_id not in entries, f"duplicate asset {asset_id}"
        assert offset >= directory_end and offset + size <= len(raw)
        entries[asset_id] = item
        occupied.append((offset, offset + size, asset_id))
    for previous, current in zip(sorted(occupied), sorted(occupied)[1:]):
        assert previous[1] <= current[0], (
            f"assets {previous[2]} and {current[2]} overlap")

    for asset_id, expected in EXPECTED_SHA256.items():
        _, offset, size, _, _, _ = entries[asset_id]
        actual = hashlib.sha256(raw[offset:offset + size]).hexdigest()
        assert actual == expected, f"asset {asset_id} native payload changed"

    # NBPPUIN1 retains the exact 64 KiB VRAM + 512-byte CGRAM state for each
    # home court. The Orlando/native frame-989 witness is team ID 18.
    _, offset, size, width, height, flags = entries[284]
    payload = raw[offset:offset + size]
    assert payload[:8] == b"NBPPUIN1" and (width, height, flags) == (
        0x10000, 0x200, 29)
    assert struct.unpack_from("<4I", payload, 8) == (1, 29, 0x10000, 0x200)
    stride = 0x10200
    states = [payload[24 + team * stride:24 + (team + 1) * stride]
              for team in range(29)]
    assert len({hashlib.sha256(state).digest() for state in states}) >= 24

    if args.native_dir:
        native = Path(args.native_dir)
        vram = (native / "scanout_0989_vram.bin").read_bytes()
        cgram = (native / "scanout_0989_cgram.bin").read_bytes()
        assert states[18] == vram + cgram, (
            "team-18 asset-pack PPU state differs from native frame 989")

    print("gameplay55 asset publication PASS: directory, hashes, 29 PPU states")


if __name__ == "__main__":
    main()
