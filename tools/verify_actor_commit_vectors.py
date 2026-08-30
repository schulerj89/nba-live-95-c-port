"""Replay native actor commits; reject incomplete captures and probe output."""

import argparse
import hashlib
import json
import re
import subprocess
from collections import Counter
from pathlib import Path

WRAM_SIZE = 0x4B00
ROM_SHA256 = '2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
EDGE_SHA256 = '2b89d947ef0351d4d4beaf0a8b8a14f0ff6798affdb81a58ff75a44cfa052a83'
ACTOR_EXITS = {0x85990F, 0x859961}


def require(condition, message):
    # Verification must remain fail-closed under python -O.
    if not condition:
        raise ValueError(message)


def integer(value, maximum=0xFFFF):
    return type(value) is int and 0 <= value <= maximum


def image(payload):
    require(isinstance(payload, str), 'input image must be hexadecimal text')
    result = bytearray.fromhex(payload)
    require(len(result) == WRAM_SIZE, f'input image must contain {WRAM_SIZE} bytes')
    return result


def patched_image(base, patches):
    result = image(base)
    require(isinstance(patches, list), 'patches must be a list')
    seen = set()
    for patch in patches:
        require(isinstance(patch, list) and len(patch) == 2, 'invalid byte patch')
        address, value = patch
        require(integer(address, WRAM_SIZE - 1) and integer(value, 255),
                'byte patch is outside the input image or byte range')
        require(address not in seen, 'duplicate byte patch address')
        seen.add(address)
        result[address] = value
    return result


def words(values, width, context):
    require(isinstance(values, list) and len(values) == width,
            f'{context}: expected exactly {width} words')
    require(all(integer(value) for value in values),
            f'{context}: expected unsigned 16-bit integer words')
    return values


def ranges(specification):
    require(isinstance(specification, list) and specification,
            'capture ranges must be nonempty')
    result = []
    for item in specification:
        require(isinstance(item, str) and re.fullmatch(r'[0-9a-fA-F]+-[0-9a-fA-F]+', item),
                'invalid capture range')
        low, high = (int(value, 16) for value in item.split('-'))
        require(0 <= low <= high < WRAM_SIZE, 'capture range outside WRAM image')
        result.append((low, high))
    return result


def memory(snapshot, required=()):
    raw = bytearray(WRAM_SIZE)
    present = bytearray(WRAM_SIZE)
    require(isinstance(snapshot.get('mem'), dict) and snapshot['mem'],
            'snapshot has no captured memory')
    for base, payload in snapshot['mem'].items():
        start = int(base, 16)
        data = bytes.fromhex(payload)
        end = start + len(data)
        require(data and 0 <= start < end <= WRAM_SIZE, 'invalid snapshot memory range')
        require(not any(present[start:end]), 'overlapping snapshot memory ranges')
        raw[start:end] = data
        present[start:end] = b'\x01' * len(data)
    for low, high in required:
        require(all(present[low:high + 1]),
                f'missing captured memory ${low:04X}-${high:04X}')
    return raw


def word(raw, address):
    return raw[address] | raw[address + 1] << 8


def raw_capture(path, entries, exits):
    payload = path.read_bytes()
    vectors = [json.loads(line) for line in payload.decode('utf-8-sig').splitlines()
               if line.strip()]
    require(vectors, 'capture contains no vectors')
    require(path.name.endswith('.vectors.jsonl'), 'expected a named .vectors.jsonl capture')
    stem = path.name[:-len('.vectors.jsonl')]
    meta = json.loads(path.with_name(stem + '.meta.json').read_text(encoding='utf-8-sig'))
    entry = int(meta['entry'], 16)
    allowed_exits = {int(value, 16) for value in meta['exits']}
    require(entry in entries and allowed_exits and allowed_exits <= exits,
            'capture routine boundary does not match this verifier')
    maximum = meta['max_calls']
    require(integer(maximum, 10000000) and 0 < len(vectors) <= maximum,
            'invalid capture maximum/count')
    complete = (path.parent / 'capture_complete.txt').read_text().strip()
    require(complete == f'label={stem} vectors={len(vectors)} orphan_exits=0 shared_exit_callbacks=0',
            'capture is incomplete, truncated, or has orphan/shared exits')
    if 'vectors_sha256' in meta:
        require(hashlib.sha256(payload).hexdigest() == meta['vectors_sha256'],
                'native capture SHA256 mismatch')
    if 'rom_file_sha256' in meta:
        require(meta['rom_file_sha256'] == ROM_SHA256, 'unexpected source ROM SHA256')
    reads, writes = ranges(meta['reads']), ranges(meta['writes'])
    for index, vector in enumerate(vectors, 1):
        require(vector['call'] == index and type(vector['call']) is int,
                'capture calls must be consecutive, unique, and start at one')
        require(integer(vector['entry_frame'], 0xFFFFFFFF) and
                integer(vector['exit_frame'], 0xFFFFFFFF) and
                vector['entry_frame'] <= vector['exit_frame'], 'invalid capture frame interval')
        require(int(vector.get('entry_pc', meta['entry']), 16) == entry and
                int(vector['exit_pc'], 16) in allowed_exits, 'invalid captured routine boundary')
        memory(vector['entry'], reads)
        memory(vector['exit'], writes)
    return vectors


