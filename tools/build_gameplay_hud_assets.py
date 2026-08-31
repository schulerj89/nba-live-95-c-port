"""Pack native HUD resources; no rendered images, guessed text or palettes.

Strings, expansion table, clock font, palette and tilemaps are direct ROM
bytes. The first63 2bpp tiles are attested native outputs of $87:B99A's three
format-$30 decompressions; independent format-$30 decoding remains pending.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct

from capture_gameplay_hud import ROM_SHA, MESEN_SHA

NATIVE_CAPTURE_SCRIPT_SHA = '843da3d0751d3da9b172c02ae5aacae558e23552e7cec1f2d16af1dc9a24af51'
NATIVE_CAPTURE_RUNNER_SHA = '0117244bd698caa8d81e8f07b511c307dd50c971ffb951a900f2a997b1b93ccf'

def sha(data):
    return hashlib.sha256(data).hexdigest()


def strict_json(text):
    def pairs(items):
        result = {}
        for key, value in items:
            if key in result:
                raise ValueError('duplicate JSON key: ' + key)
            result[key] = value
        return result
    return json.loads(text, object_pairs_hook=pairs)


def offset(address):
    return ((address >> 16) & 127)*32768 + (address & 32767)


def validate_native(native):
    native = Path(native)
    manifest = strict_json((native/'manifest.json').read_text(encoding='utf-8-sig'))
    if type(manifest.get('schema')) is not int or manifest['schema'] != 2 or manifest.get('kind') != 'natural neutral-controller CPU match first HUD publisher':
        raise ValueError('unsupported native HUD provenance schema')
    if manifest.get('accepted_capture') is not True or manifest.get('state_injection') is not False or manifest.get('rom_patch') is not False:
        raise ValueError('HUD resources require an accepted natural unpatched capture')
    if type(manifest.get('exit_code')) is not int or manifest['exit_code'] != 0 or type(manifest.get('selection')) is not int or manifest['selection'] != 1:
        raise ValueError('native HUD journey did not complete with neutral controllers')
    if manifest.get('rom_sha256') != ROM_SHA or manifest.get('mesen_sha256') != MESEN_SHA:
        raise ValueError('native HUD source version mismatch')
    isolation = manifest.get('isolation', {})
    if isolation.get('initial_saves') != [] or isolation.get('post_settings_verified') is not True:
        raise ValueError('native HUD launch isolation is incomplete')
    settings = isolation.get('settings', {})
    checks = [('Preferences', 'SingleInstance', False), ('Preferences', 'AutoLoadPatches', False),
              ('Preferences', 'OverrideSaveDataFolder', True), ('Snes', 'EnableRandomPowerOnState', False),
              ('Snes', 'RamPowerOnState', 'AllZeros'), ('Snes', 'Port1', {'Type':'SnesController'}),
              ('Snes', 'Port2', {'Type':'None'})]
    for group, key, expected in checks:
        value = settings.get(group, {}).get(key)
        if type(value) is not type(expected) or value != expected:
            raise ValueError('native HUD launch setting mismatch: '+group+'.'+key)
    required = ('capture.lua', 'capture_runner.py', 'mesen_portable.py', 'initial-mesen-settings.json',
                'observed-script-data-folder.txt', 'capture_complete.txt', 'hud.jsonl',
                'first_court.wram', 'first_court.vram', 'first_court.cgram')
    consumed = {}
    for name in required:
        data = (native/name).read_bytes()
        record = manifest.get('artifacts', {}).get(name, {})
        if set(record) != {'size', 'sha256'} or type(record['size']) is not int or record['size'] != len(data) or not isinstance(record['sha256'], str) or not re.fullmatch('[0-9a-f]{64}', record['sha256']) or record['sha256'] != sha(data):
            raise ValueError('native HUD artifact identity mismatch: '+name)
        consumed[name] = record
    if manifest.get('script_sha256') != consumed['capture.lua']['sha256'] or isolation.get('initial_settings_sha256') != consumed['initial-mesen-settings.json']['sha256']:
        raise ValueError('executed script/settings differ from captured artifacts')
    if consumed['capture.lua']['sha256'] != NATIVE_CAPTURE_SCRIPT_SHA or \
            consumed['capture_runner.py']['sha256'] != NATIVE_CAPTURE_RUNNER_SHA:
        raise ValueError('native HUD production resources require the reviewed natural capture script/runner')
    if strict_json((native/'initial-mesen-settings.json').read_text()) != settings:
        raise ValueError('native HUD settings attestation differs')
    observed = (native/'observed-script-data-folder.txt').read_text().strip()
    if observed != isolation.get('observed_script_data_folder') or not Path(observed).resolve().is_relative_to(Path(isolation['home']).resolve()):
        raise ValueError('native HUD script used another Mesen home')
    if sha((Path(isolation['home'])/'Mesen.exe').read_bytes()) != MESEN_SHA:
        raise ValueError('native HUD executed Mesen identity differs')
    if manifest.get('summary') != (native/'capture_complete.txt').read_text() or not manifest['summary'].startswith('first natural HUD publication observed; court='):
        raise ValueError('native HUD completion summary differs')
    rows = [strict_json(line) for line in (native/'hud.jsonl').read_text().splitlines()]
    first = [row for row in rows if row.get('tag') == 'first_court']
    if len(first) != 1 or first[0].get('pc') != 0x87A47A or first[0].get('court') != 0:
        raise ValueError('native first-court owner was not observed')
    for pc in (0x83CE36, 0x83D0AD, 0x83D157, 0x83D1B1, 0x83D1FD, 0x83D2E0):
        if not any(row.get('tag') == 'publisher' and row.get('pc') == pc for row in rows):
            raise ValueError('native HUD production caller not reached: '+hex(pc))
    return manifest, consumed


def build(rom, native):
    if sha(rom) != ROM_SHA:
        raise ValueError('wrong original ROM identity')
    manifest, consumed = validate_native(native)
    native = Path(native)
    vram = (native/'first_court.vram').read_bytes()
    cgram = (native/'first_court.cgram').read_bytes()
    wram = (native/'first_court.wram').read_bytes()
    if len(vram) != 65536 or len(cgram) != 512 or len(wram) != 131072 or any(vram[0x800:0xF00]):
        raise ValueError('initial native HUD buffers are incomplete or already populated')
    if int.from_bytes(wram[0x166D:0x166F], 'little') != 1:
        raise ValueError('initial controller selection is not neutral')
    palette = rom[offset(0xAFEF5E):offset(0xAFEF5E)+30]
    if palette != cgram[2:32]:
        raise ValueError('native HUD palette disagrees with direct ROM source')
    def strings(table, bank, count):
        result = bytearray()
        for i in range(count):
            pointer = struct.unpack_from('<H', rom, offset(table)+2*i)[0]
            if pointer < 0x8000:
                raise ValueError('ROM HUD string pointer outside LoROM')
            start = offset(bank << 16 | pointer)
            end = rom.index(0, start, start+32)
            result += rom[start:end+1].ljust(32, b'\0')
        return bytes(result)
    def tilemap(address, dimensions):
        start = offset(address)
        if struct.unpack_from('<3H', rom, start) != (*dimensions, 0x304):
            raise ValueError('unexpected native HUD tilemap format')
        return rom[start:start+6+dimensions[0]*dimensions[1]*2]
    sections = [vram[0x2000:0x23F0], palette,
                rom[offset(0x819CAA):offset(0x819CAA)+172],
                strings(0x80D0E2, 0x80, 29), strings(0x83D250, 0x83, 9),
                rom[offset(0xADF8C5):offset(0xADF8C5)+838],
                tilemap(0xA3FFAC, (6, 5)), tilemap(0xAFEB5F, (19, 1)),
                tilemap(0xAFEC5D, (4, 4))]
    header = bytearray(struct.pack('<8sII', b'NBHUD001', 1, len(sections)))
    cursor = 16+len(sections)*8
    for section in sections:
        header += struct.pack('<II', cursor, len(section))
        cursor += len(section)
    data = bytes(header)+b''.join(sections)
    return data, dict(rom_sha256=ROM_SHA, native_manifest_sha256=sha((native/'manifest.json').read_bytes()),
                     source_artifacts=consumed, payload_sha256=sha(data), payload_bytes=len(data),
                     ROM_sources=['81:9CAA-9D55', '80:D0E2/D126-D226', '83:D250-D2DF',
                                  'AD:F8C5-FC0A', 'AF:EF5E-EF7B', 'A3:FFAC', 'AF:EB5F', 'AF:EC5D'],
                     native_resource_owners=['87:B99A-BA53'],
                     exclusions=['rendered RGB/PNG frames', 'guessed palettes or strings',
                                 'independent format30 decompression of63 initial HUD tiles'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('rom', 'native', 'output', 'manifest'):
        parser.add_argument('--'+name, required=True, type=Path)
    args = parser.parse_args()
    data, report = build(args.rom.read_bytes(), args.native)
    args.output.write_bytes(data)
    args.manifest.write_text(json.dumps(report, indent=2)+'\n')
    print('HUD resources:', len(data), sha(data))


if __name__ == '__main__':
    main()
