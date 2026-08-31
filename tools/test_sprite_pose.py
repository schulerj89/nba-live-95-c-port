"""Original-opcode, frozen native geometry and legacy-renderer compatibility.

No native submission/OAM or full caller timing claim. New outputs use before
data only; the nine native geometry words are comparison targets, not inputs.
"""
import argparse,hashlib,json,os,random,struct,subprocess
from collections import Counter,defaultdict
from pathlib import Path
from sprite_pose_rom_oracle import oracle,read_rom,word
ROOT=Path(__file__).resolve().parents[1]
ROM_SHA='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
PACK_SHA='f564c29612928984002ed3f0389d317de639fff122baf61a7bc9ecaef2a6be09'
CAP_SHA='4f17d6675caa4ea9ab6707389b9a7e1f39ecea56c2295efddc46982906305af9'
BASE='c172877f378a102a174f58e6eae936abc8e5c781'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def sx(v):return v if v<128 else v+65280
def attest_build(exe,baseline):
 m=json.loads(exe.with_name('manifest.json').read_text())
 assert set(m)=={'schema','baseline','base_commit','source_and_headers','exe_sha256','translation_units','objects'}
 assert type(m['schema'])is int and m['schema']==1 and m['baseline']is baseline and m['base_commit']==BASE
 assert type(m['translation_units'])is int and m['translation_units']==40
 assert sha(exe)==m['exe_sha256']
 names=[ROOT/n.strip()for n in(ROOT/'nba95_sources.txt').read_text().splitlines()if n.strip()and not n.startswith('#')and n.strip()not in('src/main.c','src/nba_player_lab.c')]
 source=exe.parent/'nba_player_lab.c'if baseline else ROOT/'src/nba_player_lab.c'
 expected={str(q)for q in[*names,source,ROOT/'tools/sprite_pose_probe.c',ROOT/'tools/build_sprite_pose_probe.py',ROOT/'nba95_sources.txt',*list((ROOT/'include').glob('*.h'))]}
 assert set(m['source_and_headers'])==expected
 for name,h in m['source_and_headers'].items():assert sha(Path(name))==h,name
 assert set(m['objects'])=={p.stem+'.obj'for p in[*names,source,ROOT/'tools/sprite_pose_probe.c']}
 for name,h in m['objects'].items():assert sha(exe.parent/name)==h
 return m
def attest_capture(cap,rompath):
 assert sha(cap/'manifest.json')==CAP_SHA
 m=json.loads((cap/'manifest.json').read_text());assert m['accepted_capture']is True and m['state_injection']is False and type(m['exit_code'])is int and m['exit_code']==0
 assert m['command']==[str(cap/'portable-mesen/Mesen.exe'),'--testrunner','--timeout=300',str(rompath),str(cap/'capture.lua')]
 assert m['environment']=={'NBA95_CAPTURE_DIR':str(cap)}
 assert m['isolation']['post_settings_sha256']==sha(cap/'portable-mesen/settings.json')
 assert m['rom_sha256']==ROM_SHA and m['mesen_sha256']==sha(cap/'portable-mesen/Mesen.exe')
 assert set(m['artifacts'])=={str(p.relative_to(cap))for p in cap.rglob('*')if p.is_file()and p.name!='manifest.json'}
 for name,d in m['artifacts'].items():
  p=cap/name;assert p.resolve().is_relative_to(cap)and p.stat().st_size==d['size']and sha(p)==d['sha256'],name
 return m
