"""Independent original-ROM push/pop/RTL diagnostic; no gameplay or timing model."""
import argparse,hashlib,json
from pathlib import Path
FIELDS=('a','x','y','ps','d','sp','dbr','k','pc')
class Slice:
 def __init__(self,rom,row,data):
  self.rom=rom;self.mem=bytearray(0x4a00);self.mem[:0x2000]=data[:0x2000];self.mem[0x3400:]=data[0x2000:];self.reg={k:row['cpu_'+k]for k in FIELDS};self.executed=[]
 def fetch(self,p):return self.rom[((p>>16)&127)*32768+(p&32767)]
 def push(self,b):assert self.reg['sp']<=0x1fff;self.mem[self.reg['sp']]=b;self.reg['sp']-=1
 def pop(self):self.reg['sp']+=1;assert self.reg['sp']<=0x1fff;return self.mem[self.reg['sp']]
 def run(self,stop):
  for _ in range(30):
   pc=self.reg['k']*65536+self.reg['pc']
   if pc==stop:return
   op=self.fetch(pc);self.executed.append(pc);nxt=pc+1
   if op==0xd4:
    addr=self.fetch(pc+1);self.push(self.mem[addr+1]);self.push(self.mem[addr]);nxt=pc+2
   elif op==0x68:
    value=self.pop();value|=self.pop()<<8;self.reg['a']=value;self.reg['ps']=(self.reg['ps']&~0x82)|(128 if value&32768 else 0)|(2 if value==0 else 0)
   elif op==0x85:
    addr=self.fetch(pc+1);value=self.reg['a'];self.mem[addr]=value&255;self.mem[addr+1]=value>>8;nxt=pc+2
   elif op==0x6b:
    target=self.pop();target|=self.pop()<<8;bank=self.pop();nxt=(bank<<16)|((target+1)&65535)
   elif op==0x4c:nxt=(pc&0xff0000)|(self.fetch(pc+1)|(self.fetch(pc+2)<<8))
   else:raise AssertionError(('unsupported source opcode',hex(pc),hex(op)))
   self.reg['pc']=nxt&65535;self.reg['k']=nxt>>16
  raise AssertionError('bounded return did not reach source stop')
 def raw(self):return bytes(self.mem[:0x2000]+self.mem[0x3400:])
def main():
 p=argparse.ArgumentParser();p.add_argument('--rom',type=Path,required=True);p.add_argument('--captures',type=Path,nargs=2,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();rom=a.rom.read_bytes();assert hashlib.sha256(rom).hexdigest()=='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
 counts=dict(calls=0,return_segments=0,save_frames=0,raw_bytes_compared=0);pcs=set();crossings=[];signs={'negative':0,'positive':0,'zero':0}
 for capture in a.captures:
  rows=[json.loads(line)for line in(capture/'boundaries.jsonl').read_text().splitlines()];groups=[];current=None
  for row in rows[2:]:
   if row['tag']=='human.entry':assert current is None;current={}
   current[row['tag']]=row
   if row['tag']=='human.return':groups.append(current);current=None
  assert current is None
  for g in groups:
   data=lambda row:(capture/row['raw']).read_bytes()
   for start,end in [('init.restore','init.rtl'),('init.rtl','pass.restore'),('pass.restore','pass.rtl'),('pass.rtl','human.resume'),('human.resume','human.restore'),('human.restore','human.rtl'),('human.rtl','human.return')]:
    m=Slice(rom,g[start],data(g[start]));m.run(g[end]['pc']);assert m.raw()==data(g[end]),start
    assert m.reg=={k:g[end]['cpu_'+k]for k in FIELDS},(start,m.reg)
    counts['return_segments']+=1;counts['raw_bytes_compared']+=13824;pcs.update(m.executed)
   for start,stop,end,delta,size in [('human.entry',0x84e2ae,'pass.entry',3,2),('pass.entry',0x84df8a,'init.entry',3,16),('init.entry',0x86ab3d,'init.restore',0,16)]:
    m=Slice(rom,g[start],data(g[start]));m.run(stop);state=Slice(rom,g[end],data(g[end]));assert m.reg['sp']==state.reg['sp']+delta
    lo=m.reg['sp']+1;assert m.mem[lo:lo+size]==state.mem[lo:lo+size]
    counts['save_frames']+=1;pcs.update(m.executed)
   if g['human.entry']['court']!=g['human.return']['court']:crossings.append([g['human.entry']['court'],g['human.return']['court']])
   s=Slice(rom,g['human.entry'],data(g['human.entry']));b6=s.mem[0xb6]|s.mem[0xb7]<<8;signs['negative'if b6&32768 else'positive'if b6 else'zero']+=1;counts['calls']+=1
 assert counts['calls']==25 and counts['return_segments']==175 and crossings==[[1471,1472]]
 report=dict(passed=True,**counts,source_pcs=[f'{pc:06x}'for pc in sorted(pcs)],outer_b6_signs=signs,unchanged_crossings=crossings,scope='actual original ROM instructions against captured prestate/poststate, seven bounded segments per call; not generic CPU, interrupt or timing acceptance')
 assert not a.output.exists();a.output.write_text(json.dumps(report,indent=2));print(counts)
if __name__=='__main__':main()
