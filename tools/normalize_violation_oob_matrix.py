"""Losslessly compact controlled native parent vectors (not C expectations)."""
import argparse
import hashlib
import json
from pathlib import Path
from verify_violation_parent_vectors import memory, projection, w


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--capture', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    capture = Path(args.capture)
    source = capture / 'violation_oob.vectors.jsonl'
    meta = json.loads((capture / 'violation_oob.meta.json').read_text(encoding='utf-8-sig'))
    assert hashlib.sha256(source.read_bytes()).hexdigest() == meta['vectors_sha256']
    raw = [json.loads(line) for line in source.read_text().splitlines() if line]
    labels = [json.loads(line) for line in (capture / 'violation-oob-cases.jsonl').read_text().splitlines() if line]
    assert len(raw) == len(labels) == 46
    base = memory(raw[0]['entry'])
    calls = []
    for row, label in zip(raw, labels):
        assert row['entry_frame'] == row['exit_frame']
        before, after = memory(row['entry']), memory(row['exit'])
        owner = label['owner']
        assert w(before, 0x93E) == (owner & 0xffff)
        address = 0x34EB + owner * 256 if owner >= 0 else 0x3EEB
        assert w(before, address + 4) == (label['x'] & 0xffff)
        assert w(before, address + 8) == (label['y'] & 0xffff)
        assert w(before, 0x3EF9) == (label['vx'] & 0xffff)
        assert w(before, 0x3EFB) == (label['vy'] & 0xffff)
        calls.append({'source': 'controlled-oob-matrix', **label,
                      'native_call': row['call'], 'frame': row['entry_frame'],
                      'patches': [[a, v] for a, v in enumerate(before) if v != base[a]],
                      'expected': projection(after)})
    output = Path(args.output)
    fixture = {'schema': 'nba95-violation-parent-v2-delta', 'controlled': True,
               'provenance': {**meta, 'source': source.as_posix()},
               'base_input': base.hex(), 'calls': calls}
    output.write_text(json.dumps(fixture, separators=(',', ':')) + '\n', encoding='utf-8')
    print(f'[VIOLATION OOB NORMALIZE] controlled=true calls={len(calls)} bytes={output.stat().st_size}')


if __name__ == '__main__':
    main()
