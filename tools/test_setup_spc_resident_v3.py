"""Fresh negative native-view guards and source-boundary/latch contracts."""
import argparse, copy, importlib.util, json, struct, subprocess
from pathlib import Path
def main():
 p=argparse.ArgumentParser();p.add_argument('--verifier',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--rom',type=Path,required=True);p.add_argument('--exe',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();
 for k in vars(a):setattr(a,k,getattr(a,k).resolve())
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 spec=importlib.util.spec_from_file_location('resident_verifier',a.verifier);v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
 base=v.main(argparse.Namespace(native=a.native,rom=a.rom,exe=a.exe,output=out/'baseline'));original=v.json_lines;cases=[]
 for name in ('native_instruction_reorder','native_bus_clock','native_bus_pc','native_port_value','native_register','C_cycle_reorder','C_backwards_cycle','C_port_value','pending_timer_value','pending_dsp_value'):
  reached=[0]
  def mutated(path):
   rows=copy.deepcopy(original(path));s=str(path)
   if name.startswith('native_')and str(a.native)in s:
    if name=='native_instruction_reorder'and s.endswith('instructions.jsonl'):
     rows[1],rows[2]=rows[2],rows[1];rows[1]['event']=1;rows[2]['event']=2;reached[0]+=1
    if name=='native_register'and s.endswith('instructions.jsonl'):rows[7]['ps']^=1;reached[0]+=1
    if s.endswith('bus.jsonl'):
     targets=[r for r in rows if r['address']==0xf4 and r['kind']=='write']
     if name=='native_bus_clock':targets[0]['spc_cycle']=0;reached[0]+=1
     if name=='native_bus_pc':targets[0]['pc']=0;reached[0]+=1
     if name=='native_port_value':targets[0]['value']^=1;reached[0]+=1
   if name.startswith('C_')and Path(path).name=='00.jsonl':
    indexes=[i for i,r in enumerate(rows)if r['kind']=='cycle']
    if name=='C_cycle_reorder':i,j=indexes[1:3];rows[i],rows[j]=rows[j],rows[i];reached[0]+=1
    if name=='C_backwards_cycle':rows[indexes[2]]['cycles']=0;reached[0]+=1
    if name=='C_port_value':next(r for r in rows if r.get('bus')==3 and r['address']==0xf4)['value']^=1;reached[0]+=1
   if name.startswith('pending_')and str(a.native)in s and s.endswith('bus.jsonl'):
    address=0xfd if name=='pending_timer_value'else 0xf3
    for r in rows:
     if r['address']==address and r['kind']=='read':r['value']=255
    reached[0]+=1
   return rows
  v.json_lines=mutated;rejected=False;reason=''
  try:report=v.main(argparse.Namespace(native=a.native,rom=a.rom,exe=a.exe,output=out/name))
  except (ValueError,AssertionError)as e:rejected=True;reason=str(e)
  finally:v.json_lines=original
  positive=name.startswith('pending_');passed=reached[0]==1 and rejected!=positive
  if positive and not rejected:passed=passed and report['cases']==base['cases']
  cases.append({'name':name,'passed':passed,'reached':reached[0],'rejected':rejected,'reason':reason})
 # Mutate parsed identity declarations only; immutable native files untouched.
 original_loads=v.json.loads
 for name in ('missing_artifact','missing_source','settings_mismatch','empty_build_sources','boolean_build_status'):
  reached=[0]
  def load_mutation(s,*args,**kwargs):
   obj=original_loads(s,*args,**kwargs)
   if isinstance(obj,dict)and obj.get('kind','').startswith('cold-reset'):
    if name=='missing_artifact':obj['artifacts'].pop('spc_resident_bus.jsonl');reached[0]+=1
    if name=='missing_source':obj['sources'].pop('script');reached[0]+=1
    if name=='settings_mismatch':obj['settings']['Snes']['RamPowerOnState']='AllOnes';reached[0]+=1
   if isinstance(obj,dict)and 'compiler_exit'in obj:
    if name=='empty_build_sources':obj['sources']={};reached[0]+=1
    if name=='boolean_build_status':obj['compiler_exit']=False;reached[0]+=1
   return obj
  v.json.loads=load_mutation;rejected=False
  try:
   if name in ('empty_build_sources','boolean_build_status'):v.check_build(a.exe)
   else:v.read_native(a.native,a.rom)
  except ValueError:rejected=True
  finally:v.json.loads=original_loads
  cases.append({'name':name,'passed':rejected and reached[0]==1})
 # Real ROM bytes, poisoned non-code RAM; no captured register/ARAM state.
 mem=bytearray([0xa5]*65536);rom=a.rom.read_bytes();mem[0x380:0x870]=rom[0x4687:0x4b77]
 def run(name,pc,command,voice=7):
  seed=struct.pack('<H5B',pc,0xaa,0xbb,0xcc,0x03,0x49)+bytes([command,voice,22,33,0x77,0x66,0x55,0x44,0x12])+mem
  src=out/(name+'.input');dst=out/(name+'.output');src.write_bytes(seed)
  r=subprocess.run([str(a.exe.resolve()),str(src),str(dst)],capture_output=True,text=True);return r,[json.loads(s)for s in r.stdout.splitlines()],dst
 r,rows,dst=run('generic_ack_entry',0x441,0)
 writes=[x['value']for x in rows if x.get('bus')==3 and x['address']==0xf4]
 cases.append({'name':'generic_ack_entry','passed':r.returncode==0 and writes==[0xaa,0]and rows[-1]['pc']==0x48b and rows[-1]['cycles']==24})
 r,rows,dst=run('other_command_boundary',0x44d,1)
 cases.append({'name':'other_command_boundary','passed':r.returncode==0 and rows[-1]['pc']==0x578 and rows[-1]['phase']==0 and dst.read_bytes()[11]==0x77})
 r,rows,dst=run('zero_to_timer',0x44d,0)
 cases.append({'name':'zero_to_timer','passed':r.returncode==0 and rows[-1]['pc']==0x48b and rows[-1]['cycles']==17 and dst.read_bytes()[11:15]==bytes([0x77,0x66,0x55,0x44])})
 r,rows,dst=run('wrapped_command',0x44d,0x85,0xff)
 atack=next(x for x in rows if x['kind']=='instruction'and x['pc']==0x613)
 cases.append({'name':'wrapped_command','passed':r.returncode==0 and atack['a']==10 and atack['ps']&1==1 and dst.read_bytes()[11]==5 and dst.read_bytes()[7]==0x85 and dst.read_bytes()[15]==6})
 r,rows,dst=run('busy_input_bound',0x443,1)
 cases.append({'name':'busy_input_bound','passed':r.returncode==5 and not dst.exists()})
 src=out/'short.input';src.write_bytes(bytes(100));r=subprocess.run([str(a.exe.resolve()),str(src),str(out/'short.output')],capture_output=True)
 cases.append({'name':'short_input_reject','passed':r.returncode==2})
 r=subprocess.run([str(a.exe.resolve()),'--selftest'],capture_output=True,text=True);cases.append({'name':'C_latch_contracts','passed':r.returncode==0,'output':r.stdout.strip()})
 result={'passed':all(x['passed']for x in cases),'cases':cases,'verifier_sha256':v.sha(a.verifier),'test_sha256':v.sha(__file__)};(out/'report.json').write_text(json.dumps(result,indent=2)+'\n')
 print('PASS'if result['passed']else'FAIL',len(cases));raise SystemExit(not result['passed'])
if __name__=='__main__':main()
