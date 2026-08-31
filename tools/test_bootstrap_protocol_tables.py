"""Parsed-view corruption tests; preserve native and all generated evidence."""
import argparse,copy,importlib.util,json,subprocess,sys
from pathlib import Path
def main():
 p=argparse.ArgumentParser();p.add_argument('--native',type=Path,required=True);p.add_argument('--rom',type=Path,required=True);p.add_argument('--exe',type=Path,required=True);p.add_argument('--decoder-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 spec=importlib.util.spec_from_file_location('bootstrap_verify',Path(__file__).with_name('verify_bootstrap_tables.py'));v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
 original_load,original_rows,original_run=v.load,v.rows,v.subprocess.run
 cases=['baseline','manifest_schema_bool','native_exit_bool','missing_artifact','missing_source','wrong_script','wrong_settings','build_sources_empty','compiler_float','native_cpu_bool_cycle','native_cpu_reverse','native_spc_pc','native_bus_clock','native_bus_pc','C_exit_bool','C_exit_float','C_fetch_value','C_write_value','C_clock_backward','C_instruction_end','C_spc_byte_overflow','C_idle_address']
 root=a.output.resolve();root.mkdir(parents=True,exist_ok=False);results=[]
 for case in cases:
  reached=[False]
  def load(path):
   d=original_load(path)
   if Path(path).name=='manifest.json':
    if case=='manifest_schema_bool':d['schema']=True;reached[0]=True
    if case=='native_exit_bool':d['exit_code']=False;reached[0]=True
    if case=='missing_artifact':d['artifacts'].pop('cpu.jsonl');reached[0]=True
    if case=='missing_source':d['sources'].pop('script');reached[0]=True
    if case=='wrong_script':d['sources']['script']['sha256']='0'*64;reached[0]=True
    if case=='wrong_settings':d['settings']['Snes']['SpcClockSpeedAdjustment']=0;reached[0]=True
   if Path(path).name=='build-manifest.json':
    if case=='build_sources_empty':d['sources']={};reached[0]=True
    if case=='compiler_float':d['compiler_exit']=0.0;reached[0]=True
   return d
  def rows(path):
   d=original_rows(path);name=Path(path).name
   if name=='cpu.jsonl':
    if case=='native_cpu_bool_cycle':d[0]['cycles']=False;reached[0]=True
    if case=='native_cpu_reverse':d[1],d[2]=d[2],d[1];d[1]['event']=1;d[2]['event']=2;reached[0]=True
   if name=='spc.jsonl'and case=='native_spc_pc':d[0]['pc']=0;reached[0]=True
   if name=='cpu_bus.jsonl':
    if case=='native_bus_clock':d[0]['master']=0;reached[0]=True
    if case=='native_bus_pc':d[0]['pc']=0;reached[0]=True
   if name=='events.jsonl':
    predicates={
     'C_fetch_value':lambda x:x['kind']==0 and x['bus']==0 and x['pc']==x['address'],
     'C_write_value':lambda x:x['kind']==0 and x['bus']==1,
     'C_clock_backward':lambda x:x['kind']==0,
     'C_instruction_end':lambda x:x['kind']==0 and not x['end'],
     'C_spc_byte_overflow':lambda x:x['kind']==6,
     'C_idle_address':lambda x:x['kind']==1 and x['bus']==4}
    if case in predicates:
     x=next(x for x in d if predicates[case](x));reached[0]=True
     if case in ('C_fetch_value','C_write_value'):x['value']^=1
     elif case=='C_clock_backward':x['master']=0
     elif case=='C_instruction_end':x['end']=True
     elif case=='C_spc_byte_overflow':x['a']=256
     elif case=='C_idle_address':x['address']=1
   return d
  def run(*args,**kwargs):
   r=original_run(*args,**kwargs)
   if case in ('C_exit_bool','C_exit_float'):r.returncode=False if case=='C_exit_bool'else 0.0;reached[0]=True
   return r
  v.load,v.rows,v.subprocess.run=load,rows,run
  sys.argv=['verify','--native',str(a.native.resolve()),'--rom',str(a.rom.resolve()),'--exe',str(a.exe.resolve()),'--decoder-root',str(a.decoder_root.resolve()),'--output',str(root/case)]
  try:v.main();outcome='accepted'
  except (ValueError,KeyError,TypeError,AssertionError)as e:outcome='rejected';reason=str(e)
  good=(outcome=='accepted')if case=='baseline'else(outcome=='rejected'and reached[0])
  results.append({'case':case,'pass':good,'outcome':outcome,'mutation_reached':reached[0],'reason':reason if outcome=='rejected'else''})
  print(case,good)
 v.load,v.rows,v.subprocess.run=original_load,original_rows,original_run
 (root/'report.json').write_text(json.dumps(results,indent=2)+'\n')
 if not all(r['pass']for r in results):raise SystemExit(1)
 print('PASS',len(results),'protocol cases')
if __name__=='__main__':main()
