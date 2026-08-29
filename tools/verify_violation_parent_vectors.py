"""Replay `$87:92A5-$949E` through the production C parent boundary."""
import argparse,json,subprocess
from collections import Counter
from pathlib import Path
SIZE=0x4B00
def memory(s):
 r=bytearray(SIZE)
 for b,p in s['mem'].items():
  a=int(b,16);d=bytes.fromhex(p);r[a:a+len(d)]=d
 return r
def w(r,a):return r[a]|r[a+1]<<8
def projection(r):
 values=[w(r,a) for a in (0x936,0x93A,0x93E,0x948,0x952,0x954,0x956,
  0x964,0x966,0x968,0x96A,0x96C,0x978,0x97A,0x97C,0x92C,0x92E,
  0x9B0,0x9B2,0x9BC,0x9C6,0x9D6,0x13E7,0x472A,0x47AA,0x3EF9,0x3EFB)]
 values.extend(w(r,0x34EB+i*0x100+0x5E) for i in range(10));return values
def main():
 p=argparse.ArgumentParser();p.add_argument('--vectors',required=True);p.add_argument('--probe',required=True);p.add_argument('--pack',required=True);a=p.parse_args();path=Path(a.vectors)
 if path.suffix=='.json':rows=json.loads(path.read_text())['calls'];images=[bytes.fromhex(x['input']) for x in rows];expected=[x['expected'] for x in rows];sources=Counter(x['source'] for x in rows)
 else:
  rows=[json.loads(x) for x in path.open() if x.strip()];rows=[x for x in rows if x['entry_frame']==x['exit_frame']];images=[memory(x['entry']) for x in rows];expected=[projection(memory(x['exit'])) for x in rows];sources=Counter(Path(a.vectors).parent.name for _ in rows)
 run=subprocess.run([a.probe,a.pack],input=b''.join(images),capture_output=True,check=True);actual=[[int(v,16) for v in l.split()] for l in run.stdout.decode().splitlines() if l and not l.startswith('[')];bad=[(i+1,x,y) for i,(x,y) in enumerate(zip(expected,actual)) if x!=y]
 print(f"[VIOLATION PARENT] {'PASS' if not bad and len(actual)==len(expected) else 'FAIL'}: calls={len(expected)} sources={dict(sources)} mismatches={len(bad)}")
 for i,x,y in bad[:12]:print(i,[(n,u,v) for n,(u,v) in enumerate(zip(x,y)) if u!=v][:15])
 if bad or len(actual)!=len(expected):raise SystemExit(1)
if __name__=='__main__':main()
