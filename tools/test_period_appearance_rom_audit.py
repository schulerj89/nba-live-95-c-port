"""Actual-ROM AAB2/B572 diagnostic, fixed binary 16-bit/DP0/DB0 domain.

Instruction decoding reads the original bytes. It is never linked into C or
used to predict timing, hardware effects, or normal reachability.
"""
import argparse,hashlib,itertools,json,struct,subprocess
from pathlib import Path
class Ref:
 def __init__(self,rom,raw):
  self.rom=rom;self.mem=bytearray(raw);self.a=self.x=self.y=0;self.c=self.z=self.n=False;self.stack=[];self.pcs=set()
 def byte(self,p):
  if p>>16 in(0x7e,0x7f):return self.mem[p-0x7e0000]
  if p&65535<0x8000 and p>>16&0x7f<0x40:return self.mem[p&65535]
  return self.rom[((p>>16)&127)*32768+(p&32767)]
 def word(self,p):return self.byte(p)|(self.byte(p+1)<<8)
 def put(self,p,v):assert p<65535;self.mem[p]=v&255;self.mem[p+1]=(v>>8)&255
 def nz(self,v):v&=65535;self.n=bool(v&32768);self.z=v==0;return v
 def run(self,pc):
  for _ in range(3000):
   self.pcs.add(pc);op=self.byte(pc);arg=self.byte(pc+1);w=self.word(pc+1);nxt=pc+1
   # address modes; operand dispatch below is deliberately opcode based.
   dp={0xa5,0xa6,0xa4,0x85,0x84,0x86,0x64,0xc5,0x65,0xe5,0x45};imm={0xa9,0xa2,0xa0,0xc9,0xc0,0x29,0x49,0x09,0x69,0xe9,0x89}
   absops={0xad,0xae,0xac,0x8d,0x9c,0xcd,0xcc,0x0d};xops={0xbd,0xbc,0x9d,0x9e,0xdd,0x7d,0xfd,0xde,0x1d};yops={0xb9,0x99,0xd9,0x19}
   if op in dp:addr=arg;v=self.word(addr);nxt=pc+2
   elif op in imm:addr=None;v=w;nxt=pc+3
   elif op in absops|xops|yops:
    addr=(w+(self.x if op in xops else self.y if op in yops else 0))&65535;v=self.word(addr);nxt=pc+3
   elif op==0xbf:addr=(self.byte(pc+3)<<16|w)+self.x;v=self.word(addr);nxt=pc+4
   elif op in(0xa7,0xb7,0xd7):addr=self.word(arg)|(self.byte(arg+2)<<16);addr+=self.y if op in(0xb7,0xd7)else 0;v=self.word(addr);nxt=pc+2
   else:addr=None;v=0
   if op in(0xa5,0xa9,0xad,0xbd,0xb9,0xbf,0xa7,0xb7):self.a=self.nz(v)
   elif op in(0xa6,0xa2,0xae):self.x=self.nz(v)
   elif op in(0xa4,0xa0,0xac,0xbc):self.y=self.nz(v)
   elif op in(0x85,0x8d,0x9d,0x99):self.put(addr,self.a)
   elif op==0x84:self.put(addr,self.y)
   elif op==0x86:self.put(addr,self.x)
   elif op in(0x64,0x9c,0x9e):self.put(addr,0)
   elif op in(0xc5,0xc9,0xcd,0xdd,0xd9,0xd7,0xc0,0xcc):
    lhs=self.y if op in(0xc0,0xcc)else self.a;self.nz(lhs-v);self.c=lhs>=v
   elif op in(0x69,0x65,0x7d):r=self.a+v+self.c;self.a=self.nz(r);self.c=r>65535
   elif op in(0xe9,0xe5,0xfd):r=self.a-v-(not self.c);self.a=self.nz(r);self.c=r>=0
   elif op==0x29:self.a=self.nz(self.a&v)
   elif op in(0x49,0x45):self.a=self.nz(self.a^v)
   elif op in(0x09,0x0d,0x1d,0x19):self.a=self.nz(self.a|v)
   elif op==0x89:self.z=(self.a&v)==0
   elif op==0xde:self.put(addr,self.nz(v-1))
   elif op in(0xf0,0xd0,0x10,0x30,0xb0,0x90,0x80):
    take={0xf0:self.z,0xd0:not self.z,0x10:not self.n,0x30:self.n,0xb0:self.c,0x90:not self.c,0x80:True}[op];nxt=pc+2+((arg-256 if arg&128 else arg)if take else 0)
   elif op==0x4c:nxt=pc&0xff0000|w
   elif op in(0x20,0x22):self.run((pc&0xff0000|w)if op==0x20 else(self.byte(pc+3)<<16|w));nxt=pc+(3 if op==0x20 else 4)
   elif op in(0x60,0x6b):return
   elif op in(0xaa,0xa8,0x8a,0x98,0xbb):
    if op==0xaa:self.x=self.nz(self.a)
    elif op==0xa8:self.y=self.nz(self.a)
    elif op==0x8a:self.a=self.nz(self.x)
    elif op==0x98:self.a=self.nz(self.y)
    else:self.x=self.nz(self.y)
   elif op==0x48:self.stack.append(self.a)
   elif op==0x68:self.a=self.nz(self.stack.pop())
   elif op==0x38:self.c=True
   elif op==0x18:self.c=False
   elif op==0x3a:self.a=self.nz(self.a-1)
   elif op==0x1a:self.a=self.nz(self.a+1)
   elif op==0x0a:self.c=bool(self.a&32768);self.a=self.nz(self.a<<1)
   elif op==0xeb:self.a=(self.a<<8|self.a>>8)&65535;self.n=bool(self.a&128);self.z=(self.a&255)==0
   else:raise AssertionError((hex(pc),hex(op)))
   pc=nxt
  raise AssertionError('bounded ROM diagnostic exceeded')
