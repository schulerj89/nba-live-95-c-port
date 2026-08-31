"""Independent scheduler protocol/evidence mutations; no native parity claim.

All mutations live in memory. Original captures, manifests and probes are read-only.
The report records failures instead of hiding them behind a later successful case.
"""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
from unittest.mock import patch


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--verifier', type=Path, required=True)
    p.add_argument('--native', type=Path, required=True)
    p.add_argument('--rom', type=Path, required=True)
    p.add_argument('--exe', type=Path, required=True)
    p.add_argument('--report', type=Path, required=True)
    a = p.parse_args()
    if a.report.exists(): raise ValueError('preserve existing report')
    spec = importlib.util.spec_from_file_location('scheduler_under_audit', a.verifier)
    verify = importlib.util.module_from_spec(spec); spec.loader.exec_module(verify)
    a.native = a.native.resolve(); a.exe = a.exe.resolve()
    rows = verify.read_capture(a.native, a.rom)
    verify.check_build(a.exe)
    manifest_path = a.native / 'manifest.json'
    manifest = verify.read_json(manifest_path)
    build_path = a.exe.parent / 'build-manifest.json'
    build = verify.read_json(build_path)
    cases = []

    def reject_manifest(name, changed, target, operation):
        reader = verify.read_json
        def substitute(path):
            return changed if Path(path).resolve() == target else reader(path)
        try:
            with patch.object(verify, 'read_json', side_effect=substitute): operation()
        except (ValueError, KeyError, TypeError, OSError) as error:
            cases.append(dict(name=name, passed=True, rejection=str(error)))
        else:
            cases.append(dict(name=name, passed=False, defect='invalid attestation accepted'))

    for name in ('scheduler.jsonl', 'observed_environment.txt', 'capture_complete.txt'):
        changed = copy.deepcopy(manifest); changed['artifacts'].pop(name)
        reject_manifest('missing artifact '+name, changed, manifest_path,
                        lambda: verify.read_capture(a.native, a.rom))
    for name in ('script', 'runner', 'settings'):
        changed = copy.deepcopy(manifest); changed['sources'].pop(name)
        reject_manifest('missing capture source '+name, changed, manifest_path,
                        lambda: verify.read_capture(a.native, a.rom))
    for name, value in (('schema', True), ('exit_code', False)):
        changed = copy.deepcopy(manifest); changed[name] = value
        reject_manifest('capture bool as '+name, changed, manifest_path,
                        lambda: verify.read_capture(a.native, a.rom))
    changed = copy.deepcopy(manifest)
    changed['isolation']['post_settings_sha256'] = '0' * 64
    reject_manifest('persisted settings identity differs', changed, manifest_path,
                    lambda: verify.read_capture(a.native, a.rom))
    changed = copy.deepcopy(manifest)
    changed['isolation']['settings']['Preferences']['AutoLoadPatches'] = True
    reject_manifest('declared settings disagree with actual', changed, manifest_path,
                    lambda: verify.read_capture(a.native, a.rom))
    for source in tuple(build['sources']):
        changed = copy.deepcopy(build); changed['sources'].pop(source)
        reject_manifest('missing build source '+source, changed, build_path,
                        lambda: verify.check_build(a.exe))
    changed = copy.deepcopy(build); changed['sources'] = {}
    reject_manifest('empty build sources', changed, build_path,
                    lambda: verify.check_build(a.exe))
    for name, value in (('schema', True), ('compiler_exit', False)):
        changed = copy.deepcopy(build); changed[name] = value
        reject_manifest('build bool as '+name, changed, build_path,
                        lambda: verify.check_build(a.exe))

    for name, command in [
        ('partial hex pair', 'queue 0 8 200 0 9 1193046 '+'1g'+'00'*511+'\n'),
        ('uint32 wraparound', 'load 4294967296\nstate 1 0 0\n'),
        ('negative numeric token', 'load -0\n'),
        ('decimal suffix', 'load 12x\n'),
        ('queue hex length overflow', 'queue 0 8 200 0 9 1193046 '+'00'*513+'\n')]:
        run = subprocess.run([str(a.exe)], input=command, capture_output=True, text=True, timeout=10)
        cases.append(dict(name=name, passed=run.returncode != 0, exit=run.returncode,
                          stdout=run.stdout, stderr=run.stderr))

    records = bytearray(512)
    struct.pack_into('<B3sHH', records, 504, 1, bytes.fromhex('563412'), 1, 0x789)
    struct.pack_into('<B3sHH', records, 0, 0xFD, bytes.fromhex('bc9a78'), 2, 0xabc)
    for name, budget, expected in [
        ('two jobs wrap and exact exhaustion', 231,
         ['op 1 24 255 1193046 1 1929', 'op 8 24 0 7903932 2 2748', 'end 0 8 0 0']),
        ('completed first job then second budget stop', 230,
         ['op 1 24 255 1193046 1 1929', 'end 1 0 121 0'])]:
        actual = verify.probe_lines(a.exe, [f'queue 504 8 {budget} 0 9 1193046 '+records.hex()])
        cases.append(dict(name=name, passed=actual == expected, actual=actual, expected=expected))

    # Corrupt each queue publication output field, not an unrelated OAM DMA.
    start, end, _ = next(case for case in verify.queue_cases(rows) if case[2])
    target = next(i for i, row in enumerate(rows) if row['tag'] == 'dma.submit'
                  and start['event'] < row['event'] < end['event'])
    for field in ('dma_mode', 'bbus', 'source', 'size', 'ppu_vram_address',
                  'ppu_vram_increment', 'ppu_vram_increment_high', 'ppu_vram_remapping'):
        changed = copy.deepcopy(rows); changed[target][field] ^= 1
        try: verify.check_queues(changed, a.exe)
        except ValueError as error:
            cases.append(dict(name='corrupt native output '+field, passed=True, rejection=str(error)))
        else: cases.append(dict(name='corrupt native output '+field, passed=False))
    report = dict(scope=__doc__, cases=cases, passed=sum(c['passed'] for c in cases),
                  total=len(cases), identities={str(path.resolve()):hashlib.sha256(path.read_bytes()).hexdigest()
                  for path in (Path(__file__), a.verifier, a.exe, manifest_path, build_path)})
    a.report.parent.mkdir(parents=True, exist_ok=True)
    a.report.write_text(json.dumps(report, indent=2)+'\n')
    print(f"Independent scheduler integrity: {report['passed']}/{report['total']}")
    for case in cases:
        if not case['passed']: print('FAIL:', case['name'])
    if report['passed'] != report['total']: raise SystemExit(1)


if __name__ == '__main__': main()
