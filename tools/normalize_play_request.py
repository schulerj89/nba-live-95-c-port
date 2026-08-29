import argparse
import json
from pathlib import Path

from verify_play_request_vectors import memory, row, word


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--vectors', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    vectors = [json.loads(line) for line in Path(args.vectors).open()
               if line.strip()]
    made = [vector for vector in vectors
            if word(memory(vector['entry']), 0x0936) == 0x82]
    indexes = sorted({round(i * (len(vectors) - 1) / 24) for i in range(25)})
    chosen = [vectors[index] for index in indexes]
    calls = [{'entry_pc': vector['entry_pc'], 'exit_pc': vector['exit_pc'],
              'input': memory(vector['entry']).hex(),
              'expected': row(memory(vector['exit']), vector['exit_pc'])}
             for vector in chosen]
    Path(args.output).write_text(json.dumps({'calls': calls}, separators=(',', ':')))
    print(f'[PLAY REQUEST NORMALIZE] calls={len(calls)} made={len(made)}')


if __name__ == '__main__': main()
