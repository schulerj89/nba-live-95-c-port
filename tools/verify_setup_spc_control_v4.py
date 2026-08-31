"""Same-clock native F1 publication differential; hidden staged inputs unobserved."""
import argparse,hashlib,json,subprocess
from pathlib import Path
from setup_spc_evidence_contract_v3 import loads, capture_envelope, build_envelope, state_file, normal_spc_state
from setup_spc_state_contract_v4 import validate as validate_state, control_boundary
ROOT=Path(__file__).resolve().parents[1]
ROM_SHA='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
MESEN_SHA='d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b'
BUILD={'include/nba_setup_spc_resident.h','include/nba_setup_spc_control.h','src/nba_setup_spc_control.c','tools/setup_spc_control_probe.c','tools/build_setup_spc_control_probe.ps1'}
TF=('stage0','stage1','prevStage1','stage2','output','target','enabled','timersEnabled')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def check(ok,msg):
 if not ok:raise ValueError(msg)
def state(p):
 lines=[line.split('=',1)for line in p.read_text().splitlines()];d=dict(lines);check(len(d)==len(lines),'duplicate state field');return d
def number(s,k,lo=0,hi=255):
 text=s[k];check(text.isascii()and text.isdecimal()and str(int(text))==text and lo<=int(text)<=hi,'numeric field '+k);return int(text)
def boolean(s,k):check(s[k]in ('true','false'),'boolean field '+k);return int(s[k]=='true')
def subset(g,w):
 for k,v in w.items():
  if isinstance(v,dict):subset(g[k],v)
  else:check(type(g[k])is type(v)and g[k]==v,'persisted settings')
def read_native(p,rom):
 m=loads((p/'manifest.json').read_text());capture_envelope(m,p,rom,'cold-reset SPC F1 control observation; isolated component witness only');check(type(m['schema'])is int and m['schema']==1 and m['accepted']is True and m['state_injection']is False and m['rom_patch']is False and type(m['exit_code'])is int and m['exit_code']==0 and m['initial_saves']==[],'capture contract')
 check(set(m['sources'])=={'rom','mesen','script','runner','settings'},'source closure')
 expected={'rom':ROM_SHA,'mesen':MESEN_SHA,'script':sha(ROOT/'tools/mesen_setup_spc_control.lua'),'runner':sha(ROOT/'tools/capture_setup_spc_control.py'),'settings':sha(p/'initial-settings.json')}
 for n,v in m['sources'].items():check(v['sha256']==expected[n]and sha(v['path'])==v['sha256'],'source identity')
 names={'capture.lua','runner.py','initial-settings.json','mesen.log','spc_control_complete.txt'}|{f'spc_control_{i}_{tag}.{ext}'for i in (1,2)for tag in ('before','after')for ext in ('state','aram')}
 check(set(m['artifacts'])==names,'artifact closure')
 for n,v in m['artifacts'].items():check(type(v['bytes'])is int and (p/n).stat().st_size==v['bytes']and sha(p/n)==v['sha256'],'artifact identity')
 check(sha(rom)==ROM_SHA,'ROM identity');initial=loads((p/'initial-settings.json').read_text());check(m['settings']==initial,'manifest settings')
 check(initial['Snes']['RamPowerOnState']=='AllZeros'and initial['Snes']['EnableRandomPowerOnState']is False and initial['Preferences']['AutoLoadPatches']is False,'normal power-on')
 check(sha(p/'portable-mesen/settings.json')==m['post_settings_sha256'],'post settings identity');subset(loads((p/'portable-mesen/settings.json').read_text(encoding='utf-8-sig')),initial)
 check((p/'spc_control_complete.txt').read_text()=='ok; publications=2\n','completion');return m
def check_build(exe):
 m=loads((exe.parent/'build-manifest.json').read_text(encoding='utf-8-sig'));build_envelope(m);check(type(m['schema'])is int and m['schema']==1 and type(m['compiler_exit'])is int and m['compiler_exit']==0 and set(m['sources'])==BUILD,'build closure')
 for n,v in m['sources'].items():check(sha(ROOT/n)==v['sha256']and sha(v['path'])==v['sha256'],'build source identity')
 check(sha(exe)==m['executable']['sha256']and Path(m['executable']['path']).resolve()==exe.resolve(),'exe identity');return m
