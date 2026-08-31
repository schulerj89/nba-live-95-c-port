"""Independent bounded ROM-byte diagnostic; no source interpreter in production."""
import argparse,ctypes as C,hashlib,itertools,json,random
from pathlib import Path
U=C.c_uint16
class State(C.Structure):_fields_=[('v',U*50)]
class Tables(C.Structure):_fields_=[('thresholds',C.c_uint8*8),('pointers',U*10)]
ACTOR=0x36eb
ADDR=[ACTOR+i for i in (4,8,12,0x28,0x2a,0x2c,0x30,0x32,0x34,0x36,0x3a,0x3c,0x3e,0x40,0x4e,0x52,0x5e,0x6c,0x7e,0xa8)]
ADDR += [0x3eef,0x3ef3,0x3ef7,0x922,0x936,0,2,4,6,0x47,0x49,0xac]
ADDR += [ACTOR+i for i in (0x16,0x60,0x64,0x66,0x6e,0xc0)]
ADDR += [0xc2,0xc6,0x93a,0x93e,0x942,0x944,0x946,0x9c4,0x8e,0x90,0xaa,0xae]

class Ref:
 def __init__(self,rom,s):
  self.rom=rom;self.mem=bytearray(65536);self.a=self.x=self.y=0;self.n=self.z=self.c=False;self.pcs=set()
  for addr,value in zip(ADDR,s.v):self.put(addr,value)
  self.put(0x96,ACTOR)
 def byte(self,p):return self.rom[((p>>16)&127)*32768+(p&32767)] if p>>16 else self.mem[p]
 def word(self,p):return self.byte(p)|(self.byte(p+1)<<8)
 def put(self,p,v):self.mem[p]=v&255;self.mem[p+1]=(v>>8)&255
 def nz(self,v):v&=65535;self.n=bool(v&32768);self.z=v==0;return v
 def run(self,pc,stops=()):
  for _ in range(200):
   if pc in stops:return pc
   self.pcs.add(pc);op=self.byte(pc);arg=self.byte(pc+1);w=self.word(pc+1);nxt=pc+1
   if op in (0xa5,0xa6,0xa4,0x85,0x84,0xc5,0xe5):
    nxt=pc+2;value=self.word(arg)
    if op==0xa5:self.a=self.nz(value)
    elif op==0xa6:self.x=self.nz(value)
    elif op==0xa4:self.y=self.nz(value)
    elif op==0x85:self.put(arg,self.a)
    elif op==0x84:self.put(arg,self.y)
    elif op==0xc5:self.nz(self.a-value);self.c=self.a>=value
    else:r=self.a-value-(not self.c);self.a=self.nz(r);self.c=r>=0
   elif op in (0xa9,0xa0,0xc9,0x29,0xe9):
    nxt=pc+3
    if op==0xa9:self.a=self.nz(w)
    elif op==0xa0:self.y=self.nz(w)
    elif op==0xc9:self.nz(self.a-w);self.c=self.a>=w
    elif op==0x29:self.a=self.nz(self.a&w)
    else:r=self.a-w-(not self.c);self.a=self.nz(r);self.c=r>=0
   elif op in (0xad,0xbd,0xb9,0x9d,0x8d,0x9e,0x9c,0xfd,0xd9,0xcd):
    nxt=pc+3;addr=(w+(self.x if op in(0xbd,0x9d,0x9e,0xfd)else self.y if op in(0xb9,0xd9)else 0))&65535;value=self.word(addr)
    if op in(0xad,0xbd,0xb9):self.a=self.nz(value)
    elif op in(0x8d,0x9d):self.put(addr,self.a)
    elif op in(0x9c,0x9e):self.put(addr,0)
    elif op in(0xcd,0xd9):self.nz(self.a-value);self.c=self.a>=value
    else:r=self.a-value-(not self.c);self.a=self.nz(r);self.c=r>=0
   elif op==0xbf:nxt=pc+4;self.a=self.nz(self.word((self.byte(pc+3)<<16|w)+self.x))
   elif op in (0xf0,0xd0,0x10,0x30,0xb0,0x80):
    take={0xf0:self.z,0xd0:not self.z,0x10:not self.n,0x30:self.n,0xb0:self.c,0x80:True}[op]
    nxt=pc+2+((arg if arg<128 else arg-256)if take else 0)
   elif op==0x4c:nxt=pc&0xff0000|w
   elif op in(0x20,0x22):
    target=(pc&0xff0000|w)if op==0x20 else(self.byte(pc+3)<<16|w)
    if target in stops:return target
    self.run(target,stops);nxt=pc+(3 if op==0x20 else 4)
   elif op in(0x60,0x6b):return pc
   elif op==0xaa:self.x=self.nz(self.a)
   elif op==0x38:self.c=True
   elif op==0x3a:self.a=self.nz(self.a-1)
   elif op==0x1a:self.a=self.nz(self.a+1)
   elif op==0x0a:self.c=bool(self.a&32768);self.a=self.nz(self.a<<1)
   else:raise AssertionError((hex(pc),hex(op)))
   pc=nxt
  raise AssertionError('bounded source diagnostic exceeded')

