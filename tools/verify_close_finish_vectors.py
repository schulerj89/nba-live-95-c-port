import argparse,json,subprocess
from collections import Counter
from pathlib import Path

SIZE=0x4B00;BASE=0x34EB;STRIDE=0x100
def memory(s):
 raw=bytearray(SIZE)
 for b,p in s['mem'].items():
  q=int(b,16);d=bytes.fromhex(p);raw[q:q+len(d)]=d
 return raw
def word(r,a):return r[a]|r[a+1]<<8
def coherent(v):
 if v['entry_pc']!='86b0f7' or v['exit_pc']!='86b153':return True
 i=memory(v['entry']);o=memory(v['exit']);b=word(i,0x96)
 # B141-B150 are three direct 16-bit stores. Reject recorder samples whose
 # exit image contradicts those stores; retaining them would bless a capture
 # artifact rather than native behavior.
 return (word(o,b+4)==(word(o,0x3fef)-word(o,0))&0xffff and
         word(o,b+8)==(word(o,0x3ff3)-word(o,2))&0xffff and
         word(o,b+0xc)==(0x50-word(o,4))&0xffff)
def row(r,entry_pc=None,actor_ptr=None):
 b=word(r,0x96) if actor_ptr is None else actor_ptr
 accepted=0 if entry_pc=='86b34f' and word(r,0xaa)!=0 else 1
 values=[accepted,word(r,0x7f6),word(r,0x936),word(r,0x93e),word(r,0x948),word(r,0x94c),
  word(r,0x96a),word(r,0x1866),word(r,0x942),word(r,0x944),word(r,0x946),word(r,0x9c4),
  word(r,0x9b8),word(r,0x9c8),word(r,0x3eef),word(r,0x3ef3),word(r,0x3ef7),
  word(r,0x3ef9),word(r,0x3efb),word(r,0x3efd),word(r,0x93e)]
 values += [word(r,b+o) for o in (4,8,0xc,0xe,0x10,0x12,0x5e,0x60,0x7e,0x30,0x32,
  0x4e,0x50,0x56,0x58,0x5a,0x66,0xba,0xbc)]
 return values
def main():
 p=argparse.ArgumentParser();p.add_argument('--vectors',required=True);p.add_argument('--probe',required=True);p.add_argument('--pack',required=True);a=p.parse_args()
 doc=Path(a.vectors);normalized=doc.suffix=='.json'
 if normalized:
  calls=json.loads(doc.read_text())['calls'];images=[bytes.fromhex(c['input']) for c in calls];expected=[c['expected'] for c in calls];pairs=Counter((c['entry_pc'],c['exit_pc']) for c in calls)
 else:
  raw_vectors=[json.loads(x) for x in doc.read_text().splitlines() if x.strip()];vectors=[v for v in raw_vectors if coherent(v)];quarantined=len(raw_vectors)-len(vectors);images=[memory(v['entry']) for v in vectors];expected=[row(memory(v['exit']),v['entry_pc'],word(memory(v['entry']),0x96)) for v in vectors];pairs=Counter((v['entry_pc'],v['exit_pc']) for v in vectors)
 run=subprocess.run([a.probe,a.pack],input=b''.join(images),capture_output=True,check=True)
 actual=[[int(x,16) for x in l.split()] for l in run.stdout.decode().splitlines() if l and not l.startswith('[')]
 bad=[]
 for i,(w,g) in enumerate(zip(expected,actual),1):
  d=[(j,x,y) for j,(x,y) in enumerate(zip(w,g)) if x!=y]
  if d:
   pair=(calls[i-1]['entry_pc'],calls[i-1]['exit_pc']) if normalized else (vectors[i-1]['entry_pc'],vectors[i-1]['exit_pc'])
   bad.append((i,pair,d[:16]))
 print(f"[CLOSE FINISH] {'PASS' if len(actual)==len(expected) and not bad else 'FAIL'}: calls={len(expected)} pairs={dict(pairs)} quarantined={0 if normalized else quarantined} mismatches={len(bad)}")
 for x in bad[:10]:print(x)
 if len(actual)!=len(expected) or bad:raise SystemExit(1)
if __name__=='__main__':main()
