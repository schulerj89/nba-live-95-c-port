import argparse,json,subprocess
from collections import Counter
from pathlib import Path

SIZE=0x4B00;BASE=0x34EB;STRIDE=0x100
FIELDS=(0x5e,0x72,0x74,0x78,0x8c,0x8e)
def memory(s):
 r=bytearray(SIZE)
 for b,p in s['mem'].items():
  q=int(b,16);d=bytes.fromhex(p);r[q:q+len(d)]=d
 return r
def word(r,a):return r[a]|r[a+1]<<8
def slot(p):
 d=p-BASE
 return d//STRIDE if d>=0 and d%STRIDE==0 and d//STRIDE<10 else -1
def changed(a,z):return any(word(a,BASE+i*STRIDE+0x74)!=word(z,BASE+i*STRIDE+0x74) for i in range(10))
def expected(v,a,z):
 e=v['entry_pc'];cur=slot(word(a,0x96));rel=slot(word(a,0x9a))
 if e in ('85b9d2','85ba1d','85bae4'):ok=word(z,0xaa)!=0
 elif e in ('85bab7','85bb99'):ok=True
 elif e=='85bb6c':ok=v['exit_pc']!='85bb96'
 else:ok=changed(a,z)
 selected=0xff
 if ok:
  if e in ('85b9d2','85ba1d'):selected=word(z,BASE+rel*STRIDE+0x74)//2
  elif e=='85bae4':selected=slot(word(z,0x96))
  elif e in ('85bab7','85bb99'):selected=cur
  else:selected=slot(word(z,0x96))
 row=[int(ok),selected]
 for i in range(10):row += [word(z,BASE+i*STRIDE+o) for o in FIELDS]
 return row
def main():
 p=argparse.ArgumentParser();p.add_argument('--vectors',required=True);p.add_argument('--probe',required=True);a=p.parse_args()
 path=Path(a.vectors);normalized=path.suffix=='.json'
 if normalized:
  vs=json.loads(path.read_text())['calls']
  images=[bytes.fromhex(v['input']) for v in vs]
  want=[v['expected'] for v in vs]
 else:
  vs=[json.loads(x) for x in path.read_text().splitlines() if x.strip()]
  images=[];want=[]
  for v in vs:
   i=memory(v['entry']);z=memory(v['exit']);i[0:2]=bytes.fromhex(v['entry_pc'][2:6])[::-1];images.append(i);want.append(expected(v,i,z))
 run=subprocess.run([a.probe],input=b''.join(images),capture_output=True,check=True)
 got=[[int(x,16) for x in l.split()] for l in run.stdout.decode().splitlines()]
 bad=[]
 for n,(w,g) in enumerate(zip(want,got),1):
  d=[(j,x,y) for j,(x,y) in enumerate(zip(w,g)) if x!=y]
  if d:bad.append((n,vs[n-1]['entry_pc'],vs[n-1]['exit_pc'],d[:12]))
 print(f"[MATCHUP HELPERS] {'PASS' if len(got)==len(want) and not bad else 'FAIL'}: calls={len(want)} pairs={dict(Counter((v['entry_pc'],v['exit_pc']) for v in vs))} mismatches={len(bad)}")
 for b in bad[:12]:print(b)
 if len(got)!=len(want) or bad:raise SystemExit(1)
if __name__=='__main__':main()