def main():
 p=argparse.ArgumentParser()
 for k in('rom','replay','pack','exe','output'):p.add_argument('--'+k,type=Path,required=True)
 a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);rom=a.rom.read_bytes();assert hashlib.sha256(rom).hexdigest()=='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
 rows=json.loads(a.replay.read_text())['cases'];count=0;pcs=set();controlled=0;failures=[]
 def expected(raw,actor):
  r=Ref(rom,raw);r.run(0x87aab2);assert not r.stack;pcs.update(r.pcs);base=0x34eb+256*actor
  return dict(rng=r.word(0x7f6),owner_pointer=r.word(0x940),actor=list(struct.unpack_from('<128H',r.mem,base)))
 for row in rows:
  raw=Path(row['command'][2]).read_bytes();result=expected(raw,row['actor']);native=Path(row['command'][2]).parent/row['exit'];target=native.read_bytes();base=0x34eb+256*row['actor'];want=dict(rng=struct.unpack_from('<H',target,0x7f6)[0],owner_pointer=struct.unpack_from('<H',target,0x940)[0],actor=list(struct.unpack_from('<128H',target,base)))
  assert result==want,(row['period'],row['actor']);count+=130
 seed=Path(rows[0]['command'][2]).read_bytes();actor=rows[0]['actor'];base=0x34eb+256*actor
 for state,alt,direction in itertools.product(range(19),(0,1),(0,3,7)):
  raw=bytearray(seed)
  for off,val in[(0x38,state),(0x30,0),(0x32,0),(0x3a,0),(0x3c,0),(0x42,0),(0x44,0),(0x46,0),(0x48,0),(0xa8,alt),(0x52,direction),(0x4e,0xffff),(0xae,0xbeef)]:struct.pack_into('<H',raw,base+off,val)
  struct.pack_into('<H',raw,0xc6,0x0102 if direction==7 else 2)
  want=expected(raw,actor);path=a.output/'controlled.wram';path.write_bytes(raw)
  r=subprocess.run([str(a.exe.resolve()),str(a.pack.resolve()),str(path.resolve()),str(actor)],capture_output=True,text=True);assert r.returncode==0,(state,alt,direction,r.stderr)
  got=json.loads(r.stdout);controlled+=1
  if got!=want:failures.append(dict(state=state,alt=alt,direction=direction,difference=[(i,x,y)for i,(x,y)in enumerate(zip(got['actor'],want['actor']))if x!=y],rng=[got['rng'],want['rng']]))
 report=dict(passed=not failures,native_words=count,native_calls=len(rows),controlled_calls=controlled,source_pcs=len(pcs),failures=failures,scope='actual-ROM fixed binary16 DP0 DB0 uninterrupted diagnostic; no timing/human/normal reachability claim')
 (a.output/'report.json').write_text(json.dumps(report,indent=2));print(report)
if __name__=='__main__':main()
