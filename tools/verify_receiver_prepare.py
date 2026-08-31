"""Closed-corpus, before-only B468/AF66 verifier. New captures need review/pinning."""
import argparse,struct,json,hashlib,subprocess,os
from pathlib import Path
import copy
import mesen_portable
CAPTURES={
 'b26b804bfb879ae033a11f5d58d77b1bc2272bba67e04c8431189bb744638aba':(0,82,3,2,'87fd8da7dcf926f8b9e0a17886e49751a12c8ca4ab19bb444ace90a45a622d66','2fe407d990ad402162bf8d70cbee7278bdd1150c25a1607db6a41787a501887c'),
 '50a3256a89766d1ac1e8ecd7202a6fbc9775187c52e803fa6dc8beae82dc256b':(2,118,5,0,'bf05e5dcada0bfd8ab982a7037337a47d35c69eed96f3b73d259f24480f136a7','ddace9e7211838d979dea5f888470c54c5f8b83bdce39e13d340cef0412d9e47')}
ROM='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
PACK='f564c29612928984002ed3f0389d317de639fff122baf61a7bc9ecaef2a6be09'
MESEN='d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b'
SOURCE_NAMES=['tools/receiver_prepare_probe.c','src/nba_receiver_prepare.c','src/nba_assets.c','src/nba_ea_intro.c','src/nba_intro_text.c','src/nba_rom_font.c','src/nba_renderer.c','src/nba_snes_ppu.c']
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def need(ok,message):
 if not ok:raise ValueError(message)
def unique(items):
 out={}
 for k,v in items:need(k not in out,'duplicate JSON key '+k);out[k]=v
 return out
def read_json(path):return json.loads(path.read_text(),object_pairs_hook=unique)
def equal(a,b):
 if type(a)is not type(b):return False
 if isinstance(a,dict):return a.keys()==b.keys()and all(equal(a[k],b[k])for k in a)
 if isinstance(a,list):return len(a)==len(b)and all(equal(x,y)for x,y in zip(a,b))
 return a==b
def integer(v,lo,hi):return type(v)is int and lo<=v<=hi
def semantic_hash(value):return hashlib.sha256(json.dumps(value,sort_keys=True,separators=(',',':')).encode()).hexdigest()
def identity(path,value):
 need(type(value)is dict and set(value)=={'path','size','sha256'},'identity schema')
 need(value['path']==str(path.resolve()) and integer(value['size'],0,2**40)and type(value['sha256'])is str,'identity types/path')
 need(path.is_file()and path.stat().st_size==value['size']and sha(path)==value['sha256'],'identity mismatch '+str(path))
