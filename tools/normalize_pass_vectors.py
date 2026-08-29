import argparse
import json
from collections import defaultdict
from pathlib import Path

import verify_pass_init_vectors as pass_init
import verify_pass_release_vectors as pass_release


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--kind', choices=('init', 'release'), required=True)
    parser.add_argument('--vectors', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    module = pass_init if args.kind == 'init' else pass_release
    vectors = [json.loads(line) for line in Path(args.vectors).read_text().splitlines()
               if line.strip()]
    if args.kind == 'release':
        by_exit = defaultdict(list)
        for vector in vectors:
            by_exit[vector['exit_pc']].append(vector)
        selected = []
        for items in by_exit.values():
            indexes = sorted({0, len(items) // 3, (2 * len(items)) // 3,
                              len(items) - 1})
            selected.extend(items[index] for index in indexes)
    else:
        selected = vectors
    calls = [{
        'entry_pc': vector.get('entry_pc',
                               '86ab73' if args.kind == 'init' else '86a6b3'),
        'exit_pc': vector['exit_pc'],
        'input': module.memory(vector['entry']).hex(),
        'expected': module.expected_row(vector),
    } for vector in selected]
    Path(args.output).write_text(json.dumps({'calls': calls}, separators=(',', ':')))
    print(f'[PASS NORMALIZE] kind={args.kind} calls={len(calls)}')


if __name__ == '__main__':
    main()
