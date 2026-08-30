"""Strict, separate ten-call native `$85:AE88` formation-override witness gate."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import re
import subprocess

from normalize_formation_route import SIZE, projected, word

ROM_SHA256 = '2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
SCHEMA = 'nba95-formation-override-v1'
RANGES = ['0000-00ff', '0800-0aff', '1600-18ff', '3400-4aff']
OUTPUT_SCOPE = ['ran'] + [f'actor{i}.{field}' for i in range(10)
    for field in ('target_x', 'target_y', 'behavior_flags', 'velocity_x',
                  'velocity_y', 'movement_boost_timer')]
GATES = [0x85AE39, 0x85AE3C, 0x85AE3F, 0x85AE41, 0x85AE44, 0x85AE47,
         0x85AE49, 0x85AE4C, 0x85AE4E, 0x85AE51]
X_GATE = [0x85AE53, 0x85AE56]
OVERRIDE = [0x85AE58, 0x85AE5A, 0x85AE5D, 0x85AE5E, 0x85AE61, 0x85AE62,
            0x85AE63, 0x85AE67, 0x85AE68, 0x85AE6A, 0x85AE6D, 0x85AE6F,
            0x85AE72, 0x85AE75, 0x85AE77, 0x85AE7A,
            0x85AE88, 0x85AE8B, 0x85AE8E, 0x85AE91, 0x85AE94, 0x85AE95]


def require(condition, message):
    if not condition:
        raise ValueError(message)


def matrix():
    return [(slot, play, 'positive', -133, 224, 4 if slot == 2 else 9)
            for slot in (2, 7) for play in range(6, 10)] + [
            (2, 9, 'skip_x_nonnegative', 0, 224, -1),
            (7, 9, 'skip_y_negative', -133, -1, -1)]


def content_sha256(fixture):
    content = {key: value for key, value in fixture.items()
               if key != 'content_sha256'}
    return hashlib.sha256(json.dumps(content, sort_keys=True,
        separators=(',', ':')).encode()).hexdigest()


def validate_fixture(fixture):
    require(fixture.get('schema') == SCHEMA, 'unknown override fixture schema')
    require(fixture.get('content_sha256') == content_sha256(fixture),
            'override fixture content checksum mismatch')
    require(fixture.get('output_scope') == OUTPUT_SCOPE, 'override output scope changed')
    provenance = fixture['provenance']
    require(provenance.get('rom_file_sha256') == ROM_SHA256 and
            provenance.get('controlled') is True and
            provenance.get('matrix') == 'override10' and
            provenance.get('entry', '').lower() == '85ad6b' and
            provenance.get('exits') == ['85ad77', '85af5b'] and
            provenance.get('reads') == RANGES and provenance.get('writes') == RANGES and
            provenance.get('max_calls') == 10, 'override provenance identity changed')
    for name in ('formation_override_vectors_jsonl_sha256',
                 'formation_override_cases_jsonl_sha256',
                 'formation_override_pcs_jsonl_sha256'):
        require(re.fullmatch(r'[0-9a-f]{64}', provenance.get(name, '')) is not None,
                'missing raw native source hash')
    base = bytes.fromhex(fixture['base_input'])
    require(len(base) == SIZE, 'invalid lossless override base image')
    calls = fixture['calls']
    require(len(calls) == 10, 'expected exactly ten native override calls')
    images = []
    for index, (call, case) in enumerate(zip(calls, matrix()), 1):
        slot, play, kind, tx, ty, candidate = case
        context, actor = (0x46EB if slot == 2 else 0x476B), 0x34EB + slot * 256
        require(call.get('case') == index and call.get('native_call') == index and
                call.get('controlled') is True and call.get('exit') == '85af5b' and
                (call.get('slot'), call.get('play'), call.get('kind'), call.get('tx'),
                 call.get('ty'), call.get('candidate')) == case and
                call.get('context') == context and call.get('exclude') == slot and
                call.get('humans') == 0 and
                call.get('anchor') == (-336 if slot == 2 else 336),
                f'case {index}: native matrix identity changed')
        require(type(call.get('entry_frame')) is int and
                type(call.get('exit_frame')) is int and call['entry_frame'] >= 0 and
                0 <= call['exit_frame'] - call['entry_frame'] <= 1,
                f'case {index}: invalid native frame boundary')
        raw = bytearray(base)
        seen = set()
        for patch in call['patches']:
            require(isinstance(patch, list) and len(patch) == 2,
                    'invalid lossless override patch shape')
            address, value = patch
            require(type(address) is int and type(value) is int and
                    0 <= address < SIZE and 0 <= value <= 255 and
                    address not in seen and base[address] != value,
                    'invalid/duplicate/redundant lossless override patch')
            seen.add(address)
            raw[address] = value
        words = {0xC2:slot, 0xC6:2, 0x96:actor, 0x9E:context,
                 0x46F5:-336, 0x4775:336, 0x93E:-1, 0x968:0, 0x936:0x82,
                 0x954:slot, 0x958:tx, 0x95A:ty, 0x9A2:-1, 0x996:play,
                 0x998:0, 0x99C:0, 0x948:0, 0x97C:0, 0x5C:0,
                 actor+2:0x1200, actor+4:31, actor+6:0x3400, actor+8:-17,
                 actor+0xA:0, actor+0xC:0, actor+0xE:128, actor+0x10:-64,
                 actor+0x5C:300, actor+0x6E:0 if slot == 2 else 5,
                 actor+0x72:0, actor+0x7E:0}
        for address, value in words.items():
            require(word(raw, address) == value & 0xFFFF,
                    f'case {index}: controlled WRAM {address:04x} mismatch')
        require(word(raw, 0xE0) == word(raw, 0x3449 + slot * 4) and
                word(raw, 0xE2) == word(raw, 0x344B + slot * 4),
                f'case {index}: incoherent native roster profile pointer')
        for i in range(10):
            at = 0x34EB + i * 256
            require(word(raw, at) == i and word(raw, at+0x16) == 0xFFFF and
                    word(raw, at+0x56) == 120+i and
                    word(raw, at+0x58) == (-45-i) & 0xFFFF,
                    f'case {index}: actor identity/controller/target sentinel changed')
        expected_pcs = [0x85AD6B] + GATES
        if kind != 'skip_y_negative':
            expected_pcs += X_GATE
        if kind == 'positive':
            expected_pcs += OVERRIDE
        expected_pcs += [0x85AE97, 0x85AF44, 0x85AF5B]
        require(call['executed'] == expected_pcs,
                f'case {index}: incomplete or unexpected actual native branch path')
        want = call['expected']
        require(isinstance(want, list) and len(want) == len(OUTPUT_SCOPE) and
                all(type(value) is int and 0 <= value <= 0xFFFF for value in want) and
                want[0] == 1, f'case {index}: malformed native expected output')
        before = projected(raw, True)
        for i in range(10):
            start = 1 + 6*i
            if i == candidate:
                require(want[start:start+2] == [0xFFD8, 0x00A0] and
                        want[start+2:start+6] == before[start+2:start+6],
                        f'case {index}: native override target/output mismatch')
            elif i != slot:
                require(want[start:start+6] == before[start:start+6],
                        f'case {index}: unexpected non-current actor output')
        images.append(raw)
    return images


def self_test(fixture):
    mutations = [lambda f: f['calls'].pop(),
                 lambda f: f['calls'].__setitem__(1, copy.deepcopy(f['calls'][0])),
                 lambda f: f['calls'][0]['expected'].pop(),
                 lambda f: f['calls'][0]['expected'].__setitem__(25, 0),
                 lambda f: f['calls'][0]['executed'].remove(0x85AE8B),
                 lambda f: f['calls'][0]['patches'].append([0xE0, 0]),
                 lambda f: f['calls'][8]['executed'].insert(-3, 0x85AE88)]
    for index, mutate in enumerate(mutations, 1):
        damaged = copy.deepcopy(fixture)
        mutate(damaged)
        damaged['content_sha256'] = content_sha256(damaged)
        try:
            validate_fixture(damaged)
        except (ValueError, KeyError, TypeError):
            continue
        raise ValueError(f'mutation {index} was not rejected')
    print(f'[FORMATION OVERRIDE INTEGRITY] PASS rejected={len(mutations)}')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--vectors', required=True)
    parser.add_argument('--probe')
    parser.add_argument('--pack')
    parser.add_argument('--self-test', action='store_true')
    args = parser.parse_args()
    fixture = json.loads(Path(args.vectors).read_text())
    images = validate_fixture(fixture)
    if args.self_test:
        self_test(fixture)
        return
    if not args.probe or not args.pack:
        parser.error('--probe and --pack are required for C replay')
    run = subprocess.run([args.probe, args.pack], input=b''.join(images),
                         capture_output=True, check=True)
    lines = [line for line in run.stdout.decode().splitlines()
             if line and not line.startswith('[')]
    require(len(lines) == 10, 'C replay omitted/added output rows')
    actual = []
    for line in lines:
        tokens = line.split()
        require(len(tokens) == len(OUTPUT_SCOPE) and
                all(re.fullmatch(r'[0-9a-fA-F]{4}', token) for token in tokens),
                'C replay output width/value malformed')
        actual.append([int(token, 16) for token in tokens])
    bad = [(i, OUTPUT_SCOPE[field], want, got)
           for i, (call, row) in enumerate(zip(fixture['calls'], actual), 1)
           for field, (want, got) in enumerate(zip(call['expected'], row)) if want != got]
    print(f"[FORMATION OVERRIDE] {'FAIL' if bad else 'PASS'}: calls=10 "
          f'positive=8 skip_controls=2 fields_per_call=61 mismatches={len(bad)}')
    for mismatch in bad[:12]:
        print(mismatch)
    if bad:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
