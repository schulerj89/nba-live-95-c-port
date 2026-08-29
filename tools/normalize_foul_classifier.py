"""Compact native `$86:C4FE-$C6AC` foul-classifier calls."""
import argparse,json
from pathlib import Path
def word(m,a):return m[a]|m[a+1]<<8
def signed8(v):return v-256 if v&128 else v
def signed16(v):return v-65536 if v&32768 else v
def main():
 p=argparse.ArgumentParser();p.add_argument('--vectors',required=True);p.add_argument('--output',required=True);a=p.parse_args();calls=[]
 for r in map(json.loads,Path(a.vectors).read_text().splitlines()):
  before=bytes.fromhex(r['entry']['mem']['0000']);after=bytes.fromhex(r['exit']['mem']['0000'])
  x=r['entry']['cpu']['x'];y=r['entry']['cpu']['y'];tag=r['entry']['cpu']['a']&0xffff
  context=word(before,y+0x70);team=0 if context==0x46eb else 1
  inp=[word(before,y),word(before,x),team,word(before,y+0x6e),
       word(before,y+0x5e),word(before,y+0x4c),word(before,y+0x72),
       word(before,0x93e),word(before,0x9c8),word(before,0x93a),
       word(before,0x936),word(before,0x948),word(before,0x926),tag,
       word(before,0x17d1),word(before,0x17d3),word(before,0x7f6),
       word(before,0x964),word(before,0x9bc),word(before,0x978),
       word(before,0x97a),word(before,0x9b6),word(before,0x4713),
       word(before,0x4793),word(before,0x13e7)]
  expected=[word(after,z) for z in (0x7f6,0x964,0x9bc,0x978,0x97a,0x9b6,0x4713,0x4793,0x13e7)]
  calls.append({'call':r['call'],'input':inp,'expected':expected})
 Path(a.output).write_text(json.dumps({'routine':'$86:C4FE-$C6AC contact-foul classifier','provenance':'natural ROM execution in Mesen; no PC/ROM patching','calls':calls},separators=(',',':'))+'\n')
 print(f'normalized {len(calls)} native classifier calls')
if __name__=='__main__':main()
