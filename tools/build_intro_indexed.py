"""Build indexed EA resources; rendered RGB/PNG files are never read.

This is an interim native-resource extractor, not an emulator wrapper. It uses
attested ROM-produced VRAM/OAM for static graphics whose format-$30 decoder is
still under repair, ROM tilegroups/palettes directly, and verifies the Mode7
character-plane prefix against the existing format-$46 extractor. Its return
length truncates the native11904-byte output to11776; this is not complete
decompressor proof. The full VRAM resource comes from attested native memory.
No per-frame
images or palette-animation sequence is stored in the payload.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from snes65816_decompressor import Snes65816Decompressor
from intro_capture_resources import IntroResources, ROM_SHA256


def sha(data):
    return hashlib.sha256(data).hexdigest()


def offset(address):
    return ((address >> 16) & 127) * 32768 + (address & 32767)


def build(rom_path, native):
    rom = Path(rom_path).read_bytes()
    if len(rom) % 1024 == 512:
        rom = rom[512:]
    if sha(rom) != ROM_SHA256:
        raise ValueError('wrong original ROM')
    native = Path(native)
    capture = IntroResources(native)
    read = capture.read

    base = read('ea_000.vram', 65536)
    final = read('ea_056.vram', 65536)
    cgram = read('ea_000.cgram', 512)
    e_oam = read('ea_023.oam', 544)
    ea_oam = read('ea_056.oam', 544)
    cpu = Snes65816Decompressor(rom)
    chars = cpu.decompress(0x9F, 0xF121, 0x7F, 0)
    if len(chars) != 11776 or chars != base[1:23552:2]:
        raise ValueError('ROM Mode7 characters disagree with native VRAM')
    palettes = b''.join(rom[offset(address):offset(address) + 32]
                        for address in (0xAFF05C, 0xAFF0BC, 0xAFF0DC, 0xAFF0FC))
    for source, destination in ((0, 0x30), (0, 0xB0), (32, 0x40), (32, 0xC0)):
        if palettes[source:source + 32] != cgram[destination * 2:destination * 2 + 32]:
            raise ValueError('ROM EA palette disagrees with native CGRAM')
    a_group = rom[offset(0xADFF46):offset(0xADFF46) + 186]
    sports = rom[offset(0x82F6D8):offset(0x82F6D8) + 96]
    if struct.unpack_from('<HHH', a_group) != (18, 10, 16) or \
            struct.unpack_from('<HHH', sports) != (18, 5, 16):
        raise ValueError('unexpected original tilegroup format')
    payload = (struct.pack('<8s6I', b'NBEAIDX1', 1, 71674, 65536, 4096, 512, 544) +
               base + final[0xC000:0xD000] + cgram + palettes + e_oam + ea_oam +
               a_group + sports)
    if len(payload) != 71674:
        raise ValueError('indexed resource payload size error')
    return payload, {'rom_sha256': ROM_SHA256,
        'native_manifest_sha256': sha((native / 'manifest.json').read_bytes()),
        'source_memory': capture.consumed, 'payload_sha256': sha(payload),
        'mode7_chr_ROM_verified_bytes': len(chars),
        'ROM_tilegroups': ['AD:FF46', '82:F6D8'],
        'ROM_palettes': ['AF:F05C', 'AF:F0BC', 'AF:F0DC', 'AF:F0FC'],
        'native_resource_owners': ['82:F15C-F2EA', '82:F2FE-F332', '82:F37E-F3D8'],
        'exclusions': ['rendered images', 'per-frame palette recordings', 'audio',
                       'independent format-$30 decompression of static OBJ/base map']}


def build_text(rom_path, native):
    """Original font and strings, not a rendering of the legal/license pages.

    $80:FDC2-FDF1 decompresses AF:F2DC and clears palette0 before uploading
    four words. Its eight native CGRAM bytes are the only memory-derived
    resource here; glyphs, row-expansion table and strings are direct ROM data.
    """
    rom = Path(rom_path).read_bytes()
    if len(rom) % 1024 == 512:
        rom = rom[512:]
    if sha(rom) != ROM_SHA256:
        raise ValueError('wrong original ROM')
    capture = IntroResources(native)
    palette = capture.read('license.cgram', 512)[:8]
    if palette != capture.read('legal.cgram', 512)[:8]:
        raise ValueError('license/legal palette producer disagrees')
    font = rom[offset(0xA98000):offset(0xA99000)]
    rows = rom[offset(0x819CAA):offset(0x819D56)]
    strings = []
    for address, limit in ((0x80FCA2, 21), (0x80FCB7, 231)):
        string = rom[offset(address):offset(address)+limit].split(b'\0')[0] + b'\0'
        if len(string) > limit or any(c > 127 for c in string):
            raise ValueError('malformed ROM intro string')
        strings.append(string)
    if font[:6] != bytes.fromhex('10000e000102') or len(rows) != 172:
        raise ValueError('unexpected ROM intro font descriptor')
    payload = struct.pack('<8s8I', b'NBITEXT1', 1, len(font), len(rows), len(palette),
        len(strings[0]), len(strings[1]), 0xA98000, 0xAFF2DC) + font + rows + palette + b''.join(strings)
    return payload, {'rom_sha256': ROM_SHA256,
        'native_manifest_sha256': sha(capture.raw_manifest),
        'source_memory': capture.consumed, 'payload_sha256': sha(payload),
        'ROM_font': 'A9:8000-8FFF', 'ROM_repeat_rows': '81:9CAA-9D55',
        'ROM_strings': ['80:FCA2', '80:FCB7'], 'palette_owner': '80:FDC2-FDF1',
        'exclusions': ['rendered images', 'independent format-$30 palette decompression']}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ('rom', 'native', 'output', 'manifest'):
        parser.add_argument('--' + option, required=True, type=Path)
    args = parser.parse_args()
    data, report = build(args.rom, args.native)
    args.output.write_bytes(data)
    args.manifest.write_text(json.dumps(report, indent=2) + '\n')
    print('indexed EA resources:', len(data), sha(data))


if __name__ == '__main__':
    main()