def attest(capture,rom):
 capture=capture.resolve();digest=sha(capture/'manifest.json')
 need(digest in CAPTURES,'unreviewed capture manifest');selection,row_count,child_count,parent_count,manifest_model,rows_model=CAPTURES[digest]
 m=read_json(capture/'manifest.json')
 # The accepted corpus is deliberately closed. Bind parsed values too, so
 # no caller-supplied/mocked metadata dictionary can evade the file identity.
 need(semantic_hash(m)==manifest_model,'parsed capture differs from approved corpus')
 need(set(m)=={'schema','kind','state_injection','rom_patch','selection','requested_frames','command','environment','isolation','sources','accepted_capture','pid','exit_code','recorded_post_settings_sha256','completion','artifacts'},'capture schema')
 need(type(m['schema'])is int and m['schema']==1 and m['kind']=='C2 ordinary controller AF66/B468 inheritance','capture version')
 need(m['state_injection']is False and m['rom_patch']is False and m['accepted_capture']is True,'capture domain')
 need(type(m['selection'])is int and m['selection']==selection and type(m['requested_frames'])is int and m['requested_frames']==6000,'route')
 need(integer(m['pid'],1,2**32-1)and type(m['exit_code'])is int and m['exit_code']==0,'process result')
 expected_env={'NBA95_CAPTURE_DIR':str(capture),'NBA95_C2_SELECTION':str(selection),'NBA95_C2_FRAMES':'6000'}
 need(equal(m['environment'],expected_env),'environment')
 need(m['command']==[str(capture/'portable-mesen/Mesen.exe'),'--testrunner','--timeout=900',str(rom.resolve()),str(capture/'capture.lua')],'command')
 need(set(m['sources'])=={'rom','mesen','capture','runner','isolation_helper'},'source closure')
 for key,path in [('rom',rom),('mesen',capture/'portable-mesen/Mesen.exe'),('capture',capture/'capture.lua'),('runner',capture/'capture.py'),('isolation_helper',capture/'mesen_portable.py')]:identity(path,m['sources'][key])
 need(m['sources']['rom']['sha256']==ROM and m['sources']['mesen']['sha256']==MESEN,'ROM/Mesen')
 actual={str(p.relative_to(capture))for p in capture.rglob('*')if p.is_file()and p.name!='manifest.json'}
 need(set(m['artifacts'])==actual,'artifact closure')
 for name,value in m['artifacts'].items():
  path=capture/name;need(path.resolve().is_relative_to(capture),'artifact escape');identity(path,value)
 iso=m['isolation'];settings=read_json(capture/'initial-mesen-settings.json')
 need(iso['home']==str(capture/'portable-mesen') and iso['save_folder']==str(capture/'isolated-saves') and iso['initial_saves']==[],'private paths')
 need(equal(iso['settings'],settings)and iso['initial_settings_sha256']==sha(capture/'initial-mesen-settings.json'),'initial settings')
 # Compare recorded post-state BEFORE the legacy helper mutates its copy.
 post=sha(capture/'portable-mesen/settings.json')
 need(type(iso['post_settings_verified'])is bool and iso['post_settings_verified']is True and iso['post_settings_sha256']==post and m['recorded_post_settings_sha256']==post,'post settings')
 need(iso['observed_script_data_folder']==(capture/'observed-script-data-folder.txt').read_text().strip(),'observed home')
 need(iso['final_saves']=={p.name:sha(p)for p in(capture/'isolated-saves').glob('*')if p.is_file()},'save closure')
 mesen_portable.verify(capture,copy.deepcopy(iso))
 need(m['completion']==(capture/'capture_complete.txt').read_text(),'completion')
 rows=[json.loads(line,object_pairs_hook=unique)for line in(capture/'boundaries.jsonl').read_text().splitlines()]
 need(semantic_hash(rows)==rows_model,'parsed rows differ from approved corpus')
 need(len(rows)==row_count and sum(r['tag']=='child.entry'for r in rows)==child_count and sum(r['tag']=='receiver.entry'for r in rows)==parent_count,'route coverage')
 base=rows[0]['frame'];last_master=last_cycles=-1
 for index,r in enumerate(rows,1):
  need(r['index']==index and r['raw']==f'raw_{index:05d}.bin','row identity')
  need(all(integer(v,0,2**54)for k,v in r.items()if k not in('tag','raw')),'numeric JSON types')
  need(r['cpu_k']*65536+r['cpu_pc']==r['pc']and r['frame']==r['ppu_frame']and r['frame']-r['court']==base,'PC/frame binding')
  need(r['master_clock']>last_master and r['cpu_cycles']>last_cycles,'clock order');last_master=r['master_clock'];last_cycles=r['cpu_cycles']
  need(all(integer(r['cpu_'+k],0,65535)for k in('a','x','y','d','sp','pc'))and integer(r['cpu_ps'],0,255)and integer(r['cpu_dbr'],0,255)and integer(r['cpu_k'],0,255),'CPU word domain')
  b=(capture/r['raw']).read_bytes();need(len(b)==131072,'raw size')
  rw=lambda off:struct.unpack_from('<H',b,off)[0]
  for key,offset in [('dp96',0x96),('dp8e',0x8e),('dp9e',0x9e),('dpc2',0xc2),('dpb2',0xb2)]:need(r[key]==rw(r['cpu_d']+offset),'raw '+key)
  need(r['rng']==rw(0x7f6)and r['owner']==rw(0x93e)and r['attempt']==rw(0x904),'raw globals')
  if not r['tag'].startswith('nmi.'):need(r['cpu_d']==0 and r['cpu_ps']&0x38==0,'binary16 CPU domain')
 return rows,digest
def build_attest(exe):
 root=Path(__file__).resolve().parents[1];m=read_json(exe.parent/'manifest.json')
 need(set(m)=={'compiled_sources','headers','exe_sha256'},'build schema')
 need(set(m['compiled_sources'])==set(SOURCE_NAMES),'compiled source closure')
 need(m['compiled_sources']=={n:sha(root/n)for n in SOURCE_NAMES},'source changed since build')
 need(m['headers']=={p.name:sha(p)for p in(root/'include').glob('*.h')},'header closure')
 need(m['exe_sha256']==sha(exe),'executable identity')
