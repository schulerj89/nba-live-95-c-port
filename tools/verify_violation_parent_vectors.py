"""Replay native violation parent calls with strict fixture/output integrity."""

import argparse
import json
import subprocess
from collections import Counter
from pathlib import Path
from verify_actor_commit_vectors import (
    WRAM_SIZE, image, memory, patched_image, probe_rows, provenance,
    raw_capture, require, word, words)

SIZE = WRAM_SIZE
w = word
OOB_SHA256 = 'ef5f37a020a7b521dd55a83baedcc9c54f1e2db8d9d7f4627b44c81a9733259c'
LEGACY_SOURCES = {'none', 'interference', 'boundary', 'code5', 'code7',
                  'defensive', 'charging', 'offensive', 'deferred', 'deferred-wait'}
TIPOFF_LOG = '[TIPOFF] $86:CCFC contact -> $86:B04C receiver -> $86:99C4 deflection -> $86:D365 possession.'


def projection(raw):
    values = [w(raw, address) for address in (
        0x936, 0x93A, 0x93E, 0x948, 0x952, 0x954, 0x956,
        0x964, 0x966, 0x968, 0x96A, 0x96C, 0x978, 0x97A, 0x97C, 0x92C, 0x92E,
        0x9B0, 0x9B2, 0x9BC, 0x9C6, 0x9D6, 0x13E7, 0x472A, 0x47AA, 0x3EF9, 0x3EFB)]
    values.extend(w(raw, 0x34EB + index * 0x100 + 0x5E) for index in range(10))
    return values


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--vectors', required=True)
    parser.add_argument('--probe', required=True)
    parser.add_argument('--pack', required=True)
    args = parser.parse_args()
    path = Path(args.vectors)
    skipped = 0
    if path.suffix == '.json':
        fixture = json.loads(path.read_text())
        rows = fixture['calls']
        require(isinstance(rows, list) and rows, 'violation fixture contains no calls')
        schema = fixture['schema']
        require(schema in {'nba95-violation-parent-v1', 'nba95-violation-parent-v2-delta'},
                'unknown violation fixture schema')
        if schema == 'nba95-violation-parent-v1':
            require(len(rows) == 10 and {row['source'] for row in rows} == LEGACY_SOURCES,
                    'legacy fixture must retain each of its ten native witnesses')
        else:
            require(len(rows) == 46, 'OOB fixture must retain all 46 native cases')
            provenance(fixture, 0x8792A8, {0x8794A2}, 46, OOB_SHA256)
        images = []
        for index, vector in enumerate(rows, 1):
            if schema == 'nba95-violation-parent-v1':
                before = image(vector['input'])
            else:
                require(vector['case'] == index and vector['native_call'] == index and
                        vector.get('controlled') is True and vector['source'] == 'controlled-oob-matrix',
                        'missing, duplicate, or uncontrolled OOB case')
                before = patched_image(fixture['base_input'], vector['patches'])
                owner = vector['owner']
                require(type(owner) is int and owner in {-1, 2, 7}, 'invalid controlled owner')
                actor = 0x34EB + owner * 256 if owner >= 0 else 0x3EEB
                for name, address in (('owner', 0x93E), ('x', actor + 4),
                                      ('y', actor + 8), ('vx', 0x3EF9),
                                      ('vy', 0x3EFB), ('live', 0x936)):
                    require(type(vector[name]) is int and w(before, address) == vector[name] & 0xFFFF,
                            f'OOB case {index}: {name} label does not match native input')
            images.append(before)
        expected = [words(vector['expected'], 37, f'violation case {index}')
                    for index, vector in enumerate(rows, 1)]
        sources = Counter(vector['source'] for vector in rows)
    else:
        captured = raw_capture(path, {0x8792A5, 0x8792A8}, {0x87949E, 0x8794A2})
        rows = [vector for vector in captured if vector['entry_frame'] == vector['exit_frame']]
        skipped = len(captured) - len(rows)
        require(rows, 'capture has no same-frame isolated violation calls')
        required = ((0, 0x1FFF), (0x3400, 0x4AFF))
        images = [memory(vector['entry'], required) for vector in rows]
        expected = [projection(memory(vector['exit'], required)) for vector in rows]
        sources = Counter(path.parent.name for _ in rows)
    run = subprocess.run([args.probe, args.pack], input=b''.join(images),
                         capture_output=True, check=True)
    actual = probe_rows(run.stdout, len(expected), 37,
                        log_prefixes=('[ASSETS] Loaded asset pack: ',),
                        log_lines=(TIPOFF_LOG,))
    bad = [(index, want, got) for index, (want, got) in enumerate(zip(expected, actual), 1)
           if want != got]
    print(f"[VIOLATION PARENT] {'FAIL' if bad else 'PASS'}: calls={len(expected)} "
          f'sources={dict(sources)} mismatches={len(bad)} cross_frame_skipped={skipped}')
    for index, want, got in bad[:12]:
        print(index, [(field, a, b) for field, (a, b) in enumerate(zip(want, got)) if a != b][:15])
    if bad:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
