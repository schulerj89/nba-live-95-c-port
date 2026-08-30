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
  before=memory(r['entry']['mem']);after=memory(r['exit']['mem']);ptr=word(before,0x96);values=[word(before,ptr),word(before,0x92e),word(before,0x9aa),word(before,0x9ac),word(before,0x9ae)]
  for i in range(10):
   base=0x34eb+i*0x100;values += [word(before,base+4),word(before,base+8),word(before,base+0x30),word(before,base+0x86),word(before,base+0x8a)]
  # The production selector now requires the native live side-context anchor.
  values.append(word(before,word(before,0x9e)+0x0a))
  expected=word(after,0xaa) if r['exit_pc'].lower()=='86f60b' else 0xff
  calls.append({'call':r['call'],'exit_pc':r['exit_pc'],'input':values,'expected':expected})
 Path(a.output).write_text(json.dumps({'routine':'$86:F5D2-$F60A alternate/fallback inbound selectors','provenance':'controlled real-entry ROM calls; no PC/ROM/stack patching','calls':calls},separators=(',',':'))+'\n')
 print(f'normalized {len(calls)} alternate selector calls')
if __name__=='__main__':main()
