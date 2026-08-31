"""Independent parsed-view corruptions; original frozen files are never changed."""
import argparse, importlib.util, json, sys
from pathlib import Path
from unittest.mock import patch

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--kind', choices=('init','control'), required=True)
    for key in ('verifier','native','rom','exe','output'):p.add_argument('--'+key,type=Path,required=True)
    a=p.parse_args()
    for k,v in vars(a).items():
        if isinstance(v,Path):setattr(a,k,v.resolve())
    a.output.mkdir(parents=True,exist_ok=False)
    sys.path.insert(0,str(a.verifier.parent))
    spec=importlib.util.spec_from_file_location('audited_spc_boundary',a.verifier)
    v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
    def invoke(name):return v.main(argparse.Namespace(native=a.native,rom=a.rom,exe=a.exe,output=a.output/name))
    invoke('baseline');rows=[]
    def case(name,attribute,transform):
        original=getattr(v,attribute);hits=[]
        def changed(*args,**kwargs):
            result=original(*args,**kwargs)
            if transform(args,result):hits.append(str(args[0]))
            return result
        try:
            with patch.object(v,attribute,changed):invoke(name)
        except (ValueError,KeyError,AssertionError,TypeError) as e:rejected=True;reason=str(e)
        else:rejected=False;reason='ACCEPTED'
        assert hits,'unreachable mutation '+name
        rows.append(dict(name=name,rejected=rejected,hits=hits,reason=reason))
    if a.kind=='control':
        for field in ('spc.pc','spc.a','spc.ps','spc.dsp.counter'):
            def change(args,s):
                if args[0].name.startswith('spc_control_1_'):s.pop(field);return True
            case('missing_'+field.replace('.','_'),'state',change)
        for field,value in [('spc.pc','0'),('spc.ps','false'),('spc.a','256')]:
            def change(args,s):
                if args[0].name.startswith('spc_control_1_'):s[field]=value;return True
            case('wrong_'+field.replace('.','_'),'state',change)
        def extra(args,s):s['spc.invented']='0';return True
        case('extra_state_key','state',extra)
    else:
        for field,value in [('spc.pc','0'),('spc.cycle','0'),('spc.a','0'),('spc.ps','256')]:
            def change(args,s):
                if args[0].name=='spc_init_pending_dsp.state':s[field]=value;return True
            case('pending_'+field.replace('.','_'),'state',change)
        def missing(args,s):
            if args[0].name=='spc_init_pending_dsp.state':s.pop('spc.pc');return True
        case('pending_missing_pc','state',missing)
        def extra(args,s):s['spc.invented']='0';return True
        case('extra_state_key','state',extra)
    report=dict(passed=all(r['rejected'] for r in rows),kind=a.kind,cases=rows,verifier_sha256=v.sha(a.verifier))
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(a.kind,sum(r['rejected'] for r in rows),'/',len(rows),'rejected')
    return not report['passed']
if __name__=='__main__':raise SystemExit(main())
