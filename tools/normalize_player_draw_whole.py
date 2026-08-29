"""Compact whole native `$87:A47A-$A845` presentation calls."""
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
  before=memory(r['entry']['mem']);after=memory(r['exit']['mem'])
  calls.append({'call':r['call'],'entry_frame':r['entry_frame'],'exit_frame':r['exit_frame'],'exit_pc':r['exit_pc'],
   'object_head':word(before,0x7e44),'queue_counts':[word(after,0x8ec4),word(after,0x8ec6)],
   'presentation_copy':[word(after,z) for z in (0x60d,0x60f,0x611,0x613,0x800,0x802)]})
 Path(a.output).write_text(json.dumps({'routine':'$87:A47A-$A845 whole gameplay presentation pass','provenance':'natural CPU-vs-CPU ROM execution; no PC/ROM patching','calls':calls},separators=(',',':'))+'\n')
 print(f'normalized {len(calls)} whole draw calls')
if __name__=='__main__':main()
