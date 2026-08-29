"""Exact-ROM regressions for gameplay formation and play-control graphs."""

import argparse
import hashlib
import struct
import subprocess
import tempfile
from pathlib import Path

from extract_assets import (build_cpu_gameplay_ai_asset,
                            build_gameplay_formation_asset,
                            build_gameplay_play_control_asset,
                            load_verified_rom)


def pack_asset(path, wanted):
    raw = Path(path).read_bytes()
    if raw[:8] != b"NBA95PAK":
        raise AssertionError("invalid asset-pack magic")
    version, count = struct.unpack_from("<II", raw, 8)
    if version != 31:
        raise AssertionError(f"gameplay graphs require pack v31, got {version}")
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


def transformed(pair, play, mirror_y=False, side_anchor_x=80):
    x, y = pair
    if mirror_y:
        y = -y
    if side_anchor_x < 0:
        x = -x
        if play >= 0x0E:
            y = -y
    wrap = lambda value: (value + 0x8000) % 0x10000 - 0x8000
    return wrap(x), wrap(y)


def control_records(payload, play):
    entry = 36 + play * 8
    pointer, count, offset = struct.unpack_from("<HHI", payload, entry)
    records = [struct.unpack_from("<hhhh", payload, offset + index * 8)
               for index in range(count)]
    return pointer, records


def write_subset(path, payload, corrupt=False):
    data = bytearray(payload)
    if corrupt:
        struct.pack_into("<H", data, 48 + 2, 4)  # play 0 count must be three
    offset = 40
    raw = (b"NBA95PAK" + struct.pack("<II", 31, 1) +
           struct.pack("<6I", 274, offset, len(data), 61, 5, 1595) + data)
    path.write_bytes(raw)


def write_control_subset(path, payload, corrupt=False):
    data = bytearray(payload)
    if corrupt:
        struct.pack_into("<H", data, 36 + 2, 4)  # play 0 count must be three
    offset = 40
    raw = (b"NBA95PAK" + struct.pack("<II", 31, 1) +
           struct.pack("<6I", 275, offset, len(data), 61, 320, 0x85C6AF) +
           data)
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
    if transformed(play_10[0][0], 0x10, False, 80) != (320, -120) or \
       transformed(play_10[0][0], 0x10, False, -80) != (-320, 120):
        raise AssertionError("formation mirror no longer follows team-context anchor")

    control, control_metadata = pack_asset(args.pack, 275)
    if control_metadata != (61, 320, 0x85C6AF) or len(control) != 3084:
        raise AssertionError(
            f"invalid play-control metadata: {control_metadata}, {len(control)}")
    control_header = struct.unpack_from("<8s6IH2x", control)
    if control_header != (b"NBPLAY1\0", 1, 61, 320, 36, 524, 2560, 0xC6AF):
        raise AssertionError(f"play-control header changed: {control_header}")
    expected_control = build_gameplay_play_control_asset(
        load_verified_rom(args.rom))
    if control != expected_control:
        raise AssertionError("packed play-control graph differs from verified ROM")
    control_digest = hashlib.sha256(control[524:]).hexdigest()
    if control_digest != \
            "5d0775922793118a8e8d0b2b3c5e3a074d09f6a511742822111d057880aa48bd":
        raise AssertionError(f"play-control checksum changed: {control_digest}")
    expected_streams = {
        0x35: [(-1,4,2,-1),(-1,4,2,-1),(-1,3,2,-1),(-1,1,2,-1)],
        0x01: [(120,3,4,-1),(100,4,3,-1),(100,3,4,-1)],
        0x0F: [(-1,-1,-1,-1),(-1,-1,-1,-1),(-1,3,-1,-1),(-1,1,0,-1)],
        0x26: [(-1,4,3,-1),(-1,4,1,-1),(-1,4,1,-1),(-1,4,1,-1),
               (120,3,2,1)],
    }
    for play, expected_stream in expected_streams.items():
        pointer, actual_stream = control_records(control, play)
        if pointer < 0xC9A7 or actual_stream != expected_stream:
            raise AssertionError(
                f"play-control stream changed play=${play:02X}: "
                f"${pointer:04X} {actual_stream}")

    cpu_tables, cpu_metadata = pack_asset(args.pack, 276)
    if cpu_metadata != (29, 7, 0x85C661) or len(cpu_tables) != 246:
        raise AssertionError(f"invalid CPU table metadata: {cpu_metadata}")
    cpu_header = struct.unpack_from("<8s9I", cpu_tables)
    if cpu_header != (b"NBCAI1\0\0", 1, 29, 7, 3, 6, 44, 102, 130, 238):
        raise AssertionError(f"CPU table header changed: {cpu_header}")
    expected_cpu = build_cpu_gameplay_ai_asset(load_verified_rom(args.rom))
    if cpu_tables != expected_cpu:
        raise AssertionError("packed CPU tables differ from verified ROM")
    if hashlib.sha256(cpu_tables[130:238]).hexdigest() != \
            "e8b2d2ec179a286a66e52707957c418a9463ba0edc4d87d28779bcfd4431071e":
        raise AssertionError("pass launch table checksum changed")
    ranges = [struct.unpack_from("<HH", cpu_tables, 102 + i * 4)
              for i in range(7)]
    if ranges != [(0x1D,6),(0x18,5),(0x12,6),(0x2C,7),
                  (0x27,5),(0x23,4),(0x33,5)]:
        raise AssertionError(f"$85:C729 strategy ranges changed: {ranges}")
    pass_records = [struct.unpack_from("<hhh", cpu_tables, 130 + i * 6)
                    for i in range(18)]
    if pass_records[0] != (16,192,40) or \
       pass_records[6] != (20,240,64) or \
       pass_records[-1] != (40,0,32) or \
       cpu_tables[238:246] != bytes((2,1,2,1,1,1,1,1)):
        raise AssertionError("$86:9C6F/$A7A0 pass vectors changed")

    with tempfile.TemporaryDirectory() as directory:
        valid = Path(directory) / "formation.pak"
        invalid = Path(directory) / "bad-formation.pak"
        valid_control = Path(directory) / "play-control.pak"
        invalid_control = Path(directory) / "bad-play-control.pak"
        write_subset(valid, payload)
        write_subset(invalid, payload, corrupt=True)
        write_control_subset(valid_control, control)
        write_control_subset(invalid_control, control, corrupt=True)
        base = [str(args.exe), "--headless", "--frames", "0", "--assets"]
        good = subprocess.run(base + [str(valid)], capture_output=True, text=True)
        bad = subprocess.run(base + [str(invalid)], capture_output=True, text=True)
        good_control = subprocess.run(
            base + [str(valid_control)], capture_output=True, text=True)
        bad_control = subprocess.run(
            base + [str(invalid_control)], capture_output=True, text=True)
        if good.returncode:
            raise AssertionError(good.stdout + good.stderr)
        if bad.returncode == 0 or "formation graph is invalid" not in bad.stderr:
            raise AssertionError("runtime accepted a malformed formation graph")
        if good_control.returncode:
            raise AssertionError(good_control.stdout + good_control.stderr)
        if bad_control.returncode == 0 or \
                "play-control graph is invalid" not in bad_control.stderr:
            raise AssertionError("runtime accepted a malformed play-control graph")
    print("[GAMEPLAY GRAPH TEST] PASS: formation/control roots, streams, checksums, parsers")


if __name__ == "__main__":
    main()
