"""Independent literal-source prefix/geometry checks; no original fixture edits."""
import argparse, hashlib, json, subprocess
from pathlib import Path


def main():
    p = argparse.ArgumentParser()
    for key in ('probe', 'entry', 'rom', 'output'):
        p.add_argument('--' + key, type=Path, required=True)
    a = p.parse_args()
    a.output.mkdir(parents=True, exist_ok=False)
    original = a.entry.read_bytes()
    assert len(original) == 7936
    rom = a.rom.read_bytes()
    assert hashlib.sha256(rom).hexdigest() == '2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
    table = rom[5 * 32768 + 0x716f:5 * 32768 + 0x718f]
    assert table.hex() == '000102ff040302ff080706ff040506ff000f0eff0c0d0eff08090aff0c0b0aff'
    offsets, offset = {}, 0
    for start, size in [(0, 256), (0x500, 0x500), (0x1600, 0x300), (0x3400, 0x1600)]:
        offsets.update((start + i, offset + i) for i in range(size))
        offset += size

    def word(data, address):
        return data[offsets[address]] | data[offsets[address + 1]] << 8

    def put(data, address, value):
        data[offsets[address]] = value & 255
        data[offsets[address + 1]] = (value >> 8) & 255

    def vector(data, start, size):
        return [word(data, start + i * 2) for i in range(size)]

    def fine(x, y):
        if x == y == 0:
            return 16, 0
        key = 0
        if x & 0x8000:
            x = (-x) & 65535
            key |= 16
        if y & 0x8000:
            y = (-y) & 65535
            key |= 8
        if (y - 1 - x) & 0x8000:
            x, y = y, x
            key |= 4
        five = (x * 5) & 65535
        if (y - 1 - five) & 0x8000:
            three_half = ((x * 3) & 65535) >> 1
            key |= 2 if (y - 1 - three_half) & 0x8000 else 1
        return table[key], (y + (x >> 2)) & 65535

    globals_ = [0x90c, 0x90e, 0x910, 0x936, 0x93a, 0x93e, 0x942, 0x944, 0x946, 0x978, 0x9b8, 0x9c4, 0x9da]
    def state(data):
        return dict(actor_words=vector(data, 0x34eb, 1408), controller_words=vector(data, 0x47eb, 160), context_words=vector(data, 0x46eb, 128), profile_words=vector(data, 0x3449, 20), global_words=[word(data, x) for x in globals_])

    calls, expected = [], []
    def add(name, mode, data, want):
        path = (a.output / (name + '.bin')).resolve()
        path.write_bytes(data)
        calls.append(mode + ' ' + str(path))
        expected.append((name, want))

    points = [0, 1, 2, 3, 7, 8, 31, 127, 255, 0x1fff, 0x2000, 0x3fff, 0x4000, 0x4001, 0x7ffe, 0x7fff, 0x8000, 0x8001, 0xbfff, 0xc000, 0xc001, 0xff00, 0xfffe, 0xffff]
    for x in points:
        for y in points:
            data = bytearray(original)
            put(data, 0xaa, x)
            put(data, 0xae, y)
            direction, distance = fine(x, y)
            add(f'fine-{x:04x}-{y:04x}', 'fine', data, dict(fine=direction, distance=distance))

    for lock in (0, 1, 0x8000, 0xffff):
        for live in (0, 0x82):
            data = bytearray(original)
            passer, receiver = 0x34eb, 0x35eb
            changes = {0x96: passer, 0x8e: receiver, 0xc2: 0xbeef, 0xaa: 0x1234, 0x936: live, 0x9b8: 0x2468, passer: 3, passer + 0x46: lock, passer + 0x38: 0x789a, passer + 0x30: 0x1234, passer + 0x3a: 9, passer + 0x42: 11, passer + 0x18: 13, passer + 0x0c: 5, passer + 0x12: 7, 0x3449 + 12: 0x6ea1, 0x344b + 12: 0xcafe}
            for address, value in changes.items():
                put(data, address, value)
            result = bytearray(data)
            if lock:
                for delta, value in {0x30: 0x789a, 0x3a: 0, 0x42: 0, 0x18: 0xffff, 0x46: 0}.items():
                    put(result, passer + delta, value)
            for address, value in {0x942: 0xbeef, 0x946: 0x1234, 0x9c4: 1, receiver + 0x5e: 10, receiver + 0x60: 40}.items():
                put(result, address, value)
            if live == 0x82:
                put(result, 0x9b8, 1)
            add(f'prefix-{lock:04x}-{live:04x}', 'prefix', data, dict(route=1, prefix_words=[0x6ea1, 0xcafe, receiver], geometry_words=[], **state(result)))

    geometries = [(0, y, 0, 0, 0, 0) for y in (0, 64, 65, 120, 121, 200, 201, 280, 281, 400, 401)]
    geometries += [(256, 0x8000, 0, 0, 0, 0), (516, 0x8000, 0, 0, 0, 0)]
    geometries += [(65000, 32760, v, -v - 1, -v - 1, v) for v in (-32768, -17, -16, -15, -1, 0, 1, 15, 16, 17, 32767)]
    for index, (x, y, avx, avy, bvx, bvy) in enumerate(geometries):
        data = bytearray(original)
        passer, receiver = 0x34eb, 0x35eb
        for address, value in {0x96: passer, 0x8e: receiver, passer + 4: 0, passer + 8: 0, passer + 14: avx, passer + 16: avy, receiver + 4: x, receiver + 8: y, receiver + 14: bvx, receiver + 16: bvy, passer + 0x4e: 0xffff}.items():
            put(data, address, value)
        dx, dy = (x + (bvx >> 3) - (avx >> 4)) & 65535, (y + (bvy >> 3) - (avy >> 4)) & 65535
        direction, distance = fine(dx, dy)
        band = 0
        for limit in (65, 121, 201, 281, 401):
            if (distance - limit) & 0x8000:
                break
            band += 6
        coarse = direction >> 1
        relative = 65535 if coarse >= 8 else (coarse - 0xffff) & 7
        result = bytearray(data)
        put(result, 0x9da, distance)
        put(result, passer + 0x62, band)
        add(f'geometry-{index:02d}', 'geometry', data, dict(route=1, prefix_words=[], geometry_words=[dx, dy, direction, coarse, distance, band, distance, relative, relative, receiver], **state(result)))

    data = bytearray(original)
    for address, value in {0x96: 0x34eb, 0x9e: 0x46eb, 0x9a: 0x47eb, 0x46ef: 0x34eb, 0x47f1: 0, 0x90e: 0xabcd, 0x34eb: 0xffff}.items():
        put(data, address, value)
    for i in range(10):
        put(data, 0x34eb + i * 256 + 0x5e, 8)
    result = bytearray(data)
    put(result, 0x944, 0xabdd)
    add('no-receiver-skips-prefix-even-invalid-descriptor-index', 'chain', data, dict(route=0, prefix_words=[], geometry_words=[], **state(result)))

    run = subprocess.run([str(a.probe.resolve())], input='\n'.join(calls) + '\n', text=True, capture_output=True, timeout=60)
    assert run.returncode == 0 and len(run.stdout.splitlines()) == len(expected)
    failures = []
    for (name, want), line in zip(expected, run.stdout.splitlines()):
        actual = json.loads(line)
        if actual != want:
            failures.append(dict(name=name, fields=[k for k in want if actual.get(k) != want[k]], expected=want, actual=actual))
    report = dict(passed=not failures, cases=len(calls), fine_cases=576, prefix_cases=8, geometry_cases=len(geometries), no_receiver_cases=1, failures=failures, scope='controlled original-source contract, not native reachability', source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(), entry_sha256=hashlib.sha256(original).hexdigest())
    (a.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: v for k, v in report.items() if k != 'failures'}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