def main():
 p=argparse.ArgumentParser()
 for key in('dll','rom','output'):p.add_argument('--'+key,type=Path,required=True)
 a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);rom=a.rom.read_bytes()
 assert hashlib.sha256(rom).hexdigest()=='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
 dll=C.CDLL(str(a.dll.resolve()));assert C.sizeof(State)==dll.audit_state_size()
 tables=Tables();tables.thresholds[:]=rom[0x327a0:0x327a8];tables.pointers[:]=[int.from_bytes(rom[0x39c7b+2*i:0x39c7d+2*i],'little')for i in range(10)]
 counts={};pcs=set();routes={};r=random.Random(0xa6b3)
 def seed():
  s=State();s.v[:]=[(i*71+0xa531)&65535 for i in range(50)];s.v[6]=0;s.v[16]=15;s.v[32]=0;s.v[35]=3;s.v[38]=2;s.v[41]=2;s.v[44]=3;return s
 def test(mode,s):
  ref=Ref(rom,s);fn=getattr(dll,'nba_human_pass_release_'+mode)
  if mode=='step':
   end=ref.run(0x86a6b3,(0x86a629,0x86a6f8,0x85ad6b,0x8699c4,0x86a790))
   result={0x86a6ca:1,0x86a776:2,0x86a790:3,0x8699c4:4,0x86a78f:5,0x86a629:6,0x86a6f8:7,0x85ad6b:8}[end]
   assert result!=3,'attachment separately proved by natural pose scopes'
   fn.argtypes=[C.c_void_p,C.POINTER(Tables),C.POINTER(State)];fn.restype=C.c_int
   assert fn(None,C.byref(tables),C.byref(s))==result;routes[str(result)]=routes.get(str(result),0)+1
  else:
   ref.run({'turn':0x86a7a8,'after_launch':0x86a75f,'normalize':0x869846,'dispatch':0x879244}[mode],(0x879258,))
   fn.argtypes=[C.POINTER(State)];fn.restype=C.c_bool if mode=='dispatch'else None
   result=fn(C.byref(s))
   if mode=='dispatch':assert result
  assert list(s.v)==[ref.word(addr)for addr in ADDR],(mode,[(i,v,ref.word(addr))for i,(addr,v)in enumerate(zip(ADDR,s.v))if v!=ref.word(addr)])
  counts[mode]=counts.get(mode,0)+1;pcs.update(ref.pcs)
 for family in range(65536):
  s=seed();s.v[37]=family;test('after_launch',s)
 for family in range(65536):
  s=seed();s.v[37]=family;test('step',s)
 edges=[0,1,2,3,4,7,8,9,15,16,0x7fff,0x8000,0x8001,0x8004,0xfff8,0xffff]
 for facing,direction in itertools.product(edges,repeat=2):s=seed();s.v[14]=facing;s.v[35]=direction;test('turn',s)
 for timer,delta in itertools.product(edges,repeat=2):s=seed();s.v[41]=0xffff;s.v[33]=timer;s.v[39]=delta;test('step',s)
 for family,receiver in itertools.product(edges,(0x8000,0xffff)):
  s=seed();s.v[37]=family;s.v[44]=receiver;test('step',s)
 for family in edges:
  s=seed();s.v[37]=family;s.v[32]=0xffff;s.v[24]=0;test('step',s)
 for _ in range(100):
  s=seed();s.v[36]=r.randrange(65536);s.v[40]=s.v[36]if _&1 else r.randrange(65536);test('normalize',s)
 test('dispatch',seed())
 report=dict(passed=True,cases=counts,routes=routes,source_pcs=len(pcs),scope='original-ROM memory/boundary diagnostic; binary16 DP0; attachment and external children excluded; no cycles/natural reachability')
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(report)
if __name__=='__main__':main()
