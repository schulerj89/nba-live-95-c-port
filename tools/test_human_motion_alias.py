"""Independent controlled $91C3/$A82C carried-X contracts; no native injection.

Each case copies an existing native entry to a NEW file and changes explicit
input words for a C-only source contract. Native source fixtures stay unchanged.
The source oracle is AB06 LDA +72,X / BEQ / SEC / SBC original DP C6 / BPL / LDA 0.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess

RANGES = [(0, 0x100), (0x500, 0x500), (0x1600, 0x300), (0x3400, 0x1600)]


def offset(address):
    done = 0
    for base, size in RANGES:
        if base <= address < base + size:
            return done + address - base
        done += size
    raise ValueError(f'address outside sparse snapshot: {address:x}')


def word(data, address):
    return struct.unpack_from('<H', data, offset(address))[0]


def write(data, address, value):
    struct.pack_into('<H', data, offset(address), value)


def decrease(value, delta):
    if value == 0:
        return 0
    difference = (value - delta) & 0xffff
    return 0 if difference & 0x8000 else difference


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probe', type=Path, required=True)
    parser.add_argument('--rom', type=Path, required=True)
    parser.add_argument('--entry', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    original = args.entry.read_bytes()
    if len(original) != sum(n for _, n in RANGES):
        raise ValueError('wrong sparse input length')
    actor, record, context = [word(original, a) for a in (0x96, 0x9a, 0x9e)]
    if not 0x47eb <= record <= 0x48ab:
        raise ValueError('selected +72 must fit existing controller vector')
    # name, live word, full z word, actor timer, controller-relative timer, delta
    cases = [
        ('airborne_controller_word', 0, 8, 5, 7, 2),
        ('airborne_controller_zero', 0, 8, 5, 0, 2),
        ('z_high_byte_is_not_zero', 0, 0x100, 5, 7, 2),
        ('z_negative_is_not_zero', 0, 0xffff, 5, 7, 2),
        ('wrapped_cmp_negative', 0x8080, 8, 5, 7, 2),
        ('borrow_does_not_clamp', 0, 8, 5, 2, 0xffff),
        ('negative_result_clamps', 0, 8, 5, 1, 2),
        ('high_delta_wraps_positive', 0, 8, 5, 0x8001, 0x8000),
        ('zero_delta', 0, 8, 5, 7, 0),
        ('live81_actor_word', 0x81, 8, 5, 7, 2),
        ('live80_actor_word', 0x80, 8, 5, 7, 2),
        ('wrapped_cmp_nonnegative_actor_word', 0x807f, 8, 5, 7, 2),
    ]
    inputs, expectations = [], []
    for name, live, z, boost, tail, delta in cases:
        data = bytearray(original)
        changes = {0x936: live, actor + 0xc: z, actor + 0x72: boost,
                   record + 0x72: tail, 0xc6: delta,
                   # Avoid unrelated stage gates, using explicit semantic inputs.
                   0x978: 0, actor + 0x7a: 0, actor + 0x7e: 0,
                   0x946: 0xffff, 0x952: (word(data, context + 0xc) + 1) & 0xffff}
        for address, value in changes.items():
            write(data, address, value)
        path = args.output / (name + '.bin')
        path.write_bytes(data)
        carried = bool(((live - 0x80) & 0x8000) and z)
        expected = dict(accelerator_call=0x85a82c,
                        actor_boost=boost if carried else decrease(boost, delta),
                        controller_relative_72=decrease(tail, delta) if carried else tail,
                        velocity_x=word(data, actor + 0xe), velocity_y=word(data, actor + 0x10))
        expectations.append(dict(name=name, kind='controlled C source contract',
                                 changed_words={hex(k): v for k, v in changes.items()}, expected=expected))
        inputs.append('motion ' + str(path.resolve()))
    run = subprocess.run([str(args.probe.resolve()), str(args.rom.resolve())],
                         input='\n'.join(inputs) + '\n', text=True, capture_output=True, timeout=120)
    (args.output / 'stdout.txt').write_text(run.stdout)
    (args.output / 'stderr.txt').write_text(run.stderr)
    if run.returncode:
        raise ValueError(f'probe failed: {run.returncode}')
    lines = run.stdout.splitlines()
    if lines.pop(0) != '[ROM] Loaded successfully: "NBA Live \'95         " (Reset: 0x800D, Headered: No, Size: 1536 KiB)':
        raise ValueError('unexpected loader diagnostic')
    if len(lines) != len(cases):
        raise ValueError('wrong C result count')
    for result, line in zip(expectations, lines):
        parsed = json.loads(line)
        actual = dict(accelerator_call=parsed['accelerator_call'], actor_boost=parsed['actor_words'][0x72 // 2],
                      controller_relative_72=parsed['controller_words'][(record + 0x72 - 0x47eb) // 2],
                      velocity_x=parsed['actor_words'][7], velocity_y=parsed['actor_words'][8])
        result.update(actual=actual, passed=actual == result['expected'])
    report = dict(kind='controlled C contracts; not native injected executions',
                  entry=str(args.entry.resolve()), entry_sha256=hashlib.sha256(original).hexdigest(),
                  probe_sha256=hashlib.sha256(args.probe.read_bytes()).hexdigest(),
                  cases=expectations, passed=all(x['passed'] for x in expectations))
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=report['passed'], cases=len(cases), failures=[x['name'] for x in expectations if not x['passed']])))
    raise SystemExit(0 if report['passed'] else 1)


if __name__ == '__main__':
    main()
