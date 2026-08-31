"""Strict, isolated SPC resident component differential. Never a phase oracle."""
from setup_spc_fetch_contract import validate_spc_fetches
import argparse, bisect, hashlib, json, re, struct, subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
ROM_SHA='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
MESEN_SHA='d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b'
BUILD={'include/nba_setup_spc_resident.h','src/nba_setup_spc_resident.c','tools/setup_spc_resident_probe.c','tools/build_setup_spc_resident_probe.ps1'}
FIELDS={'pc','a','x','y','sp','ps','spc_cycle','cpu_cycles','master_clock','event'}|{f'{n}{i}'for n in ('input','output')for i in range(4)}
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def require(ok,msg):
 if not ok:raise ValueError(msg)
def integer(v,lo,hi):return type(v)is int and lo<=v<=hi
def json_lines(p):return [json.loads(s)for s in Path(p).read_text().splitlines()]
def subset(g,w):
 for k,v in w.items():
  if isinstance(v,dict):subset(g[k],v)
  else:require(type(g[k])is type(v)and g[k]==v,'persisted settings differ')
def read_native(p,rom):
 m=json.loads((p/'manifest.json').read_text());require(m['schema']==1 and type(m['schema'])is int and m['accepted']is True and m['state_injection']is False and m['rom_patch']is False and type(m['exit_code'])is int and m['exit_code']==0 and m['initial_saves']==[],'capture contract')
 require(set(m['sources'])=={'rom','mesen','script','runner','settings'},'required source identities')
 expected={'rom':ROM_SHA,'mesen':MESEN_SHA,'script':sha(ROOT/'tools/mesen_setup_spc_resident.lua'),'runner':sha(ROOT/'tools/capture_setup_spc_resident.py'),'settings':sha(p/'initial-settings.json')}
 for k,v in m['sources'].items():require(v['sha256']==expected[k] and sha(v['path'])==v['sha256'],'source identity '+k)
 require(sha(rom)==ROM_SHA,'ROM identity')
 names={'capture.lua','runner.py','initial-settings.json','mesen.log','spc_resident_complete.txt'}|{f'spc_resident_{n}.jsonl'for n in ('instructions','bus','cpu_ports','boundaries')}|{f'spc_resident_{n}.{ext}'for n in ('upload_entry','poll_entry','end')for ext in ('state','aram')}
 require(set(m['artifacts'])==names,'required artifacts')
 for n,v in m['artifacts'].items():require(type(v['bytes'])is int and (p/n).stat().st_size==v['bytes']and sha(p/n)==v['sha256'],'artifact identity '+n)
 require(sha(p/'portable-mesen/settings.json')==m['post_settings_sha256'],'post settings identity')
 initial=json.loads((p/'initial-settings.json').read_text());require(initial==m['settings'],'manifest settings differ');subset(json.loads((p/'portable-mesen/settings.json').read_text(encoding='utf-8-sig')),initial)
 require(initial['Snes']['RamPowerOnState']=='AllZeros'and initial['Snes']['EnableRandomPowerOnState']is False and initial['Preferences']['AutoLoadPatches']is False,'normal reset settings')
 require((p/'spc_resident_complete.txt').read_text()=='ok; commands=8\n','completion')
 rows={n:json_lines(p/f'spc_resident_{n}.jsonl')for n in ('instructions','bus','cpu_ports','boundaries')}
 for name,items in rows.items():
  for i,r in enumerate(items):
   extra=set()if name=='instructions'else {'tag'}if name=='boundaries'else {'address','value','kind'}
   require(set(r)==FIELDS|extra,f'{name} schema')
   for k in FIELDS:require(integer(r[k],0,65535 if k=='pc'else 2**63-1 if k in ('spc_cycle','cpu_cycles','master_clock','event')else 255),f'{name} numeric {k}')
   require(r['event']==i,f'{name} event order')
   if i:
    require(r['spc_cycle']>=items[i-1]['spc_cycle'] and r['cpu_cycles']>=items[i-1]['cpu_cycles'] and r['master_clock']>=items[i-1]['master_clock'],f'{name} chronology')
    if name=='instructions':require(r['spc_cycle']>items[i-1]['spc_cycle'],'instruction chronology')
   if extra=={'address','value','kind'}:require(integer(r['address'],0,0xffffff if name=='cpu_ports'else 65535)and integer(r['value'],0,255)and r['kind']in ('read','write'),f'{name} bus schema')
 ni=rows['instructions'];bounds=rows['boundaries'];nb=rows['bus'];times=[r['spc_cycle']for r in ni]
 require(len(ni)==270 and len(bounds)==123 and bounds[0]['tag']=='upload_entry'and bounds[0]['pc']==0x380 and bounds[-2]['tag']=='poll_entry'and bounds[-2]['pc']==0x447 and bounds[-1]['tag']=='end'and bounds[-1]['pc']==0x443,'normal bounded route')
 for r in nb:
  require(times[0]<=r['spc_cycle']<=bounds[-1]['spc_cycle'],'native bus clock outside scope')
  pos=bisect.bisect_left(times,r['spc_cycle']);candidates=ni[max(0,pos-1):min(len(ni),pos+1)]
  require(any(t['pc']==r['pc']and t['spc_cycle']<=r['spc_cycle']for t in candidates),'native bus PC association')
 payload=rom.read_bytes()[0x4687:0x4b77]
 for tag in ('upload_entry','poll_entry','end'):require((p/f'spc_resident_{tag}.aram').read_bytes()[0x380:0x870]==payload,'uploaded source identity')
 return m,rows
