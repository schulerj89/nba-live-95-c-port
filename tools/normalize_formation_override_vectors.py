"""Normalize ten native Mesen calls; expected outputs come only from ROM exits."""
import argparse
import hashlib
import json
from pathlib import Path

from normalize_formation_route import SIZE, projected
from verify_formation_override_vectors import (
    OUTPUT_SCOPE, RANGES, SCHEMA, content_sha256, require, validate_fixture)


def snapshot_memory(snapshot):
    raw = bytearray(SIZE)
    expected = {item[:4]: int(item[5:], 16)-int(item[:4], 16)+1 for item in RANGES}
    require(set(snapshot['mem']) == set(expected), 'native WRAM range missing/added')
    for address, size in expected.items():
        data = bytes.fromhex(snapshot['mem'][address])
        require(len(data) == size, 'truncated native WRAM snapshot')
        start = int(address, 16)
        raw[start:start+size] = data
    require(snapshot['cpu'], 'missing genuine native CPU snapshot')
    return raw


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--capture', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    capture = Path(args.capture)
    meta = json.loads((capture/'formation_override.meta.json').read_text(encoding='utf-8-sig'))
    require((capture/'capture_complete.txt').read_text().strip() ==
            'label=formation_override vectors=10 orphan_exits=0 shared_exit_callbacks=0',
            'incomplete native capture')
    datasets = []
    for name in ('formation_override.vectors.jsonl', 'formation-override-cases.jsonl',
                 'formation-override-pcs.jsonl'):
        source = capture/name
        key = name.replace('.', '_').replace('-', '_') + '_sha256'
        require(meta.get(key) == hashlib.sha256(source.read_bytes()).hexdigest(),
                f'native source checksum mismatch: {name}')
        rows = [json.loads(line) for line in source.read_text().splitlines() if line]
        require(len(rows) == 10, f'expected ten native rows: {name}')
        datasets.append(rows)
    vectors, labels, traces = datasets
    base = snapshot_memory(vectors[0]['entry'])
    calls = []
    for index, (vector, label, trace) in enumerate(zip(vectors, labels, traces), 1):
        require(vector['call'] == label['case'] == trace['case'] == index and
                vector['entry_pc'] == '85ad6b' and vector['exit_pc'] == '85af5b',
                'native call pairing/boundary mismatch')
        before, after = snapshot_memory(vector['entry']), snapshot_memory(vector['exit'])
        calls.append({**label, 'native_call':vector['call'],
                      'entry_frame':vector['entry_frame'], 'exit_frame':vector['exit_frame'],
                      'exit':vector['exit_pc'], 'executed':trace['executed'],
                      'patches':[[at, value] for at, value in enumerate(before) if value != base[at]],
                      'expected':projected(after, True)})
    fixture = {'schema':SCHEMA, 'output_scope':OUTPUT_SCOPE,
               'provenance':{**meta, 'source':(capture/'formation_override.vectors.jsonl').as_posix()},
               'base_input':base.hex(), 'calls':calls}
    fixture['content_sha256'] = content_sha256(fixture)
    validate_fixture(fixture)
    Path(args.output).write_text(json.dumps(fixture, separators=(',', ':'))+'\n', encoding='utf-8')
    print('[FORMATION OVERRIDE NORMALIZE] calls=10 positive=8 skip_controls=2 native_only=true')


if __name__ == '__main__':
    main()
