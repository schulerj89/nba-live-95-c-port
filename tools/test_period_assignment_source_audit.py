"""D85E fixed source-dataflow diagnostic, not a production implementation.

Original ROM actor/statistic tables, carried 3471/34A1 pointers and ROM roster
profile bytes are authoritative. No C asset accessor/table supplies expected
values. Binary M/X/D and canonical parent actor IDs are the supported domain.
"""
import argparse,hashlib,itertools,json,random,struct,subprocess
from pathlib import Path
def source(rom,raw):
 mem=bytearray(raw)
 def word(a):return struct.unpack_from('<H',mem,a)[0]
 def put(a,v):struct.pack_into('<H',mem,a,v&65535)
 def rw(addr):return int.from_bytes(rom[((addr>>16)&127)*32768+(addr&32767):][:2],'little')
 def rb(addr):return rom[((addr>>16)&127)*32768+(addr&32767)]
 actors=[rw(0x879c7b+2*i)for i in range(10)]
 # D869/D888 populate current actor's base matchup. D7B8 copies carried
 # lineup pointers, not regenerated selected-team records.
 for i in range(10):
  selector=mem[0x159a+i]&127;put(actors[i]+0x76,2*(selector+(5 if i<5 else 0)))
 for i in range(10):
  side=i//5;slot=word(0x46f9+side*128+(i%5)*2);table=0x3471+side*48
  mem[0x3449+i*4:0x344d+i*4]=mem[table+slot*4:table+slot*4+4]
  put(0x3435+i*2,rw(0x879c8f+2*(slot+side*12)))
 # D8B2 obtains roster/alternate destination through the actual actor ID.
 for a in actors:
  actor_id=word(a);pointer=struct.unpack_from('<I',mem,0x3449+4*actor_id)[0]
  put(a+0x6c,rb(pointer+8));paired=actors[word(a+0x76)//2];put(paired+0x78,actor_id*2)
 for side in range(2):
  items=[]
  for i in range(side*5,side*5+5):
   a=actors[i];offset=word(a+0x76);put(a+0x74,offset);pointer=struct.unpack_from('<I',mem,0x3449+2*offset)[0]
   # D920/D9AD SEP20 makes addition wrap at eight bits before REP/AND.
   value=(rb(pointer+0x36)+rb(pointer+0x37))&255;helping=bool(mem[0x159a+i]&128);put(actors[offset//2]+0x80,int(helping))
   if helping:value+=100
   items.append((value,offset))
  # D73E bubble-restarts after swapping a strictly smaller previous key.
  while True:
   for i in range(4):
    if ((items[i][0]-items[i+1][0])&32768):items[i],items[i+1]=items[i+1],items[i];break
   else:break
  for i,(key,offset)in enumerate(items):put(0x9da+4*i,key);put(0x9dc+4*i,offset);mem[0x4734+128*side+i]=offset&255;put(actors[offset//2]+0x92,4-i)
 return mem
def main():
 p=argparse.ArgumentParser()
 for k in('rom','exe','pack','replay','output'):p.add_argument('--'+k,type=Path,required=True)
 a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);rom=a.rom.read_bytes();assert hashlib.sha256(rom).hexdigest()=='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
 assert rom[0x35920:0x3592f]==bytes.fromhex('e220b79a18a03700779ac22029ff00')
 rows=json.loads(a.replay.read_text())['cases'];regions=[(0x34d3,24),(0x34eb,2816),(0x3435,60),(0x9da,20),(0x4734,5),(0x47b4,5)]
 def diff(got,want):return[dict(address=hex(p),actual=got[p],expected=want[p])for start,size in regions for p in range(start,start+size)if got[p]!=want[p]]
 native=0
 for row in rows:
  if row['mode']!='assignment':continue
  before=Path(row['command'][2]);rr=[json.loads(s)for s in(before.parent/'boundaries.jsonl').read_text().splitlines()];after=next(r for r in rr if r['tag']=='assignment.after');expected=source(rom,before.read_bytes());assert not diff(expected,(before.parent/after['raw']).read_bytes());native+=1
 seed=Path(next(c for c in rows if c['mode']=='assignment')['command'][2]).read_bytes();r=random.Random(0xd85e);controlled=0
 def probe(raw,label):
  inp=a.output/(label+'.input');out=a.output/(label+'.output');inp.write_bytes(raw)
  result=subprocess.run([str(a.exe.resolve()),str(a.pack.resolve()),str(inp.resolve()),'assignment',str(out.resolve())],capture_output=True)
  assert result.returncode==0,(label,result.returncode);return out.read_bytes()
 for permutation in itertools.permutations(range(5)):
  raw=bytearray(seed)
  for i in range(10):raw[0x159a+i]=permutation[i%5 if i<5 else 4-i%5]|r.choice((0,128));struct.pack_into('<H',raw,0x46f9+(i//5)*128+(i%5)*2,r.randrange(12))
  want=source(rom,raw);got=probe(raw,'controlled');assert not diff(got,want),diff(got,want)[:10];controlled+=1
 failures=[]
 for label,mutate in [('noncanonical-actor-ID',lambda b:struct.pack_into('<H',b,0x34eb,1)),('carried-roster-pointer',lambda b:b.__setitem__(slice(0x3471,0x3475),b[0x3475:0x3479]))]:
  raw=bytearray(seed);mutate(raw);got=probe(raw,label);want=source(rom,raw);d=diff(got,want);assert d;failures.append(dict(name=label,differences=d));(a.output/(label+'.rom')).write_bytes(want)
 report=dict(passed=True,native_calls=native,controlled_permutations=controlled,unsupported_raw_prestate_differences=failures,scope='fixed source-dataflow; 120 bijective selectors and changed roster slots/flags; no natural permutation or general memory alias claim')
 (a.output/'report.json').write_text(json.dumps(report,indent=2));print(report)
if __name__=='__main__':main()
