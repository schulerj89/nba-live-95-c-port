import argparse
import json
from collections import defaultdict
from pathlib import Path

from verify_matchup_helper_vectors import expected, memory


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--vectors', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()

    raw = [json.loads(line) for line in Path(args.vectors).read_text().splitlines()
           if line.strip()]
    by_pair = defaultdict(list)
    for vector in raw:
        by_pair[(vector['entry_pc'], vector['exit_pc'])].append(vector)

    # Preserve every observed control-flow pair. Four spread witnesses per pair
    # are enough for ordinary scans; retain every rare one-way-help witness
    # because that path found a real symmetric-vs-one-way porting error.
    chosen = []
    for pair, vectors in sorted(by_pair.items()):
        if pair == ('85bae4', '85bbbe') or len(vectors) <= 4:
            selected = vectors
        else:
            indexes = sorted({0, len(vectors) // 3, (2 * len(vectors)) // 3,
                              len(vectors) - 1})
            selected = [vectors[index] for index in indexes]
        chosen.extend(selected)

    calls = []
    for vector in chosen:
        before = memory(vector['entry'])
        after = memory(vector['exit'])
        before[0:2] = bytes.fromhex(vector['entry_pc'][2:6])[::-1]
        calls.append({
            'entry_pc': vector['entry_pc'],
            'exit_pc': vector['exit_pc'],
            'input': before.hex(),
            'expected': expected(vector, before, after),
        })
    Path(args.output).write_text(json.dumps({'calls': calls}, separators=(',', ':')))
    print(f'[MATCHUP NORMALIZE] calls={len(calls)} pairs={len(by_pair)}')


if __name__ == '__main__':
    main()
