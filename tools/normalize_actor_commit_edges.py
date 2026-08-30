"""Losslessly compact actual native actor-clamp outcomes; no C goldens."""
import argparse
import hashlib
import json
from pathlib import Path
from verify_actor_commit_vectors import memory, word, row


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--capture', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    capture = Path(args.capture)
    source = capture / 'actor_commit_edges.vectors.jsonl'
    meta = json.loads((capture / 'actor_commit_edges.meta.json').read_text(encoding='utf-8-sig'))
    assert hashlib.sha256(source.read_bytes()).hexdigest() == meta['vectors_sha256']
    vectors = [json.loads(line) for line in source.read_text().splitlines() if line]
    labels = [json.loads(line) for line in (capture / 'actor-commit-edge-cases.jsonl').read_text().splitlines() if line]
    traces = [json.loads(line) for line in (capture / 'actor-commit-edge-pcs.jsonl').read_text().splitlines() if line]
    assert len(vectors) == len(labels) == len(traces) == 56
    base = memory(vectors[0]['entry'])
    calls = []
    for vector, label, trace in zip(vectors, labels, traces):
        before, after = memory(vector['entry']), memory(vector['exit'])
        actor = word(before, 0x96)
        assert word(before, 0xc6) == 2
        for name, offset in [('x',4),('y',8),('xf',2),('yf',6),('vx',14),('vy',16),('mode',0x5e),('timer',0x60)]:
            assert word(before, actor + offset) == label[name] & 0xffff, (label['case'], name)
        calls.append({**label, 'native_call': vector['call'],
                      'frame': vector['entry_frame'], 'exit_pc': vector['exit_pc'],
                      'executed': trace['executed'],
                      'patches': [[a,v] for a,v in enumerate(before) if v != base[a]],
                      'expected': row(after, actor)})
    fixture = {'schema':'nba95-actor-commit-edges-v1', 'controlled':True,
               'provenance':{**meta,'source':source.as_posix()},
               'base_input':base.hex(),'calls':calls}
    Path(args.output).write_text(json.dumps(fixture,separators=(',',':'))+'\n',encoding='utf-8')
    print(f'[ACTOR EDGE NORMALIZE] controlled=true calls={len(calls)}')


if __name__ == '__main__':
    main()
