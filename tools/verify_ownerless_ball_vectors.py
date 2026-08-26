"""Replay live `$85:9A6A-$A7C7` ownerless-ball vectors through C."""
import argparse
import json
import subprocess
from pathlib import Path

WRAM_SIZE = 0x4B00

def memory(snapshot):
    raw = bytearray(WRAM_SIZE)
    for base, payload in snapshot["mem"].items():
        start = int(base, 16)
        data = bytes.fromhex(payload)
        raw[start:start + len(data)] = data
    return raw

def word(raw, address):
    return raw[address] | raw[address + 1] << 8

def expected(raw):
    return [word(raw, a) for a in (
        0x3EED, 0x3EEF, 0x3EF1, 0x3EF3, 0x3EF5, 0x3EF7,
        0x3EF9, 0x3EFB, 0x3EFD, 0x0936, 0x0948, 0x094A, 0x094C,
        0x0962, 0x096A, 0x097C, 0x096E, 0x0970, 0x0920,
        0x0972, 0x0942, 0x0944, 0x0946, 0x09B8, 0x13E5, 0x13E7,
        0x07F6, 0x4711, 0x4791)]

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--vectors', required=True)
    ap.add_argument('--probe', required=True)
    args = ap.parse_args()
    all_vectors = [json.loads(line) for line in Path(args.vectors).open()
                   if line.strip()]
    # `$85:9A6A` is also reached by an owned-ball contact continuation. This
    # verifier owns only the negative `$093E` ownerless path represented by
    # cpu_update_live_ball; keep the distinct owned continuation out.
    vectors = [vector for vector in all_vectors
               if word(memory(vector['entry']), 0x093E) == 0xFFFF]
    images = [memory(v['entry']) for v in vectors]
    wants = [expected(memory(v['exit'])) for v in vectors]
    run = subprocess.run([args.probe], input=b''.join(images),
                         capture_output=True, check=True)
    got = [[int(item, 16) for item in line.split()]
           for line in run.stdout.decode().splitlines()]
    bad = []
    for index, (want, actual) in enumerate(zip(wants, got), 1):
        if want != actual:
            bad.append((index, [(i, a, b) for i, (a, b) in
                        enumerate(zip(want, actual)) if a != b][:12]))
    if len(got) != len(wants):
        raise AssertionError(f'probe returned {len(got)} rows, expected {len(wants)}')
    if bad:
        for item in bad[:12]: print('call', item[0], item[1])
        raise SystemExit(f'[OWNERLESS BALL] FAIL: vectors={len(vectors)} mismatches={len(bad)}')
    print(f'[OWNERLESS BALL] PASS: vectors={len(vectors)} mismatches=0')

if __name__ == '__main__': main()