def source_check(rom):
 text=(ROOT/'src/nba_setup_spc_resident.c').read_text();body=text.split('static const Source source[]={',1)[1].split('};',1)[0];data=rom.read_bytes();count=0
 for match in re.finditer(r'\{(0x[0-9a-f]+),(\d+),\{([^}]*)\}\}',body):
  pc=int(match[1],16);n=int(match[2]);b=bytes(int(x,0)for x in match[3].split(','));require(len(b)==n and data[0x4687+pc-0x380:0x4687+pc-0x380+n]==b,f'compiled source {pc:04X}');count+=1
 require(count==24,'source closure');return count
def check_build(exe):
 m=json.loads((exe.parent/'build-manifest.json').read_text(encoding='utf-8-sig'));require(type(m['schema'])is int and m['schema']==1 and type(m['compiler_exit'])is int and m['compiler_exit']==0 and set(m['sources'])==BUILD,'build closure')
 require(m['executable']['sha256']==sha(exe)and Path(m['executable']['path']).resolve()==exe.resolve(),'executable identity')
 for n,v in m['sources'].items():require(sha(ROOT/n)==v['sha256']and sha(v['path'])==v['sha256'],'build source identity '+n)
 return m
def useful(r):
 # Mesen's Lua SPC read callback also contains DSP RAM activity. Attribute only
 # I/O, CALL stack writes, and uploaded source/table reads, not that DSP work.
 a=r['address'];return 0xf2<=a<=0xf7 or a==0xfd or (r['kind']=='write'and r['pc']==0x44a and 0x100<=a<=0x1ff)or (r['kind']=='read'and 0x380<=a<0x870)
