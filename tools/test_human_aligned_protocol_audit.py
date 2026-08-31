"""Additional typed-output and diagnostic protocol cases; captures unchanged."""
import argparse
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
from unittest.mock import patch


def main():
    p=argparse.ArgumentParser()
    for key in ('verifier','capture','probe','rom','output'):
        p.add_argument('--'+key,type=Path,required=True)
    a=p.parse_args()
    a.output.mkdir(parents=True,exist_ok=False)
    sys.path.insert(0,str(a.verifier.resolve().parent))
    spec=importlib.util.spec_from_file_location('action_protocol_audit',a.verifier)
    v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
    report=v.verify(a.capture.resolve(),a.probe.resolve(),a.rom.resolve(),a.output/'baseline.json')
    assert report['passed']
    stdout=(a.output/'baseline.probe-stdout.txt').read_text()
    stderr=(a.output/'baseline.probe-stderr.txt').read_text()
    lines=stdout.splitlines();cases=[]
    selected=next(i for i,line in enumerate(lines) if 'dp_words' in json.loads(line))
    def changed(name,fn):
        row=json.loads(lines[selected]);fn(row)
        cases.append((name,'\n'.join([*lines[:selected],json.dumps(row),*lines[selected+1:]])+'\n',stderr))
    changed('missing dp vector',lambda r:r.pop('dp_words'))
    changed('extra C field',lambda r:r.update(ignored_result=1))
    changed('invalid route highword',lambda r:r.update(route=65536))
    changed('invalid profile byte highword',lambda r:r.update(profile_byte=256))
    changed('short actor vector',lambda r:r['actor_words'].pop())
    changed('noninteger descriptor word',lambda r:r['dp_words'].__setitem__(1,False))
    cases.append(('duplicate result key',stdout.replace('"route":','"route":0,"route":',1),stderr))
    cases.append(('extra failure stderr',stdout,stderr+'ERROR: unexpected native input\n'))
    cases.append(('missing asset load diagnostic',stdout,''))
    cases.append(('forged asset diagnostic',stdout,'[ASSETS] Loaded a different asset pack\n'))
    checks=[]
    for i,(name,out,err)in enumerate(cases):
        try:
            with patch.object(v.subprocess,'run',return_value=subprocess.CompletedProcess([],0,out,err)):
                result=v.verify(a.capture.resolve(),a.probe.resolve(),a.rom.resolve(),a.output/f'case-{i:02d}.json')
            rejected=not result['passed'];reason='C mismatch'if rejected else'accepted'
        except (ValueError,KeyError,TypeError,AssertionError)as error:rejected=True;reason=str(error)
        checks.append(dict(name=name,rejected=rejected,reason=reason))
    result=dict(passed=all(c['rejected']for c in checks),checks=checks,verifier_sha256=v.sha(a.verifier))
    (a.output/'report.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
    return 0 if result['passed']else 1


if __name__=='__main__':raise SystemExit(main())
