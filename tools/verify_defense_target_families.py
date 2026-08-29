import argparse
import json
import subprocess
from collections import Counter
from pathlib import Path

SIZE = 0x4B00


def memory(snapshot):
    raw = bytearray(SIZE)
    for base, payload in snapshot['mem'].items():
        start = int(base, 16)
        data = bytes.fromhex(payload)
        raw[start:start + len(data)] = data
    return raw


def word(raw, at):
    return raw[at] | raw[at + 1] << 8


def expected(vector, before, after):
    entry = int(vector['entry_pc'][2:], 16)
    subject = word(before, 0x96)
    # E96F has a dedicated early return at E9A8. Velocity changes observed at
    # the common E82E exit belong to E7FD's steering callback, not this pure
    # target selector, and must not be mistaken for the early-stop branch.
    velocity_stopped = vector['exit_pc'] == '86e9a8'
    written = not velocity_stopped
    return [entry,
            word(after, subject + 0x56) if written else 0,
            word(after, subject + 0x58) if written else 0,
            int(written), int(velocity_stopped)]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--vectors', nargs='+', required=True)
    parser.add_argument('--probe', required=True)
    parser.add_argument('--rom', required=True)
    args = parser.parse_args()
    normalized = len(args.vectors) == 1 and Path(args.vectors[0]).suffix == '.json'
    if normalized:
        vectors = json.loads(Path(args.vectors[0]).read_text())['calls']
        images = [bytes.fromhex(vector['input']) for vector in vectors]
        wanted = [vector['expected'] for vector in vectors]
    else:
        vectors = []
        for filename in args.vectors:
            vectors.extend(json.loads(line) for line in Path(filename).open()
                           if line.strip())
        images, wanted = [], []
        for vector in vectors:
            before, after = memory(vector['entry']), memory(vector['exit'])
            before[0:2] = int(vector['entry_pc'][2:], 16).to_bytes(2, 'little')
            images.append(before)
            wanted.append(expected(vector, before, after))
    run = subprocess.run([args.probe, args.rom], input=b''.join(images),
                         capture_output=True, check=True)
    actual = [[int(value, 16) for value in line.split()][:5]
              for line in run.stdout.decode().splitlines()]
    bad = []
    for index, (want, got) in enumerate(zip(wanted, actual), 1):
        if want != got:
            bad.append((index, vectors[index - 1]['entry_pc'], want, got))
    census = Counter(vector['entry_pc'] for vector in vectors)
    print(f"[DEFENSE TARGET FAMILY] {'PASS' if not bad and len(actual)==len(wanted) else 'FAIL'}: calls={len(wanted)} entries={dict(census)} mismatches={len(bad)}")
    for item in bad[:12]: print(item)
    if bad or len(actual) != len(wanted): raise SystemExit(1)


if __name__ == '__main__':
    main()
