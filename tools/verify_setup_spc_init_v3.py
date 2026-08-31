"""Isolated source-init differential, not a normal SPC state initializer."""
import argparse,bisect,hashlib,json,re,struct,subprocess
from pathlib import Path
from setup_spc_evidence_contract_v4 import loads, capture_envelope, build_envelope, state_file, normal_spc_state
ROOT=Path(__file__).resolve().parents[1]
ROM_SHA='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
MESEN_SHA='d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b'
BUILD={'include/nba_setup_spc_resident.h','include/nba_setup_spc_init.h','src/nba_setup_spc_init.c','tools/setup_spc_init_probe.c','tools/build_setup_spc_init_probe.ps1'}
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def check(ok,msg):
 if not ok:raise ValueError(msg)
def unpack(p,fmt):
 b=Path(p).read_bytes();check(len(b)%struct.calcsize(fmt)==0,'binary record length');return list(struct.iter_unpack(fmt,b))
def state(p):return state_file(p)
def subset(g,w):
 for k,v in w.items():
  if isinstance(v,dict):subset(g[k],v)
  else:check(type(g[k])is type(v)and g[k]==v,'persisted settings')
def native(p,rom):
 m=loads((p/'manifest.json').read_text());capture_envelope(m,p,rom,'cold-reset SPC initializer observation; isolated component witness only',timeout=300,script_timeout=60);check(type(m['schema'])is int and m['schema']==1 and m['accepted']is True and m['state_injection']is False and m['rom_patch']is False and type(m['exit_code'])is int and m['exit_code']==0 and m['initial_saves']==[],'capture contract')
 check(set(m['sources'])=={'rom','mesen','script','runner','settings'},'source closure')
 expected={'rom':ROM_SHA,'mesen':MESEN_SHA,'script':sha(ROOT/'tools/mesen_setup_spc_init.lua'),'runner':sha(ROOT/'tools/capture_setup_spc_init.py'),'settings':sha(p/'initial-settings.json')}
 for n,v in m['sources'].items():check(v['sha256']==expected[n]and sha(v['path'])==v['sha256'],'native source identity')
 names={'capture.lua','runner.py','initial-settings.json','mesen.log','spc_init_complete.txt'}|{f'spc_init_{n}.bin'for n in ('instructions','writes','io_reads')}|{f'spc_init_{n}.{e}'for n in ('entry','post_control','dsp_entry','pending_dsp')for e in ('aram','state')}
 check(set(m['artifacts'])==names,'artifact closure')
 for n,v in m['artifacts'].items():check(type(v['bytes'])is int and (p/n).stat().st_size==v['bytes']and sha(p/n)==v['sha256'],'artifact identity')
 check(sha(rom)==ROM_SHA,'ROM identity');initial=loads((p/'initial-settings.json').read_text());check(m['settings']==initial,'manifest settings')
 payload=rom.read_bytes()[0x4687:0x4b77]
 for tag in ('entry','post_control','dsp_entry','pending_dsp'):
  ram=(p/f'spc_init_{tag}.aram').read_bytes();check(len(ram)==65536 and ram[0x380:0x870]==payload,'native ROM to ARAM source identity')
  normal_spc_state(state(p/f'spc_init_{tag}.state'))
 check(initial['Snes']['RamPowerOnState']=='AllZeros'and initial['Snes']['EnableRandomPowerOnState']is False and initial['Preferences']['AutoLoadPatches']is False,'power-on contract')
 check(sha(p/'portable-mesen/settings.json')==m['post_settings_sha256'],'post settings identity');subset(loads((p/'portable-mesen/settings.json').read_text(encoding='utf-8-sig')),initial)
 ni=unpack(p/'spc_init_instructions.bin','<H5BQ');nw=unpack(p/'spc_init_writes.bin','<HHBQ');nr=unpack(p/'spc_init_io_reads.bin','<HHBQ');times=[x[-1]for x in ni]
 check(ni[0][0]==0x380 and ni[4][0]==0x387 and ni[-1][0]==0x3db,'source endpoints')
 for tag,row in (('entry',ni[0]),('post_control',ni[4]),('dsp_entry',ni[-1])):
  st=state(p/f'spc_init_{tag}.state')
  for index,key in enumerate(('pc','a','x','y','sp','ps','cycle')):check(st['spc.'+key]==str(row[index]),'state/native boundary '+key)
 check(not any(row[1]==0xf0 for row in nw),'SPC speed/write preconditions may not change')
 check(len(ni)==192818 and len(nw)==64395 and (p/'spc_init_complete.txt').read_text()=='ok; instructions=192818; writes=64395\n','bounded native counts')
 check(all(a<b for a,b in zip(times,times[1:])),'native instruction chronology')
 for group in (nw,nr):
  check(all(a[-1]<=b[-1]for a,b in zip(group,group[1:])),'native bus chronology')
  for pc,addr,value,clock in group:
   check(times[0]<=clock<=times[-1]+6,'native bus clock bounds');i=bisect.bisect_left(times,clock);check(any(n[0]==pc and n[-1]<=clock for n in ni[max(0,i-1):min(len(ni),i+1)]),'native bus PC association')
 check(len(nr)==3 and nr[0]==(0x384,0xf1,0,ni[0][-1]+20),'source control read')
 check(nw[0]==(0x384,0xf1,0x30,ni[0][-1]+22)and ni[4][-1]==nw[0][-1],'source pending control publication witness')
 before=state(p/'spc_init_post_control.state')
 check(nr[1]==(0x3d8,0xf2,int(before['spc.dspReg']),ni[-1][-1]-2),'source DSP address read')
 check(nr[2][:2]==(0x3db,0xf3)and nr[2][-1]==ni[-1][-1]+6,'pending DSP read witness')
 return m,ni,nw,nr
