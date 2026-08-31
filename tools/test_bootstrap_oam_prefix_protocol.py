"""Reached parsed-view corruptions for the bounded OAM-write child."""
import argparse,contextlib,importlib.util,io,json,sys
from pathlib import Path
from unittest.mock import patch

def main():
    p=argparse.ArgumentParser()
    for key in ('verifier','native','rom','exe','decoder-root','output'):
        p.add_argument('--'+key,type=Path,required=True)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    sys.path.insert(0,str(a.verifier.parent));spec=importlib.util.spec_from_file_location('oam_v',a.verifier)
    v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
    old_rows,old_state=v.rows,v.state
    cases=['baseline','oam_missing','oam_value','oam_address','oam_end','wram_source_address',
           'native_priority','native_internal_address','native_ram_address','native_terminal_pc']
    result=[]
    for case in cases:
        hit=[False]
        def rows(path):
            data=old_rows(path);name=Path(path).name
            if name=='events.jsonl' and case.startswith('oam_'):
                pos=next(i for i,e in enumerate(data)if e['kind']==0 and e['address']==0x2103)
                if case=='oam_missing':data.pop(pos)
                elif case=='oam_value':data[pos]['value']^=1
                elif case=='oam_address':data[pos]['address']=0x2102
                elif case=='oam_end':data[pos]['end']=False
                hit[0]=True
            elif name=='events.jsonl' and case=='wram_source_address':
                e=next(e for e in data if e['kind']==0 and e['pc']==0x808188 and e['address']==0x8fe)
                e['address']=0x7e08fe;hit[0]=True
            elif name=='boundaries.jsonl' and case=='native_terminal_pc':
                e=next(e for e in data if e['tag']=='cpu_ppu_status_request');e['cpu_pc']+=1;hit[0]=True
            return data
        def state(path):
            data=old_state(path)
            if Path(path).name=='cpu_ppu_status_request.state' and case.startswith('native_') and case!='native_terminal_pc':
                key={'native_priority':'ppu.enableOamPriority','native_internal_address':'ppu.internalOamAddress',
                     'native_ram_address':'ppu.oamRamAddress'}[case]
                data[key]='false' if key.endswith('Priority') else str(int(data[key])+1);hit[0]=True
            return data
        argv=['verify','--native',str(a.native.resolve()),'--rom',str(a.rom.resolve()),'--exe',str(a.exe.resolve()),
              '--decoder-root',str(a.decoder_root.resolve()),'--output',str((a.output/case).resolve())]
        outcome='accepted';reason=''
        try:
            with patch.object(sys,'argv',argv),patch.object(v,'rows',side_effect=rows),patch.object(v,'state',side_effect=state),contextlib.redirect_stdout(io.StringIO()):v.main()
        except (ValueError,AssertionError,KeyError,TypeError) as e:outcome='rejected';reason=str(e)
        passed=outcome==('accepted'if case=='baseline'else'rejected')and(case=='baseline'or hit[0])
        result.append(dict(case=case,passed=passed,mutation_reached=hit[0],outcome=outcome,reason=reason));print(case,passed,reason,flush=True)
    (a.output/'report.json').write_text(json.dumps(result,indent=2)+'\n')
    return 0 if all(r['passed']for r in result)else 1

if __name__=='__main__':raise SystemExit(main())
