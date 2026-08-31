"""Fixed original D5DB instruction diagnostic, binary16 DP0; no timing model."""
import argparse,ctypes as C,hashlib,json,random,struct
from pathlib import Path
class Sort(C.Structure):_fields_=[('x',C.c_int16*11),('link',C.c_uint16*11),('object',C.c_uint16*12)]
def original(rom,raw):
 mem=bytearray(raw);a=x=y=0;c=n=z=False;stack=[];pc=0x86d5db;pcs=set()
 def word(addr):return struct.unpack_from('<H',mem,addr)[0]
 def put(addr,v):struct.pack_into('<H',mem,addr,v&65535)
 def nz(v):
  nonlocal n,z
  v&=65535;n=bool(v&32768);z=v==0;return v
 for _ in range(10000):
  pcs.add(pc);off=0x30000+(pc&32767);op=rom[off];arg=rom[off+1];w=int.from_bytes(rom[off+1:off+3],'little');nxt=pc+1
  if op in(0xa5,0xa6,0xa4,0x85,0xc5,0xe6,0xc6,0xd4):
   nxt=pc+2;v=word(arg)
   if op==0xa5:a=nz(v)
   elif op==0xa6:x=nz(v)
   elif op==0xa4:y=nz(v)
   elif op==0x85:put(arg,a)
   elif op==0xc5:nz(a-v);c=a>=v
   elif op==0xd4:stack.append(v)
   else:put(arg,nz(v+(1 if op==0xe6 else -1)))
  elif op in(0xb2,0x92):
   nxt=pc+2;address=word(arg)
   if op==0xb2:a=nz(word(address))
   else:put(address,a)
  elif op in(0xa9,0x69):
   nxt=pc+3
   if op==0xa9:a=nz(w)
   else:v=a+w+c;a=nz(v);c=v>65535
  elif op in(0xbd,0xb9,0x9d,0x99):
   nxt=pc+3;address=(w+(x if op in(0xbd,0x9d)else y))&65535
   if op in(0xbd,0xb9):a=nz(word(address))
   else:put(address,a)
  elif op in(0xf0,0x10,0x30,0x80):
   take={0xf0:z,0x10:not n,0x30:n,0x80:True}[op];nxt=pc+2+((arg-256 if arg&128 else arg)if take else 0)
  elif op==0xaa:x=nz(a)
  elif op==0x18:c=False
  elif op==0x68:a=nz(stack.pop())
  elif op==0x6b:assert not stack;return mem,pcs
  else:raise AssertionError((hex(pc),hex(op)))
  pc=nxt
 raise AssertionError('bounded source exceeded')
def main():
 p=argparse.ArgumentParser()
 for k in('rom','dll','replay','output'):p.add_argument('--'+k,type=Path,required=True)
 a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);rom=a.rom.read_bytes();assert hashlib.sha256(rom).hexdigest()=='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
 dll=C.CDLL(str(a.dll.resolve()));fn=dll.nba_period_object_sort;fn.argtypes=[C.POINTER(Sort)];fn.restype=C.c_bool;cases=json.loads(a.replay.read_text())['cases'];r=random.Random(0xd5db);pcs=set();native=controlled=0
 def compare(raw):
  s=Sort();s.x[:]=[struct.unpack_from('<h',raw,0x34ef+i*256)[0]for i in range(11)];s.link[:]=[struct.unpack_from('<H',raw,0x34ff+i*256)[0]for i in range(11)];s.object[:]=struct.unpack_from('<12H',raw,0x34d3);assert fn(C.byref(s));expected,p=original(rom,raw);pcs.update(p)
  differences=[dict(address=hex(0x34d3+2*i),actual=v,expected=struct.unpack_from('<H',expected,0x34d3+2*i)[0])for i,v in enumerate(s.object)if v!=struct.unpack_from('<H',expected,0x34d3+2*i)[0]]
  differences +=[dict(address=hex(0x34ff+256*i),actual=v,expected=struct.unpack_from('<H',expected,0x34ff+256*i)[0])for i,v in enumerate(s.link)if v!=struct.unpack_from('<H',expected,0x34ff+256*i)[0]]
  return differences,expected
 for case in cases:
  if case['mode']!='sort':continue
  raw=Path(case['command'][2]).read_bytes();diff,expected=compare(raw);assert not diff;native+=1
 seed=Path(next(c for c in cases if c['mode']=='sort')['command'][2]).read_bytes()
 for i in range(512):
  raw=bytearray(seed);order=list(range(11));r.shuffle(order);struct.pack_into('<H',raw,0x34d1,0)
  for j,actor in enumerate(order):struct.pack_into('<H',raw,0x34d3+2*j,0x34eb+256*actor)
  for j in range(11):struct.pack_into('<H',raw,0x34ef+256*j,r.choice([0,1,0x7fff,0x8000,0x8001,0xffff])if i<256 else r.randrange(65536));struct.pack_into('<H',raw,0x34ff+256*j,0xa500+j)
  diff,_=compare(raw);assert not diff,(i,diff);controlled+=1
 raw=bytearray(seed)
 for i in range(11):struct.pack_into('<H',raw,0x34d3+2*i,0x34eb+256*i);struct.pack_into('<H',raw,0x34ef+256*i,0 if i==1 else i+1)
 struct.pack_into('<H',raw,0x34cf,0);struct.pack_into('<H',raw,0x34d1,0x4000);struct.pack_into('<H',raw,0x4004,100)
 diff,expected=compare(raw);assert diff
 (a.output/'nonzero-sentinel.input').write_bytes(raw);(a.output/'nonzero-sentinel.rom').write_bytes(expected)
 report=dict(passed=True,native_calls=native,controlled_zero_sentinel=controlled,source_pcs=len(pcs),unsupported_nonzero_sentinel_difference=diff,scope='original ROM memory diagnostic; sort requires zero preceding sentinel; no timing or natural nonzero-sentinel claim')
 (a.output/'report.json').write_text(json.dumps(report,indent=2));print(report)
if __name__=='__main__':main()