def build(exe):
 m=loads((exe.parent/'build-manifest.json').read_text(encoding='utf-8-sig'));build_envelope(m);check(type(m['schema'])is int and m['schema']==1 and type(m['compiler_exit'])is int and m['compiler_exit']==0 and set(m['sources'])==BUILD,'build closure')
 for n,v in m['sources'].items():check(sha(ROOT/n)==v['sha256']and sha(v['path'])==v['sha256'],'build source identity')
 check(sha(exe)==m['executable']['sha256']and Path(m['executable']['path']).resolve()==exe.resolve(),'executable identity');return m
def source(rom):
 body=(ROOT/'src/nba_setup_spc_init.c').read_text().split('static const Source source[]={',1)[1].split('};',1)[0];data=rom.read_bytes();pcs=set()
 for m in re.finditer(r'\{(0x[0-9a-f]+),(\d+),\{([^}]*)\}\}',body):
  pc=int(m[1],16);b=bytes(int(s,0)for s in m[3].split(','));check(len(b)==int(m[2])and data[0x4687+pc-0x380:0x4687+pc-0x380+len(b)]==b,'compiled source bytes');pcs.add(pc)
 check(len(pcs)==49,'source closure');return pcs
def seed(p,tag):
 s=state(p/f'spc_init_{tag}.state');regs=[int(s[f'spc.{n}'])for n in ('pc','a','x','y','sp','ps')]
 ports=bytes(int(s[f'spc.{n}[{i}]'])for n in ('cpuRegs','outputReg')for i in range(4))
 return struct.pack('<H5B',*regs)+ports+bytes([int(s['spc.dspReg'])])+(p/f'spc_init_{tag}.aram').read_bytes()
