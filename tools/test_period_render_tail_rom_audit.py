"""Independent FBFF fixed-ROM instruction diagnostic; no timing or CPU claim.

D5DB reuses the auditor's earlier literal-ROM diagnostic. FBFF is interpreted
from original bytes here, independently of the candidate's loop structure.
Only binary16, DP0, low-WRAM data accesses are admitted.
"""
import argparse,hashlib,json,random,struct,subprocess
from pathlib import Path
from test_period_sort_rom_audit import original as collision
REGIONS=[(0x34d3,24),(0x34eb,3072),(0x7e44,24),(0x84a,4)]
def original(rom,raw):
 mem,pcs=collision(rom,raw);a=x=y=0;n=z=c=False;pc=0xfbff;returns=[]
 def word(addr):return struct.unpack_from('<H',mem,addr)[0]
 def put(addr,v):struct.pack_into('<H',mem,addr,v&65535)
 def nz(v):
  nonlocal n,z
  v&=65535;n=bool(v&32768);z=v==0;return v
 for steps in range(30000):
  pcs.add(0x800000|pc);offset=pc&32767;op=rom[offset];arg=rom[offset+1];w=int.from_bytes(rom[offset+1:offset+3],'little');nxt=pc+1
  if op in(0xa5,0xa6,0x85,0x84,0x64,0x65,0xe5,0xe6,0xc5):
   addr=arg;v=word(addr);nxt=pc+2
  elif op in(0xa9,0xc9):addr=None;v=w;nxt=pc+3
  elif op in(0xbc,0xb9,0xd9,0x9d,0xf9,0xed,0x99):
   addr=(w+(x if op in(0xbc,0x9d)else y if op in(0xb9,0xd9,0xf9,0x99)else 0))&65535;v=word(addr);nxt=pc+3
  else:addr=None;v=0
  if op in(0xa5,0xa9,0xb9):a=nz(v)
  elif op==0xa6:x=nz(v)
  elif op==0xbc:y=nz(v)
  elif op in(0x85,0x9d,0x99):put(addr,a)
  elif op==0x84:put(addr,y)
  elif op==0x64:put(addr,0)
  elif op==0xe6:put(addr,nz(v+1))
  elif op in(0xc9,0xc5,0xd9):nz(a-v);c=a>=v
  elif op==0x65:r=a+v+c;a=nz(r);c=r>65535
  elif op in(0xe5,0xed,0xf9):r=a-v-(not c);a=nz(r);c=r>=0
  elif op==0x0a:c=bool(a&32768);a=nz(a<<1)
  elif op==0x4a:c=bool(a&1);a=nz(a>>1)
  elif op==0x6a:carry=c;c=bool(a&1);a=nz((a>>1)|(32768 if carry else 0))
  elif op==0x3a:a=nz(a-1)
  elif op==0x98:a=nz(y)
  elif op==0x38:c=True
  elif op==0x18:c=False
  elif op in(0x90,0xb0,0x10):
   take={0x90:not c,0xb0:c,0x10:not n}[op];nxt=pc+2+((arg-256 if arg&128 else arg)if take else 0)
  elif op==0x20:returns.append(pc+3);nxt=w
  elif op==0x60:nxt=returns.pop()
  elif op==0x6b:
   assert not returns
   # The actual caller increments a two-word counter only after both sorts.
   caller=rom[0x361f7:0x36208];assert caller==bytes.fromhex('22 db d5 86 22 ff fb 80 ee 4a 08 d0 03 ee 4c 08 6b')
   lo=int.from_bytes(caller[9:11],'little');hi=int.from_bytes(caller[14:16],'little');put(lo,word(lo)+1)
   if word(lo)==0:put(hi,word(hi)+1)
   return mem,pcs
  else:raise AssertionError((hex(pc),hex(op)))
  pc=nxt
 raise AssertionError('bounded FBFF diagnostic exhausted')
def main():
 p=argparse.ArgumentParser()
 for name in('rom','exe','replay','output'):p.add_argument('--'+name,type=Path,required=True)
 a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);rom=a.rom.read_bytes();assert hashlib.sha256(rom).hexdigest()=='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
 cases=json.loads(a.replay.read_text())['cases'];pcs=set();counts={'native':0,'controlled':0};r=random.Random(0xfc69);edge=[0,1,2,3,0x7fff,0x8000,0x8001,0xfffe,0xffff]
 def difference(left,right):return[(hex(i),left[i],right[i])for start,size in REGIONS for i in range(start,start+size)if left[i]!=right[i]]
 for row in cases:
  source=Path(row['command'][1]);expected,seen=original(rom,source.read_bytes());pcs.update(seen)
  rows=[json.loads(line)for line in(source.parent/'boundaries.jsonl').read_text().splitlines()];target=next(v for v in rows if v['tag']=='formation.return')
  assert not difference(expected,(source.parent/target['raw']).read_bytes());assert not difference(expected,Path(row['command'][2]).read_bytes());counts['native']+=1
 seed=Path(cases[0]['command'][1]).read_bytes()
 for case in range(384):
  raw=bytearray(seed);draw=list(range(12));r.shuffle(draw);order=list(range(11));r.shuffle(order)
  for i in range(12):
   for off in(4,8):struct.pack_into('<H',raw,0x34eb+i*256+off,r.choice(edge)if case<192 else r.randrange(65536))
   # All equal keys are a separate stable-order branch, with poisoned depths.
   if case<12:struct.pack_into('<HH',raw,0x34ef+i*256,case,0);struct.pack_into('<H',raw,0x34f3+i*256,case)
   struct.pack_into('<H',raw,0x34eb+i*256+0x68,0xa500+i);struct.pack_into('<H',raw,0x7e44+2*i,0x34eb+draw[i]*256)
  for i in range(11):struct.pack_into('<H',raw,0x34d3+2*i,0x34eb+order[i]*256);struct.pack_into('<H',raw,0x34ff+i*256,0xbe00+i)
  struct.pack_into('<HH',raw,0x84a,0xffff if case%2 else case,0xffff if case%3 else 0xbeef);struct.pack_into('<H',raw,0x860,r.choice(edge)if case<192 else r.randrange(65536))
  expected,seen=original(rom,raw);pcs.update(seen);inp=a.output/'controlled.input';out=a.output/'controlled.output';inp.write_bytes(raw)
  result=subprocess.run([str(a.exe.resolve()),str(inp),str(out)],capture_output=True);assert type(result.returncode)is int and result.returncode==0 and result.stdout==b'' and result.stderr==b''
  actual=out.read_bytes();assert len(actual)==131072;assert not difference(actual,expected),(case,difference(actual,expected))
  # The host adapter must not write unrelated WRAM or original DP scratch.
  owned={i for start,size in REGIONS for i in range(start,start+size)};assert all(x==raw[i]for i,x in enumerate(actual)if i not in owned)
  if case<12:assert actual[0x7e44:0x7e5c]==raw[0x7e44:0x7e5c]
  counts['controlled']+=1
 report=dict(passed=True,**counts,native_bytes=counts['native']*3124,controlled_bytes=counts['controlled']*3124,source_pcs=len(pcs),rom_sha256=hashlib.sha256(rom).hexdigest(),scope='Independent original-byte fixed binary16 DP0 DB0 diagnostic; owned projection only; no cycle, register, hardware or natural extreme-case claim')
 (a.output/'report.json').write_text(json.dumps(report,indent=2));print(report)
if __name__=='__main__':main()