def main(a):
 p=a.native.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);m,rows=read_native(p,a.rom);build=check_build(a.exe);source_count=source_check(a.rom)
 ni,nb=rows['instructions'],rows['bus'];scopes=[]
 for i,r in enumerate(ni):
  if r['pc']in (0x44d,0x443)or i==0:
   stop=0x622 if r['pc']==0x44d else 0x48b;j=i
   while ni[j]['pc']!=stop:j+=1
   scopes.append((i,j))
 require(len(scopes)==16,'source scopes')
 initial=(p/'spc_resident_poll_entry.aram').read_bytes();results=[];total_i=total_b=0
 for k,(i,j)in enumerate(scopes):
  entry=ni[i];limit=ni[j]['spc_cycle']+4;mem=bytearray(initial)
  for r in nb:
   if r['spc_cycle']>entry['spc_cycle']:break
   if r['kind']=='write':mem[r['address']]=r['value']
  s=entry;dsp=0
  states=dict(line.split('=',1)for line in (p/'spc_resident_poll_entry.state').read_text().splitlines());dsp=int(states['spc.dspReg'])
  for r in nb:
   if r['spc_cycle']>entry['spc_cycle']:break
   if r['kind']=='write'and r['address']==0xf2:dsp=r['value']
  seed=struct.pack('<H5B',s['pc'],s['a'],s['x'],s['y'],s['sp'],s['ps'])+bytes(s[f'{n}{z}']for n in ('input','output')for z in range(4))+bytes([dsp])+mem
  path=out/f'{k:02d}.input';path.write_bytes(seed);trace=out/f'{k:02d}.jsonl';endpoint=out/f'{k:02d}.output'
  with trace.open('w')as f:run=subprocess.run([str(a.exe.resolve()),str(path),str(endpoint)],stdout=f,stderr=subprocess.PIPE,text=True)
  require(type(run.returncode)is int and run.returncode==0 and type(run.stderr)is str and run.stderr=='','fresh C probe integer exit status');cr=json_lines(trace);ci=[];cb=[];cycle=0;last=None
  for ci_index,r in enumerate(cr):
   require(r['kind']in ('instruction','cycle','stop'),'C row kind')
   if r['kind']=='cycle':
    require(set(r)=={'kind','pc','cycles','bus','address','value','end'}and integer(r['pc'],0,65535)and integer(r['cycles'],0,2**63-1)and integer(r['bus'],1,4)and integer(r['address'],0,65535)and integer(r['value'],0,255)and type(r['end'])is bool,'C cycle schema')
    require(r['cycles']==cycle+1 and last is not None and r['pc']==last['pc'],'C cycle chronology');cycle+=1
    if r['bus']in (2,3):cb.append({'pc':r['pc'],'spc_cycle':s['spc_cycle']+2*cycle,'address':r['address'],'value':r['value'],'kind':'read'if r['bus']==2 else'write'})
   else:
    require(set(r)=={'kind','pc','cycles','a','x','y','sp','ps','phase'}and all(integer(r[z],0,65535 if z=='pc'else 2**63-1 if z=='cycles'else 255)for z in r if z!='kind'),'C state schema')
    require(r['cycles']==cycle,'C state chronology')
    if r['kind']=='instruction':require(r['phase']==0 and (last is None or cr[ci_index-1].get('end')is True),'C instruction boundary');ci.append(r);last=r
    else:require(r is cr[-1]and r['pc']==ni[j]['pc']and r['phase']==2,'unresolved boundary')
  validate_spc_fetches(ci,cr,a.rom.read_bytes())
  require(len(ci)==j-i+1,'instruction count')
  for c,n in zip(ci,ni[i:j+1]):
   require(all(c[z]==n[z]for z in ('pc','a','x','y','sp','ps'))and c['cycles']*2==n['spc_cycle']-s['spc_cycle'],'source register/cycle divergence')
  expected=[{z:r[z]for z in ('pc','spc_cycle','address','value','kind')}for r in nb if s['spc_cycle']<r['spc_cycle']<=limit and useful(r)]
  require(cb==expected,'source attributed data access divergence')
  result=endpoint.read_bytes();require(len(result)==65552,'endpoint length')
  require(tuple(struct.unpack('<H5B',result[:7]))==tuple(ni[j][z]for z in ('pc','a','x','y','sp','ps')),'endpoint registers')
  require(result[7:11]==seed[7:11],'component must not change CPU input latches')
  require(result[11:15]==bytes(ni[j][f'output{z}']for z in range(4)),'separate output latches')
  want_dsp=dsp
  for r in cb:
   if r['kind']=='write'and r['address']==0xf2:want_dsp=r['value']
  require(result[15]==want_dsp,'DSP address latch')
  for r in cb:
   if r['kind']=='write':mem[r['address']]=r['value']
  require(result[16:]==mem,'C ARAM effects outside source writes')
  results.append({'entry':s['pc'],'stop':ni[j]['pc'],'instructions':len(ci),'accepted_cycles':cycle,'data_accesses':len(cb),'trace_sha256':sha(trace),'endpoint_sha256':sha(endpoint)})
  total_i+=len(ci);total_b+=len(cb)
 report={'passed':True,'scope':'isolated components only; no timer/DSP/CPU-clock visibility or normal state model','source_states':source_count,'instruction_states':total_i,'attributed_accesses':total_b,'cases':results,'native_manifest_sha256':sha(p/'manifest.json'),'build_manifest_sha256':sha(a.exe.parent/'build-manifest.json'),'verifier_sha256':sha(__file__),'fetch_contract_sha256':sha(ROOT/'tools/setup_spc_fetch_contract.py')}
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS',total_i,total_b,len(scopes));return report
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--native',type=Path,required=True);p.add_argument('--rom',type=Path,required=True);p.add_argument('--exe',type=Path,required=True);p.add_argument('--output',type=Path,required=True);main(p.parse_args())
