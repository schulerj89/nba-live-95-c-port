"""Independent fixed-source dataflow, not an instruction interpreter.

Read destinations/literals from the original known reset, actor-loop, clock,
and table-prefix blocks. No CPU state, opcode dispatch or timing simulation.
"""
import hashlib,struct
ROM_SHA='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
def block(rom,pc,n):return rom[0x30000+(pc&0x7fff):0x30000+(pc&0x7fff)+n]
def w(data,a):return struct.unpack_from('<H',data,a)[0]
def put(data,a,value):struct.pack_into('<H',data,a,value&65535)
def expect(test):
 if not test:raise ValueError('original fixed source contract')
def reference(rom,before):
 expect(hashlib.sha256(rom).hexdigest()==ROM_SHA and len(before)==131072)
 state=bytearray(before)
 # DCA6..DCCA and DD15..DD2A are exact consecutive absolute STZ blocks.
 for first,last in((0xdca6,0xdccd),(0xdd15,0xdd2d)):
  for pc in range(first,last,3):
   q=block(rom,pc,3);expect(q[0]==0x9c);put(state,w(q,1),0)
 for pc in(0xdccd,0xdcd3):
  q=block(rom,pc,6);expect(q[0]==0xa9 and q[3]==0x8d);put(state,w(q,4),w(q,1))
 q=block(rom,0xdcd9,3);expect(q[0]==0x9c);put(state,w(q,1),0)
 setup=block(rom,0xdcdc,9);expect(setup[0]==0xa2 and setup[3]==0xa0 and setup[6]==0xa9)
 base,count,zero=w(setup,1),w(setup,4),w(setup,7)
 stride=block(rom,0xdd0c,9);expect(stride==bytes.fromhex('8a18690001aa88d0cd'))
 for actor in range(count):
  p=base+actor*w(stride,3)
  for pc in(0xdce5,0xdce8,0xdceb):
   q=block(rom,pc,3);expect(q[0]==0x9d);put(state,p+w(q,1),zero)
  for pc in range(0xdcee,0xdd03,3):
   q=block(rom,pc,3);expect(q[0]==0x9e);put(state,p+w(q,1),0)
  q=block(rom,0xdd03,9);expect(q[0]==0xa9 and q[3]==q[6]==0x9d)
  put(state,p+w(q,4),w(q,1));put(state,p+w(q,7),w(q,1))
 reset=bytes(state)
 q=block(rom,0xdd2d,26)
 expect(q==bytes.fromhex('ad2609c90400900cadb1170aaabf92e3868d0c0aad0c0a8d2809'))
 period=w(state,w(q,1));quarter=w(state,w(q,9))
 if period>=w(q,4):
  if quarter>=4:return False,before,[] # bounded table address, not a native branch
  table=q[14]|q[15]<<8|q[16]<<16
  value=w(rom,((table>>16)&0x7f)*0x8000+(table&0x7fff)+2*quarter)
  put(state,w(q,18),value)
 put(state,w(q,24),w(state,w(q,21)));clock=bytes(state)
 for pc in(0xdd47,0xdd50):
  q=block(rom,pc,9 if pc==0xdd47 else 6);expect(q[0]==0xa9 and q[3]==0x8d)
  put(state,w(q,4),w(q,1))
  if pc==0xdd47:expect(q[6]==0x8d);put(state,w(q,7),w(q,1))
 q=block(rom,0xdd56,8);expect(q==bytes.fromhex('ad2609c90200d01a'))
 if w(state,w(q,1))==w(q,4):
  for pc in(0xdd5e,0xdd6b):
   q=block(rom,pc,13);expect(q[0]==0xa2 and q[3]==0xbd and q[6]==0x49 and q[9]==0x1a and q[10]==0x9d)
   address=w(q,1)+w(q,4);expect(w(q,4)==w(q,11));put(state,address,(w(state,address)^w(q,7))+1)
 q=block(rom,0xdd78,8);expect(q[0]==0xa2 and q[3]==0xbd and q[6]==0x85);put(state,q[7],w(state,w(q,1)+w(q,4)))
 q=block(rom,0xdd80,9);expect(q[0]==0xa9 and q[3]==q[6]==0x8d);put(state,w(q,4),w(q,1));put(state,w(q,7),w(q,1))
 q=block(rom,0xdd89,8);expect(q[0]==0xa9 and q[3]==0x85 and q[5]==0x9c);put(state,q[4],w(q,1));put(state,w(q,6),0)
 expect(block(rom,0xdd91,6)==bytes.fromhex('a0eb34a20000'))
 return True,bytes(state),[reset,clock,bytes(state)]
