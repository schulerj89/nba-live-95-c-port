"""Additional protocol mutations; actual C/native files remain untouched."""
import argparse,importlib.util,json,sys
from pathlib import Path

def main():
    p=argparse.ArgumentParser()
    for name in ('native','rom','exe','decoder-root','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    spec=importlib.util.spec_from_file_location('bootstrap_verify_v2',Path(__file__).with_name('verify_bootstrap_v2.py'))
    v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v);original=v.rows
    cases=['baseline','CPU_fetch_address','CPU_delete_idle','CPU_duplicate_idle','SPC_delete_fetch',
           'SPC_duplicate_fetch','SPC_entry_master','SPC_final_end','SPC_late_address_width',
           'resident_marker_missing','F1_marker_pc','CPU_write_sample','CPU_spc_count']
    results=[]
    for case in cases:
        hit=[False]
        def rows(path):
            d=original(path)
            if Path(path).name!='events.jsonl' or case=='baseline':return d
            predicates={
                'CPU_fetch_address':lambda q:q['kind']==0 and q['bus']==0 and q['pc']==q['address'],
                'CPU_delete_idle':lambda q:q['kind']==0 and q['bus']==2,
                'CPU_duplicate_idle':lambda q:q['kind']==0 and q['bus']==2,
                'SPC_delete_fetch':lambda q:q['kind']==1 and q['bus']==1,
                'SPC_duplicate_fetch':lambda q:q['kind']==1 and q['bus']==1,
                'SPC_entry_master':lambda q:q['kind']==6,
                'SPC_final_end':lambda q:q['kind']==1,
                'SPC_late_address_width':lambda q:q['kind']==1 and q['bus']==3 and q['pc']==0x38b,
                'resident_marker_missing':lambda q:q['kind']==3,
                'F1_marker_pc':lambda q:q['kind']==2,
                'CPU_write_sample':lambda q:q['kind']==0 and q['bus']==1,
                'CPU_spc_count':lambda q:q['kind']==0}
            indices=[i for i,q in enumerate(d)if predicates[case](q)];assert indices
            i=indices[-1]if case=='SPC_final_end'else indices[0];q=d[i];hit[0]=True
            if case in ('CPU_delete_idle','SPC_delete_fetch','resident_marker_missing'):d.pop(i)
            elif case in ('CPU_duplicate_idle','SPC_duplicate_fetch'):d.insert(i,dict(q))
            elif case=='CPU_fetch_address':q['address']+=1
            elif case=='SPC_entry_master':q['master']+=2
            elif case=='SPC_final_end':assert not q['end'];q['end']=True
            elif case=='SPC_late_address_width':q['address']=65536
            elif case=='F1_marker_pc':q['pc']+=1
            elif case=='CPU_write_sample':q['sample_master']-=2
            elif case=='CPU_spc_count':q['spc']+=2
            return d
        v.rows=rows
        sys.argv=['verify','--native',str(a.native.resolve()),'--rom',str(a.rom.resolve()),'--exe',str(a.exe.resolve()),'--decoder-root',str(a.decoder_root.resolve()),'--output',str((a.output/case).resolve())]
        try:v.main();outcome='accepted';reason=''
        except(ValueError,KeyError,TypeError,AssertionError)as e:outcome='rejected';reason=str(e)
        good=outcome==('accepted'if case=='baseline'else'rejected')and(case=='baseline'or hit[0])
        results.append({'case':case,'pass':good,'mutation_reached':hit[0],'outcome':outcome,'reason':reason})
        print(case,good)
    v.rows=original;(a.output/'report.json').write_text(json.dumps(results,indent=2)+'\n')
    if not all(r['pass']for r in results):raise SystemExit(1)
    print('PASS',len(results),'profile protocol cases')

if __name__=='__main__':main()
