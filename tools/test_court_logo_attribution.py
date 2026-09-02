"""Chicago/Orlando team tile streams; full layout proof is in the frame smoke."""

import argparse
import hashlib
import struct
import sys
from pathlib import Path

from extract_assets import Snes65816Decompressor, load_verified_rom


TEAMS = {
    3: ("Chicago", 0xB4A0,
        "eb960a5a6b10705396f8dfc35970142704d9ae8325a5d7135388355ac0675acb"),
    18: ("Orlando", 0xB520,
         "5a2f51d48a8ffb5cdb9b8a865d66e019fb4affae761f80ee0651e50534958fca"),
}


def load_pack(path):
    raw = path.read_bytes()
    if raw[:8] != b"NBA95PAK" or struct.unpack_from("<I", raw, 8)[0] != 31:
        raise AssertionError("expected production pack v31")
    assets = {}
    for index in range(struct.unpack_from("<I", raw, 12)[0]):
        ident, offset, size, width, height, flags = struct.unpack_from(
            "<6I", raw, 16 + index * 24)
        assets[ident] = (raw[offset:offset + size], width, height, flags)
    return assets


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pack", required=True)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--native-root")
    args = parser.parse_args()
    assets = load_pack(Path(args.pack))
    rom = load_verified_rom(args.rom)

    court_map, width, height, address = assets[279]
    if (width, height, address) != (148, 52, 0xA08000) or \
            court_map != rom[0x100000:0x100006 + 148 * 52 * 2]:
        raise AssertionError("production court map differs from $A0:8000-$BC25")

    states, width, height, teams = assets[284]
    state_size = 0x10000 + 0x200
    if states[:8] != b"NBPPUIN1" or (width, height, teams) != (0x10000, 0x200, 29):
        raise AssertionError("production indexed court inputs changed")

    native_root = Path(args.native_root) if args.native_root else None
    for team, (name, destination, expected_hash) in TEAMS.items():
        graphics_entry = 0x266B5 + team * 4
        source = rom[graphics_entry] | (rom[graphics_entry + 1] << 8)
        bank = rom[graphics_entry + 2]
        decompressor = Snes65816Decompressor(rom)
        decompressor.decompress(bank, source, 0x7E, 0x8FEE)
        source_tiles = bytes(decompressor.wram[0x8FEE:0x98AE])

        state_offset = 24 + team * state_size
        packed_vram = states[state_offset:state_offset + 0x10000]
        packed_cgram = states[state_offset + 0x10000:state_offset + state_size]
        packed_tiles = packed_vram[destination:destination + len(source_tiles)]
        if source_tiles != packed_tiles or \
                hashlib.sha256(packed_tiles).hexdigest() != expected_hash:
            raise AssertionError(f"{name} logo/floor tiles differ from ROM source")

        palette_entry = 0x265BD + team * 4
        palette_address = rom[palette_entry] | (rom[palette_entry + 1] << 8)
        palette_bank = rom[palette_entry + 2]
        palette_offset = ((palette_bank & 0x7F) * 0x8000 +
                          (palette_address & 0x7FFF))
        palette = rom[palette_offset:palette_offset + 0xD6]
        for start, end, source_start, source_end in (
                (0, 2, 0, 2), (34, 38, 2, 6),
                (64, 192, 0x20, 0xA0), (240, 246, 0xD0, 0xD6)):
            if packed_cgram[start:end] != palette[source_start:source_end]:
                raise AssertionError(f"{name} court palette differs from ROM source")

        if native_root:
            directory = native_root / f"team_{team:02d}"
            native_vram = (directory / "court_vram.bin").read_bytes()
            native_cgram = (directory / "court_cgram.bin").read_bytes()
            if native_vram[destination:destination + len(source_tiles)] != packed_tiles:
                raise AssertionError(f"{name} packed tiles differ from native PPU")
            for start, end, _, _ in (
                    (0, 2, 0, 2), (34, 38, 2, 6),
                    (64, 192, 0x20, 0xA0), (240, 246, 0xD0, 0xD6)):
                if native_cgram[start:end] != packed_cgram[start:end]:
                    raise AssertionError(f"{name} packed palette differs from native PPU")

        print(f"[COURT LOGO] {name}: ROM/native/pack tiles {expected_hash[:16]} PASS")

    print("[TEST] PASS: Chicago/Orlando team tile streams match ROM; layout checked separately")


if __name__ == "__main__":
    main()