def main():
 p=argparse.ArgumentParser();p.add_argument('--capture',type=Path,action='append',required=True);p.add_argument('--exe',type=Path,required=True);p.add_argument('--pack',type=Path,required=True);p.add_argument('--rom',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--af66',action='store_true');a=p.parse_args()
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);pairs=[]
 need(sha(a.rom)==ROM and sha(a.pack)==PACK,'ROM/pack identity');build_attest(a.exe.resolve());attestations=[]
 for folder in a.capture:
  rows,digest=attest(folder,a.rom);attestations.append(digest)
  for e in rows:
   if e['tag']!=('receiver.entry' if a.af66 else 'child.entry'):continue
   if a.af66:x=next(v for v in rows if v['tag']=='receiver.restore' and v['afcall']==e['afcall'])
   else:x=next(v for v in rows if v['tag']=='child.exit' and v['call']==e['call'])
   assert e['cpu_d']==0 and e['cpu_dbr']==0x7e and not(e['cpu_ps']&0x38)
   before=(folder/e['raw']).read_bytes();after=(folder/x['raw']).read_bytes();assert len(before)==len(after)==131072
   pairs.append((folder,e,x,before,after))
 need(bool(pairs),'zero comparison not accepted');need(len(attestations)==len(set(attestations)),'duplicate capture')
 source=out/'before.bin';source.write_bytes(struct.pack('<I',len(pairs))+b''.join(q[3]for q in pairs))
 command=[str(a.exe.resolve()),str(a.pack.resolve()),str(a.rom.resolve()),str(source)]
 if a.af66:command.append('--af66')
 env={k:v for k,v in os.environ.items()if not k.startswith('NBA95')}
 run=subprocess.run(command,env=env,capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
 (out/'stdout.txt').write_bytes(run.stdout);(out/'stderr.txt').write_bytes(run.stderr)
 report=dict(command=command,exit_code=run.returncode,exe_sha256=sha(a.exe),pack_sha256=sha(a.pack),rom_sha256=sha(a.rom),input_sha256=sha(source),pairs=len(pairs),attested_capture_manifests=attestations,scope=('AF66 to AFA3'if a.af66 else'B468 to B624')+' owned actor/global/DP/math words; no whole WRAM/CPU/timing claim',failures=[])
 try:
  need(type(run.returncode)is int and run.returncode==0,'probe exit status')
  pack=a.pack.read_bytes();count=struct.unpack_from('<I',pack,12)[0]
  expected=f"[ASSETS] Loaded asset pack: '{a.pack.resolve()}' ({len(pack)} bytes, {count} assets)\r\n".encode()
  assert run.stderr==expected,(run.stderr,expected)
  actor_offsets=[2,4,6,8,0x88,0x60,0xb2,0x6e,0xe,0x10,0xba,0xbc,0x4c,0x4a,0x7e,0x4e,0x50,0x56,0x58,0x66]
  global_offsets=[0x7f6,0x904,0x936,0x91c,0,2,4,6,0x14,0x18,0x1a,0x47,0x49,0x4f,0x51,0xaa,0xac,0xae,0xb0,0xb2,0xb4,0xb6,0xb8,0xba,0xcc,0xce,0xd0,0x806,0x808,0x80a,0x80c,0x80e,0x810,0x824]
  expected_rows=[]
  for n,(folder,e,x,before,after)in enumerate(pairs):
   actor=struct.unpack_from('<H',before,0x8e if a.af66 else 0x96)[0]
   expected_rows.extend((n,off,struct.unpack_from('<H',after,off)[0])for off in[actor+i for i in actor_offsets]+global_offsets)
   if a.af66:
    passer=struct.unpack_from('<H',before,0x96)[0]
    expected_rows.extend((n,off,struct.unpack_from('<H',after,off)[0])for off in[passer+0x7e,actor+0x5e])
  lines=run.stdout.decode('ascii').splitlines();assert len(lines)==len(expected_rows)
  for line,(n,addr,want)in zip(lines,expected_rows):
   parts=line.split();assert len(parts)==4 and parts[0]=='W';idx,got_addr,value=map(int,parts[1:]);assert(idx,got_addr)==(n,addr) and 0<=value<=65535
   need(line==f'W {n} {addr} {value}','noncanonical response')
   if value!=want:
    folder,e,x,_,_=pairs[n];report['failures'].append(dict(capture=str(folder),call=e['call'],court=e['court'],address=hex(addr),expected=want,actual=value))
  report['compared_words']=len(expected_rows);report['passed']=not report['failures']
 finally:(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report,indent=2));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
