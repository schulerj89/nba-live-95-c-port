"""Add the original 87:BA9F shot-clock maps to reviewed indexed HUD resources.

Initial CHR remains an attested decompressor output, not rendered pixels. The
legacy format30 decoder does not close that source route; do not silently use
its partial output. Every new shot-clock map is read directly from pinned ROM.
"""
import argparse
import json
from pathlib import Path
import struct
from build_gameplay_hud_assets import build as build_base, offset, sha


def build(rom, native):
    previous, report = build_base(rom, native)
    sections = []
    for i in range(9):
        start, size = struct.unpack_from('<II', previous, 16+8*i)
        sections.append(previous[start:start+size])
    maps = bytearray()
    addresses = []
    for i in range(11):
        address = struct.unpack_from('<I', rom, offset(0x87BA9F)+4*i)[0]
        start = offset(address)
        if address >> 16 != 0xAF or struct.unpack_from('<3H', rom, start) != (4, 4, 0x304):
            raise ValueError('unexpected original shot-clock map')
        maps += rom[start:start+38]
        addresses.append(f'{address:06X}')
    sections.append(bytes(maps))
    result = bytearray(struct.pack('<8sII', b'NBHUD001', 2, 10))
    cursor = 96
    for section in sections:
        result += struct.pack('<II', cursor, len(section))
        cursor += len(section)
    result += b''.join(sections)
    report.update(base_payload_sha256=sha(previous), payload_sha256=sha(result),
                  payload_bytes=len(result), version=2, shot_clock_ROM_sources=addresses)
    return bytes(result), report


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('rom', 'native', 'output', 'manifest'):
        p.add_argument('--'+name, required=True, type=Path)
    a = p.parse_args()
    payload, report = build(a.rom.read_bytes(), a.native)
    a.output.write_bytes(payload)
    a.manifest.write_text(json.dumps(report, indent=2)+'\n')
    print('HUD lifecycle resource:', len(payload), sha(payload))
