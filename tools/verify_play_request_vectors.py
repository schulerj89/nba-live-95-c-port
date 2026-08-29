import argparse
import json
import subprocess
from pathlib import Path

SIZE = 0x4B00


def memory(snapshot):
    raw = bytearray(SIZE)
    for base, payload in snapshot['mem'].items():
        at = int(base, 16)
        data = bytes.fromhex(payload)
        raw[at:at + len(data)] = data
    return raw


def word(raw, at): return raw[at] | raw[at + 1] << 8


def row(raw, exit_pc=None):
    values = [word(raw, at) for at in
              (0x0994, 0x0996, 0x0998, 0x099A, 0x099C, 0x099E,
               0x09A2, 0x09A4, 0x09AA, 0x09AC, 0x09AE, 0x09D0,
               0x07F6, 0x0936, 0x471B, 0x479B)]
    if exit_pc == '85b353': values[3] = word(raw, 0x00AA)
    values.extend(word(raw, 0x3569 + i * 0x100) for i in range(10))
    return values


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--vectors', required=True)
    parser.add_argument('--probe', required=True)
    parser.add_argument('--pack', required=True)
    args = parser.parse_args()
    path = Path(args.vectors)
    normalized = path.suffix == '.json'
    if normalized:
        vectors = json.loads(path.read_text())['calls']
        images = [bytes.fromhex(vector['input']) for vector in vectors]
        expected = [vector['expected'] for vector in vectors]
    else:
        vectors = [json.loads(line) for line in path.open() if line.strip()]
        images = [memory(vector['entry']) for vector in vectors]
        expected = [row(memory(vector['exit']), vector['exit_pc'])
                    for vector in vectors]
    run = subprocess.run([args.probe, args.pack], input=b''.join(images),
                         capture_output=True, check=True)
    actual = [[int(value, 16) for value in line.split()]
              for line in run.stdout.decode().splitlines()
              if len(line.split()) == 26]
    bad = []
    for index, (want, got) in enumerate(zip(expected, actual), 1):
        diff = [(field, a, b) for field, (a, b) in
                enumerate(zip(want, got)) if a != b]
        if diff: bad.append((index, diff[:12]))
    print(f"[PLAY REQUEST] {'PASS' if not bad and len(actual)==len(expected) else 'FAIL'}: calls={len(expected)} mismatches={len(bad)}")
    for item in bad[:12]: print(item)
    if bad or len(actual) != len(expected): raise SystemExit(1)


if __name__ == '__main__': main()
