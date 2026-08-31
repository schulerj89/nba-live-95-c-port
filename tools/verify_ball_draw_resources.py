"""Closed native resource/projection corpus; no full draw/OAM timing claim."""
import argparse,copy,hashlib,json,struct
from collections import Counter,defaultdict
from pathlib import Path
from mesen_portable import verify
CAPTURE_SHA='4f17d6675caa4ea9ab6707389b9a7e1f39ecea56c2295efddc46982906305af9'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',required=True,type=Path);p.add_argument('--rom',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args()
 cap=a.capture.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 assert sha(cap/'manifest.json')==CAPTURE_SHA
 m=json.loads((cap/'manifest.json').read_text());assert type(m['schema'])is int and m['schema']==1 and m['state_injection']is False and m['accepted_capture']is True and type(m['exit_code'])is int and m['exit_code']==0
 assert sha(a.rom)==m['rom_sha256']=='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
 assert m['mesen_sha256']=='d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b'
 expected={str(p.relative_to(cap))for p in cap.rglob('*')if p.is_file()and p.name!='manifest.json'}
 assert expected==set(m['artifacts'])
 for name,identity in m['artifacts'].items():
  q=cap/name;assert q.resolve().is_relative_to(cap)and q.stat().st_size==identity['size']and sha(q)==identity['sha256']
 assert m['command']==[str(cap/'portable-mesen/Mesen.exe'),'--testrunner','--timeout=300',str(a.rom.resolve()),str(cap/'capture.lua')]
 assert m['environment']=={'NBA95_CAPTURE_DIR':str(cap)}
 # Check actual post-settings identity before the helper can change a copy.
 iso=m['isolation'];assert iso['post_settings_sha256']==sha(cap/'portable-mesen/settings.json')
 verify(cap,copy.deepcopy(iso))
 rom=a.rom.read_bytes()
 def rb(pc,n):return rom[((pc>>16)&127)*32768+(pc&32767):][:n]
 for pc,h in {0x87a4e1:'bd2a0085d6',0x87a517:'bd2c0085d4',0x87a5fb:'bd280029fbffc00300b0030904009d2800',0x87a699:'221eaf80',0x87a6a4:'2292ad80',0x80b111:'a48ea692a5a2186d973f2244b380'}.items():assert rb(pc,len(bytes.fromhex(h)))==bytes.fromhex(h),hex(pc)
 rows=[json.loads(line)for line in(cap/'boundaries.jsonl').read_text().splitlines()];assert len(rows)==271
 groups=defaultdict(list);priorclock=0;tags=Counter()
 for index,r in enumerate(rows,1):
  assert r['index']==index and type(r['index'])is int and isinstance(r['tag'],str)and isinstance(r['raw'],str)
  assert all(type(v)is int for k,v in r.items()if k not in('tag','raw'))
  assert r['pc']==(r['cpu_k']<<16)|r['cpu_pc'];assert 0<=r['cpu_ps']<=255 and 0<=r['cpu_dbr']<=255
  assert all(0<=r['cpu_'+k]<=65535 for k in ['a','x','y','sp','d','pc'])
  assert r['master_clock']>=priorclock and r['ppu_frame']==r['frame'] and 0<=r['court']<600
  priorclock=r['master_clock'];data=(cap/r['raw']).read_bytes();assert len(data)==131072
  r['words']=struct.unpack('<65536H',data);r['data']=data;groups[r['call']].append(r);tags[r['tag']]+=1
 def word(r,address):return struct.unpack_from('<H',r['data'],address)[0]
 full=culled=compared=balls=0;pass_directions=set();details=[]
 for call,events in groups.items():
  if call==0:assert len(events)==1 and events[0]['tag']=='court.entry';continue
  e=events[0];assert e['tag']=='actor.entry'and e['cpu_x']==e['actor'];actor=e['actor']
  assert 0x34eb<=actor<=0x3deb and (actor-0x34eb)%256==0 and e['cpu_ps']&0x38==0 and e['cpu_d']==0
  upper,lower=word(e,actor+0x2a),word(e,actor+0x2c)
  if len(events)==1:
   assert word(e,actor+0x6a)==0xffce;culled+=1;continue
  full+=1;assert events[-1]['tag']=='actor.return'
  ready=[r for r in events if r['tag']=='resources.ready'];assert len(ready)==1
  drawn=[r for r in events if r['tag']in('subject.entry','ordinary.entry')];assert len(drawn)==1
  if word(e,actor+0x5e)==15:pass_directions.add(word(e,actor+0x52))
  for r in events:
   if r['tag']in('resources.ready','direction.commit','subject.entry','ordinary.entry'):
    assert r['cpu_ps']&0x38==0 and r['cpu_d']==0
    assert [word(r,0xd6),word(r,0xd4),word(r,actor+0x2a),word(r,actor+0x2c)]==[upper,lower,upper,lower];compared+=4
  be=[r for r in events if r['tag']=='ball.entry'];bs=[r for r in events if r['tag']=='ball.submit'];assert len(be)==len(bs)
  for before,submit in zip(be,bs):
   assert submit['cpu_x']==word(before,0x92)and submit['cpu_y']==word(before,0x8e)
   assert submit['cpu_a']==(word(before,0xa2)+word(before,0x3f97))&65535 and word(submit,0)==0x81d;balls+=1;compared+=4
  details.append({'call':call,'court':e['court'],'actor':actor,'mode':word(e,actor+0x5e),'resolved_direction':word(e,actor+0x52),'upper':upper,'lower':lower,'draw_route':drawn[0]['tag'],'nmi':sum(r['tag']=='nmi.entry'for r in events)})
 report={'passed':True,'capture_sha256':CAPTURE_SHA,'complete_draws':full,'entry_only_source_culled':culled,'ball_submissions':balls,'owned_words':compared,'pass_directions':sorted(pass_directions),'tags':dict(tags),'calls':details,'limits':['Closed immutable capture only; source-owned resource/projection words, not complete CPU/DP/OAM or interrupt timing.','Eleven culled entry records stop before D4 publication; their raw +6A=FFCE proves the source early-skip domain, not an observed exit.','Independent head/body mirror, jersey order and shared graphics queue remain separate draw integration scope.']}
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='calls'},indent=2))
if __name__=='__main__':main()
