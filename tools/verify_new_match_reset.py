"""Native startup projection and separate C postgame/menu integration check."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROM_SHA256 = '2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
NATIVE_KEYS = {'period', 'scores', 'timeouts', 'roster_order'}
SOURCE_KEYS = {'kind', 'snapshot', 'snapshot_sha256', 'run_manifest',
               'run_manifest_sha256', 'boundary', 'controlled_setup', 'caveat'}


def reject_duplicates(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f'duplicate JSON key: {key}')
        result[key] = value
    return result


def strict_json(text):
    return json.loads(text, object_pairs_hook=reject_duplicates)


def word(value):
    if type(value) is not int or not 0 <= value <= 0xFFFF:
        raise ValueError(f'expected uint16 integer, got {value!r}')


def validate_projection(native):
    if not isinstance(native, dict) or set(native) != NATIVE_KEYS:
        raise ValueError('unexpected native projection fields')
    word(native['period'])
    for name in ('scores', 'timeouts'):
        values = native[name]
        if not isinstance(values, list) or len(values) != 2:
            raise ValueError(f'{name} must contain exactly two values')
        for value in values:
            word(value)
    roster = native['roster_order']
    if not isinstance(roster, list) or len(roster) != 2:
        raise ValueError('roster_order must contain exactly two teams')
    for team in roster:
        if not isinstance(team, list) or len(team) != 12:
            raise ValueError('each roster must contain twelve values')
        for value in team:
            word(value)
        if sorted(team) != list(range(12)):
            raise ValueError('each roster must be a permutation of 0..11')


def validate_fixture(fixture):
    if not isinstance(fixture, dict) or set(fixture) != {'schema', 'rom_sha256', 'source', 'native'}:
        raise ValueError('unexpected fixture fields')
    if type(fixture['schema']) is not int or fixture['schema'] != 1 or fixture['rom_sha256'] != ROM_SHA256:
        raise ValueError('fixture schema/ROM identity mismatch')
    source = fixture['source']
    if not isinstance(source, dict) or set(source) != SOURCE_KEYS or any(
            not isinstance(value, str) or not value for value in source.values()):
        raise ValueError('invalid fixture provenance')
    for name in ('snapshot_sha256', 'run_manifest_sha256'):
        if re.fullmatch('[0-9a-f]{64}', source[name]) is None:
            raise ValueError(f'invalid provenance hash: {name}')
    if source['kind'] != 'native-first-court-projection' or source['boundary'] != '87:A47A first on-court draw':
        raise ValueError('invalid native capture boundary')
    for name in ('snapshot', 'run_manifest'):
        value = Path(source[name])
        if value.is_absolute() or '..' in value.parts or value.parts[0] != '.analysis':
            raise ValueError('invalid local provenance path')
    validate_projection(fixture['native'])


def compare_projection(actual, expected):
    validate_projection(actual)
    validate_projection(expected)
    if actual != expected:
        raise ValueError(f'native startup projection mismatch: {actual!r} != {expected!r}')


def parse_rows(output):
    prefix = 'NEW_MATCH_PROJECTION '
    rows = []
    for line in output.splitlines():
        if line.startswith(prefix):
            row = strict_json(line[len(prefix):])
            if not isinstance(row, dict) or set(row) != NATIVE_KEYS | {'side'}:
                raise ValueError('malformed C projection row')
            if type(row['side']) is not int or row['side'] not in (0, 1):
                raise ValueError('invalid C projection side')
            native = {key: value for key, value in row.items() if key != 'side'}
            validate_projection(native)
            rows.append((row['side'], native))
    if len(rows) != 2 or [side for side, _ in rows] != [0, 1]:
        raise ValueError('missing both production return journeys')
    return rows


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--fixture', required=True, type=Path)
    parser.add_argument('--probe', required=True, type=Path)
    parser.add_argument('--pack', required=True, type=Path)
    parser.add_argument('--rom', required=True, type=Path)
    parser.add_argument('--native-snapshot', type=Path)
    args = parser.parse_args()
    fixture = strict_json(args.fixture.read_text(encoding='utf-8'))
    validate_fixture(fixture)
    rom = args.rom.read_bytes()
    if len(rom) % 0x8000 == 512:
        rom = rom[512:]
    if hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise SystemExit('new-match fixture/ROM identity mismatch')
    # Independent instruction evidence for a NEW match, distinct from DD2D
    # preserving the existing period during quarter/overtime restart.
    offset = 6 * 0x8000 + 0xDBE8 - 0x8000
    if rom[offset:offset + 3] != bytes((0x9C, 0x26, 0x09)):
        raise SystemExit('native DBE8 period-reset opcode mismatch')
    expected = fixture['native']
    if args.native_snapshot:
        snapshot = args.native_snapshot.read_bytes()
        if len(snapshot) != 0x20000 or hashlib.sha256(snapshot).hexdigest() != fixture['source']['snapshot_sha256']:
            raise SystemExit('native snapshot identity mismatch')
        manifest = args.native_snapshot.with_name('run.json').read_bytes()
        if hashlib.sha256(manifest).hexdigest() != fixture['source']['run_manifest_sha256']:
            raise SystemExit('native run manifest identity mismatch')
        recorded_run = strict_json(manifest.decode('utf-8'))
        if recorded_run['sources']['rom']['sha256'] != ROM_SHA256 or recorded_run['sources']['baseline.wram']['sha256'] != fixture['source']['snapshot_sha256']:
            raise SystemExit('native run provenance mismatch')
        def native_word(address):
            return int.from_bytes(snapshot[address:address + 2], 'little')
        original = {
            'period': native_word(0x926),
            'scores': [native_word(0x4711), native_word(0x4791)],
            'timeouts': [native_word(0x4715), native_word(0x4795)],
            'roster_order': [[native_word(base + i * 2) for i in range(12)]
                             for base in (0x46F9, 0x4779)],
        }
        compare_projection(expected, original)
    run = subprocess.run([str(args.probe), str(args.pack), str(args.rom)],
                         text=True, capture_output=True, timeout=60)
    print(run.stdout, end='')
    if run.returncode:
        raise SystemExit(run.stderr or 'new-match C integration failed')
    for _side, row in parse_rows(run.stdout):
        compare_projection(row, expected)
    print('new-match reset: native startup projection matches; C return journeys=2; '
          'native second-match timing/full-state parity NOT claimed')


if __name__ == '__main__':
    try:
        main()
    except (ValueError, KeyError, TypeError, OSError, subprocess.TimeoutExpired) as error:
        raise SystemExit(f'new-match reset verification failed: {error}') from error
