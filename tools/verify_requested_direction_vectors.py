import argparse,json,subprocess
from pathlib import Path
SIZE=0x4b00
def mem(s):
 r=bytearray(SIZE)
 for b,p in s['mem'].items():a=int(b,16);d=bytes.fromhex(p);r[a:a+len(d)]=d
 return r
def word(r,a):return r[a]|r[a+1]<<8
def main():
 p=argparse.ArgumentParser();p.add_argument('--vectors',required=True);p.add_argument('--probe',required=True);a=p.parse_args();path=Path(a.vectors)
 if path.suffix=='.json':
  vs=json.loads(path.read_text())['calls'];images=[bytes.fromhex(v['input']) for v in vs];want=[v['expected'] for v in vs]
 else:
  vs=[json.loads(l) for l in path.open() if l.strip()];vs=[v for v in vs if v['entry_frame']==v['exit_frame']];images=[bytes(mem(v['entry'])) for v in vs];want=[]
  for v in vs:
   before=mem(v['entry']);after=mem(v['exit']);slot=word(before,0xc2);want.append(word(after,0x34eb+slot*0x100+0x50))
 run=subprocess.run([a.probe],input=b''.join(images),capture_output=True,check=True);got=[int(l,16) for l in run.stdout.decode().splitlines() if l]
 bad=[(i,x,y) for i,(x,y) in enumerate(zip(want,got),1) if x!=y]
 print(f"[REQUESTED DIRECTION] {'PASS' if not bad and len(got)==len(want) else 'FAIL'}: calls={len(want)} mismatches={len(bad)}")
 for x in bad[:12]:print(x)
 if bad or len(got)!=len(want):raise SystemExit(1)
if __name__=='__main__':main()
