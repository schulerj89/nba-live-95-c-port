"""Corrupt real native publication traces to test the C asset boundary.

This is parser integrity, not game equivalence. The two positive streams
come from the independently captured production pack, never from C output.
"""
import argparse
from pathlib import Path
import struct
import subprocess
import tempfile


def records(data):
    offset = 16
    for _ in range(struct.unpack_from('<I', data, 12)[0]):
        state = offset
        count_at = offset + 34
        vram, cgram = struct.unpack_from('<HH', data, count_at)
        writes = count_at + 4
        yield state, count_at, writes, vram, cgram
        offset = writes + vram * 4 + cgram * 3
    if offset != len(data):
        raise ValueError('source publication stream has trailing bytes')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probe', type=Path, required=True)
    parser.add_argument('--pack', type=Path, required=True)
    args = parser.parse_args()
    pack = args.pack.read_bytes()
    if pack[:8] != b'NBA95PAK':
        raise ValueError('invalid source asset pack')
    entries = {}
    for index in range(struct.unpack_from('<I', pack, 12)[0]):
        asset, offset, size, *_ = struct.unpack_from('<6I', pack, 16 + 24 * index)
        if asset in (145, 155):
            entries[asset] = pack[offset:offset + size]
    if set(entries) != {145, 155}:
        raise ValueError('source pack lacks both Rules publication traces')
    tested = 0
    with tempfile.TemporaryDirectory(prefix='nba95-publications-integrity-') as temp:
        path = Path(temp) / 'trace.bin'
        for asset, raw in entries.items():
            if raw[:12] != b'NBSPPU3\0' + struct.pack('<I', 3):
                raise ValueError('production source must contain NBSPPU3')
            count = struct.unpack_from('<I', raw, 12)[0]
            def check(data, expected):
                nonlocal tested
                path.write_bytes(data)
                run = subprocess.run([str(args.probe.resolve()), str(path), str(count)],
                                     capture_output=True, timeout=10)
                if run.returncode != expected:
                    raise AssertionError(f'asset{asset}: decoder returned {run.returncode}, expected{expected}')
                tested += 1
            check(raw, 0)
            rows = list(records(raw))
            changes = [(0, 0), (8, 2), (12, 0), (16, 16), (17, 32),
                       (18, 32), (19, 0), (23, 4), (21, 255),
                       (24, 1), (26, 1), (28, 2), (29, 2)]
            for offset, value in changes:
                altered = bytearray(raw)
                altered[offset] = value
                check(altered, 1)
            for state, counts, writes, vram, cgram in rows:
                if vram >= 2:
                    altered = bytearray(raw)
                    altered[writes + 3] = 3
                    check(altered, 1)
                    altered = bytearray(raw)
                    altered[writes + 4:writes + 6] = altered[writes:writes + 2]
                    check(altered, 1)
                    break
            else:
                raise ValueError('native trace has no multi-write VRAM frame')
            for state, counts, writes, vram, cgram in rows:
                if cgram >= 2:
                    at = writes + vram * 4
                    altered = bytearray(raw)
                    struct.pack_into('<H', altered, at, 512)
                    check(altered, 1)
                    altered = bytearray(raw)
                    altered[at + 3:at + 5] = altered[at:at + 2]
                    check(altered, 1)
                    break
            else:
                raise ValueError('native trace has no multi-write CGRAM frame')
            check(raw[:-1], 1)
            check(raw + b'\0', 1)
    print(f'PASS: {tested} native-positive/corrupt-publication decoder checks')


if __name__ == '__main__':
    main()
