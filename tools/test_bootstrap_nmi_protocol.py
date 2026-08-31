"""Fresh probe plus reached in-memory corruptions; immutable captures unchanged.

These are implementer regressions, not an independent audit. Every negative
case must reach its mutation and the actual full verifier must reject it.
"""
import argparse,contextlib,importlib.util,io,json,sys
from pathlib import Path
from unittest.mock import patch

def main():
    p=argparse.ArgumentParser()
    for k in ('verifier','native','rom','exe','decoder-root','output'):p.add_argument('--'+k,type=Path,required=True)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    sys.path.insert(0,str(a.verifier.parent));spec=importlib.util.spec_from_file_location('v',a.verifier)
    v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
    old_rows,old_state=v.rows,v.state
    cases=['baseline','nmi_missing_cycle','nmi_duplicate_cycle','nmi_sample','nmi_complete',
           'nmi_stack','nmi_vector_byte','nmi_marker_order','nmi_marker_pc','nmi_bus_pc',
           'nmi_register_read','state_open_bus','state_auto_next','state_auto_enable',
           'state_need','state_delay','state_missing_field','native_vector_hook','native_terminal_hook']
    reports=[]
    for case in cases:
        hit=[False]
        def rows(path):
            d=old_rows(path)
            if Path(path).name=='events.jsonl'and case.startswith('nmi_'):
                positions=[i for i,x in enumerate(d)if x['kind']==9];assert len(positions)==8
                e=d[positions[0]]
                if case=='nmi_missing_cycle':d.pop(positions[0])
                elif case=='nmi_duplicate_cycle':d.insert(positions[0],dict(e))
                elif case=='nmi_sample':e['sample_master']=0
                elif case=='nmi_complete':d[positions[-1]]['end']=False
                elif case=='nmi_stack':d[positions[2]]['value']^=1
                elif case=='nmi_vector_byte':d[positions[-1]]['value']^=1
                elif case=='nmi_marker_order':
                    j=next(i for i,x in enumerate(d)if x['kind']==10)
                    d.insert(positions[-1],d.pop(j))
                elif case=='nmi_marker_pc':next(x for x in d if x['kind']==10)['pc']+=1
                elif case=='nmi_bus_pc':e['pc']+=1
                elif case=='nmi_register_read':
                    next(x for x in d if x['kind']==0 and x['bus']==0 and x['address']&65535==0x4210)['value']^=16
                else:raise AssertionError(case)
                hit[0]=True
            if Path(path).name=='boundaries.jsonl'and case.startswith('native_'):
                tag='nmi_vector_entry'if case=='native_vector_hook'else'cpu_oam_request'
                next(x for x in d if x['tag']==tag)['cpu_pc']+=1;hit[0]=True
            return d
        def state(path):
            d=old_state(path)
            if Path(path).name=='final.nmi'and case.startswith('state_'):
                if case=='state_open_bus':d['memoryManager.openBus']='0'
                elif case=='state_auto_next':d['internalRegisters.autoReadNextClock']=str(int(d['internalRegisters.autoReadNextClock'])+128)
                elif case=='state_auto_enable':d['internalRegisters.enableAutoJoypadRead']='false'
                elif case=='state_need':d['cpu.needNmi']='true'
                elif case=='state_delay':d['cpu.nmiFlagCounter']='1'
                elif case=='state_missing_field':del d['internalRegisters.irqFlag']
                else:raise AssertionError(case)
                hit[0]=True
            return d
        argv=['verify','--native',str(a.native.resolve()),'--rom',str(a.rom.resolve()),'--exe',str(a.exe.resolve()),'--decoder-root',str(a.decoder_root.resolve()),'--output',str((a.output/case).resolve())]
        outcome='accepted';reason=''
        try:
            with patch.object(sys,'argv',argv),patch.object(v,'rows',side_effect=rows),patch.object(v,'state',side_effect=state),contextlib.redirect_stdout(io.StringIO()):v.main()
        except(ValueError,AssertionError,KeyError,TypeError)as e:outcome='rejected';reason=str(e)
        passed=outcome==('accepted'if case=='baseline'else'rejected')and(case=='baseline'or hit[0])
        reports.append(dict(case=case,passed=passed,mutation_reached=hit[0],outcome=outcome,reason=reason))
        print(case,passed,reason,flush=True)
    (a.output/'report.json').write_text(json.dumps(reports,indent=2)+'\n')
    return 0 if all(r['passed']for r in reports)else 1
if __name__=='__main__':raise SystemExit(main())
