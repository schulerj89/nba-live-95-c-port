"""Test-only bounded original-ROM execution of BC07's period-domain closure.
No production code imports this file. No cycle/timing prediction. Registers are
16-bit binary, DB/DP map WRAM. Calls/stacks are diagnostic host lists; compare
only declared typed gameplay/scratch fields, not CPU stack or register parity.
Unsupported children/record reads stop before executing them, as the C API does.
"""
import hashlib,struct
ROM_SHA='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'

def original(rom,initial,mapping):
 assert hashlib.sha256(rom).hexdigest()==ROM_SHA
 mem=bytearray(65536);owned=set()
 for _,addr,width in mapping:owned.update(range(addr,addr+width))
 def put(addr,value,width=2):
  assert 0<=addr<=65536-width
  mem[addr]=value&255
  if width==2:mem[addr+1]=(value>>8)&255
 def word(addr):
  # C0C2 reads one ignored high byte before its AND#00FF; all other
  # accessed data bytes must be explicitly supplied by the typed projection.
  assert addr in owned and ((addr+1)&65535 in owned or pc==0x85c0c2),(hex(pc),hex(addr),'unprojected data read')
  return mem[addr]|mem[(addr+1)&65535]<<8
 def code(addr):
  assert addr&65535>=0x8000,hex(addr)
  return rom[((addr>>16)&127)*32768+(addr&32767)]
 for (_,addr,width),value in zip(mapping,initial):put(addr,value,width)
 def projection():return [mem[o]if w==1 else word(o)for _,o,w in mapping]
 def valid_actor(value):return 0x34eb<=value<=0x3deb and (value-0x34eb)%256==0
 a=x=y=0;c=n=z=False;pc=0x86e1e5;stack=[];calls=[];rows=[];pcs=set();steps=0;writes=[]
 def nz(v):
  nonlocal n,z
  v&=65535;n=bool(v&32768);z=v==0;return v
 def store(addr,value):put(addr&65535,value);writes.append((pc,addr&65535,value&65535))
 for steps in range(50000):
  boundary=None
  if pc in (0x86e1ee,0x86e1f7):boundary=(1 if pc==0x86e1ee else 2,0)
  elif pc==0x85bf51 and not valid_actor(y):boundary=(3,y)
  elif pc==0x85bfab and (x&1 or x>18):boundary=(3,x)
  elif pc==0x85c05c and not valid_actor(x):boundary=(3,x)
  elif pc in (0x85bf5b,0x85bf98,0x85c0dd):boundary=(4,word(0x9a))
  if boundary:
   k,pointer=boundary;rows.append(dict(kind=k,pc=pc,completed_calls=len(rows)+(1 if k<3 else 0),record_pointer=pointer,words=projection()))
   if k!=1:break
  assert (0x85b95c<=pc<=0x85c0f5 or 0x85f347<=pc<=0x85f3ba or 0x80cee7<=pc<=0x80cefc or 0x86e1e5<=pc<=0x86e1f3),hex(pc)
  pcs.add(pc);op=code(pc);b=code(pc+1);v=b|code(pc+2)<<8;nxt=pc+1
  # All values and branch decisions below are read from original ROM opcodes.
  if op in (0xa9,0xa2,0xa0,0xc9,0xe0,0x29,0x49,0x69,0xe9,0x09):
   nxt=pc+3;operand=v;mode='immediate'
  elif op in (0xa5,0xa6,0xa4,0x85,0x86,0x84,0x64,0x05,0xc5,0x65,0xe5,0x06,0x46,0xc6,0xe6):
   nxt=pc+2;addr=b;operand=word(addr);mode='dp'
  elif op in (0xad,0xae,0xac,0x8d,0x8e,0x8c,0x9c,0xcd,0xce,0xed):
   nxt=pc+3;addr=v;operand=word(addr);mode='abs'
  elif op in (0xbd,0xbc,0x9d,0x9e,0xdd,0xfd):
   nxt=pc+3;addr=(v+x)&65535;operand=word(addr);mode='absx'
  elif op in (0xb9,0xbe,0x99,0xd9,0xf9):
   nxt=pc+3;addr=(v+y)&65535;operand=word(addr);mode='absy'
  elif op==0xb2:
   nxt=pc+2;addr=word(b);operand=word(addr);mode='indirect'
  elif op==0xbf:
   nxt=pc+4;addr=(code(pc+3)<<16|v)+x;operand=code(addr)|code(addr+1)<<8;mode='longx'
  else:mode=None
  if mode:
   if op in (0xa9,0xa5,0xad,0xbd,0xb9,0xb2,0xbf):a=nz(operand)
   elif op in (0xa2,0xa6,0xae,0xbe):x=nz(operand)
   elif op in (0xa0,0xa4,0xac,0xbc):y=nz(operand)
   elif op in (0x85,0x8d,0x9d,0x99):store(addr,a)
   elif op in (0x86,0x8e):store(addr,x)
   elif op in (0x84,0x8c):store(addr,y)
   elif op in (0x64,0x9c,0x9e):store(addr,0)
   elif op in (0xc9,0xc5,0xcd,0xdd,0xd9):nz(a-operand);c=a>=operand
   elif op==0xe0:nz(x-operand);c=x>=operand
   elif op==0x29:a=nz(a&operand)
   elif op==0x49:a=nz(a^operand)
   elif op in (0x05,0x09):a=nz(a|operand)
   elif op in (0x69,0x65):total=a+operand+c;a=nz(total);c=total>65535
   elif op in (0xe9,0xe5,0xfd,0xf9,0xed):total=a-operand-(not c);a=nz(total);c=total>=0
   elif op==0x06:c=bool(operand&32768);store(addr,nz(operand<<1))
   elif op==0x46:c=bool(operand&1);store(addr,nz(operand>>1))
   elif op in (0xc6,0xce):store(addr,nz(operand-1))
   elif op==0xe6:store(addr,nz(operand+1))
   else:raise AssertionError((hex(pc),hex(op),'mode'))
  elif op in (0x10,0x30,0x90,0xb0,0xd0,0xf0,0x80):
   take={0x10:not n,0x30:n,0x90:not c,0xb0:c,0xd0:not z,0xf0:z,0x80:True}[op];nxt=pc+2+((b-256 if b&128 else b)if take else 0)
  elif op==0x4c:nxt=pc&0xff0000|v
  elif op==0x22:calls.append(pc+4);nxt=code(pc+3)<<16|v
  elif op==0x6b:nxt=calls.pop()
  elif op==0xaa:x=nz(a)
  elif op==0xa8:y=nz(a)
  elif op==0x8a:a=nz(x)
  elif op==0x98:a=nz(y)
  elif op==0x1a:a=nz(a+1)
  elif op==0x3a:a=nz(a-1)
  elif op==0x0a:c=bool(a&32768);a=nz(a<<1)
  elif op==0x4a:c=bool(a&1);a=nz(a>>1)
  elif op==0x18:c=False
  elif op==0x38:c=True
  elif op==0x48:stack.append(a)
  elif op==0x68:a=nz(stack.pop())
  elif op==0xda:stack.append(x)
  elif op==0xfa:x=nz(stack.pop())
  else:raise AssertionError((hex(pc),hex(op),'unsupported bounded ROM instruction'))
  pc=nxt
 else:raise AssertionError('bounded ROM instruction limit')
 assert not stack
 return rows,pcs,steps+1,writes