def invoke(exe,pack,mode,inputs,out,label):
 data=b''.join(struct.pack('<'+'H'*len(v),*v)for v in inputs)
 env={k:v for k,v in os.environ.items()if not k.startswith('NBA95')}
 run=subprocess.run([str(exe),str(pack),mode],input=data,capture_output=True,env=env,creationflags=subprocess.CREATE_NO_WINDOW)
 (out/(label+'.input.bin')).write_bytes(data);(out/(label+'.output.bin')).write_bytes(run.stdout);(out/(label+'.stderr.txt')).write_bytes(run.stderr)
 assert type(run.returncode)is int and run.returncode==0
 expected=f"[ASSETS] Loaded asset pack: '{pack}' (89442736 bytes, 264 assets)\r\n".encode()
 assert run.stderr==expected,(label,run.stderr)
 n=36 if mode=='pose'else 22;assert len(run.stdout)==len(inputs)*n*2
 return [list(row)for row in struct.iter_unpack('<'+'H'*n,run.stdout)]
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for name in('exe','baseline','pack','rom','capture','output'):p.add_argument('--'+name,type=Path,required=True)
 a=p.parse_args();a=argparse.Namespace(**{k:v.resolve()for k,v in vars(a).items()})
 out=a.output;out.mkdir(parents=True,exist_ok=False)
 assert sha(a.rom)==ROM_SHA and sha(a.pack)==PACK_SHA
 attest_build(a.exe,False);attest_build(a.baseline,True);attest_capture(a.capture,a.rom)
 rom=a.rom.read_bytes();records=[json.loads(s)for s in(a.capture/'boundaries.jsonl').read_text().splitlines()]
 assert len(records)==271;groups=defaultdict(list);last=-1
 tags={'court.entry':[0x87a47a],'actor.entry':[0x87a4e1],'resources.ready':[0x87a51c],'direction.commit':[0x87a61e],'subject.entry':[0x80af1e],'ordinary.entry':[0x80ad92],'actor.return':[0x87a6a8],'ball.entry':[0x80b0ff],'ball.submit':[0x80b11b],'nmi.entry':[0x80815a],'nmi.exit':[0x808171,0x80859b]}
 for i,r in enumerate(records,1):
  assert all(type(v)is int for k,v in r.items()if k not in('tag','raw'))
  assert r['index']==i and r['raw']==f'raw-{i:05d}.bin' and r['pc']==r['cpu_k']*65536+r['cpu_pc']
  assert r['pc']in tags[r['tag']],r
  assert all(0<=r['cpu_'+k]<=65535 for k in('a','x','y','sp','d','pc'))
  assert 0<=r['cpu_ps']<=255 and 0<=r['cpu_dbr']<=255 and r['frame']==r['ppu_frame'] and r['master_clock']>=last
  last=r['master_clock'];groups[r['call']].append(r)
 native=[];expected=[];details=[];allpcs=set();directions=set();routes=Counter()
 for call,rows in groups.items():
  draws=[r for r in rows if r['tag']in('ordinary.entry','subject.entry')]
  if not draws:continue
  assert len(draws)==1 and rows[-1]['tag']=='actor.return';e=draws[0];end=rows[-1]
  assert e['cpu_d']==0 and e['cpu_dbr']==126 and e['cpu_ps']&0x38==0
  before=(a.capture/e['raw']).read_bytes();after=(a.capture/end['raw']).read_bytes();assert len(before)==len(after)==131072
  w=lambda p:word(before,p)
  inputs=[w(p)for p in(0xd6,0xd4,0xda,0xd8,0x47,0x51,0xc0)]+[e['cpu_a'],w(0x884),e['cpu_x'],e['cpu_y']]
  actor=e['actor'];direction=w(actor+0x52);assert 0<=direction<8
  assert w(0x51)==sx(read_rom(rom,0xacb6b3+inputs[0])[0])
  visibility=sx(read_rom(rom,0xacc7e3+inputs[0])[0])
  assert w(0xd8)==(visibility if visibility&32768 else int.from_bytes(read_rom(rom,0x87a98e+direction*2,2),'little'))
  assert [w(0xd4),w(0xd6),w(0xc0),w(0xc2),w(0x47)]==[w(actor+0x2c),w(actor+0x2a),w(actor+0x4e),direction,w(actor+0x28)]
  projected,pcs,balls=oracle(rom,inputs,e['tag']=='subject.entry',before);allpcs|=pcs
  expected.append([word(after,p)for p in(0xaa,0xac,0x49,0xb2,0xb4,0xb6,0xb8,0xdc,0xde)])
  native.append(inputs);routes[e['tag']]+=1;directions.add(direction)
  details.append({'call':call,'court':e['court'],'actor':actor,'direction':direction,'route':e['tag'],'input_raw':e['raw'],'output_raw':end['raw'],'source_projection':projected,'excluded_ball_boundaries':balls})
 assert len(native)==43 and directions==set(range(8))
 actual=invoke(a.exe,a.pack,'pose',native,out,'native')
 for index,(got,exp,d)in enumerate(zip(actual,expected,details)):
  assert got[0]==1 and got[2:5]+got[6:12]==exp,(index,got,exp)
  assert got==d['source_projection'],(index,got,d)
 (out/'native-details.json').write_text(json.dumps(details,indent=2)+'\n')
 # Controlled source domain. Registers/words are not claimed naturally reached.
 cases=[];labels=Counter();seed=native[0].copy()
 def add(v,label):cases.append(v);labels[label]+=1
 for flags in range(65536):v=seed.copy();v[4]=flags;add(v,'all_flags_47')
 for movement in range(65536):v=seed.copy();v[6]=movement;v[3]=0x591;v[5]=0xffff;add(v,'all_movement_c0')
 for body in range(8):
  for head in range(8):
   for order in(0,0x7fff,0x8000,0xffff):
    for number in(0,0x591,0x592,0x593,0x7fff,0x8000,0xffff):
     v=seed.copy();v[4]=(0x8000 if body<3 else 0)|(4 if head<3 else 0);v[5]=order;v[6]=body;v[3]=number
     v[2]=0x049c+int.from_bytes(read_rom(rom,0x84c36e+head*2,2),'little')
     add(v,'facing_order_number_cross')
 rng=random.Random(0x80ad92)
 for resource in range(0x830):
  for flags in(0,1,2,3,4,7,0x8000,0x8007):
   v=seed.copy();v[0]=resource;v[1]=(resource*3)%0x830;v[4]=flags;v[8]=rng.choice([0,1,0x7fff,0xffff]);v[9]=rng.choice([0,0x7fff,0x8000,0xffff]);v[10]=rng.randrange(65536);v[7]=rng.choice([0,0x3000,0x4000,0xffff]);add(v,'all_table_indices_wrapping')
 got=invoke(a.exe,a.pack,'pose',cases,out,'controlled')
 for i,(v,g)in enumerate(zip(cases,got)):
  exp,pcs,_=oracle(rom,v);allpcs|=pcs
  if g!=exp:
   (out/'first-failure.json').write_text(json.dumps({'index':i,'input':v,'actual':g,'expected':exp},indent=2)+'\n');raise AssertionError(('source',i))
 # Source AF1E ordering with B0FF as a named excluded boundary, across all
 # body/head combinations (same typed body list; ball ordering is not output).
 for v in cases[131072:132864]:
  exp,pcs,_=oracle(rom,v,True);allpcs|=pcs;assert exp==oracle(rom,v)[0]
 compat=[]
 for team in range(29):
  for direction in range(8):
   for v in native:
    compat.append([team,(team+direction)%12,team%2,direction,v[0],v[1],v[9],v[10]])
 cg=invoke(a.exe,a.pack,'compat',compat,out,'compat-candidate');bg=invoke(a.baseline,a.pack,'compat',compat,out,'compat-baseline')
 assert cg==bg
 invalid=[]
 for idx in(0,1):
  for value in(0x830,0x831,0x7fff,0x8000,0xffff):v=seed.copy();v[idx]=value;invalid.append(v)
 assert invoke(a.exe,a.pack,'pose',invalid,out,'invalid-resources')==[[0]*36 for _ in invalid]
 attest_build(a.exe,False);attest_build(a.baseline,True);attest_capture(a.capture,a.rom)
 report={'passed':True,'rom_sha256':ROM_SHA,'pack_sha256':PACK_SHA,'capture_sha256':CAP_SHA,'exe_sha256':sha(a.exe),'baseline_exe_sha256':sha(a.baseline),'native_groups':43,'native_geometry_words':43*9,'source_before_caller_words':43*7,'routes':dict(routes),'resolved_directions':sorted(directions),'controlled_cases':len(cases),'controlled_output_words':len(cases)*36,'controlled_categories':dict(labels),'source_pcs':sorted(hex(0x800000|pc)for pc in allpcs),'compatibility_cases':len(compat),'resource_domain_rejections':len(invalid),'limits':['43 native groups validate nine geometry/flip words only; individual native B348 submissions were not captured.','ROM diagnostic intercepts B348/B0FF as external boundaries with parent projected words preserved. It does not execute OAM allocation, graphics queue or ball interleave.','No runtime literal-field adapter: legacy compatibility entry preserves old combined-direction behavior; full head-order/body status/current C0 producers remain required.','Controlled values, including C0 out of facing range and wrapped coordinates/glyph counter, do not extend natural reachability.']}
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='source_pcs'},indent=2))
if __name__=='__main__':main()
