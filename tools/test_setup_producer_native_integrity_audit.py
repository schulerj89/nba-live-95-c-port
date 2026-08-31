"""Independent native-view chronology/association mutations; fixtures unchanged."""
import argparse, hashlib, importlib.util, json, sys
from pathlib import Path
from unittest.mock import patch


def main():
    parser = argparse.ArgumentParser()
    for name in ('verifier', 'native', 'previous', 'rom', 'exe', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    a = parser.parse_args()
    a.output.mkdir(parents=True, exist_ok=False)
    sys.path.insert(0, str(a.verifier.resolve().parent))
    spec = importlib.util.spec_from_file_location('producer_native_audit', a.verifier)
    v = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(v)
    command = ['verify', '--native', str(a.native.resolve()), '--previous-native', str(a.previous.resolve()), '--rom', str(a.rom.resolve()), '--exe', str(a.exe.resolve())]
    def run(name):
        with patch.object(sys, 'argv', command + ['--output', str((a.output / name).resolve())]):
            v.main()
    run('baseline')
    reader = v.json_lines
    mutations = []
    def dma_backward(rows):
        index = next(i for i, r in enumerate(rows) if r['dma_active'] == 2)
        for offset in (0, 1):
            row = rows[index + offset]
            mutations.append(dict(event=row['event'], old_master=row['master_clock'], new_master=offset * 4))
            row['master_clock'] = offset * 4
    def write_backward(rows):
        row = next(r for r in rows if r['kind'] == 'write' and not r['dma_active'])
        mutations.append(dict(event=row['event'], old_master=row['master_clock'], new_master=0))
        row['master_clock'] = 0
    def dma_pc(rows):
        for row in rows:
            if row['dma_active'] == 2:
                mutations.append(dict(event=row['event'], old_pc=row['pc'], new_pc=0))
                row['pc'] = 0
                return
    def reorder_instructions(rows):
        rows[1], rows[2] = rows[2], rows[1]
        for index in (1, 2):
            rows[index]['instruction'] = index
        mutations.append(dict(swapped_indices=[1, 2], note='ordinals renumbered; CPU/master clocks expose reversal'))
    def reorder_bus(rows):
        index = next(i for i in range(len(rows)-1) if rows[i]['kind'] == 'write' and rows[i + 1]['dma_active'] == 0)
        rows[index], rows[index + 1] = rows[index + 1], rows[index]
        for position in (index, index + 1):
            rows[position]['event'] = position
        mutations.append(dict(swapped_indices=[index, index + 1], note='ordinals renumbered; CPU/master clocks expose reversal'))
    cases = [('dma_pair_before_scope', 'producer_bus.jsonl', dma_backward),
             ('cpu_write_before_scope', 'producer_bus.jsonl', write_backward),
             ('dma_wrong_source_pc', 'producer_bus.jsonl', dma_pc),
             ('native_instruction_clock_reversal', 'producer_instructions.jsonl', reorder_instructions),
             ('native_mixed_bus_clock_reversal', 'producer_bus.jsonl', reorder_bus)]
    results = []
    for name, file, mutate in cases:
        mutations = []
        def altered(path):
            rows = reader(path)
            if path.name == file:
                mutate(rows)
            return rows
        try:
            with patch.object(v, 'json_lines', side_effect=altered):
                run(name)
        except (ValueError, AssertionError, KeyError, TypeError) as error:
            rejected, reason = True, str(error)
        else:
            rejected, reason = False, ''
        assert mutations, 'mutation unreachable'
        results.append(dict(name=name, rejected=rejected, reason=reason, mutations=mutations))
        print(name, rejected, flush=True)
    report = dict(passed=all(r['rejected'] for r in results), cases=results, verifier_sha256=hashlib.sha256(a.verifier.read_bytes()).hexdigest(), native_manifest_sha256=hashlib.sha256((a.native / 'manifest.json').read_bytes()).hexdigest())
    (a.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
