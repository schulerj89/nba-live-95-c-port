"""Compare full-word boundary arithmetic with the exact F34F CMP/BPL contract."""
import argparse, hashlib, json, subprocess
from pathlib import Path


def source(x, y):
    if x == 0 and y == 0:
        return 8, 0
    key = 0
    if x & 0x8000:
        x = (-x) & 65535
        key |= 8
    if y & 0x8000:
        y = (-y) & 65535
        key |= 4
    compared = ((y - 1) - x) & 65535
    if compared == 0 or compared & 0x8000:
        x, y = y, x
        key |= 2
    x = (x << 1) & 65535
    if ((y - 1 - x) & 0x8000) != 0:
        key |= 1
    direction = (0, 1, 2, 1, 4, 3, 2, 3, 0, 7, 6, 7, 4, 5, 6, 5)[key]
    return direction, (y + (x >> 3)) & 65535


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--probe', type=Path, required=True)
    p.add_argument('--rom', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    assert not a.output.exists()
    rom = a.rom.read_bytes()
    assert hashlib.sha256(rom).hexdigest() == '2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
    offset = 5 * 32768 + 0x737a
    assert rom[offset:offset + 9].hex() == 'a5ae3ac5aaf0021011'
    offset = 5 * 32768 + 0x7394
    assert rom[offset:offset + 9].hex() == '06aaa5ae3ac5aa1007'
    points = [0, 1, 2, 3, 7, 8, 31, 127, 255, 0x1fff, 0x2000, 0x3fff,
              0x4000, 0x4001, 0x7ffe, 0x7fff, 0x8000, 0x8001, 0xbfff,
              0xc000, 0xc001, 0xff00, 0xfffe, 0xffff]
    pairs = [(x, y) for x in points for y in points]
    run = subprocess.run([str(a.probe.resolve())], input=''.join(f'{x:x} {y:x}\n' for x, y in pairs), text=True, capture_output=True, timeout=30)
    assert run.returncode == 0 and len(run.stdout.splitlines()) == len(pairs)
    failures = []
    for (x, y), line in zip(pairs, run.stdout.splitlines()):
        actual = tuple(map(int, line.split()))
        expected = source(x, y)
        if actual != expected:
            failures.append(dict(dx=x, dy=y, expected=expected, actual=actual))
    report = dict(passed=not failures, cases=len(pairs), failures=failures,
                  scope='controlled arithmetic source contract; no natural coordinate reachability claim',
                  probe_sha256=hashlib.sha256(a.probe.read_bytes()).hexdigest())
    a.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=report['passed'], cases=len(pairs), failures=len(failures))))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
