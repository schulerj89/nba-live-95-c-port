"""Negative protocol tests for first fill; never modify native or C files."""
import argparse,importlib.util,json,sys
from pathlib import Path

def main():
    p=argparse.ArgumentParser()
    for name in('native','rom','exe','decoder-root','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    spec=importlib.util.spec_from_file_location('fill_verify',Path(__file__).with_name('verify_bootstrap_tables.py'))
    v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
    old_rows,old_run,old_state,old_bytes=v.rows,v.subprocess.run,v.state,Path.read_bytes
    cases=['baseline','DMA_read_address','DMA_read_value','DMA_write_address','DMA_write_value',
           'DMA_master','DMA_delete_pair','DMA_duplicate_pair','DMA_sync_end','DMA_native_pc',
           'DMA_native_cycles','after_dma_boundary_pc','return_boundary_cycles','final_CPU_field',
           'final_VRAM_byte','summary_dma_bytes','summary_vram_address','summary_sync_counter',
           'summary_source_index','last_SPC_write_address','last_SPC_write_value']
    results=[]
    for case in cases:
        hit=[False]
        def rows(path):
            d=old_rows(path);name=Path(path).name
            if name=='boundaries.jsonl'and case in('after_dma_boundary_pc','return_boundary_cycles'):
                tag='cpu_after_dma'if case=='after_dma_boundary_pc'else'cpu_fill_return'
                q=next(q for q in d if q['tag']==tag);q['cpu_pc'if case=='after_dma_boundary_pc'else'cpu']+=1;hit[0]=True
            if name=='cpu_bus.jsonl'and case in('DMA_native_pc','DMA_native_cycles'):
                q=next(q for q in d if q['pc']==0x808a90 and q['address']==22)
                q['pc'if case=='DMA_native_pc'else'cycles']-=1;hit[0]=True
            if name!='events.jsonl':return d
            if case.startswith('DMA_')and case not in('DMA_native_pc','DMA_native_cycles'):
                if case=='DMA_sync_end':q=next(q for q in d if q['kind']==8);q['master']+=2;hit[0]=True;return d
                write=case in('DMA_write_address','DMA_write_value')
                i=next(i for i,q in enumerate(d)if q['kind']==7 and q['bus']==int(write));q=d[i];hit[0]=True
                if case.endswith('_address'):q['address']+=1
                elif case.endswith('_value'):q['value']^=1
                elif case=='DMA_master':q['master']+=2
                elif case in('DMA_delete_pair','DMA_duplicate_pair'):
                    j=next(j for j in range(i+1,len(d))if d[j]['kind']==7);assert d[j]['bus']==1
                    if case=='DMA_delete_pair':d.pop(j);d.pop(i)
                    else:d.insert(j,dict(d[j]));d.insert(i,dict(d[i]))
            if case in('last_SPC_write_address','last_SPC_write_value'):
                q=next(q for q in reversed(d)if q['kind']==1 and q['bus']==3);assert q['pc']==0x3ca
                if case.endswith('_address'):q['address']+=1
                else:q['value']^=1
                hit[0]=True
            return d
        def run(*args,**kwargs):
            r=old_run(*args,**kwargs)
            if case.startswith('summary_'):
                lines=r.stdout.splitlines();q=json.loads(lines[-1]);q[case[len('summary_'):]]+=1
                lines[-1]=json.dumps(q);r.stdout='\n'.join(lines)+'\n';hit[0]=True
            return r
        def state(path):
            q=old_state(path)
            if Path(path).name=='final.state'and case=='final_CPU_field':q['cpu.a']=str(int(q['cpu.a'])^1);hit[0]=True
            return q
        def read_bytes(path):
            q=old_bytes(path)
            if Path(path).name=='final.vram'and case=='final_VRAM_byte':q=bytes([q[0]^1])+q[1:];hit[0]=True
            return q
        v.rows,v.subprocess.run,v.state,Path.read_bytes=rows,run,state,read_bytes
        sys.argv=['verify','--native',str(a.native.resolve()),'--rom',str(a.rom.resolve()),'--exe',str(a.exe.resolve()),'--decoder-root',str(a.decoder_root.resolve()),'--output',str((a.output/case).resolve())]
        try:v.main();outcome='accepted';reason=''
        except(ValueError,AssertionError,KeyError,TypeError)as e:outcome='rejected';reason=str(e)
        good=outcome==('accepted'if case=='baseline'else'rejected')and(case=='baseline'or hit[0])
        results.append({'case':case,'pass':good,'mutation_reached':hit[0],'outcome':outcome,'reason':reason});print(case,good)
    v.rows,v.subprocess.run,v.state,Path.read_bytes=old_rows,old_run,old_state,old_bytes
    (a.output/'report.json').write_text(json.dumps(results,indent=2)+'\n')
    if not all(r['pass']for r in results):raise SystemExit(1)
    print('PASS',len(results),'DMA protocol cases')

if __name__=='__main__':main()
