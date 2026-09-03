"""Extract the original out-of-bounds strings, preserving every existing asset."""
import argparse
import json
from pathlib import Path
import struct

from regenerate_oob_reference import ROM_SHA256, sha
from upgrade_gameplay_hud_pack import unpack


def build(rom):
    import hashlib
    if hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("expected verified USA ROM")

    def offset(address):
        return (address >> 16 & 127) * 32768 + (address & 32767)

    def pointer(table, index):
        return struct.unpack_from("<H", rom, offset(table) + index * 2)[0]

    addresses = [0x850000 | pointer(0x85945F, 2), 0x83DB9D]
    addresses += [0x800000 | pointer(0x80D350 if i < 27 else 0x80D0E2, i)
                  for i in range(29)]
    strings = []
    for address in addresses:
        start = offset(address)
        end = rom.index(0, start, start + 32)
        strings.append(rom[start:end + 1].ljust(32, b"\0"))
    if strings[0].rstrip(b"\0") != b"Out Of Bounds" or strings[1].rstrip(b"\0") != b"Ball":
        raise ValueError("original string pointers changed")
    return struct.pack("<8sII", b"NBOOB001", 1, 31) + b"".join(strings), addresses


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("rom", "base-pack", "output", "manifest"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists() or args.manifest.exists():
        raise ValueError("output and manifest must be new")
    payload, addresses = build(args.rom.read_bytes())
    old = unpack(args.base_pack.read_bytes())
    if any(r[0] == 289 for r in old):
        raise ValueError("OOB resource already exists")
    records = old + [(289, 0, 0, 0, payload)]
    cursor = 16 + len(records) * 24
    result = bytearray(b"NBA95PAK" + struct.pack("<II", 31, len(records)))
    for key, width, height, flags, data in records:
        result += struct.pack("<6I", key, cursor, len(data), width, height, flags)
        cursor += len(data)
    result += b"".join(r[4] for r in records)
    if unpack(result)[:-1] != old:
        raise AssertionError("existing assets changed")
    args.output.write_bytes(result)
    report = {"rom_sha256": ROM_SHA256, "base_sha256": sha(args.base_pack),
              "output_sha256": sha(args.output), "preserved_assets": len(old),
              "new_asset": 289, "payload_bytes": len(payload),
              "source_addresses": [f"{a:06X}" for a in addresses]}
    args.manifest.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report))


if __name__ == "__main__":
    main()
