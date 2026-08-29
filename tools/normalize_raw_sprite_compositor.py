"""Compact native `$80:B344-$B529` calls into low-OAM witnesses."""
import argparse,json
from pathlib import Path
def memory(s):
 out={}
 for base,payload in s.items():
  start=int(base,16);out.update((start+i,v) for i,v in enumerate(bytes.fromhex(payload)))
 return out
def word(m,a):return m[a]|m[a+1]<<8
def main():
 p=argparse.ArgumentParser();p.add_argument('--vectors',required=True);p.add_argument('--output',required=True);a=p.parse_args();calls=[]
 for r in map(json.loads,Path(a.vectors).read_text().splitlines()):
  before=memory(r['entry']['mem']);after=memory(r['exit']['mem']);cpu=r['entry']['cpu']
  start=word(before,0x5f3);end=word(after,0x5f3)
  if end<start or (end-start)%4:raise AssertionError((start,end))
  entries=[];high_pointer=word(before,0x5e7);high_slot=word(before,0x5e9)
  for z in range(start,end,4):
   raw=[after[z+i] for i in range(4)]
   meta=(after[high_pointer]>>((3-high_slot)*2))&3
   signed_x=(raw[0]-256 if meta&1 else raw[0])&0xffff
   entries.append([signed_x,raw[1],raw[2],raw[3],1 if meta&2 else 0])
   high_slot-=1
   if high_slot<0:high_slot=3;high_pointer+=1
  direct=r['entry_pc'].lower()=='80b348'
  resource=cpu['a']&0xffff if direct else word(before,0)
  attribute=word(before,0x14) if direct else cpu['a']&0xffff
  calls.append({'call':r['call'],'entry_pc':r['entry_pc'],'exit_pc':r['exit_pc'],'input':[resource,attribute,cpu['x']&0xffff,cpu['y']&0xffff],'expected':entries})
 Path(a.output).write_text(json.dumps({'routine':'$80:B344-$B529 raw ROM sprite compositor','provenance':'natural CPU-vs-CPU real-entry calls; no PC/ROM/stack patching','calls':calls},separators=(',',':'))+'\n')
 print(f'normalized {len(calls)} calls, {sum(len(c["expected"]) for c in calls)} OAM entries, {len({tuple(c["input"]) for c in calls})} inputs')
if __name__=='__main__':main()
