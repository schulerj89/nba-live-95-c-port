"""Independent bounded original instruction proof of DCA6-DD97 data ownership.

No child execution or timing. Binary16, DP0 and effective WRAM absolute bank.
The state includes V/C/N/Z so native boundary register facts are also checked,
separately from the C module's data-only contract.
"""
import argparse,hashlib,json,random,struct,subprocess,sys
from pathlib import Path
def reference(rom,raw,entry_ps=0):
 mem=bytearray(raw);a=x=y=0;c=bool(entry_ps&1);z=bool(entry_ps&2);n=bool(entry_ps&128);vflag=bool(entry_ps&64);p=0xdca6;pcs=set();stages=[]
 def word(addr):return struct.unpack_from('<H',mem,addr)[0]
 def put(addr,value):struct.pack_into('<H',mem,addr,value&65535)
 def nz(value):
  nonlocal n,z
  value&=65535;n=bool(value&32768);z=value==0;return value
 for _ in range(1000):
  if p in(0xdd2d,0xdd47,0xdd97):
   stages.append((bytes(mem),dict(a=a,x=x,y=y,ps=(entry_ps&0x3c)|int(c)|(2 if z else 0)|(128 if n else 0)|(64 if vflag else 0))))
   if p==0xdd97:return stages,pcs
  pcs.add(p);offset=0x30000+(p&32767);op=rom[offset];b=rom[offset+1];w=int.from_bytes(rom[offset+1:offset+3],'little');nxt=p+1
  if op in(0xa9,0xa2,0xa0,0xc9,0x69,0x49):value=w;addr=None;nxt=p+3
  elif op in(0x9c,0x8d,0xad,0x9d,0x9e,0xbd):
   addr=(w+(x if op in(0x9d,0x9e,0xbd)else 0))&65535;value=word(addr);nxt=p+3
  elif op==0x85:addr=b;value=word(b);nxt=p+2
  elif op==0xbf:
   addr=((rom[offset+3]&127)*32768+(w&32767))+x;value=int.from_bytes(rom[addr:addr+2],'little');nxt=p+4
  else:addr=None;value=0
  if op in(0xa9,0xad,0xbd,0xbf):a=nz(value)
  elif op==0xa2:x=nz(value)
  elif op==0xa0:y=nz(value)
  elif op in(0x9c,0x9e):put(addr,0)
  elif op in(0x8d,0x9d,0x85):put(addr,a)
  elif op==0xc9:nz(a-value);c=a>=value
  elif op==0x69:
   old=a;total=old+value+c;a=nz(total);c=total>65535;vflag=bool((~(old^value)&(old^a))&32768)
  elif op==0x49:a=nz(a^value)
  elif op==0x18:c=False
  elif op==0x8a:a=nz(x)
  elif op==0xaa:x=nz(a)
  elif op==0x88:y=nz(y-1)
  elif op==0x1a:a=nz(a+1)
  elif op==0x0a:c=bool(a&32768);a=nz(a<<1)
  elif op in(0x90,0xd0):take=not c if op==0x90 else not z;nxt=p+2+((b-256 if b&128 else b)if take else 0)
  else:raise AssertionError((hex(p),hex(op)))
  p=nxt
 raise AssertionError('bounded prefix exceeded')
def main():
 p=argparse.ArgumentParser()
 for k in('source','rom','exe','output'):p.add_argument('--'+k,type=Path,required=True)
 a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);sys.path.insert(0,str(a.source.resolve()/'tools'));import verify_period_entry_prefix as v
 rom=a.rom.read_bytes();assert hashlib.sha256(rom).hexdigest()==v.ROM_SHA;v.build(a.exe.resolve());pcs=set();native=0;controlled=0;regs=0;r=random.Random(0xdca60831)
 for period in range(4):
  cap=v.OWNER/'build/period-restart-attribution-v1'/f'period-{period}-ready1-children-v{3 if period==3 else 2}';_,rows=v.attest(cap,a.rom.resolve());expected,seen=reference(rom,rows[0]['memory'],rows[0]['ps']);pcs.update(seen)
  for (raw,registers),row in zip(expected,rows[1:]):
   assert raw==row['memory'];assert all(row[k]==value for k,value in registers.items()),(period,registers,row);native+=65536;regs+=4
 for case in range(72):
  raw=bytearray(r.randbytes(131072));period=[0,1,2,3,4,5,0x7fff,0x8000,0xffff][case%9];quarter=(case//9)%4 if period>=4 else r.randrange(65536)
  for addr,value in [(0x926,period),(0x17b1,quarter),(0x46f5,[0,0x8000,0xffff,1][case%4]),(0x4775,r.randrange(65536)),(0x9ba,0xa500+case),(0x9b0,0x8000),(0x9b2,0xffff)]:struct.pack_into('<H',raw,addr,value)
  expected,seen=reference(rom,raw);pcs.update(seen);inp=a.output/'controlled.input';inp.write_bytes(raw)
  got,_=v.probe(a.exe.resolve(),a.rom.resolve(),inp,False)
  for row,(want,_)in zip(got,expected):assert row['result']==1 and row['words']==list(struct.unpack('<65536H',want))
  controlled+=3*65536
 report=dict(passed=True,native_words=native,native_register_fields=regs,controlled_cases=72,controlled_words=controlled,source_pcs=len(pcs),rom_sha256=v.ROM_SHA,scope='independent actual-ROM binary16 DP0 data/flag diagnostic; no timing, stack, E/DBR observation or normal extreme-state claim')
 (a.output/'report.json').write_text(json.dumps(report,indent=2));print(report)
if __name__=='__main__':main()
