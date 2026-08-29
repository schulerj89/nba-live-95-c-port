import argparse
import json
from collections import defaultdict
from pathlib import Path

from verify_defense_target_families import expected, memory


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--vectors', nargs='+', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    by_entry = defaultdict(list)
    for filename in args.vectors:
        for line in Path(filename).open():
            if line.strip():
                vector = json.loads(line)
                by_entry[vector['entry_pc']].append(vector)
    chosen = []
    for vectors in by_entry.values():
        # Twenty evenly spread real calls retain changing positions, arc/rating
        # decisions, direction-table choices, and late-game state without
        # checking a multi-megabyte raw recorder artifact into the repository.
        indexes = sorted({round(i * (len(vectors) - 1) / 19) for i in range(20)})
        chosen.extend(vectors[index] for index in indexes)
    calls = []
    for vector in chosen:
        before, after = memory(vector['entry']), memory(vector['exit'])
        before[0:2] = int(vector['entry_pc'][2:], 16).to_bytes(2, 'little')
        calls.append({'entry_pc': vector['entry_pc'], 'exit_pc': vector['exit_pc'],
                      'input': before.hex(),
                      'expected': expected(vector, before, after)})
    Path(args.output).write_text(json.dumps({'calls': calls}, separators=(',', ':')))
    print(f'[DEFENSE TARGET NORMALIZE] calls={len(calls)} entries={len(by_entry)}')


if __name__ == '__main__':
    main()
