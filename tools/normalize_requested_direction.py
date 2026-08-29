import argparse,json
from pathlib import Path
from verify_requested_direction_vectors import mem,word
def main():
 p=argparse.ArgumentParser();p.add_argument('--vectors',required=True);p.add_argument('--output',required=True);a=p.parse_args();vs=[json.loads(l) for l in Path(a.vectors).open() if l.strip()];vs=[v for v in vs if v['entry_frame']==v['exit_frame']]
 indexes=sorted({round(i*(len(vs)-1)/31) for i in range(32)});calls=[]
 for i in indexes:
  v=vs[i];b=mem(v['entry']);z=mem(v['exit']);slot=word(b,0xc2);calls.append({'input':b.hex(),'expected':word(z,0x34eb+slot*0x100+0x50)})
 Path(a.output).write_text(json.dumps({'calls':calls},separators=(',',':')));print(f'[REQUESTED DIRECTION NORMALIZE] calls={len(calls)}')
if __name__=='__main__':main()
