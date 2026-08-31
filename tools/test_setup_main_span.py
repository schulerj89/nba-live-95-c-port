"""C regression: irrelevant source-tail pixels must not alter RGB or raw VRAM."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
from PIL import Image


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'probe', 'rom', 'pack', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(exist_ok=False)
    identities = {name: {'path': str(getattr(args, name).resolve()),
        'sha256': hashlib.sha256(getattr(args, name).read_bytes()).hexdigest()}
        for name in ('exe', 'probe', 'rom', 'pack')}
    raw = bytearray(args.pack.read_bytes())
    count = struct.unpack_from('<I', raw, 12)[0]
    for index in range(count):
        asset, offset = struct.unpack_from('<II', raw, 16 + 24 * index)
        if asset == 133:
            source = offset + (9 * 32 + 144 // 8) * 2
            target = offset + (9 * 32 + 216 // 8) * 2
            assert raw[target:target + 2] != raw[source:source + 2]
            raw[target:target + 2] = raw[source:source + 2]
            break
    else:
        raise AssertionError('missing Season variant')
    poisoned = output / 'poisoned.pak'
    poisoned.write_bytes(raw)
    runs = []
    results = {}
    for name, pack in (('pristine', args.pack.resolve()), ('poisoned', poisoned)):
        bmp = output / (name + '.bmp')
        canvas = output / (name + '.vram')
        commands = [([str(args.exe.resolve()), '--headless', '--setup-only',
            '--setup-main-row', '0', '--setup-main-right', '1', '--frames', '200',
            '--rom', str(args.rom.resolve()), '--assets', str(pack),
            '--dump-frame', str(bmp)], None),
            ([str(args.probe.resolve()), str(pack), str(canvas)], '1 0 0 3\n')]
        for i, (command, stdin) in enumerate(commands):
            run = subprocess.run(command, input=stdin, text=True,
                capture_output=True, check=True, timeout=30)
            (output / f'{name}-{i}.log').write_text(run.stdout + run.stderr)
            runs.append({'command': command, 'stdin': stdin})
        assert canvas.stat().st_size == 65536
        results[name] = {'rgb': hashlib.sha256(Image.open(bmp).convert('RGB').tobytes()).hexdigest(),
                         'vram': hashlib.sha256(canvas.read_bytes()).hexdigest()}
    assert results['pristine'] == results['poisoned'], results
    for name, identity in identities.items():
        assert hashlib.sha256(getattr(args, name).read_bytes()).hexdigest() == identity['sha256']
    report = {'result': 'PASS', 'scope': __doc__, 'sources': identities,
              'commands': runs, 'outputs': results}
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print('PASS Main span: altered source tail leaves complete RGB and 65536-byte canvas unchanged')


if __name__ == '__main__':
    main()
