"""Bounded original-byte F78B/F8D9 diagnostic, never production code.

Binary D0. Models M/X widths, byte pushes and the source's 8x8 hardware
multiply result at its reads; no CPU cycles, hardware latency or stack-address
parity claim. Actual opcode order drives scratch writes and signed edge paths.
"""
import argparse,hashlib,json,random,subprocess,sys
from pathlib import Path
class Ref:
 def __init__(self,rom,raw,a,x,y):
  self.rom=rom;self.mem=dict(raw);self.a=a;self.x=x;self.y=y;self.db=0x7e;self.m8=self.x8=False;self.c=self.n=self.z=False;self.stack=[];self.calls=[];self.mul_a=0;self.product=0;self.pcs=set()
 def read(self,addr,width=2):
  if addr>=0x800000:
   offset=((addr>>16)&127)*32768+(addr&32767);return int.from_bytes(self.rom[offset:offset+width],'little')
  if addr==0x4216:return self.product&((1<<(width*8))-1)
  return sum(self.mem[addr+i]<<(8*i)for i in range(width))
 def write(self,addr,value,width=2):
  if addr==0x4202:assert width==1;self.mul_a=value&255;return
  if addr==0x4203:assert width==1;self.product=self.mul_a*(value&255);return
  for i in range(width):assert addr+i in self.mem;self.mem[addr+i]=(value>>(8*i))&255
 def nz(self,value,width=2):
  value&=(1<<(width*8))-1;self.n=bool(value&(1<<(width*8-1)));self.z=value==0;return value
 def aset(self,value):
  width=1 if self.m8 else 2;v=self.nz(value,width);self.a=(self.a&0xff00)|v if self.m8 else v
 def push(self,value,width):
  for i in reversed(range(width)):self.stack.append((value>>(8*i))&255)
 def pop(self,width):return sum(self.stack.pop()<<(8*i)for i in range(width))
 def run(self,pc):
  for steps in range(5000):
   assert 0x85f78b<=pc<=0x85f928 or 0x869846<=pc<=0x86986c or 0x8699c4<=pc<=0x869c6e;self.pcs.add(pc);off=((pc>>16)&127)*32768+(pc&32767);op=self.rom[off];b=self.rom[off+1];w=int.from_bytes(self.rom[off+1:off+3],'little');aw=1 if self.m8 else 2;iw=1 if self.x8 else 2;amask=(1<<(aw*8))-1;addr=None;nxt=pc+1;value=0
   if op in(0xa9,0xc9,0x49,0x69,0xe9,0x09):value=b if aw==1 else w;nxt=pc+1+aw
   elif op in(0xa0,0xe0,0xc0):value=b if iw==1 else w;nxt=pc+1+iw
   elif op in(0xa5,0xa6,0xa4,0x85,0x86,0x84,0x64,0x65,0xe6,0x66):addr=b;nxt=pc+2;value=self.read(addr,iw if op in(0xa6,0xa4,0x86,0x84)else aw)
   elif op in(0xad,0xae,0xac,0x8d,0x8e,0x8c,0x9c,0xee,0xce,0x6d,0xcd,0xed,0x0e,0x2e,0x4e,0x6e):
    addr=w;nxt=pc+3;value=0 if w in(0x4202,0x4203)else self.read(addr,iw if op in(0xae,0xac,0x8e,0x8c)else aw)
   elif op in(0xbd,0xbc,0x9d,0x9e,0x7d,0xfd):addr=(w+self.x)&65535;nxt=pc+3;value=self.read(addr,iw if op==0xbc else aw)
   elif op==0xa7:addr=self.read(b,3);nxt=pc+2;value=self.read(addr,aw)
   if op in(0xa9,0xa5,0xad,0xbd,0xa7):self.aset(value)
   elif op in(0xa6,0xae):self.x=self.nz(value,iw)
   elif op in(0xa0,0xa4,0xac,0xbc):self.y=self.nz(value,iw)
   elif op in(0x85,0x8d,0x9d):self.write(addr,self.a,aw)
   elif op in(0x86,0x8e):self.write(addr,self.x,iw)
   elif op in(0x84,0x8c):self.write(addr,self.y,iw)
   elif op in(0x9c,0x9e,0x64):self.write(addr,0,aw)
   elif op in(0xc9,0xcd,0xe0,0xc0):
    lhs=self.x if op==0xe0 else self.y if op==0xc0 else self.a&amask;self.nz(lhs-value,iw if op in(0xe0,0xc0)else aw);self.c=lhs>=value
   elif op in(0x69,0x6d,0x65,0x7d):total=(self.a&amask)+value+self.c;self.aset(total);self.c=total>amask
   elif op in(0xed,0xe9,0xfd):total=(self.a&amask)-value-(not self.c);self.aset(total);self.c=total>=0
   elif op==0x49:self.aset((self.a&amask)^value)
   elif op==0x09:self.aset((self.a&amask)|value)
   elif op in(0xee,0xce,0xe6):self.write(addr,self.nz(value+(1 if op in(0xee,0xe6)else -1),aw),aw)
   elif op in(0x0e,0x2e,0x4e,0x6e,0x66):
    old_c=self.c
    if op in(0x0e,0x2e):self.c=bool(value&(1<<(aw*8-1)));value=(value<<1)|(int(old_c)if op==0x2e else 0)
    else:self.c=bool(value&1);value=(value>>1)|((int(old_c)<<(aw*8-1))if op in(0x6e,0x66)else 0)
    self.write(addr,self.nz(value,aw),aw)
   elif op==0x1a:self.aset((self.a&amask)+1)
   elif op==0xe8:self.x=self.nz(self.x+1,iw)
   elif op==0xc8:self.y=self.nz(self.y+1,iw)
   elif op==0x88:self.y=self.nz(self.y-1,iw)
   elif op==0x8a:self.aset(self.x)
   elif op==0x98:self.aset(self.y)
   elif op==0xaa:self.x=self.nz(self.a,iw)
   elif op==0xeb:self.a=((self.a&255)<<8)|(self.a>>8);self.nz(self.a&255,1)
   elif op==0x6a:old_c=self.c;self.c=bool(self.a&1);self.aset(((self.a&amask)>>1)|(int(old_c)<<(aw*8-1)))
   elif op==0xd4:self.push(self.read(b,2),2);nxt=pc+2
   elif op in(0x48,0xda):self.push(self.a if op==0x48 else self.x,aw if op==0x48 else iw)
   elif op==0x68:self.aset(self.pop(aw))
   elif op==0xfa:self.x=self.nz(self.pop(iw),iw)
   elif op==0x8b:self.push(self.db,1)
   elif op==0x4b:self.push(pc>>16,1)
   elif op==0xab:self.db=self.nz(self.pop(1),1)
   elif op in(0xe2,0xc2):
    assert b in(0x20,0x30);self.m8=op==0xe2;nxt=pc+2
    if b&0x10:self.x8=op==0xe2
    if self.x8:self.x&=255;self.y&=255
   elif op==0x18:self.c=False
   elif op==0x38:self.c=True
   elif op in(0xb0,0x90,0x10,0x30,0xf0,0xd0,0x80):
    take={0xb0:self.c,0x90:not self.c,0x10:not self.n,0x30:self.n,0xf0:self.z,0xd0:not self.z,0x80:True}[op];nxt=pc+2+((b-256 if b&128 else b)if take else 0)
   elif op==0x22:self.calls.append(pc+4);nxt=(self.rom[off+3]<<16)|w
   elif op==0x20:self.calls.append(pc+3);nxt=(pc&0xff0000)|w
   elif op==0x4c:nxt=(pc&0xff0000)|w
   elif op in(0x6b,0x60):
    if not self.calls:assert not self.stack;return [self.a,self.x,self.y],self.mem,self.pcs
    nxt=self.calls.pop()
   elif op==0xea:pass
   else:raise AssertionError((hex(pc),hex(op)))
   pc=nxt
  raise AssertionError('bounded arithmetic exceeded')
