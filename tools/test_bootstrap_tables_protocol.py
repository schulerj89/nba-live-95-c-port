"""Reachable corruptions for the new initialization boundaries only.

Mutations change the reader's parsed views after immutable fixtures are read;
never change ROM/native/C files or use poststates to initialize the candidate.
"""
import argparse,importlib.util,json,sys
from pathlib import Path

def main():
    p=argparse.ArgumentParser()
    for name in ('native','rom','exe','decoder-root','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
    spec=importlib.util.spec_from_file_location('tables_verify',Path(__file__).with_name('verify_bootstrap_tables.py'))
    v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
    old_rows,old_run,old_bytes=v.rows,v.subprocess.run,Path.read_bytes
    tags=('cpu_clear_after','cpu_table_after','cpu_da72_return','cpu_ab7e_return','cpu_nmi_enable')
    cases=['baseline']+['PC:'+t for t in tags]+['WRAM:'+t for t in tags]+[
        'clear_write_address','clear_write_value','clear_write_duplicate',
        'queue_final_byte','head_final_byte','tail_final_byte','budget_final_byte','OAM_flag_final_byte',
        'table_ROM_fetch','publication_write','stop_PC']
    results=[]
    for case in cases:
        hit=[False]
        def rows(path):
            d=old_rows(path)
            if Path(path).name=='boundaries.jsonl'and case.startswith('PC:'):
                q=next(q for q in d if q['tag']==case.split(':')[1]);q['cpu_pc']+=1;hit[0]=True
            if Path(path).name!='events.jsonl':return d
            if case.startswith('clear_write_'):
                i=next(i for i,q in enumerate(d)if q['kind']==0 and q['bus']==1 and q['pc']==0x8080c5 and q['address']==0x12c)
                q=d[i];hit[0]=True
                if case.endswith('address'):q['address']+=1
                elif case.endswith('value'):q['value']=1
                else:d.insert(i,dict(q))
            if case=='table_ROM_fetch':
                q=next(q for q in d if q['kind']==0 and q['bus']==0 and q['pc']==0x8080f0 and q['address']==0x80971f);q['value']^=1;hit[0]=True
            if case=='publication_write':
                q=next(q for q in d if q['kind']==0 and q['bus']==1 and q['pc']==0x80acbc and q['address']==0x7e0566);q['value']=0;hit[0]=True
            return d
        def read_bytes(path):
            data=old_bytes(path)
            if not Path(path).resolve().is_relative_to(a.output):return data
            if case.startswith('WRAM:'):
                tag=case.split(':')[1];name='final.wram'if tag=='cpu_nmi_enable'else tag+'.wram'
                if Path(path).name==name:data=bytes([data[0]^1])+data[1:];hit[0]=True
            fields={'queue_final_byte':0x12c,'head_final_byte':0x35,'tail_final_byte':0x37,'budget_final_byte':0x39,'OAM_flag_final_byte':0x566}
            if case in fields and Path(path).name=='final.wram':
                i=fields[case];data=data[:i]+bytes([data[i]^1])+data[i+1:];hit[0]=True
            return data
        def run(*args,**kwargs):
            r=old_run(*args,**kwargs)
            if case=='stop_PC':
                lines=r.stdout.splitlines();s=json.loads(lines[-1]);s['boundary_pc']+=1;lines[-1]=json.dumps(s);r.stdout='\n'.join(lines)+'\n';hit[0]=True
            return r
        v.rows,v.subprocess.run,Path.read_bytes=rows,run,read_bytes
        sys.argv=['verify','--native',str(a.native.resolve()),'--rom',str(a.rom.resolve()),'--exe',str(a.exe.resolve()),'--decoder-root',str(a.decoder_root.resolve()),'--output',str(a.output/case.replace(':','-'))]
        try:v.main();outcome='accepted';reason=''
        except(ValueError,AssertionError,KeyError,TypeError)as e:outcome='rejected';reason=str(e)
        good=outcome==('accepted'if case=='baseline'else'rejected')and(case=='baseline'or hit[0])
        results.append(dict(case=case,passed=good,mutation_reached=hit[0],outcome=outcome,reason=reason));print(case,good)
    v.rows,v.subprocess.run,Path.read_bytes=old_rows,old_run,old_bytes
    (a.output/'report.json').write_text(json.dumps(results,indent=2)+'\n')
    if not all(r['passed']for r in results):raise SystemExit(1)
    print('PASS',len(results),'tables protocol cases')

if __name__=='__main__':main()