def provenance(fixture, entry, exits, count, source_sha256):
    meta = fixture['provenance']
    require(fixture.get('controlled') is True and meta.get('controlled') is True,
            'fixture must retain controlled-native provenance')
    require(int(meta['entry'], 16) == entry and
            {int(value, 16) for value in meta['exits']} == exits,
            'fixture has unexpected native entry/exit boundaries')
    require(meta['max_calls'] == count and meta['rom_file_sha256'] == ROM_SHA256 and
            meta['vectors_sha256'] == source_sha256,
            'fixture has unexpected native count or source hashes')
    require(isinstance(meta['source'], str) and meta['source'].endswith('.vectors.jsonl'),
            'fixture must identify its retained raw source')
    ranges(meta['reads'])
    ranges(meta['writes'])


def probe_rows(stdout, count, width, log_prefixes=(), log_lines=()):
    require(count > 0, 'cannot verify zero vectors')
    actual = []
    for line_number, line in enumerate(stdout.decode().splitlines(), 1):
        if not line.strip() or line in log_lines or line.startswith(log_prefixes):
            continue
        values = line.split()
        require(len(values) == width, f'probe line {line_number}: expected {width} words, got {len(values)}')
        require(all(re.fullmatch(r'[0-9a-fA-F]{4}', value) for value in values),
                f'probe line {line_number}: malformed 16-bit word')
        actual.append([int(value, 16) for value in values])
    require(len(actual) == count, f'probe returned {len(actual)} rows for {count} vectors')
    return actual


def actor_pointer(entry):
    actor = word(entry, 0x96)
    require(actor in range(0x34EB, 0x3DEB + 1, 0x100), 'invalid captured actor pointer')
    require(word(entry, 0xC6) == 2, 'actor probe requires native integration counter C6=2')
    return actor


def row(raw, base):
    return [word(raw, base + offset) for offset in (
        0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C,
        0x0E, 0x10, 0x12, 0x4A, 0x4C, 0x4E, 0xA2,
        0x94, 0x96, 0x98, 0x9A, 0xA0, 0x60)]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--vectors', required=True)
    parser.add_argument('--probe', required=True)
    args = parser.parse_args()
    path = Path(args.vectors)
    if path.suffix == '.json':
        fixture = json.loads(path.read_text())
        require(fixture['schema'] == 'nba95-actor-commit-edges-v1', 'unknown actor fixture schema')
        vectors = fixture['calls']
        require(isinstance(vectors, list) and len(vectors) == 56,
                'actor edge fixture must retain all 56 native cases')
        provenance(fixture, 0x8596B5, ACTOR_EXITS, 56, EDGE_SHA256)
        entries = []
        for index, vector in enumerate(vectors, 1):
            require(vector['case'] == index and vector['native_call'] == index and
                    vector.get('controlled') is True, 'missing, duplicate, or uncontrolled actor case')
            require(int(vector['exit_pc'], 16) in ACTOR_EXITS, 'invalid actor case exit')
            trace = vector['executed']
            require(isinstance(trace, list) and trace and trace[0] == 0x8596B5 and
                    all(integer(pc, 0xFFFFFF) for pc in trace), 'invalid native instruction trace')
            entry = patched_image(fixture['base_input'], vector['patches'])
            actor = actor_pointer(entry)
            for name, offset in (('x', 4), ('y', 8), ('xf', 2), ('yf', 6),
                                 ('vx', 14), ('vy', 16), ('mode', 0x5E), ('timer', 0x60)):
                require(type(vector[name]) is int and word(entry, actor + offset) == vector[name] & 0xFFFF,
                        f'actor case {index}: {name} label does not match native input')
            entries.append(entry)
        expected = [words(vector['expected'], 19, f'actor case {index}')
                    for index, vector in enumerate(vectors, 1)]
    else:
        vectors = raw_capture(path, {0x85963D, 0x8596B5}, ACTOR_EXITS)
        entries, expected = [], []
        for vector in vectors:
            entry = memory(vector['entry'], ((0, 0xFF),))
            actor = actor_pointer(entry)
            memory(vector['entry'], ((actor, actor + 0xA3),))
            end = memory(vector['exit'], ((actor, actor + 0xA3),))
            entries.append(entry)
            expected.append(row(end, actor))
    run = subprocess.run([args.probe], input=b''.join(entries),
                         capture_output=True, check=True)
    actual = probe_rows(run.stdout, len(expected), 19)
    mismatches = []
    for index, (want, got) in enumerate(zip(expected, actual), 1):
        differences = [(field, a, b) for field, (a, b) in enumerate(zip(want, got)) if a != b]
        if differences:
            mismatches.append((index, differences[:8]))
    exits = Counter(vector['exit_pc'] for vector in vectors)
    if mismatches:
        for call, differences in mismatches[:12]:
            print(f'call {call}: {differences}')
        raise SystemExit(f'[ACTOR COMMIT] FAIL: vectors={len(vectors)} '
                         f'mismatches={len(mismatches)} exits={dict(exits)}')
    print(f'[ACTOR COMMIT] PASS: vectors={len(vectors)} mismatches=0 exits={dict(exits)}')


if __name__ == '__main__':
    main()
