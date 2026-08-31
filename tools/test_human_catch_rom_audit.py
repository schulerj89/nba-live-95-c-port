"""Bounded original-ROM diagnostic only; not a production CPU or timing model."""
import argparse,ctypes,hashlib,json,random
from pathlib import Path
U=ctypes.c_uint16
class Actor(ctypes.Structure):
 _fields_=[(k,U)for k in ('x','y','team','order_cursor','mode','timer','flags','pass_band','axis_88')]
class State(ctypes.Structure):
 _fields_=[('actors',Actor*11),('order',U*13)]+[(k,U)for k in ('source_slot','receiver_slot','basket_x','indirect_word_42','profile_word_39','rng_07f6','attempt_0904','aa','ae','ac','b2','b6','ba','be','candidate_92')]
class RomLeaf:
 def __init__(self,rom,s):
  self.rom=rom;self.mem=bytearray(65536);self.a=self.x=0;self.y=0x34eb+s.receiver_slot*256;self.c=self.n=self.z=0;self.stack=[];self.pcs=set()
  self.put(0,0x500);self.put(2,0);self.put(0x542,s.indirect_word_42);self.put(0xe0,0x600);self.put(0xe2,0);self.put(0x639,s.profile_word_39)
  self.put(0x96,0x34eb+s.source_slot*256);self.put(0x8e,0x34eb+s.receiver_slot*256);self.put(0x9e,0x46eb);self.put(0x46f5,s.basket_x)
  for k,addr in [('rng_07f6',0x7f6),('attempt_0904',0x904),('candidate_92',0x92)]+[(k,int(k,16))for k in ('aa','ae','ac','b2','b6','ba','be')]:self.put(addr,getattr(s,k))
  for i,a in enumerate(s.actors):
   for k,off in [('x',4),('y',8),('team',0x6e),('mode',0x5e),('timer',0x60),('flags',0x7e),('pass_band',0x62),('axis_88',0x88)]:self.put(0x34eb+i*256+off,getattr(a,k))
   self.put(0x34eb+i*256+0x14,0x34d1+2*a.order_cursor)
  for i,slot in enumerate(s.order):self.put(0x34d1+2*i,0 if slot==65535 else 0x34eb+256*slot)
 def byte(self,addr):
  bank=addr>>16;lo=addr&65535
  if not bank:return self.mem[lo]
  assert lo>=0x8000,hex(addr)
  return self.rom[(bank&127)*32768+(lo&32767)]
 def word(self,a):return self.byte(a)|(self.byte((a+1)&0xffffff)<<8)
 def put(self,a,v):self.mem[a&65535]=v&255;self.mem[(a+1)&65535]=(v>>8)&255
 def nz(self,v):v&=65535;self.n=bool(v&32768);self.z=v==0;return v
 def run(self,pc,stop=None):
  for _ in range(5000):
   if pc==stop or isinstance(stop,tuple)and pc in stop:return pc
   self.pcs.add(pc);op=self.byte(pc);arg=self.byte(pc+1);word=self.word(pc+1);nxt=pc+1
   if op in (0xa5,0xa6,0xa4,0x85,0x84,0x86,0x64,0x05,0x45,0xc5,0x65,0xe5,0x06,0x46,0xd4,0xb2,0xb7):
    nxt=pc+2;v=self.word(arg)
    if op==0xa5:self.a=self.nz(v)
    elif op==0xa6:self.x=self.nz(v)
    elif op==0xa4:self.y=self.nz(v)
    elif op==0x85:self.put(arg,self.a)
    elif op==0x84:self.put(arg,self.y)
    elif op==0x86:self.put(arg,self.x)
    elif op==0x64:self.put(arg,0)
    elif op==0x05:self.a=self.nz(self.a|v)
    elif op==0x45:self.a=self.nz(self.a^v)
    elif op==0xc5:self.nz(self.a-v);self.c=self.a>=v
    elif op==0x65:t=self.a+v+self.c;self.a=self.nz(t);self.c=t>65535
    elif op==0xe5:t=self.a-v-(not self.c);self.a=self.nz(t);self.c=t>=0
    elif op==0x06:self.c=bool(v&32768);self.put(arg,self.nz(v<<1))
    elif op==0x46:self.c=v&1;self.put(arg,self.nz(v>>1))
    elif op==0xd4:self.stack.append(v)
    elif op==0xb2:self.a=self.nz(self.word(v))
    elif op==0xb7:self.a=self.nz(self.word((v+(self.byte(arg+2)<<16)+self.y)&0xffffff))
   elif op in (0xa9,0xa2,0xa0,0x49,0x29,0x09,0xc9,0x69,0xe9):
    nxt=pc+3
    if op==0xa9:self.a=self.nz(word)
    elif op==0xa2:self.x=self.nz(word)
    elif op==0xa0:self.y=self.nz(word)
    elif op==0x49:self.a=self.nz(self.a^word)
    elif op==0x29:self.a=self.nz(self.a&word)
    elif op==0x09:self.a=self.nz(self.a|word)
    elif op==0xc9:self.nz(self.a-word);self.c=self.a>=word
    elif op==0x69:t=self.a+word+self.c;self.a=self.nz(t);self.c=t>65535
    elif op==0xe9:t=self.a-word-(not self.c);self.a=self.nz(t);self.c=t>=0
   elif op in (0xbd,0xb9,0xad,0x99,0x8d,0x9c,0xd9,0xdd):
    nxt=pc+3;addr=(word+(self.x if op in(0xbd,0xdd)else self.y if op in(0xb9,0x99,0xd9)else 0))&65535;v=self.word(addr)
    if op in(0xbd,0xb9,0xad):self.a=self.nz(v)
    elif op in(0x99,0x8d):self.put(addr,self.a)
    elif op==0x9c:self.put(addr,0)
    else:self.nz(self.a-v);self.c=self.a>=v
   elif op==0xf9:
    nxt=pc+3;v=self.word((word+self.y)&65535);t=self.a-v-(not self.c);self.a=self.nz(t);self.c=t>=0
   elif op==0xbf:
    nxt=pc+4;addr=(word+(self.byte(pc+3)<<16)+self.x)&0xffffff;self.a=self.nz(self.word(addr))
   elif op in(0x30,0x10,0xf0,0xd0,0x90,0xb0,0x80):
    take={0x30:self.n,0x10:not self.n,0xf0:self.z,0xd0:not self.z,0x90:not self.c,0xb0:self.c,0x80:True}[op];nxt=pc+2+((arg if arg<128 else arg-256)if take else 0)
   elif op==0x4c:nxt=(pc&0xff0000)|word
   elif op==0x22:
    target=word|(self.byte(pc+3)<<16)
    if target==stop or isinstance(stop,tuple)and target in stop:return target
    self.run(target);nxt=pc+4
   elif op==0x6b:return pc
   elif op==0xda:self.stack.append(self.x)
   elif op==0xfa:self.x=self.nz(self.stack.pop())
   elif op==0x48:self.stack.append(self.a)
   elif op==0x68:self.a=self.nz(self.stack.pop())
   elif op==0xaa:self.x=self.nz(self.a)
   elif op==0x1a:self.a=self.nz(self.a+1)
   elif op==0x3a:self.a=self.nz(self.a-1)
   elif op==0x0a:self.c=bool(self.a&32768);self.a=self.nz(self.a<<1)
   elif op==0x4a:self.c=self.a&1;self.a=self.nz(self.a>>1)
   elif op==0x18:self.c=0
   elif op==0x38:self.c=1
   else:raise AssertionError(('unsupported diagnostic opcode',hex(pc),hex(op)))
   pc=nxt
  raise AssertionError('bounded original source did not return')
 def compare(self,s):
  for k,addr in [('rng_07f6',0x7f6),('attempt_0904',0x904),('candidate_92',0x92)]+[(k,int(k,16))for k in('aa','ae','ac','b2','b6','ba','be')]:assert getattr(s,k)==self.word(addr),(k,getattr(s,k),self.word(addr))
  assert self.word(0x96)==0x34eb+s.source_slot*256 and self.word(0x8e)==0x34eb+s.receiver_slot*256
  for i,a in enumerate(s.actors):
   for k,off in [('x',4),('y',8),('team',0x6e),('mode',0x5e),('timer',0x60),('flags',0x7e),('pass_band',0x62),('axis_88',0x88)]:assert getattr(a,k)==self.word(0x34eb+i*256+off),(i,k)
