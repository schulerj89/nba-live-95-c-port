"""Controlled F1C1 and DF7A edge checks; no native fixture modification."""
import argparse, hashlib, json, subprocess
from pathlib import Path


def metric_source(aa, ae):
    if aa & 0x8000:
        aa = (-aa) & 65535
    if ae & 0x8000:
        ae = (-ae) & 65535
    # Literal F1D7 BMI selects the second half; CMP uses its subtraction sign.
    if (ae - aa) & 0x8000:
        b2 = (ae << 1) & 65535
        if (aa - b2) & 0x8000:
            a = ((ae >> 1) + ae) & 65535
            a >>= 2
            return (a + aa) & 65535, 0x85f21c
        return (aa + (ae >> 2)) & 65535, 0x85f228
    b2 = (aa << 1) & 65535
    if (ae - b2) & 0x8000:
        a = ((aa >> 1) + aa) & 65535
        a >>= 2
        return (a + ae) & 65535, 0x85f1f3
    return ((aa >> 2) + ae) & 65535, 0x85f1ff


def main():
    p = argparse.ArgumentParser()
    for key in ('probe', 'metric_probe', 'entry', 'rom', 'output'):
        p.add_argument('--' + key.replace('_', '-'), type=Path, required=True)
    a = p.parse_args()
    a.output.mkdir(parents=True, exist_ok=False)
    rom = a.rom.read_bytes()
    assert hashlib.sha256(rom).hexdigest() == '2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
    body = rom[5 * 32768 + 0x71c1:5 * 32768 + 0x7229]
    assert hashlib.sha256(body).hexdigest() == '3774e5b5b071d2e8bead32614dfb306c7db2aa4dc105287dd2cc29ed95705e30'
    points = [0, 1, 2, 3, 7, 8, 31, 127, 255, 0x1fff, 0x2000, 0x3fff,
              0x4000, 0x4001, 0x7ffe, 0x7fff, 0x8000, 0x8001, 0xbfff,
              0xc000, 0xc001, 0xff00, 0xfffe, 0xffff]
    pairs = [(x, y) for x in points for y in points]
    run = subprocess.run([str(a.metric_probe.resolve())], input=''.join(f'{x:x} {y:x}\n' for x, y in pairs), text=True, capture_output=True, timeout=30)
    assert run.returncode == 0 and len(run.stdout.splitlines()) == len(pairs)
    metrics = [dict(dx=x, dy=y, expected=metric_source(x, y)[0], source_exit=metric_source(x, y)[1], actual=int(line)) for (x, y), line in zip(pairs, run.stdout.splitlines())]

    original = a.entry.read_bytes()
    assert len(original) == 7936
    ranges = [(0, 0x100), (0x500, 0x500), (0x1600, 0x300), (0x3400, 0x1600)]
    offsets, offset = {}, 0
    for base, size in ranges:
        offsets.update((base + i, offset + i) for i in range(size))
        offset += size
    checks = []

    def check(name, direction, candidates, expected_route, expected_score, expected_handoff):
        data = bytearray(original)
        def put(address, value):
            data[offsets[address]] = value & 255
            data[offsets[address + 1]] = (value >> 8) & 255
        for address, value in {0x96: 0x34eb, 0x9e: 0x46eb, 0x9a: 0x47eb, 0x90e: 0xabcd, 0x46ef: 0x34eb, 0x46f7: 0, 0x47f1: direction}.items():
            put(address, value)
        for i in range(10):
            for delta, value in {4: 0, 8: 0, 0x5e: 8, 0x8c: 100}.items():
                put(0x34eb + i * 256 + delta, value)
        put(0x34eb + 0x5e, 0)
        for slot, values in candidates.items():
            for delta, value in values.items():
                put(0x34eb + slot * 256 + delta, value)
        path = (a.output / (name + '.bin')).resolve()
        path.write_bytes(data)
        run = subprocess.run([str(a.probe.resolve())], input='pass ' + str(path) + '\n', text=True, capture_output=True, timeout=30)
        assert run.returncode == 0
        observed = json.loads(run.stdout)
        expected = dict(route=expected_route, score=expected_score, handoff_words=expected_handoff)
        passed = all(observed[key] == value for key, value in expected.items()) and observed['global_words'][6] == 0xabdd
        checks.append(dict(name=name, passed=passed, expected=expected, actual={key: observed[key] for key in expected}, controller_tag=observed['global_words'][6]))

    check('no_receiver_still_tags_controller', 0, {}, 0, 1600, [])
    check('accepted_exact_1600_still_no_initializer', 0, {1: {8: 1600, 0x5e: 0}}, 0, 1600, [])
    check('directional_later_tie', 0, {1: {8: 100, 0x5e: 0}, 2: {8: 100, 0x5e: 0}}, 1, 100, [2, 0x36eb])
    check('neutral_earlier_tie', 8, {1: {8: 100, 0x5e: 0}, 2: {8: 100, 0x5e: 0}}, 1, 100, [1, 0x35eb])
    check('native_suffix_allows_later_less_favored', 8, {1: {8: 120, 0x5e: 0, 0x8c: 50}, 2: {8: 80, 0x5e: 0, 0x8c: 150}}, 1, 80, [2, 0x36eb])
    check('directional_mode_FFFF_accepted_by_CMP7_sign', 0, {1: {8: 100, 0x5e: 65535}}, 1, 100, [1, 0x35eb])
    check('directional_mode_8007_accepted_by_CMP7_sign', 0, {1: {8: 100, 0x5e: 0x8007}}, 1, 100, [1, 0x35eb])
    check('directional_mode_8006_rejected_by_CMP7_sign', 0, {1: {8: 100, 0x5e: 0x8006}}, 0, 1600, [])
    check('directional_mode_7_rejected', 0, {1: {8: 100, 0x5e: 7}}, 0, 1600, [])
    report = dict(passed=all(r['actual'] == r['expected'] for r in metrics) and all(r['passed'] for r in checks), metric_cases=metrics, selection_cases=checks, source_entry_sha256=hashlib.sha256(original).hexdigest(), scope='controlled source-contract checks, not native reachability')
    (a.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=report['passed'], metric_cases=len(metrics), selection_cases=checks)))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
