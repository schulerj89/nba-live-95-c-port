"""Independent parsed-evidence/probe-protocol cases; original files unchanged."""
import argparse
import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
from unittest.mock import patch


def main():
    p = argparse.ArgumentParser()
    for name in ('verifier', 'capture', 'probe', 'output'):
        p.add_argument('--' + name, type=Path, required=True)
    a = p.parse_args()
    assert not a.output.exists()
    sys.path.insert(0, str(a.verifier.resolve().parent))
    spec = importlib.util.spec_from_file_location('inbound_independent', a.verifier)
    v = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(v)
    baseline = v.verify(a.capture, a.probe)
    assert baseline['passed']
    reader = v.read
    checks = []
    reached = []

    def test(name, filename, mutation):
        reached.clear()
        def changed(path):
            value = copy.deepcopy(reader(path))
            if Path(path).name == filename:
                mutation(value)
                reached.append(str(path))
            return value
        try:
            with patch.object(v, 'read', side_effect=changed):
                result = v.verify(a.capture, a.probe)
            rejected, reason = not result['passed'], 'result mismatch' if not result['passed'] else 'accepted'
        except (ValueError, KeyError, TypeError, AssertionError) as error:
            rejected, reason = True, str(error)
        assert reached, name
        checks.append(dict(name=name, rejected=rejected, reason=reason))

    test('extra_manifest_field', 'manifest.json', lambda m: m.update(state_injection=True))
    test('float_controlled_addresses', 'manifest.json', lambda m: m.update(controlled_words=[float(x) for x in m['controlled_words']]))
    test('wrong_isolation_method', 'manifest.json', lambda m: m['isolation'].update(method='shared user runtime'))
    test('extra_isolation_field', 'manifest.json', lambda m: m['isolation'].update(unattested_state='yes'))
    for key in ('capture', 'runner', 'isolation_helper'):
        source = reader(a.capture / 'manifest.json')['sources'][key]
        alias = a.output.parent / ('alias-' + Path(source['path']).name)
        alias.write_bytes(Path(source['path']).read_bytes())
        test('source_path_alias_' + key, 'manifest.json', lambda m, k=key, path=alias: m['sources'][k].update(path=str(path.resolve())))
    test('extra_wrong_dispatch_branch', 'pcs.json', lambda pcs: pcs.insert(2, 0x85c450))
    good = ' '.join(f'{x:04x}' for x in baseline['actual']) + '\n'
    try:
        with patch.object(v.subprocess, 'run', return_value=subprocess.CompletedProcess([], 0, good, 'probe diagnostic failure\n')):
            result = v.verify(a.capture, a.probe)
        rejected, reason = not result['passed'], 'accepted' if result['passed'] else 'mismatch'
    except (ValueError, KeyError, TypeError, AssertionError) as error:
        rejected, reason = True, str(error)
    checks.append(dict(name='probe_stderr_noise', rejected=rejected, reason=reason))
    report = dict(passed=all(c['rejected'] for c in checks), checks=checks,
                  verifier_sha256=v.mesen_portable.sha(a.verifier),
                  capture_manifest_sha256=v.mesen_portable.sha(a.capture / 'manifest.json'))
    a.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
