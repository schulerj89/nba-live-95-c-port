"""Meaningful nonzero clear/boundary tests plus immutable-view corruption tests."""
import argparse,importlib.util,json,struct,subprocess
from pathlib import Path
def main():
 p=argparse.ArgumentParser();p.add_argument('--verifier',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--rom',type=Path,required=True);p.add_argument('--exe',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();
 for k in vars(a):setattr(a,k,getattr(a,k).resolve())
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 spec=importlib.util.spec_from_file_location('spc_init_verifier',a.verifier);v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
 baseline=v.main(argparse.Namespace(native=a.native,rom=a.rom,exe=a.exe,output=out/'baseline'));original=v.unpack;cases=[]
 for name in ('native_instruction_order','native_register','native_write_clock','native_write_pc','native_write_value','native_control_value','C_instruction_order','C_write_clock','C_write_value','pending_dsp_value'):
  hit=[0]
  def changed(path,fmt):
   rows=original(path,fmt);s=str(path)
   def alter(i,j,val):r=list(rows[i]);r[j]=val;rows[i]=tuple(r);hit[0]+=1
   if str(a.native)in s:
    if s.endswith('instructions.bin'):
     if name=='native_instruction_order':rows[6],rows[7]=rows[7],rows[6];hit[0]+=1
     if name=='native_register':alter(6,1,99)
    if s.endswith('writes.bin'):
     if name=='native_write_clock':alter(3,3,0)
     if name=='native_write_pc':alter(3,0,0)
     if name=='native_write_value':alter(3,2,99)
     if name=='native_control_value':alter(0,2,49)
    if name=='pending_dsp_value'and s.endswith('io_reads.bin'):alter(2,2,255)
   elif Path(path).name=='clear.instructions'and name=='C_instruction_order':rows[2],rows[3]=rows[3],rows[2];hit[0]+=1
   elif Path(path).name=='clear.writes':
    if name=='C_write_clock':alter(2,3,0)
    if name=='C_write_value':alter(2,2,99)
   return rows
  v.unpack=changed;rejected=False;reason=''
  try:r=v.main(argparse.Namespace(native=a.native,rom=a.rom,exe=a.exe,output=out/name))
  except ValueError as e:rejected=True;reason=str(e)
  finally:v.unpack=original
  positive=name=='pending_dsp_value';passed=hit[0]==1 and rejected!=positive
  if positive and not rejected:passed=passed and baseline['cases']==r['cases']
  cases.append({'name':name,'passed':passed,'rejected':rejected,'reason':reason})
 mem=bytearray([0xa5]*65536);mem[0x380:0x870]=a.rom.read_bytes()[0x4687:0x4b77]
 inp=struct.pack('<H5B',0x387,0xaa,0xbb,0xcc,0xdd,0x49)+bytes([3,4,5,6,13,14,15,16,0x11])+mem
 r,ci,cw,end=v.probe(a.exe,out,'poison_clear',inp)
 expected=bytearray(mem)
 for lo,hi in ((0,0x7b),(0x100,0x37f),(0x870,0x8fe),(0x900,0xffff)):expected[lo:hi+1]=bytes(hi-lo+1)
 expected[0:5]=bytes([0,0,0x8f,0xf7,0x20]);expected[0xf2]=0x6c
 cases.append({'name':'poison_clear_exact_bounds','passed':end[16:]==expected and end[16+0x8ff]==0xa5 and not any(x[1]==0x8ff for x in cw)and end[7:15]==inp[7:15]and end[15]==0x6c and end[5]==0xdd and r['cycles']==835242})
 inp=struct.pack('<H5B',0x380,0xaa,0xbb,0xcc,0xdd,0x69)+bytes([3,4,5,6,13,14,15,16,0x11])+mem
 r,ci,cw,end=v.probe(a.exe,out,'pending_control',inp)
 cases.append({'name':'pending_control_keeps_hardware','passed':r['control']is True and r['cycles']==10 and end[7:]==inp[7:]and end[3]==255 and end[5]==255 and end[6]&0x20==0 and not cw})
 # Strict binary lengths and only legal source entry/DP are admitted.
 for name,data in [('truncated',inp[:-1]),('extended',inp+b'x'),('invalid_entry',b'\x81\x03'+inp[2:]),('post_control_P1',struct.pack('<H5B',0x387,0,0,0,255,0x20)+inp[7:])]:
  path=out/(name+'.input');path.write_bytes(data);run=subprocess.run([str(a.exe.resolve()),str(path),str(out/(name+'.i')),str(out/(name+'.w')),str(out/(name+'.o'))],capture_output=True)
  cases.append({'name':name,'passed':run.returncode==(2 if name in ('truncated','extended')else 3)})
 original_loads=v.json.loads
 for name in ('missing_trace_identity','missing_script_identity','empty_build_sources','boolean_compiler_status','wrong_manifest_settings'):
  hit=[0]
  def loads(s,*args,**kwargs):
   d=original_loads(s,*args,**kwargs)
   if isinstance(d,dict)and d.get('kind','').startswith('cold-reset'):
    if name=='missing_trace_identity':d['artifacts'].pop('spc_init_writes.bin');hit[0]+=1
    if name=='missing_script_identity':d['sources'].pop('script');hit[0]+=1
    if name=='wrong_manifest_settings':d['settings']['Snes']['RamPowerOnState']='AllOnes';hit[0]+=1
   if isinstance(d,dict)and 'compiler_exit'in d:
    if name=='empty_build_sources':d['sources']={};hit[0]+=1
    if name=='boolean_compiler_status':d['compiler_exit']=False;hit[0]+=1
   return d
  v.json.loads=loads;rejected=False
  try:
   if name in ('empty_build_sources','boolean_compiler_status'):v.build(a.exe)
   else:v.native(a.native,a.rom)
  except ValueError:rejected=True
  finally:v.json.loads=original_loads
  cases.append({'name':name,'passed':hit[0]==1 and rejected})
 result={'passed':all(c['passed']for c in cases),'cases':cases,'verifier_sha256':v.sha(a.verifier),'test_sha256':v.sha(__file__)};(out/'report.json').write_text(json.dumps(result,indent=2)+'\n');print('PASS'if result['passed']else'FAIL',len(cases));raise SystemExit(not result['passed'])
if __name__=='__main__':main()
