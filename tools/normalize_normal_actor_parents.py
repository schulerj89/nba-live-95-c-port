import argparse,json
from collections import defaultdict
from pathlib import Path
from verify_normal_actor_parent_vectors import memory,projection

def main():
 p=argparse.ArgumentParser();p.add_argument('--vectors',nargs='+',required=True);p.add_argument('--output',required=True);a=p.parse_args()
 by=defaultdict(list)
 for f in a.vectors:
  for line in Path(f).open():
   if not line.strip():continue
   v=json.loads(line)
   if v['entry_frame']==v['exit_frame']:by[v['entry_pc']].append(v)
 calls=[]
 for entry,items in by.items():
  # Retain timer-hold and due decisions across the capture, including both
  # parent return sites, without checking in the multi-megabyte raw corpus.
  chosen=[]
  def is_due(v):
   raw=memory(v['entry']);slot=raw[0xc2]|raw[0xc3]<<8;base=0x34eb+slot*0x100
   return (raw[base+0x60]|raw[base+0x61]<<8)<=0x20
  for due in (False,True):
   group=[v for v in items if is_due(v)==due]
   if not group:continue
   indexes=sorted({round(i*(len(group)-1)/15) for i in range(16)})
   chosen.extend(group[i] for i in indexes)
  for v in chosen:
   before=memory(v['entry']);after=memory(v['exit'])
   calls.append({'entry_pc':entry,'exit_pc':v['exit_pc'],'input':before.hex(),'expected':projection(after)})
 Path(a.output).write_text(json.dumps({'calls':calls},separators=(',',':')))
 print(f'[NORMAL ACTOR NORMALIZE] calls={len(calls)} entries={len(by)}')
if __name__=='__main__':main()