def main():
 p=argparse.ArgumentParser();p.add_argument('--dll',type=Path,required=True);p.add_argument('--rom',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 rom=a.rom.read_bytes();assert hashlib.sha256(rom).hexdigest()=='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
 lib=ctypes.CDLL(str(a.dll.resolve()));counts={};routes={};pcs=set();edges=[0,1,2,3,7,8,31,127,255,0x1fff,0x2000,0x3fff,0x4000,0x4001,0x7ffe,0x7fff,0x8000,0x8001,0xbfff,0xc000,0xc001,0xff00,0xfffe,0xffff]
 def seed():
  s=State();s.source_slot=0;s.receiver_slot=1;s.basket_x=336;s.indirect_word_42=0x7e50;s.profile_word_39=91;s.rng_07f6=0xdead;s.attempt_0904=0x1234
  for k in ('aa','ae','ac','b2','b6','ba','be','candidate_92'):setattr(s,k,0x5678)
  for i,actor in enumerate(s.actors):actor.x=(i*67-335)&65535;actor.y=(i*43-200)&65535;actor.team=i%2;actor.order_cursor=i+1;actor.timer=0x4321;actor.flags=0xa5a5
  s.order[:]=[65535,*range(11),65535];return s
 def case(mode,s,pc,stop=None):
  ref=RomLeaf(rom,s)
  if mode=='attempt':ref.a=s.ae
  end=ref.run(pc,stop);flow=mode in('receiver','attempt','prepare');fn=getattr(lib,'nba_human_pass_catch_'+mode);fn.argtypes=[ctypes.POINTER(State)];fn.restype=ctypes.c_int if flow else ctypes.c_bool
  want={0x86ae10:0,0x86af66:1,0x86b468:2}[end]if flow else True
  assert fn(ctypes.byref(s))==want,(mode,hex(end));ref.compare(s);counts[mode]=counts.get(mode,0)+1;pcs.update(ref.pcs)
  if flow:routes[mode+':'+hex(end)]=routes.get(mode+':'+hex(end),0)+1
 for x in edges:
  for y in edges:
   s=seed();s.aa=x;s.ae=y;case('direction',s,0x85f02d)
   s=seed();s.actors[1].x=x;s.actors[1].y=y;s.basket_x=0x8000;case('geometry',s,0x86ad3d,0x86ad98)
 for seed_value in range(65536):
  s=seed();s.rng_07f6=seed_value;case('rng',s,0x80cee7)
 for band in range(0,31,6):
  s=seed();s.actors[0].pass_band=band;case('receiver',s,0x86af66,0x86b468)
 r=random.Random(9530)
 for i in range(2000):
  s=seed();s.source_slot=r.randrange(10);s.basket_x=r.choice(edges);s.actors[s.source_slot].order_cursor=r.randrange(1,12)
  for actor in s.actors:actor.x=r.choice(edges);actor.y=r.choice(edges);actor.team=r.randrange(3)
  order=list(range(11));r.shuffle(order);s.order[:]=[65535,*order,65535]
  case('lane',s,0x85f5e4)
 for i in range(1000):
  s=seed();s.aa=r.choice(edges);s.ae=r.choice(edges);s.ba=r.choice(edges);s.b6=r.choice(edges);s.rng_07f6=r.randrange(65536);s.profile_word_39=r.choice([0,75,76,83,84,91,92,255]);s.actors[0].axis_88=r.choice([0,1,2,7,8,9,65535])
  case('attempt',s,0x86ad98,(0x86ae10,0x86af66))
  s=seed();s.indirect_word_42=r.randrange(65536);s.actors[1].x=r.randrange(65536);s.actors[1].y=r.randrange(65536);s.actors[0].pass_band=r.choice(range(0,31,6));s.rng_07f6=r.randrange(65536);s.profile_word_39=r.choice([0,75,76,83,84,91,92,255]);s.actors[0].axis_88=r.choice([0,1,2,7,8,9,65535])
  case('prepare',s,0x86ad3d,(0x86ae10,0x86b468))
 for band in range(0,31,6):
  s=seed();s.actors[1].x=250;s.actors[1].y=30;s.profile_word_39=255;s.actors[0].pass_band=band
  for actor in s.actors:actor.team=0
  case('prepare',s,0x86ad3d,(0x86ae10,0x86b468));assert s.receiver_slot==0 and s.source_slot==1 and s.actors[1].timer==[56,61,66,76,86,0x8eca][band//6]
 report=dict(passed=True,cases=counts,routes=routes,rom_pc_count=len(pcs),pcs=[f'{p:06x}'for p in sorted(pcs)],scope='D0/M0/X0/DP0 diagnostic ROM instruction execution and C API guards; deliberately controlled words/order, no natural reachability or CPU timing claim',dll_sha256=hashlib.sha256(a.dll.read_bytes()).hexdigest())
 assert not a.output.exists();a.output.write_text(json.dumps(report,indent=2));print(counts)
if __name__=='__main__':main()
