"""Literal source-contract guards using private synthetic tables, not native coverage."""
import argparse, itertools, json, subprocess
from pathlib import Path

def base_state():
    s = [0x55aa] * 32
    s[0:8] = [65530, 32767, 65520, 0, 0, 0, 0, 0]
    s[10:12] = [0, 0]
    s[14], s[17], s[19], s[24], s[25] = 4, 1, 0, 0, 0
    return s

def signed_byte(value):
    return value - 256 if value >= 128 else value

def oracle(mode, initial, data):
    s = initial.copy()
    if mode in (2, 3, 5):
        if s[6] >= 57 or s[7] >= 57 or s[14] > 8: return [0, *initial]
        # The private table defines only state0 and phase0..15, with state
        # lookup failures/blank literal phase entries explicitly exercised.
        if s[6] != 0 or s[7] != 0: return [0, *initial]
        phases = [(s[10] * 2) & 65535, (s[11] * 2) & 65535]
        if 0x9300 + phases[0] > 0xfffe or (0x9500 if s[19] else 0x9400) + phases[1] > 0xfffe:
            return [0, *initial]
        s[3] = (s[3] & 32767) | (32768 if s[14] < 3 else 0)
        s[31] = s[14] * 2 + 8
        s[5] = (302 if s[19] else 202) + phases[1] // 2 if phases[1] < 32 else 0
        s[4] = 114 + phases[0] // 2 if phases[0] < 32 else 0
        if s[4] < 240 and s[17] == (1 if s[14] < 3 else 0): s[4] += 40
        s[29], s[30] = 0x9300, 0x84
        s[8:10], s[12:14] = s[6:8], s[10:12]
        if s[14] != 8: s[15] = s[14]
    if mode in (0, 1, 3, 5):
        if s[4] >= 0x830 or s[5] >= 0x830: return [0, *initial]
        if mode != 0: s[25] = 0
        point = int(s[25] != 0)
        ly, lz = map(signed_byte, data[:2])
        ux, uy, uz = [signed_byte((value + point) % 256) for value in data[2:]]
        flags = s[3] ^ (3 if s[3] & 32768 else 0)
        if flags & 2: ly = -ly
        if flags & 1: uy = -uy
        # Python floor division is the independent signed midpoint oracle.
        mid = (ly + uy) // 2
        s[25:28] = [(mid - 2 * ux) % 65536, (mid + 2 * ux) % 65536, (ux - lz - uz) % 65536]
        s[28], s[29] = (65535 if flags & 1 else 0), (65535 if flags & 2 else 0)
        if mode != 0:
            s[23], s[20], s[21] = s[20], (s[0] + s[25]) % 65536, (s[1] + s[26]) % 65536
        if mode in (3, 5): s[22] = (s[2] + s[27]) % 65536
    if mode in (4, 5):
        s[16], s[18] = 15, s[18] | 6
        if (s[24] - 128) % 65536 >= 32768: s[24] = 2
    return [1, *s]

def main():
    p = argparse.ArgumentParser(); p.add_argument('--probe', type=Path, required=True); p.add_argument('--output', type=Path, required=True); a = p.parse_args()
    cases = []
    def add(label, mode, s, values): cases.append((label, mode, s, values, oracle(mode, s, values)))
    vectors = [[0, 0, 0, 0, 0], [9, 0, 0, 1, 0], [255, 0, 0, 0, 0], [128, 128, 128, 128, 128],
               [127, 127, 127, 127, 127], [255, 1, 127, 128, 255], [128, 0, 0, 127, 0], [1, 255, 255, 254, 128]]
    for mode, flags, point, values in itertools.product((0, 1), (0, 1, 2, 3, 0x8000, 0x8001, 0x8002, 0xffff), (0, 1, 256, 65535), vectors):
        s = base_state(); s[3], s[25] = flags, point; add('offset/attach', mode, s, values)
    for mode, facing, alternate, variant, phase in itertools.product((2, 3, 5), (0, 2, 3, 7, 8), (0, 65535), (0, 1, 256), (0, 1, 15, 0x8000)):
        s = base_state(); s[14], s[19], s[17], s[10], s[11] = facing, alternate, variant, phase, phase
        add('resolver/prefix/combined', mode, s, [128, 255, 127, 255, 128])
    for live in (0, 1, 127, 128, 129, 0x7fff, 0x8000, 0x807f, 0x8080, 0xffff):
        s = base_state(); s[24] = live; add('wrapped commit', 4, s, vectors[0])
    for mode, index, value in [(0, 4, 0x830), (1, 5, 0x830), (2, 6, 57), (3, 7, 1), (5, 14, 9), (2, 10, 0x4000), (2, 11, 0x4000)]:
        s = base_state(); s[index] = value; add('invalid domain no mutation', mode, s, vectors[0])
    data = '\n'.join(' '.join(map(str, [mode, *s, *values])) for _, mode, s, values, _ in cases) + '\n'
    run = subprocess.run([str(a.probe.resolve())], input=data, text=True, capture_output=True)
    assert run.returncode == 0 and not run.stderr, run.stderr
    rows = [list(map(int, line.split())) for line in run.stdout.splitlines()]; assert len(rows) == len(cases)
    failures = [dict(index=i, label=c[0], inputs=c[2], bytes=c[3], expected=c[4], actual=actual) for i, (c, actual) in enumerate(zip(cases, rows)) if c[4] != actual]
    report = dict(passed=not failures, cases=len(cases), failures=failures)
    a.output.write_text(json.dumps(report, indent=2)); print(json.dumps(report))
    return 1 if failures else 0

if __name__ == '__main__': raise SystemExit(main())
