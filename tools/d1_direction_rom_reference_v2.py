"""Test-only bounded ROM direction executor. Not runtime code or timing proof.

Original A52C..A5FA plus its sole F02D child; 16-bit binary arithmetic and
legal movement directions 0..7. No expected values are imported from C.
"""
import hashlib,json
from pathlib import Path

ROM=None
def set_rom(path):
 global ROM
 ROM=Path(path).read_bytes()
 assert hashlib.sha256(ROM).hexdigest()=='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
PCS=set()
def byte(pc):return ROM[((pc>>16)&127)*32768+(pc&32767)]
def word(pc):return byte(pc)|(byte(pc+1)<<8)

def execute(pc,mem,x=0):
 a=y=0;c=n=z=False;stack=[];returns=[]
 def read(addr):
  addr&=65535
  return mem.get(addr,0)|mem.get((addr+1)&65535,0)<<8
 def write(addr,value):
  mem[addr&65535]=value&255;mem[(addr+1)&65535]=(value>>8)&255
 def flags(v):return bool(v&32768),v==0
 for steps in range(300):
  if pc==0x87a5fb:return y,steps
  assert 0x87a52c<=pc<=0x87a5fa or 0x85f02d<=pc<=0x85f099,hex(pc)
  PCS.add(pc);op=byte(pc);p=pc;pc+=1
  if op in [0xbd,0xb9,0xad,0xc9,0x89,0x29,0x09,0x49,0xfd,0xec]:
   v=word(pc);pc+=2
   if op==0xbd:a=read(v+x);n,z=flags(a)
   elif op==0xb9:a=read(v+y);n,z=flags(a)
   elif op==0xad:a=read(v);n,z=flags(a)
   elif op in [0xc9,0xec]:
    left=a if op==0xc9 else x;operand=v if op==0xc9 else read(v)
    result=(left-operand)&65535;n,z=flags(result);c=left>=operand
   elif op==0x89:z=(a&v)==0
   elif op==0x29:a&=v;n,z=flags(a)
   elif op==0x09:a|=v;n,z=flags(a)
   elif op==0x49:a^=v;n,z=flags(a)
   else:
    result=a-read(v+x)-int(not c);a=result&65535;c=result>=0;n,z=flags(a)
  elif op==0xbf:
   address=byte(pc)|(byte(pc+1)<<8)|(byte(pc+2)<<16);pc+=3
   a=word(address+x);n,z=flags(a)
  elif op in [0xa5,0x05,0x85,0x64,0xc5,0x06]:
   dp=byte(pc);pc+=1;v=read(dp)
   if op==0xa5:a=v;n,z=flags(a)
   elif op==0x05:a|=v;n,z=flags(a)
   elif op==0x85:write(dp,a)
   elif op==0x64:write(dp,0)
   elif op==0xc5:n,z=flags((a-v)&65535);c=a>=v
   else:c=bool(v&32768);v=(v<<1)&65535;write(dp,v);n,z=flags(v)
  elif op==0xa6:x=read(byte(pc));pc+=1;n,z=flags(x)
  elif op in [0xf0,0xd0,0x10,0x30,0xb0,0x90,0x80]:
   delta=byte(pc);pc+=1
   take={0xf0:z,0xd0:not z,0x10:not n,0x30:n,0xb0:c,0x90:not c,0x80:True}[op]
   if take:pc+=delta-256 if delta&128 else delta
  elif op==0xa9:a=word(pc);pc+=2;n,z=flags(a)
  elif op==0x4c:pc=(pc&0xff0000)|word(pc)
  elif op==0x22:
   target=byte(pc)|(byte(pc+1)<<8)|(byte(pc+2)<<16);assert target==0x85f02d
   returns.append(pc+3);pc=target
  elif op==0x6b:
   if not returns:return read(0xaa),steps
   pc=returns.pop()
  elif op in [0xda,0x48]:stack.append(x if op==0xda else a)
  elif op==0xfa:x=stack.pop();n,z=flags(x)
  elif op==0x68:a=stack.pop();n,z=flags(a)
  elif op==0x38:c=True
  elif op==0x3a:a=(a-1)&65535;n,z=flags(a)
  elif op==0x1a:a=(a+1)&65535;n,z=flags(a)
  elif op==0x4a:c=bool(a&1);a>>=1;n,z=flags(a)
  elif op==0x0a:c=bool(a&32768);a=(a<<1)&65535;n,z=flags(a)
  elif op==0xaa:x=a;n,z=flags(x)
  elif op==0xa8:y=a;n,z=flags(y)
  elif op==0x98:a=y;n,z=flags(a)
  else:raise AssertionError((hex(p),hex(op)))
 raise AssertionError('bound exceeded')

def put(mem,addr,value):mem[addr]=value&255;mem[(addr+1)&65535]=(value>>8)&255
def facing(dx,dy):
 m={};put(m,0xaa,dx);put(m,0xae,dy);return execute(0x85f02d,m)[0]

def caller(*,actor,current,mode,status,upper,anchor,receiver,camera,ax,ay,tx,ty,bx,by):
 assert 0<=actor<10 and 0<=current<8
 receiver&=65535
 assert receiver>=32768 or receiver<10
 ptr=0x34eb+actor*256;m={}
 for off,val in [(4,ax),(8,ay),(0x52,current),(0x5e,mode),(0x28,status),(0x30,upper),(0x88,anchor)]:put(m,ptr+off,val)
 if receiver<10:
  target=word(0x879c7b+receiver*2)
  # Actual same-actor aliases cannot have independent target coordinates.
  if target==ptr:assert (tx&65535,ty&65535)==(ax&65535,ay&65535)
  else:put(m,target+4,tx);put(m,target+8,ty)
 put(m,0x0946,receiver);put(m,0x0940,camera);put(m,0x3eef,bx);put(m,0x3ef3,by)
 return execute(0x87a52c,m,ptr)[0]

if __name__=='__main__':
 import argparse
 p=argparse.ArgumentParser();p.add_argument('--rom',type=Path,required=True);a=p.parse_args();set_rom(a.rom)
 cases=[(0,1),(1,2),(-1,2),(32767,-32768),(-32768,1),(0,0)]
 out={'scope':'source-only direction result; binary16-bit; no timing/register/native reachability',
      'facing':[{'dx':x,'dy':y,'direction':facing(x,y)}for x,y in cases]}
 base=dict(actor=0,current=0,mode=10,status=0,upper=0,anchor=0,receiver=1,camera=0,ax=0,ay=0,tx=0,ty=1,bx=0,by=1)
 out['mode10_counterexample']=caller(**base)
 out['pcs']=[hex(x)for x in sorted(PCS)]
 print(json.dumps(out,indent=2))
