"""Exact-ROM regression vectors for the `$85:AD6B` formation graph."""

import argparse
import hashlib
import struct
import subprocess
import tempfile
from pathlib import Path

from extract_assets import build_gameplay_formation_asset, load_verified_rom


def pack_asset(path, wanted):
    raw = Path(path).read_bytes()
    if raw[:8] != b"NBA95PAK":
        raise AssertionError("invalid asset-pack magic")
    version, count = struct.unpack_from("<II", raw, 8)
    if version != 24:
        raise AssertionError(f"formation graph requires pack v24, got {version}")
    for index in range(count):
        asset_id, offset, size, width, height, flags = struct.unpack_from(
            "<6I", raw, 16 + index * 24)
        if asset_id == wanted:
            return raw[offset:offset + size], (width, height, flags)
    raise AssertionError(f"missing asset {wanted}")


def coordinates(payload, play, role):
    entry = 48 + (play * 5 + role) * 8
    pointer, count, offset = struct.unpack_from("<HHI", payload, entry)
    return pointer, [struct.unpack_from("<hh", payload, offset + index * 4)
                     for index in range(count)]


def transformed(pair, play, mirror_y=False, ball_x=0):
    x, y = pair
    if mirror_y:
        y = -y
    if ball_x < 0:
        x = -x
        if play >= 0x0E:
            y = -y
    wrap = lambda value: (value + 0x8000) % 0x10000 - 0x8000
    return wrap(x), wrap(y)


def write_subset(path, payload, corrupt=False):
    data = bytearray(payload)
    if corrupt:
        struct.pack_into("<H", data, 48 + 2, 4)  # play 0 count must be three
    offset = 40
    raw = (b"NBA95PAK" + struct.pack("<II", 24, 1) +
           struct.pack("<6I", 274, offset, len(data), 61, 5, 1595) + data)
    path.write_bytes(raw)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pack", required=True)
    parser.add_argument("--exe", required=True)
    parser.add_argument("--rom", required=True)
    args = parser.parse_args()

    payload, metadata = pack_asset(args.pack, 274)
    if metadata != (61, 5, 1595) or len(payload) != 8868:
        raise AssertionError(f"invalid formation metadata: {metadata}, {len(payload)}")
    header = struct.unpack_from("<8s7I5H2x", payload)
    if header != (b"NBFORM1\0", 1, 61, 5, 1595, 48, 2488, 6380,
                  0xC745, 0xC7BF, 0xC839, 0xC8B3, 0xC92D):
        raise AssertionError(f"formation header changed: {header}")
    expected = build_gameplay_formation_asset(load_verified_rom(args.rom))
    if payload != expected:
        raise AssertionError("packed formation graph differs from verified ROM")
    digest = hashlib.sha256(payload[2488:]).hexdigest()
    if digest != "e6c71d3e45e12c1f5bf691a23f7d952e6f989798d78024d77518ac7f7437941c":
        raise AssertionError(f"formation coordinate checksum changed: {digest}")

    play_01 = (
        [(80, 0), (40, -40), (32, 40)],
        [(40, 128), (80, 168), (48, 152)],
        [(24, -152), (8, -128), (-24, -168)],
        [(-336, 96), (-328, -64), (-280, 48)],
        [(-328, -88), (-336, 64), (-344, -96)],
    )
    play_10 = (
        [(320, -120), (296, -176), (296, -200), (296, -176)],
        [(216, -80), (240, 0), (240, 0), (240, 0)],
        [(384, -56), (336, 56), (336, 72), (336, 56)],
        [(320, 24), (304, 160), (304, 176), (304, 160)],
        [(96, 24), (240, 8), (176, -48), (176, -48)],
    )
    for play, vectors in ((0x01, play_01), (0x10, play_10)):
        pointers = []
        for role, vector in enumerate(vectors):
            pointer, actual = coordinates(payload, play, role)
            pointers.append(pointer)
            if actual != vector:
                raise AssertionError(
                    f"formation vector changed play=${play:02X} role={role}: {actual}")
        if any(abs(b - a) // 4 != len(vectors[0]) or abs(b - a) % 4
               for a, b in zip(pointers, pointers[1:])):
            raise AssertionError(f"formation pointer spans changed play=${play:02X}")
    if transformed(play_01[0][0], 0x01, True, -1) != (-80, 0) or \
       transformed(play_10[0][0], 0x10, True, -1) != (-320, -120):
        raise AssertionError("$85:AD6B mirror transform changed")

    with tempfile.TemporaryDirectory() as directory:
        valid = Path(directory) / "formation.pak"
        invalid = Path(directory) / "bad-formation.pak"
        write_subset(valid, payload)
        write_subset(invalid, payload, corrupt=True)
        base = [str(args.exe), "--headless", "--frames", "0", "--assets"]
        good = subprocess.run(base + [str(valid)], capture_output=True, text=True)
        bad = subprocess.run(base + [str(invalid)], capture_output=True, text=True)
        if good.returncode:
            raise AssertionError(good.stdout + good.stderr)
        if bad.returncode == 0 or "formation graph is invalid" not in bad.stderr:
            raise AssertionError("runtime accepted a malformed formation graph")
    print("[FORMATION ASSET TEST] PASS: roots, pointers, counts, vectors, checksum, parser")


if __name__ == "__main__":
    main()
