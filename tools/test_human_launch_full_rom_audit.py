"""Independent original-instruction data differential for controlled99C4 inputs."""
import argparse,json,random,sys,subprocess
from pathlib import Path
from test_human_launch_math_rom_audit import Ref
def main():
 p=argparse.ArgumentParser()
 for k in('source','capture','rom','exe','output'):p.add_argument('--'+k,type=Path,required=True)
 a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);sys.path.insert(0,str(a.source.resolve()/'tools'));import verify_human_pass_launch as v
 rom=a.rom.read_bytes();assert v.sha(a.rom)==v.ROM_SHA;rows=[json.loads(s)for s in(a.capture/'boundaries.jsonl').read_text().splitlines()];original=v.raw(a.capture,next(r for r in rows if r['tag']=='launch.entry'));cases=[];rng=random.Random(0x8699c4);edge=[0,1,15,16,191,192,361,362,0x7fff,0x8000,0xfe96,0xff40,0xffff]
 for family in(0xffff,1,0):
  for band in range(0,31,6):
   for alias in(False,True):
    for upper in(0x2a,0x2b,0x2c):
     raw=original.copy();case=len(cases);source=0x34eb+(case%10)*256;receiver=source if alias else 0x34eb+((case+5)%10)*256
     def put(addr,value):raw[addr]=value&255;raw[addr+1]=(value>>8)&255
     put(0x96,source);put(0x8e,receiver)
     for actor in set((source,receiver)):
      for offset in(4,8,12,14,16,0x28,0x5a,0x60,0x64,0x7e):put(actor+offset,rng.choice(edge))
     put(source+0xc0,family);put(source+0x62,band);put(source+0x30,upper);put(source+0x5e,[14,15,0][case%3]);put(receiver+0x5e,14 if case%2 else 15);put(source+0x6e,5 if case%2 else 0)
     put(0x93a,5 if case%4 else 0);put(0x936,rng.choice([0,0x80,0x81,0x82,0x8000,0xffff]))
     for addr in(0x3eef,0x3ef3,0x3ef7):put(addr,rng.choice(edge))
     path=a.output/f'case-{case}.input';path.write_bytes(bytes(raw[i]for start,size in v.RANGES for i in range(start,start+size)));cases.append((dict(case=case,source=source,receiver=receiver,alias=alias,family=family,band=band,upper=upper),raw,path))
 result=subprocess.run([str(a.exe.resolve()),str(a.rom.resolve())],input='\n'.join('launch|'+str(path)+'|0|0|0'for _,_,path in cases)+'\n',text=True,capture_output=True);assert type(result.returncode)is int and result.returncode==0 and result.stderr=='';actual=[json.loads(s,object_pairs_hook=v.unique)for s in result.stdout.splitlines()];assert len(actual)==len(cases);pcs=set();failures=[]
 for (case,raw,path),got in zip(cases,actual):
  _,mem,seen=Ref(rom,raw,0,0,0).run(0x8699c4);pcs.update(seen)
  want=dict(result=1,return_words=[],dp_words=v.words(mem,0,128),actor_words=v.words(mem,0x34eb,1408),controller_words=v.words(mem,0x47eb,160),context_words=v.words(mem,0x46eb,128),profile_words=v.words(mem,0x3449,20),order_words=v.words(mem,0x34d1,13),global_words=[v.word(mem,x)for x in v.GLOBALS],math_words=[v.word(mem,x)for x in v.MATH])
  if not v.strict_equal(got,want):failures.append(dict(**case,fields=[k for k in want if got[k]!=want[k]]))
 report=dict(passed=not failures,cases=[case for case,_,_ in cases],compared_values=len(cases)*1887,source_pcs=len(pcs),failures=failures,scope='controlled source-only original-instruction memory contract including alias/clamps/normalization; not natural branch coverage or hardware/stack/timing proof')
 (a.output/'report.json').write_text(json.dumps(report,indent=2));print({k:value for k,value in report.items()if k!='cases'})
if __name__=='__main__':main()
