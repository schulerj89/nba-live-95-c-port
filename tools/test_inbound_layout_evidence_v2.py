"""Bounded metadata/output corruption checks; original captures untouched."""
import argparse
import copy
import json
from pathlib import Path
import subprocess
from unittest.mock import patch
import verify_inbound_layout_v2 as v


def main():
    p = argparse.ArgumentParser()
    for name in ('capture','probe','output'):
        p.add_argument('--'+name,type=Path,required=True)
    a = p.parse_args()
    assert not a.output.exists()
    assert v.verify(a.capture,a.probe)['passed']
    original = v.read
    checks = []

    def run(name, context):
        reason = ''
        try:
            with context:
                result = v.verify(a.capture,a.probe)
            rejected = not result['passed']
            reason = 'C mismatch' if rejected else 'accepted'
        except (ValueError,KeyError,TypeError) as e:
            rejected,reason = True,str(e)
        checks.append(dict(name=name,rejected=rejected,reason=reason))

    def mutate(name, filename, fn):
        def changed(path):
            data = copy.deepcopy(original(path))
            if Path(path).name == filename:
                fn(data)
            return data
        run(name,patch.object(v,'read',side_effect=changed))

    for key,value in [('case',True),('case',9),('case',2),('schema',True),
                      ('controlled',1),('cpu_writes',True),('rom_patch',True),('exit_code',1)]:
        mutate(f'manifest {key}={value}','manifest.json',lambda m,k=key,x=value:m.update({k:x}))
    mutate('missing artifact','manifest.json',lambda m:m['artifacts'].pop('entry.bin'))
    mutate('wrong script source','manifest.json',lambda m:m['sources']['capture'].update(sha256='0'*64))
    mutate('extra environment','manifest.json',lambda m:m['environment'].update(NBA95_FAKE='1'))
    mutate('state seed','manifest.json',lambda m:m['arguments'].append('--savestate=fake'))
    mutate('changed settings','manifest.json',lambda m:m['isolation']['settings']['Snes'].update(RamPowerOnState='Random'))
    for key in ('a','x','y','ps','sp','d','dbr','k','pc'):
        mutate('CPU word overflow '+key,'entry.json',lambda s,k=key:s['cpu'].update({k:65536}))
    mutate('clock offset','entry.json',lambda s:s.update(frame=s['frame']+1,court=s['court']+1))
    mutate('backward exit','exit.json',lambda s:s.update(frame=s['frame']-1,court=s['court']-1))
    mutate('float clock','entry.json',lambda s:s.update(frame=float(s['frame'])))
    mutate('wrong exit PC','exit.json',lambda s:s['cpu'].update(pc=0xc5bf))
    mutate('missing edge branch','pcs.json',lambda pcs:pcs.remove(0x85c50b))
    mutate('float PC','pcs.json',lambda pcs:pcs.__setitem__(0,float(pcs[0])))
    baseline = v.verify(a.capture,a.probe)
    good = ' '.join(f'{x:04x}' for x in baseline['actual'])+'\n'
    for name,text in [('missing result',''),('extra result',good+good),('large word','10000 '+good.split(' ',1)[1]),
                      ('float word','1.0 '+good.split(' ',1)[1]),('changed result','0000 '+good.split(' ',1)[1])]:
        run(name,patch.object(v.subprocess,'run',return_value=subprocess.CompletedProcess([str(a.probe)],0,text,'')))
    report = dict(passed=all(c['rejected'] for c in checks),checks=checks,
                  verifier_sha256=v.mesen_portable.sha(v.__file__),test_sha256=v.mesen_portable.sha(__file__))
    a.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=report['passed'],cases=len(checks))))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
