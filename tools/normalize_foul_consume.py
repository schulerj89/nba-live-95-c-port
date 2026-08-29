"""Compact controlled native `$85:93F5-$945E` event-consumer calls."""
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
 ins=(0x9b6,0x964,0x8f0,0x9ba,0x497f,0x4937,0x9bc,0x13e7,0x93a,0x13e9,0x8de,0x8e2,0x8e6,0x8e8,0x9ca,0x9cc)
 outs=(0x9b6,0x964,0x8f0,0x9ba,0x497f,0x4937,0x9bc,0x13e7,0x13e9,0x8de,0x8e2,0x8e6,0x8e8)
 for r in map(json.loads,Path(a.vectors).read_text().splitlines()):
  before=memory(r['entry']['mem']);after=memory(r['exit']['mem'])
  calls.append({'call':r['call'],'input':[word(before,z) for z in ins],'expected':[word(after,z) for z in outs]})
 Path(a.output).write_text(json.dumps({'routine':'$85:93F5-$945E pending-event consumer','provenance':'controlled WRAM at real native entry; no PC/ROM/stack patching','calls':calls},separators=(',',':'))+'\n')
 print(f'normalized {len(calls)} foul-consumer cases')
if __name__=='__main__':main()