def main():
 p=argparse.ArgumentParser()
 for k in('source','capture','rom','exe','output'):p.add_argument('--'+k,type=Path,required=True)
 a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);sys.path.insert(0,str(a.source.resolve()/'tools'));import verify_human_pass_launch as v
 rom=a.rom.read_bytes();assert hashlib.sha256(rom).hexdigest()==v.ROM_SHA;rows=[json.loads(s)for s in(a.capture/'boundaries.jsonl').read_text().splitlines()];entry=next(r for r in rows if r['tag']=='launch.entry');raw=v.raw(a.capture,entry);inp=a.output/'original-entry.bin';inp.write_bytes(bytes(raw[i]for start,size in v.RANGES for i in range(start,start+size)));r=random.Random(0xf78bf8d9);cases=[];pcs=set()
 edge=[0,1,2,255,256,257,0x7fff,0x8000,0xff00,0xfeff,0xffff]
 for x in edge:
  for y in edge:cases.append(('mul',[x,y,0x3deb]));cases.append(('divide',[x,0x8000 if x&1 else 0x7fff,y]))
 for i in range(512):cases.append(('mul'if i%2 else'divide',[r.randrange(65536),r.randrange(65536),r.randrange(65536)]))
 text='\n'.join(mode+'|'+str(inp)+'|'+'|'.join(map(str,args))for mode,args in cases)+'\n';result=subprocess.run([str(a.exe.resolve()),str(a.rom.resolve())],input=text,capture_output=True,text=True);assert type(result.returncode)is int and result.returncode==0 and result.stderr=='';actual=[json.loads(s,object_pairs_hook=v.unique)for s in result.stdout.splitlines()];assert len(actual)==len(cases);failures=[]
 for i,((mode,args),got)in enumerate(zip(cases,actual)):
  ret,mem,seen=Ref(rom,raw,*args).run(0x85f78b if mode=='mul'else 0x85f8d9);pcs.update(seen)
  want=dict(result=1,return_words=ret,dp_words=v.words(mem,0,128),actor_words=v.words(mem,0x34eb,1408),controller_words=v.words(mem,0x47eb,160),context_words=v.words(mem,0x46eb,128),profile_words=v.words(mem,0x3449,20),order_words=v.words(mem,0x34d1,13),global_words=[v.word(mem,x)for x in v.GLOBALS],math_words=[v.word(mem,x)for x in v.MATH])
  if not v.strict_equal(got,want):failures.append(dict(case=i,mode=mode,args=args,fields=[k for k in want if got[k]!=want[k]]))
 report=dict(passed=not failures,cases=len(cases),compared_values=len(cases)*1890,source_pcs=len(pcs),failures=failures,scope='independent original-opcode arithmetic data diagnostic; hardware8x8 result observed at source reads only, no latency/CPU-stack/timing claim; controlled operands not natural reachability')
 (a.output/'report.json').write_text(json.dumps(report,indent=2));print(report)
if __name__=='__main__':main()
