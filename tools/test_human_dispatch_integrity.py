"""Independent human-stage evidence/protocol mutations; native files stay read-only.

One actual fresh probe invocation supplies cached stdout for later mutations.
Every replay must request exactly the same executable/ROM and native prestates.
The verifier still reads and projects all original native boundary files.
"""
import argparse,copy,hashlib,importlib.util,json,subprocess,sys
from pathlib import Path
from unittest.mock import patch

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('verifier','capture','probe','rom','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);a.capture=a.capture.resolve()
    sys.path.insert(0,str(a.verifier.resolve().parent))
    spec=importlib.util.spec_from_file_location('human_under_audit',a.verifier)
    v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
    mp=a.capture/'manifest.json';original=v.read_json(mp);reader=v.read_json;cases=[];record=[]
    runner=subprocess.run
    def actual(*args,**kwargs):
        run=runner(*args,**kwargs);record.append((copy.deepcopy(args),copy.deepcopy(kwargs),copy.deepcopy(run)));return run
    def verify(label):return v.verify(a.capture,a.probe.resolve(),a.rom.resolve(),a.output/label)
    with patch.object(v.subprocess,'run',side_effect=actual):baseline=verify('baseline')
    assert baseline['passed'] and baseline['compared_values']>0 and len(record)==1
    cases.append(dict(name='actual nonempty native baseline',passed=True,values=baseline['compared_values']))
    recorded_args,recorded_kwargs,recorded_run=record[0]
    def cached(*args,**kwargs):
        assert args==recorded_args and kwargs.get('input')==recorded_kwargs.get('input'),'mutation changed requested native prestate stream'
        return copy.deepcopy(recorded_run)
    def reject(name,operation):
        try:result=operation()
        except (ValueError,KeyError,TypeError,OSError,AssertionError) as error:
            cases.append(dict(name=name,passed=True,rejection=str(error)))
        else:cases.append(dict(name=name,passed=result['passed'] is False,values=result['compared_values']))
        (a.output/'progress.json').write_text(json.dumps(cases,indent=2)+'\n')
        print(name,cases[-1]['passed'],flush=True)
    mutations=[
        ('selection bool',lambda m:m.__setitem__('selection',False)),
        ('requested frames bool',lambda m:m.__setitem__('requested_frames',True)),
        ('requested frames negative',lambda m:m.__setitem__('requested_frames',-1)),
        ('float sparse range',lambda m:m['sparse_ranges'][0].__setitem__(0,0.0)),
        ('missing executed command',lambda m:m.pop('arguments')),
        ('wrong command ROM',lambda m:m['arguments'].__setitem__(3,'wrong-rom.sfc')),
        ('missing environment',lambda m:m.pop('environment')),
        ('wrong selection environment',lambda m:m['environment'].__setitem__('NBA95_HUMAN_SELECTION','2')),
        ('wrong frame environment',lambda m:m['environment'].__setitem__('NBA95_HUMAN_FRAMES','3000')),
        ('empty declared settings',lambda m:m['isolation'].__setitem__('settings',{})),
        ('wrong declared home',lambda m:m['isolation'].__setitem__('home',str(a.output))),
        ('wrong observed home attestation',lambda m:m['isolation'].__setitem__('observed_script_data_folder',str(a.output))),
        ('wrong persisted settings hash',lambda m:m['isolation'].__setitem__('post_settings_sha256','0'*64)),
        ('wrong final save hash',lambda m:m['isolation'].__setitem__('final_saves',{})),
        ('missing trace artifact',lambda m:m['artifacts'].pop('boundaries.jsonl')),
        ('missing runner identity',lambda m:m['sources'].pop('runner')),
    ]
    for index,(name,edit) in enumerate(mutations):
        changed=copy.deepcopy(original);edit(changed)
        def read(path):return copy.deepcopy(changed) if Path(path).resolve()==mp else reader(path)
        def operation():
            with patch.object(v,'read_json',side_effect=read),patch.object(v.subprocess,'run',side_effect=cached):return verify(f'manifest-{index}')
        reject(name,operation)
    def edit_json(text,field,edit):
        lines=text.splitlines()
        for index,line in enumerate(lines[1:],1):
            obj=json.loads(line)
            if field in obj:
                edit(obj);lines[index]=json.dumps(obj);return '\n'.join(lines)+'\n'
        raise AssertionError('mutation field absent from baseline')
    output_mutations=[
        ('wrong gate or B route',lambda t:edit_json(t,'next_pc',lambda o:o.__setitem__('next_pc',o['next_pc']^1))),
        ('wrong accelerator call',lambda t:edit_json(t,'accelerator_call',lambda o:o.__setitem__('accelerator_call',o['accelerator_call']^1))),
        ('wrong actor final word',lambda t:edit_json(t,'actor_words',lambda o:o['actor_words'].__setitem__(-1,o['actor_words'][-1]^1))),
        ('wrong controller final word',lambda t:edit_json(t,'controller_words',lambda o:o['controller_words'].__setitem__(-1,o['controller_words'][-1]^1))),
        ('wrong context final word',lambda t:edit_json(t,'context_words',lambda o:o['context_words'].__setitem__(-1,o['context_words'][-1]^1))),
        ('float route output',lambda t:edit_json(t,'next_pc',lambda o:o.__setitem__('next_pc',float(o['next_pc'])))),
        ('truncated vector',lambda t:edit_json(t,'actor_words',lambda o:o['actor_words'].pop())),
        ('extra unframed output',lambda t:t+'unexpected\n'),
    ]
    for index,(name,edit) in enumerate(output_mutations):
        def output(*args,**kwargs):
            run=cached(*args,**kwargs);run.stdout=edit(run.stdout);return run
        def operation():
            with patch.object(v.subprocess,'run',side_effect=output):return verify(f'output-{index}')
        reject(name,operation)
    result=dict(kind=__doc__,verifier_sha256=hashlib.sha256(a.verifier.read_bytes()).hexdigest(),
        manifest_sha256=hashlib.sha256(mp.read_bytes()).hexdigest(),probe_sha256=hashlib.sha256(a.probe.read_bytes()).hexdigest(),
        cases=cases,passed=all(c['passed'] for c in cases))
    (a.output/'report.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(passed=result['passed'],cases=len(cases),failed=[c['name'] for c in cases if not c['passed']])))
    return 0 if result['passed'] else 1

if __name__=='__main__':raise SystemExit(main())
