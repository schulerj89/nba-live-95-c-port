"""Test-only bounded original instruction diagnostic, never linked to runtime.
Only init, twelve isolated depth blocks and FC80 are executed. Projection's
excluded screen/culling/indicator body is not interpreted or claimed here.
No CPU cycle, stack, NMI, OAM, or generic emulator contract.
"""
import hashlib,struct
ROM_SHA='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
def original(rom,operation,order,depth,xs,ys,camera):
 assert hashlib.sha256(rom).hexdigest()==ROM_SHA
 memory=bytearray(65536);writes=[];pcs=set();steps=0
 def put(p,v):struct.pack_into('<H',memory,p,v&65535)
 def word(p):return struct.unpack_from('<H',memory,p)[0]
 def code(p):return rom[((p>>16)&127)*32768+(p&32767)]
 for i in range(12):
  put(0x7e44+2*i,order[i]);put(0x3553+256*i,depth[i])
  put(0x34ef+256*i,xs[i]);put(0x34f3+256*i,ys[i])
 put(0x860,camera)
 a=x=y=0;c=n=z=False
 def nz(v):
  nonlocal n,z
  v&=65535;n=bool(v&32768);z=v==0;return v
 def execute(start,stop):
  nonlocal a,x,y,c,n,z,steps
  pc=start
  for _ in range(1000):
   if pc==stop:return
   assert 0x80fbe9<=pc<=0x80fbfc or 0x80fc80<=pc<=0x80fc9f or 0x87a3b6<=pc<=0x87a3ce,hex(pc)
   op=code(pc);b=code(pc+1);v=b|code(pc+2)<<8;nxt=pc+1;pcs.add(pc);steps+=1
   if op in (0xa9,0xa2,0xa0,0x69,0xc9):
    nxt=pc+3
    if op==0xa9:a=nz(v)
    elif op==0xa2:x=nz(v)
    elif op==0xa0:y=nz(v)
    elif op==0x69:t=a+v+c;a=nz(t);c=t>65535
    else:nz(a-v);c=a>=v
   elif op==0xa4:y=nz(word(b));nxt=pc+2
   elif op in (0xbc,0xb9,0xbd,0xbe,0xd9,0xfd,0xed,0x99,0x9d):
    nxt=pc+3;addr=(v+(y if op in(0xb9,0xbe,0xd9,0x99)else x if op in(0xbc,0xbd,0xfd,0x9d)else 0))&65535
    if op in(0x99,0x9d):put(addr,a);writes.append((pc,addr,a))
    else:
     operand=word(addr)
     if op==0xbc:y=nz(operand)
     elif op==0xbe:x=nz(operand)
     elif op in(0xbd,0xb9):a=nz(operand)
     elif op==0xd9:nz(a-operand);c=a>=operand
     else:t=a-operand-(not c);a=nz(t);c=t>=0
   elif op==0xc8:y=nz(y+1)
   elif op==0xca:x=nz(x-1)
   elif op==0x98:a=nz(y)
   elif op==0x18:c=False
   elif op==0x38:c=True
   elif op==0x6a:t=a;a=nz((a>>1)|(int(c)<<15));c=bool(t&1)
   elif op in(0x30,0x10,0xd0,0x80):
    take=n if op==0x30 else not n if op==0x10 else not z if op==0xd0 else True
    nxt=pc+2+((b-256 if b&128 else b)if take else 0)
   else:raise AssertionError((hex(pc),hex(op)))
   pc=nxt
  raise AssertionError('bounded draw reference overrun')
 if operation==0:execute(0x80fbe9,0x80fbfe)
 if operation in(1,3):
  for slot in range(11,-1,-1):put(0x96,slot*2);execute(0x87a3b6,0x87a3d1)
 if operation in(2,3):execute(0x80fc80,0x80fca1)
 return dict(order=[word(0x7e44+2*i)for i in range(12)],depth=[word(0x3553+256*i)for i in range(12)],steps=steps,pcs=sorted(pcs),writes=writes)
