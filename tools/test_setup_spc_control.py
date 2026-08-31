"""Nonzero carried hardware edges and strict immutable native-view guards."""
import argparse,copy,importlib.util,json,subprocess
from pathlib import Path
def main():
 p=argparse.ArgumentParser();p.add_argument('--verifier',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--rom',type=Path,required=True);p.add_argument('--exe',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 spec=importlib.util.spec_from_file_location('spc_control_verifier',a.verifier);v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v);v.main(argparse.Namespace(native=a.native,rom=a.rom,exe=a.exe,output=out/'baseline'));cases=[]
 base=bytearray([0,1,1,1,91,92,93,94])
 for i in range(3):base.extend([7 if i==2 else 64,1,0,201,250,197,0,0])
 base.extend([11,12,13,14,81,82,83,84,5]);base.extend([0xa5]*65536)
 for name,val in [('enable_one',1),('enable_all',7),('repeated_enable',1),('disable_all',0),('clear_A',0x10),('clear_B',0x20),('clear_all_keep_pending',0x30),('ignored_bits',0x48),('write_gate_preserves_effects',0xb7),('IPL_visibility',0x80)]:
  inp=bytearray(base);inp[0]=val
  if name=='repeated_enable':inp[14]=1
  if name=='disable_all':
   for i in range(3):inp[8+8*i+6]=1
  if name=='write_gate_preserves_effects':inp[1]=0
  got=v.run(a.exe,out,name,inp);expected=bytearray(inp)
  expected[2]=int(bool(val&0x80))
  if inp[1]:expected[41+0xf1]=val
  # Bit truth table, using distinct sentinels for visible/staged/output/RAM.
  selected={0:[],0x10:[0,1],0x20:[2,3],0x30:[0,1,2,3]}[val&0x30]
  for z in selected:expected[4+z]=expected[32+z]=0
  for z in range(3):
   start=8+8*z;edge=bool(val&(1<<z));expected[start+6]=int(edge)
   if edge and inp[start+6]==0:expected[start+3]=expected[start+4]=0
  cases.append({'name':name,'passed':got==expected})
 # Reading/debug hooks never receive a clock or timer response from these tests.
 original_state=v.state
 for name in ('native_clock_change','native_zero_clock','native_output_corruption','native_prescaler_reset','native_missing_enable_edge','native_source_pc','native_numeric_bool'):
  hit=[0]
  def changed(path):
   d=original_state(path)
   if path.name=='spc_control_2_after.state':
    if name=='native_clock_change':d['spc.cycle']=str(int(d['spc.cycle'])+2)
    if name=='native_zero_clock':d['spc.cycle']='0'
    if name=='native_output_corruption':d['spc.outputReg[0]']='0'
    if name=='native_prescaler_reset':d['spc.timer0.stage0']='0'
    if name=='native_missing_enable_edge':d['spc.timer0.output']='15'
    if name=='native_source_pc':d['source_pc']='0'
    if name=='native_numeric_bool':d['spc.timer0.stage2']='false'
    hit[0]+=1
   return d
  v.state=changed;rejected=False;reason=''
  try:v.main(argparse.Namespace(native=a.native,rom=a.rom,exe=a.exe,output=out/name))
  except ValueError as e:rejected=True;reason=str(e)
  finally:v.state=original_state
  cases.append({'name':name,'passed':rejected and hit[0]==1,'reason':reason})
 original_run=v.run
 for name,index in [('C_output_latch',36),('C_prescaler',8),('C_pending_flag',3),('C_staged_input',4)]:
  hit=[0]
  def corrupt(exe,where,label,data):
   b=bytearray(original_run(exe,where,label,data))
   if label=='2':b[index]^=1;hit[0]+=1
   return bytes(b)
  v.run=corrupt;rejected=False
  try:v.main(argparse.Namespace(native=a.native,rom=a.rom,exe=a.exe,output=out/name))
  except ValueError:rejected=True
  finally:v.run=original_run
  cases.append({'name':name,'passed':rejected and hit[0]==1})
 original_process=v.subprocess.run
 for name,code in [('bool_process_exit',False),('float_process_exit',0.0)]:
  def wrong_status(*args,**kwargs):r=original_process(*args,**kwargs);r.returncode=code;return r
  v.subprocess.run=wrong_status;rejected=False
  try:v.run(a.exe,out,name,base)
  except ValueError:rejected=True
  finally:v.subprocess.run=original_process
  cases.append({'name':name,'passed':rejected})
 original_loads=v.json.loads
 for name in ('missing_artifact','missing_runner','empty_build_sources','boolean_build_status','settings_mismatch'):
  hit=[0]
  def changed(s,*args,**kwargs):
   d=original_loads(s,*args,**kwargs)
   if isinstance(d,dict)and d.get('kind','').startswith('cold-reset'):
    if name=='missing_artifact':d['artifacts'].pop('spc_control_2_after.state');hit[0]+=1
    if name=='missing_runner':d['sources'].pop('runner');hit[0]+=1
    if name=='settings_mismatch':d['settings']['Snes']['RamPowerOnState']='AllOnes';hit[0]+=1
   if isinstance(d,dict)and 'compiler_exit'in d:
    if name=='empty_build_sources':d['sources']={};hit[0]+=1
    if name=='boolean_build_status':d['compiler_exit']=False;hit[0]+=1
   return d
  v.json.loads=changed;rejected=False
  try:
   if name in ('empty_build_sources','boolean_build_status'):v.check_build(a.exe)
   else:v.read_native(a.native,a.rom)
  except ValueError:rejected=True
  finally:v.json.loads=original_loads
  cases.append({'name':name,'passed':rejected and hit[0]==1})
 for name,mut in [('short',lambda b:b[:-1]),('long',lambda b:b+b'x'),('bad_bool',lambda b:b[:1]+bytes([2])+b[2:]),('bad_stage1',lambda b:b[:9]+bytes([2])+b[10:])]:
  path=out/(name+'.input');path.write_bytes(mut(base));r=subprocess.run([str(a.exe.resolve()),str(path),str(out/(name+'.output'))],capture_output=True);cases.append({'name':name,'passed':r.returncode==(2 if name in ('short','long')else 3)})
 result={'passed':all(c['passed']for c in cases),'cases':cases,'verifier_sha256':v.sha(a.verifier),'test_sha256':v.sha(__file__)};(out/'report.json').write_text(json.dumps(result,indent=2)+'\n');print('PASS'if result['passed']else'FAIL',len(cases));raise SystemExit(not result['passed'])
if __name__=='__main__':main()
