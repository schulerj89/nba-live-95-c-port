"""Same corruption inputs against old/new SPC evidence verifiers; no file edits."""
import argparse, importlib.util, json
from pathlib import Path
from unittest.mock import patch

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--kind',choices=('init','control'),required=True)
    for key in ('verifier','native','rom','exe','output'):
        parser.add_argument('--'+key,type=Path,required=True)
    a=parser.parse_args()
    for key,value in vars(a).items():
        if isinstance(value,Path):setattr(a,key,value.resolve())
    a.output.mkdir(parents=True,exist_ok=False)
    spec=importlib.util.spec_from_file_location('spc_evidence_test',a.verifier)
    v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
    def invoke(name):
        return v.main(argparse.Namespace(native=a.native,rom=a.rom,exe=a.exe,output=a.output/name))
    invoke('baseline');results=[]
    def test(name,context,touched):
        try:
            with context:result=invoke(name)
        except (ValueError,TypeError,KeyError,AssertionError,IndexError) as error:
            rejected=True;reason=str(error)
        else:rejected=not result['passed'];reason='accepted'
        assert touched, 'unreachable corruption: '+name
        results.append(dict(name=name,rejected=rejected,reason=reason));print(name,rejected,flush=True)
    original=Path.read_text
    changes=[('missing_arguments',lambda m:m.pop('arguments')),
             ('wrong_arguments',lambda m:m.update(arguments=[])),
             ('wrong_kind',lambda m:m.update(kind='incorrect capture')),
             ('extra_manifest_key',lambda m:m.update(extra=0)),
             ('extra_source_key',lambda m:m['sources']['script'].update(extra=0)),
             ('wrong_script_path',lambda m:m['sources']['script'].update(path=str(v.ROOT/f'tools/mesen_setup_spc_{a.kind}.lua'))),
             ('extra_artifact_key',lambda m:m['artifacts']['capture.lua'].update(extra=0)),
             ('extra_settings_key',lambda m:m['settings']['Snes'].update(extra=0))]
    for name,change in changes:
        touched=[]
        def read(path,*args,**kwargs):
            text=original(path,*args,**kwargs)
            if path==a.native/'manifest.json':
                value=json.loads(text);change(value);text=json.dumps(value);touched.append(True)
            return text
        test(name,patch.object(Path,'read_text',read),touched)
    for name,filename,change in [
        ('duplicate_manifest_key','manifest.json',lambda s:s.replace('"schema":','"schema":0,"schema":',1)),
        ('extra_build_key','build-manifest.json',lambda s:s.replace('"schema":','"extra":0,"schema":',1)),
        ('duplicate_build_key','build-manifest.json',lambda s:s.replace('"schema":','"schema":0,"schema":',1))]:
        touched=[]
        def read(path,*args,**kwargs):
            text=original(path,*args,**kwargs)
            if path.name==filename:
                changed=change(text);assert text!=changed;text=changed;touched.append(True)
            return text
        test(name,patch.object(Path,'read_text',read),touched)
    if a.kind=='init':
        for name,old,new in [('internal_speed','spc.internalSpeed=0','spc.internalSpeed=1'),
                             ('external_speed','spc.externalSpeed=0','spc.externalSpeed=1'),
                             ('writes_disabled','spc.writeEnabled=true','spc.writeEnabled=false'),
                             ('duplicate_state','spc.writeEnabled=true','spc.writeEnabled=false\nspc.writeEnabled=true')]:
            touched=[]
            def read(path,*args,**kwargs):
                text=original(path,*args,**kwargs)
                if path==a.native/'spc_init_post_control.state':
                    assert old in text;text=text.replace(old,new);touched.append(True)
                return text
            test(name,patch.object(Path,'read_text',read),touched)
        runner=v.subprocess.run
        for name,change in [('extra_terminal_cycle',lambda d:d.update(cycles=d['cycles']+1)),
                            ('wrong_instruction_count',lambda d:d.update(instructions=0)),
                            ('wrong_write_count',lambda d:d.update(writes=0))]:
            touched=[]
            def run(*args,**kwargs):
                r=runner(*args,**kwargs)
                if 'clear.input' in str(args[0]):
                    d=json.loads(r.stdout);change(d);r.stdout=json.dumps(d);touched.append(True)
                return r
            test(name,patch.object(v.subprocess,'run',run),touched)
        read_bytes=Path.read_bytes;touched=[]
        def read(path):
            data=read_bytes(path)
            if path.name=='clear.output':
                data=data[:15]+bytes([data[15]^1])+data[16:];touched.append(True)
            return data
        test('wrong_DSP_address_latch',patch.object(Path,'read_bytes',read),touched)
    result=dict(passed=all(r['rejected'] for r in results),cases=results,verifier_sha256=v.sha(a.verifier),test_sha256=v.sha(__file__))
    (a.output/'report.json').write_text(json.dumps(result,indent=2)+'\n')
    return 0 if result['passed'] else 1

if __name__=='__main__':raise SystemExit(main())
