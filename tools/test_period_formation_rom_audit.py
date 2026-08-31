"""Bounded actual-ROM coordinate proof, independent of the formation C table."""
import argparse,hashlib,importlib.util,json,struct
from pathlib import Path

def original(rom,period,tip,anchor,pair):
 mem=bytearray(65536);a=pair;x=(40 if 0<period<4 else 0)+8*pair;y=0x34eb+256*pair;c=n=z=False;stack=[];pc=0x86dda7;pcs=set()
 def put(addr,val):mem[addr]=val&255;mem[addr+1]=(val>>8)&255
 def word(addr):return mem[addr]|mem[addr+1]<<8
 def code(addr):return rom[((addr>>16)&127)*32768+(addr&32767)]
 def nz(v):
  nonlocal n,z
  v&=65535;n=bool(v&32768);z=v==0;return v
 put(0x926,period);put(0x932,tip);put(0xb6,anchor)
 for _ in range(200):
  if pc==0x86df27:break
  pcs.add(pc);op=code(pc);arg=code(pc+1);w=arg|code(pc+2)<<8;nxt=pc+1
  if op in(0xa5,0xa4,0x85,0x84):
   nxt=pc+2
   if op==0xa5:a=nz(word(arg))
   elif op==0xa4:y=nz(word(arg))
   elif op==0x85:put(arg,a)
   else:put(arg,y)
  elif op in(0xad,0x99):
   nxt=pc+3
   if op==0xad:a=nz(word(w))
   else:put((w+y)&65535,a)
  elif op==0xbf:
   nxt=pc+4;addr=(code(pc+3)<<16|w)+x;a=nz(code(addr)|code(addr+1)<<8)
  elif op in(0x49,0x29,0xe9,0xc9,0xe0):
   nxt=pc+3
   if op==0x49:a=nz(a^w)
   elif op==0x29:a=nz(a&w)
   elif op==0xe9:r=a-w-(not c);a=nz(r);c=r>=0
   else:v=x if op==0xe0 else a;nz(v-w);c=v>=w
  elif op==0x1a:a=nz(a+1)
  elif op==0x38:c=True
  elif op==0x5a:stack.append(y)
  elif op==0x7a:y=nz(stack.pop())
  elif op==0x4c:nxt=pc&0xff0000|w
  elif op in(0xf0,0xd0,0x10,0x80):
   take={0xf0:z,0xd0:not z,0x10:not n,0x80:True}[op];nxt=pc+2+((arg-256 if arg&128 else arg)if take else 0)
  else:raise AssertionError((hex(pc),hex(op)))
  pc=nxt
 else:raise AssertionError('ROM coordinate bound')
 assert not stack
 return {actor:{name:word(0x34eb+actor*256+off)for name,off in [('field_a6',0xa6),('x',4),('y',8),('target_x',0x56),('target_y',0x58),('direction',0x4e),('requested_direction',0x50),('movement_direction',0x52)]}for actor in(pair,pair+5)},pcs

def main():
 p=argparse.ArgumentParser()
 for k in('verifier','rom','exe','output'):p.add_argument('--'+k,type=Path,required=True)
 a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);spec=importlib.util.spec_from_file_location('period_v',a.verifier);v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
 rom=a.rom.read_bytes();assert hashlib.sha256(rom).hexdigest()==v.ROM_SHA
 assert rom[0x35de7:0x35def]==bytes.fromhex('a5ba49ffff1a85ba')
 names=v.mapping();index={name:i for i,(name,_)in enumerate(names)};differences=[];count=0;pcs=set();cases=0
 for period in range(5):
  for tip in(0,5):
   for anchor in(-336,336):
    label=f'{period}-{tip}-{anchor}';rows=v.run_probe(a.exe,a.output,label,v.binary_input(period,tip,(anchor,-anchor),[(0xa513+37*i)&65535 for i in range(len(names))]));cases+=1
    for pair in range(5):
     expected,touched=original(rom,period,tip,anchor,pair);pcs|=touched;row=next(r for r in rows if r['kind']==1 and r['actor']==pair)
     for actor,fields in expected.items():
      for field,want in fields.items():
       count+=1;got=row['words'][index[f'actor{actor}.{field}']]
       if got!=want:differences.append(dict(case=label,actor=actor,field=field,C=got,ROM=want))
 report=dict(passed=not differences,cases=cases,comparisons=count,source_pcs=len(pcs),differences=differences,scope='controlled source-only coordinate proof, no native reachability/timing')
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(cases,count,len(differences),'differences');return bool(differences)
if __name__=='__main__':raise SystemExit(main())
