"""Preserve the actual controlled Mesen branch observations as a fixture."""
import argparse
import hashlib
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--capture', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    capture = Path(args.capture)
    source = capture / 'inbound-side-gate.jsonl'
    meta = json.loads((capture / 'inbound-side-gate.meta.json').read_text(
        encoding='utf-8-sig'))
    digest = hashlib.sha256(source.read_bytes()).hexdigest()
    assert digest == meta['vectors_sha256']
    calls = [json.loads(line) for line in source.read_text().splitlines() if line]
    assert len(calls) == 40 and all(c['controlled'] for c in calls)
    output = Path(args.output)
    if output.exists():
        parser.error('fixture output must be new')
    fixture = {'schema': 1, 'controlled': True,
               'provenance': {**meta, 'source': source.as_posix()}, 'calls': calls}
    output.write_text(json.dumps(fixture, indent=2) + '\n', encoding='utf-8')
    print(f'[INBOUND SIDE NORMALIZE] controlled=true calls={len(calls)}')


if __name__ == '__main__':
    main()
