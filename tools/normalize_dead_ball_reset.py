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
  i=memory(r['entry']['mem']);o=memory(r['exit']['mem']);owner=word(i,0x93e);mode=word(i,0x34eb+owner*0x100+0x5e) if owner<10 else 0xffff
  outmode=word(o,0x34eb+owner*0x100+0x5e) if owner<10 else 0xffff
  calls.append({'call':r['call'],'input':[word(i,0x93a),owner,word(i,0x3eef),word(i,0x3ef3),word(i,0x3ef9),word(i,0x3efb),mode],
   'expected':[word(o,z) for z in (0x952,0x936,0x92e,0x9d6,0x92c,0x9c6,0x968,0x96a,0x9b0,0x9b2)]+[outmode,word(o,0x3ef9),word(o,0x3efb),word(o,0x93e),word(o,0x97c),word(o,0x910),word(o,0x940),word(o,0x9c4)]})
 Path(a.output).write_text(json.dumps({'routine':'$87:9B38-$9BC8 common dead-ball reset','provenance':'controlled real-entry ROM calls; no PC/ROM/stack patching','calls':calls},separators=(',',':'))+'\n')
 print(f'normalized {len(calls)} dead-ball reset calls')
if __name__=='__main__':main()
