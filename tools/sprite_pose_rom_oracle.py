"""Test-only original-opcode AD92/AF1E projection; B348/B0FF are boundaries.

Not a production interpreter. No queue or child execution claim: external
calls preserve the explicitly projected parent words in this diagnostic.
"""
import struct

def offset(pc):return ((pc>>16)&127)*32768+(pc&32767)
def read_rom(rom,pc,n=1):return rom[offset(pc):offset(pc)+n]
def word(data,p):return struct.unpack_from('<H',data,p)[0]
def oracle(rom,inputs,subject=False,raw=None):
 mem=bytearray(65536)if raw is None else bytearray(raw[:65536])
 def put(p,v):struct.pack_into('<H',mem,p,v&65535)
 for p,v in zip([0xd6,0xd4,0xda,0xd8,0x47,0x51,0xc0,0x4f,0x884],inputs):put(p,v)
 a=inputs[7];x=inputs[9];y=inputs[10];pc=0xaf1e if subject else 0xad92
 c=n=z=False;calls=[];visited=set();balls=[]
 kinds={0xae65:0,0xaeda:0,0xae94:1,0xaf14:1,0xaeac:2,0xaef0:2,0xaec1:3,
        0xb00a:0,0xb093:0,0xb039:1,0xb0e1:1,0xb051:2,0xb0bd:2,0xb07a:3}
 def nz(v):return bool(v&32768),v==0
 for _ in range(600):
  at=pc;visited.add(at);b=read_rom(rom,0x800000|pc,4);op=b[0];pc+=1
  if op in(0x85,0x64,0xa5,0xc6,0x86,0x84,0xa4,0x65,0xa6):
   p=b[1];pc+=1
   if op==0x85:put(p,a)
   elif op==0x64:put(p,0)
   elif op==0xa5:a=word(mem,p);n,z=nz(a)
   elif op==0xc6:v=(word(mem,p)-1)&65535;put(p,v);n,z=nz(v)
   elif op==0x86:put(p,x)
   elif op==0x84:put(p,y)
   elif op==0xa4:y=word(mem,p);n,z=nz(y)
   elif op==0xa6:x=word(mem,p);n,z=nz(x)
   else:v=a+word(mem,p)+c;c=v>65535;a=v&65535;n,z=nz(a)
  elif op in(0x49,0x89,0x29,0x09,0xc9):
   v=b[1]|b[2]<<8;pc+=2
   if op==0x49:a^=v;n,z=nz(a)
   elif op==0x89:z=(a&v)==0
   elif op==0x29:a&=v;n,z=nz(a)
   elif op==0x09:a|=v;n,z=nz(a)
   else:c=a>=v;n,z=nz((a-v)&65535)
  elif op in(0x10,0x30,0xf0,0xd0,0x90,0xb0,0x80):
   pc+=1;take={0x10:not n,0x30:n,0xf0:z,0xd0:not z,0x90:not c,0xb0:c,0x80:True}[op]
   if take:pc+=(b[1]^128)-128
  elif op==0xaa:x=a;n,z=nz(x)
  elif op==0xbf:
   p=(b[1]|b[2]<<8|b[3]<<16)+x;pc+=3;a=int.from_bytes(read_rom(rom,p,2),'little');n,z=nz(a)
  elif op in(0x1a,0x3a):a=(a+(1 if op==0x1a else -1))&65535;n,z=nz(a)
  elif op==0x18:c=False
  elif op==0x4c:pc=b[1]|b[2]<<8
  elif op in(0xee,0x9c,0xad):
   p=b[1]|b[2]<<8;pc+=2
   if op==0xee:v=(word(mem,p)+1)&65535;put(p,v);n,z=nz(v)
   elif op==0x9c:put(p,0)
   else:a=word(mem,p);n,z=nz(a)
  elif op==0x22:
   assert b[1:]==bytes.fromhex('48b380');pc+=3
   calls.append([kinds[at],a,word(mem,0x14),word(mem,0x884),x,y])
  elif op==0x20:
   assert subject and b[1:3]==bytes.fromhex('ffb0');pc+=2;balls.append(at)
  elif op==0x6b:
   geom=[word(mem,p)for p in(0xaa,0xac,0x49,0x884,0xb2,0xb4,0xb6,0xb8,0xdc,0xde)]
   return [1,len(calls),*geom,*sum(calls,[]),*([0]*((4-len(calls))*6))],visited,balls
  else:raise AssertionError((hex(at),hex(op)))
 raise AssertionError('instruction bound')