def pack(s,ram,staged=(91,92,93,94),pending=1):
 data=bytearray([number(s,'value'),boolean(s,'spc.writeEnabled'),boolean(s,'spc.romEnabled'),pending,*staged])
 for i in range(3):
  for name in TF:
   key=f'spc.timer{i}.{name}';data.append(boolean(s,key)if name in ('enabled','timersEnabled')else number(s,key,0,(15 if i==2 else 127)if name=='stage0'else 1 if name in ('stage1','prevStage1')else 255))
 for name in ('cpuRegs','outputReg'):
  for i in range(4):data.append(number(s,f'spc.{name}[{i}]'))
 data.append(number(s,'spc.dspReg'));check(len(ram)==65536,'ARAM size');return bytes(data)+ram
def run(exe,out,name,data):
 src=out/(name+'.input');dst=out/(name+'.output');src.write_bytes(data)
 r=subprocess.run([str(exe.resolve()),str(src),str(dst)],capture_output=True);check(type(r.returncode)is int and r.returncode==0 and type(r.stdout)is bytes and type(r.stderr)is bytes and r.stdout==b''and r.stderr==b'','fresh probe integer exit status/output');b=dst.read_bytes();check(len(b)==65577,'output length');return b
def main(a):
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);p=a.native;read_native(p,a.rom);check_build(a.exe);rom=a.rom.read_bytes();results=[]
 for i,pc,value in ((1,0x384,0x30),(2,0x3ec,1)):
  before=state(p/f'spc_control_{i}_before.state');after=state(p/f'spc_control_{i}_after.state')
  check(set(before)==set(after),'state schema identity')
  for s in (before,after):control_boundary(s,pc,value)
  off=0x4687+pc-0x380;check(rom[off:off+3]==bytes([0x8f,value,0xf1]),'ROM publication instruction')
  check(number(before,'spc.cycle',1,2**63-1)==number(after,'spc.cycle',1,2**63-1),'native commit must be same SPC clock')
  if results:check(int(before['spc.cycle'])>results[-1]['same_clock'],'native publication chronology')
  allowed={'spc.romEnabled'}|{f'spc.cpuRegs[{z}]'for z in range(4)}|{f'spc.timer{z}.{k}'for z in range(3)for k in ('enabled','stage2','output')}
  check(all(before[k]==after[k]for k in before if k not in allowed),'unowned native state changed')
  pre=(p/f'spc_control_{i}_before.aram').read_bytes();post=(p/f'spc_control_{i}_after.aram').read_bytes()
  check(pre[0x380:0x870]==rom[0x4687:0x4b77]and post[0x380:0x870]==pre[0x380:0x870],'resident ROM identity')
  inp=pack(before,pre);got=run(a.exe,out,str(i),inp)
  # Hidden staged-input/pending-update fields are absent from Lua Map. Their
  # deliberate synthetic sentinel values are NOT native initial-state claims.
  staged=list(inp[4:8])
  for z in range(4):
   if value&(0x10 if z<2 else 0x20):staged[z]=0
  expected=pack(after,post,staged,1);check(got==expected,'native visible hardware/ARAM or source hidden-field contract divergence')
  results.append({'pc':pc,'value':value,'same_clock':int(before['spc.cycle']),'visible_fields':35,'output_sha256':hashlib.sha256(got).hexdigest()})
 report={'passed':True,'scope':'post-cycle F1 commit only; hidden staged inputs synthetic, not native-observed; no clock advancement','cases':results,'native_manifest_sha256':sha(p/'manifest.json'),'build_manifest_sha256':sha(a.exe.parent/'build-manifest.json'),'verifier_sha256':sha(__file__),'evidence_contract_sha256':sha(ROOT/'tools/setup_spc_evidence_contract_v3.py'),'state_contract_sha256':sha(ROOT/'tools/setup_spc_state_contract_v4.py')};(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS 2 publications,70 visible fields,2 fullARAM endpoints');return report
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--native',type=Path,required=True);p.add_argument('--rom',type=Path,required=True);p.add_argument('--exe',type=Path,required=True);p.add_argument('--output',type=Path,required=True);main(p.parse_args())
