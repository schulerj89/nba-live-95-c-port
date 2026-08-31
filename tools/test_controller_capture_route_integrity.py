"""Additional independent route/command attestation guards, with in-memory edits."""
import argparse,copy,hashlib,importlib.util,json,sys
from pathlib import Path
from unittest.mock import patch

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for key in ('verifier','capture','probe','rom','report'):p.add_argument('--'+key,type=Path,required=True)
    a=p.parse_args()
    if a.report.exists():raise ValueError('preserve earlier report')
    a.capture=a.capture.resolve();mp=a.capture/'manifest.json'
    sys.path.insert(0,str(a.verifier.resolve().parent))
    spec=importlib.util.spec_from_file_location('route_verifier',a.verifier)
    v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
    original=v.read_json(mp);reader=v.read_json;cases=[]
    def run():return v.verify(a.capture,a.probe.resolve(),a.rom.resolve(),['initialize','allocate','input','transfer','acquire'])
    baseline=run();assert baseline['passed'] and baseline['compared_words']>0
    mutations=[
        ('team variant enabled',lambda m:m['environment'].__setitem__('NBA95_CONTROL_TEAM_VARIANT','1')),
        ('team variant omitted',lambda m:m['environment'].pop('NBA95_CONTROL_TEAM_VARIANT')),
        ('pause route enabled',lambda m:m['environment'].__setitem__('NBA95_CONTROL_PAUSE_AT','50')),
        ('pause route omitted',lambda m:m['environment'].pop('NBA95_CONTROL_PAUSE_AT')),
        ('court limit above executed runner domain',lambda m:(m.__setitem__('court_frames',999999),m['environment'].__setitem__('NBA95_CONTROL_COURT_FRAMES','999999'))),
        ('executed command omitted',lambda m:m.pop('arguments')),
        ('different command ROM path',lambda m:m['arguments'].__setitem__(3,'another-rom.sfc')),
        ('extra command option',lambda m:m['arguments'].append('--invented-option')),
        ('unknown source identity',lambda m:m['sources'].__setitem__('undeclared',copy.deepcopy(m['sources']['capture']))),
        ('unsupported live-pass claim',lambda m:m.__setitem__('live_pass',True)),
        ('undeclared live-pass environment',lambda m:m['environment'].__setitem__('NBA95_CONTROL_LIVE_PASS','1')),
    ]
    for name,edit in mutations:
        changed=copy.deepcopy(original);edit(changed)
        def read(path):return copy.deepcopy(changed) if Path(path).resolve()==mp else reader(path)
        try:
            with patch.object(v,'read_json',side_effect=read):result=run()
        except (ValueError,KeyError,TypeError,OSError) as e:cases.append(dict(name=name,passed=True,rejection=str(e)))
        else:cases.append(dict(name=name,passed=result['passed'] is False,words=result['compared_words']))
    report=dict(verifier_sha256=hashlib.sha256(a.verifier.read_bytes()).hexdigest(),
                manifest_sha256=hashlib.sha256(mp.read_bytes()).hexdigest(),
                baseline_words=baseline['compared_words'],cases=cases,passed=all(c['passed'] for c in cases))
    a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=report['passed'],cases=len(cases),failed=[c['name'] for c in cases if not c['passed']])))
    return 0 if report['passed'] else 1

if __name__=='__main__':raise SystemExit(main())
