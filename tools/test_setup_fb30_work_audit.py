"""Independent FB30 trace protocol mutations, with unchanged native evidence.

Every case freshly executes the requested built C probe. Only the Python view
of a generated C trace is changed; source, native capture and generated files
are never rewritten. The output preserves the actual (uncorrupted) traces plus
the exact in-memory mutation used to test acceptance.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
from unittest.mock import patch


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('verifier', 'native', 'previous', 'rom', 'exe', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    sys.path.insert(0, str(args.verifier.resolve().parent))
    spec = importlib.util.spec_from_file_location('codec_under_audit', args.verifier)
    v = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(v)
    command = ['verify', '--native', str(args.native.resolve()), '--previous-native', str(args.previous.resolve()),
               '--rom', str(args.rom.resolve()), '--exe', str(args.exe.resolve())]
    def run(name):
        with patch.object(sys, 'argv', command + ['--output', str((args.output / name).resolve())]):
            v.main()
    run('baseline')
    reader = v.json_lines
    baseline = reader(args.output / 'baseline/AEA0AF-native-entry.jsonl')
    ci = [r for r in baseline if r['kind'] == 'instruction']
    _, instructions, writes = v.native_records(args.native.resolve())
    ni, _, _ = v.separate_native_effects(1, instructions, writes)
    # Identify an actual recorded refresh so conservation alone can mask an
    # impossible preceding negative interval. No target values are invented.
    target = next(i for i in range(1, len(ci) - 1)
                  if ni[i + 1]['master_clock'] - ni[i]['master_clock']
                  - (ci[i + 1]['master'] - ci[i]['master']) == 40
                  and ci[i]['master'] - ci[i - 1]['master'] < 40)
    changes = []
    def backward(rows):
        instructions = [r for r in rows if r['kind'] == 'instruction']
        row = instructions[target]
        changes.append(dict(instruction=target, pc=row['pc'], before=row['master'], after=row['master'] - 40,
                            resulting_previous_duration=row['master'] - 40 - instructions[target - 1]['master']))
        row['master'] -= 40
    def reorder(rows):
        index = next(i for i in range(len(rows) - 1)
                     if rows[i]['kind'] == 'write' and rows[i + 1]['kind'] == 'instruction')
        changes.append(dict(swapped_rows=[index, index + 1], cycles=[rows[index]['cycle'], rows[index + 1]['cycle']]))
        rows[index], rows[index + 1] = rows[index + 1], rows[index]
    def change_first(rows, kind, field):
        row = next(r for r in rows if r['kind'] == kind)
        changes.append(dict(kind=kind, pc=row['pc'], field=field, before=row[field], after=row[field] ^ 1))
        row[field] ^= 1
    cases = [('backward_intrinsic_master', backward), ('reversed_mixed_trace_order', reorder),
             ('corrupt_instruction_register', lambda r: change_first(r, 'instruction', 'a')),
             ('corrupt_instruction_cycle', lambda r: change_first(r, 'instruction', 'cycle')),
             ('corrupt_cpu_write_cycle', lambda r: change_first(r, 'write', 'cycle')),
             ('corrupt_cpu_write_value', lambda r: change_first(r, 'write', 'value'))]
    results = []
    for name, mutation in cases:
        changes = []
        def altered(path):
            rows = reader(path)
            if path.name == 'AEA0AF-native-entry.jsonl':
                mutation(rows)
            return rows
        with patch.object(v, 'json_lines', side_effect=altered):
            try:
                run(name)
            except (ValueError, AssertionError, KeyError, TypeError) as error:
                result = dict(name=name, passed=True, rejection=str(error), mutations=changes)
            else:
                result = dict(name=name, passed=False, rejection=None, mutations=changes)
        if len(changes) != 1:
            raise AssertionError('mutation was not reached exactly once')
        results.append(result)
        print(name, result['passed'], flush=True)
    report = dict(kind=__doc__, verifier_sha256=hashlib.sha256(args.verifier.read_bytes()).hexdigest(),
                  probe_sha256=hashlib.sha256(args.exe.read_bytes()).hexdigest(),
                  native_manifest_sha256=hashlib.sha256((args.native / 'manifest.json').read_bytes()).hexdigest(),
                  cases=results, passed=all(r['passed'] for r in results))
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=report['passed'], cases=len(results), failed=[r['name'] for r in results if not r['passed']])))
    raise SystemExit(0 if report['passed'] else 1)


if __name__ == '__main__':
    main()
