"""Native five-child HUD projection; full shared-task comparison fails until scheduled."""
import argparse
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile

from build_gameplay_hud_assets import validate_native, sha, ROM_SHA

STEPS = [(0x83d0ad, 0x83d156), (0x83d157, 0x83d1b0), (0x83d1b1, 0x83d1fc),
         (0x83d1fd, 0x83d24f), (0x83d2e0, 0x83d332)]
INPUTS = (0x46eb, 0x476b, 0x4711, 0x4791, 0x926, 0x8e4, 0x928, 0x92a,
          0x492b, 0x8de, 0x8e8, 0x9b4, 0x13e7, 0x8e6)
OUTPUTS = (0x8f6, 0x8ee, 0x9b4, 0x13e7, 0x8e6, 0x8f4, 0x8de, 0x928, 0x92a)
OWNED = (0x8f6, 0x8ee, 0x9b4, 0x8e6, 0x8f4)
BUFFERS = {'map': (0x4a70, 0x5070), 'chr': (0x5070, 0x58c0), 'clock': (0x4a60, 0x4a68)}


def word(data, address):
    return struct.unpack_from('<H', data, address)[0]


def blob(native, manifest, name, size):
    data = (native / name).read_bytes()
    entry = manifest.get('artifacts', {}).get(name, {})
    if set(entry) != {'size', 'sha256'} or type(entry['size']) is not int or \
            len(data) != size or entry['size'] != size or entry['sha256'] != sha(data):
        raise ValueError('invalid native publisher artifact: ' + name)
    return data


def read_output(stdout):
    rows = [line for line in stdout.splitlines() if line.startswith('HUD_PUBLICATION')]
    if len(rows) != 5:
        raise ValueError('expected exactly five C publisher rows')
    parsed = []
    for index, line in enumerate(rows):
        if re.fullmatch(r'HUD_PUBLICATION(?: [0-9]+){13}', line) is None:
            raise ValueError('malformed C publisher row')
        values = list(map(int, line.split()[1:]))
        if values[:3] != [index, STEPS[index][0], index + 1] or not 0 <= values[3] <= 15 or \
                any(value > 65535 for value in values[4:]):
            raise ValueError('invalid C publisher sequence/domain')
        parsed.append(values)
    return parsed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('native', 'rom', 'probe', 'pack', 'report'):
        parser.add_argument('--' + name, required=True, type=Path)
    parser.add_argument('--bounded-owned-fields', action='store_true',
                        help='Accept only child buffers/owned fields; explicitly retain shared-task FAIL')
    args = parser.parse_args()
    if sha(args.rom.read_bytes()) != ROM_SHA:
        raise ValueError('wrong original ROM')
    manifest, _ = validate_native(args.native)
    native = []
    lines = []
    for pre, post in STEPS:
        before = blob(args.native, manifest, f'publisher_{pre:06x}.wram', 131072)
        after = blob(args.native, manifest, f'publisher_{post:06x}.wram', 131072)
        native.append(after)
        lines.append(' '.join(map(str, [pre, *(word(before, address) for address in INPUTS)])))
    with tempfile.TemporaryDirectory(prefix='nba95-hud-children-') as temp:
        output = Path(temp)
        run = subprocess.run([str(args.probe.resolve()), str(args.pack.resolve()), temp],
            input='\n'.join(lines) + '\n', capture_output=True, text=True, timeout=30, check=True)
        states = read_output(run.stdout)
        records, owned_failures, shared_failures = [], [], []
        for index, after in enumerate(native):
            for name, (first, last) in BUFFERS.items():
                actual = (output / f'stage_{index:02d}.{name}').read_bytes()
                expected = after[first:last]
                if len(actual) != len(expected):
                    raise ValueError('wrong C publisher buffer size')
                bad = [i for i, (a, b) in enumerate(zip(expected, actual)) if a != b]
                record = dict(routine=STEPS[index][0], buffer=name, compared_bytes=len(expected),
                              different_bytes=len(bad), first_differences=bad[:16])
                records.append(record)
                if bad:
                    owned_failures.append(record)
            for address, actual in zip(OUTPUTS, states[index][4:]):
                expected = word(after, address)
                if actual != expected:
                    mismatch = dict(routine=STEPS[index][0], address=address, expected=expected, actual=actual)
                    (owned_failures if address in OWNED else shared_failures).append(mismatch)
    report = dict(scope='native-entry five-child working buffers and canonical-state projection; no scanout timing claim',
        native_manifest_sha256=sha((args.native / 'manifest.json').read_bytes()),
        probe_sha256=sha(args.probe.read_bytes()), pack_sha256=sha(args.pack.read_bytes()),
        comparisons=records, owned_addresses=list(OWNED), shared_addresses=[a for a in OUTPUTS if a not in OWNED],
        owned_failures=owned_failures, shared_task_failures=shared_failures,
        full_shared_state_pass=not owned_failures and not shared_failures,
        bounded_owned_fields_requested=args.bounded_owned_fields,
        scanout_and_parent_timing='FAIL: native asynchronous task/DMA cadence not implemented')
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    if owned_failures or (shared_failures and not args.bounded_owned_fields):
        raise AssertionError(f'FAIL:{len(owned_failures)} owned and{len(shared_failures)} shared-state differences; see report')
    print('PASS:five native child working buffers/owned fields; shared task/scanout parity remains FAIL')


if __name__ == '__main__':
    main()