def probe(exe,out,name,data):
 src=out/f'{name}.input';src.write_bytes(data);ip=out/f'{name}.instructions';wp=out/f'{name}.writes';ep=out/f'{name}.output'
 r=subprocess.run([str(exe.resolve()),str(src),str(ip),str(wp),str(ep)],capture_output=True,text=True);check(type(r.returncode)is int and r.returncode==0 and type(r.stdout)is str and type(r.stderr)is str and r.stderr=='','probe integer exit status failed '+r.stderr)
 result=loads(r.stdout);check(set(result)=={'pc','phase','cycles','instructions','writes','control'}and type(result['control'])is bool and all(type(v)is int and 0<=v<=2**63-1 for k,v in result.items()if k!='control'),'probe numeric schema')
 ci=unpack(ip,'<H5BQ');cw=unpack(wp,'<HHBQ');check(ci and ci[0][-1]==0 and all(a[-1]<b[-1]for a,b in zip(ci,ci[1:])),'C instruction chronology')
 check(all(a[-1]<b[-1]for a,b in zip(cw,cw[1:])),'C write chronology')
 # The final instruction remains incomplete: F1 write after four accepted
 # cycles at0384, or DSP read after two fetch cycles at03DB.
 check(result['pc']in (0x384,0x3db) and result['phase']==(4 if result['pc']==0x384 else 2),'bounded source stop')
 check(result['pc']==ci[-1][0] and result['cycles']==ci[-1][-1]+result['phase'],'exact pending instruction cycles')
 check(result['instructions']==len(ci)-1 and result['writes']==len(cw),'source summary counts')
 return result,ci,cw,ep.read_bytes()
def main(a):
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);m,ni,nw,nr=native(a.native,a.rom);build(a.exe);pcs=source(a.rom);results=[]
 for name,i,j,tag,endpoint in [('control',0,4,'entry','post_control'),('clear',4,len(ni),'post_control','dsp_entry')]:
  inp=seed(a.native,tag);r,ci,cw,final=probe(a.exe,out,name,inp);base=ni[i][-1]
  check(len(ci)==j-i and all(c[:-1]==n[:-1]and c[-1]*2==n[-1]-base for c,n in zip(ci,ni[i:j])),'source instruction/register/cycle divergence')
  expected=[x[:-1]+((x[-1]-base)//2,)for x in nw if base<x[-1]<=base+2*r['cycles']]
  check(all((x[-1]-base)%2==0 for x in nw if base<x[-1]<=base+2*r['cycles'])and cw==expected,'source writes/cycles divergence')
  check(r['pc']==(0x384 if name=='control'else 0x3db)and r['phase']==(4 if name=='control'else 2)and r['control']==(name=='control'),'pending hardware boundary')
  check(len(final)==65552 and final[7:15]==inp[7:15],'directional ports unchanged')
  want_dsp=int(state(a.native/f'spc_init_{endpoint}.state')['spc.dspReg'])
  check(final[15]==want_dsp,'native DSP address latch')
  check(tuple(struct.unpack('<H5B',final[:7]))==ci[-1][:-1],'pending register boundary')
  mem=bytearray(inp[16:])
  for pc,addr,val,clock in cw:mem[addr]=val
  check(final[16:]==mem,'C ARAM writes outside source')
  if name=='clear':check(final[16:]==(a.native/'spc_init_dsp_entry.aram').read_bytes(),'native complete ARAM endpoint')
  else:check(final[16:]==inp[16:]and r['cycles']==10,'F1 publication must remain pending')
  results.append({'name':name,'instruction_states':len(ci),'cycles':r['cycles'],'writes':len(cw),'output_sha256':hashlib.sha256(final).hexdigest()})
 report={'passed':True,'scope':'isolated 0380/control and post-control RAM-clear slices; no normal hardware initialization','source_states':len(pcs),'native_witnessed_states':len({n[0]for n in ni}),'cases':results,'native_manifest_sha256':sha(a.native/'manifest.json'),'verifier_sha256':sha(__file__),'evidence_contract_sha256':sha(ROOT/'tools/setup_spc_evidence_contract_v4.py'),'build_manifest_sha256':sha(a.exe.parent/'build-manifest.json')};(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS',results);return report
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--native',type=Path,required=True);p.add_argument('--rom',type=Path,required=True);p.add_argument('--exe',type=Path,required=True);p.add_argument('--output',type=Path,required=True);main(p.parse_args())
