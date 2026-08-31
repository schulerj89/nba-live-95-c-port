"""Independent controller verifier rejection tests; native files remain read-only."""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
from unittest.mock import patch


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('verifier', 'capture', 'probe', 'rom', 'report'):
        p.add_argument('--'+name, required=True, type=Path)
    a = p.parse_args()
    if a.report.exists(): raise ValueError('preserve existing audit report')
    a.capture = a.capture.resolve()
    sys.path.insert(0, str(a.verifier.resolve().parent))
    spec = importlib.util.spec_from_file_location('controller_under_audit', a.verifier)
    v = importlib.util.module_from_spec(spec); spec.loader.exec_module(v)
    manifest_path = a.capture/'manifest.json'
    original = v.read_json(manifest_path)
    if original['selection'] != 1: raise ValueError('use immutable neutral capture with no input calls')
    modes = ['initialize','allocate','input','transfer','acquire']
    cases = []

    def verify(selected=modes):
        return v.verify(a.capture, a.probe.resolve(), a.rom.resolve(), selected)

    clean = verify()
    if not clean['passed'] or clean['compared_words'] == 0:
        raise ValueError('baseline must reach successful actual native comparison')
    cases.append(dict(name='unmodified nonempty native baseline', passed=True,
                      words=clean['compared_words'], calls=clean['calls']))

    def reject(name, operation):
        try:
            result = operation()
        except (ValueError, KeyError, TypeError, OSError, AssertionError) as error:
            cases.append(dict(name=name, passed=True, rejection=str(error)))
        else:
            cases.append(dict(name=name, passed=result.get('passed') is False,
                              words=result.get('compared_words'), failures=result.get('failures')))

    def manifest_case(name, edit):
        changed = copy.deepcopy(original); edit(changed)
        reader = v.read_json
        def read(path):
            return copy.deepcopy(changed) if Path(path).resolve() == manifest_path else reader(path)
        def check():
            with patch.object(v, 'read_json', side_effect=read): return verify()
        reject(name, check)

    for key in ('ownership.jsonl','capture_complete.txt','capture.lua','initial-mesen-settings.json','observed_environment.txt'):
        if key in original['artifacts']:
            manifest_case('missing artifact '+key, lambda m, k=key: m['artifacts'].pop(k))
    for key in ('capture','runner','isolation_helper'):
        if key not in original['sources']: raise ValueError('fixture source missing '+key)
        manifest_case('missing source '+key, lambda m, k=key: m['sources'].pop(k))
    for key, value in [('schema',2.0),('selection',True),('selection',-1),('court_frames',False),
                       ('exit_code',False),('rom_patch',0),('state_injection',0)]:
        manifest_case('invalid '+key+' '+repr(value), lambda m,k=key,x=value: m.__setitem__(k,x))
    manifest_case('wrong persisted settings hash', lambda m: m['isolation'].__setitem__('post_settings_sha256','0'*64))
    manifest_case('empty declared settings', lambda m: m['isolation'].__setitem__('settings',{}))
    manifest_case('integer settings verification', lambda m: m['isolation'].__setitem__('post_settings_verified',1))
    manifest_case('wrong final save identity', lambda m: m['isolation'].__setitem__('final_saves',{'fake.srm':'0'*64}))
    manifest_case('wrong declared selection environment', lambda m: m['environment'].__setitem__('NBA95_CONTROL_SELECTION','0'))
    manifest_case('float artifact byte size', lambda m: m['artifacts']['ownership.jsonl'].__setitem__('bytes',float(m['artifacts']['ownership.jsonl']['bytes'])))
    reject('requested input with zero comparisons', lambda: verify(['input']))

    # Corrupt actual fresh probe responses, never synthesize expected values.
    runner = subprocess.run
    def output_case(name, edit):
        def changed(*args, **kwargs):
            run = runner(*args, **kwargs)
            if run.returncode: raise ValueError('clean probe unexpectedly failed')
            run.stdout = edit(run.stdout)
            return run
        def check():
            with patch.object(v.subprocess, 'run', side_effect=changed): return verify()
        reject(name, check)

    def change_json(text, edit):
        obj = json.loads(text); edit(obj); return json.dumps(obj)+'\n'
    for field in ('records','assignments','previous','counts','cursors'):
        output_case('wrong '+field+' final word', lambda t,f=field: change_json(t,lambda o:o[f].__setitem__(-1,o[f][-1]^1)))
    output_case('missing output field', lambda t: change_json(t,lambda o:o.pop('cursors')))
    output_case('extra output field', lambda t: change_json(t,lambda o:o.__setitem__('ignored',0)))
    output_case('bool output word', lambda t: change_json(t,lambda o:o['counts'].__setitem__(0,False)))
    output_case('float output word', lambda t: change_json(t,lambda o:o['counts'].__setitem__(0,float(o['counts'][0]))))
    output_case('truncated output list', lambda t: change_json(t,lambda o:o['records'].pop()))
    output_case('unframed extra stdout', lambda t:'unframed diagnostics\n'+t)
    output_case('duplicate JSON response', lambda t:t+t)
    result = dict(kind='independent in-memory controller verifier mutations; no native files edited',
                  verifier_sha256=hashlib.sha256(a.verifier.read_bytes()).hexdigest(),
                  manifest_sha256=hashlib.sha256(manifest_path.read_bytes()).hexdigest(),
                  probe_sha256=hashlib.sha256(a.probe.read_bytes()).hexdigest(),
                  cases=cases, passed=all(c['passed'] for c in cases))
    a.report.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(passed=result['passed'],cases=len(cases),
                         failed=[c['name'] for c in cases if not c['passed']])))
    return 0 if result['passed'] else 1


if __name__ == '__main__': raise SystemExit(main())
