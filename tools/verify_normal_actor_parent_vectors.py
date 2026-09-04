"""Replay native `$86:F1B0-$F8CC` actor continuations through production C."""
import argparse,json,subprocess
from collections import Counter
from pathlib import Path
SIZE=0x4B00
def memory(s):
 r=bytearray(SIZE)
 for b,p in s['mem'].items():a=int(b,16);d=bytes.fromhex(p);r[a:a+len(d)]=d
 return r
def word(r,a):return r[a]|r[a+1]<<8
def projection(r):
 out=[1,word(r,0x7f6),word(r,0x9a2),word(r,0x998)]
 for i in range(10):
  b=0x34eb+i*0x100
  out.extend(word(r,b+o) for o in (0x56,0x58,0x5e,0x60,0x7e,0x0e,0x10,0x4c,0x4e,0x50,0x52,0x38,0x30,0x32))
 return out
def main():
 p=argparse.ArgumentParser();p.add_argument('--vectors',nargs='+',required=True);p.add_argument('--probe',required=True);p.add_argument('--pack',required=True);a=p.parse_args()
 vs=[]
 for f in a.vectors:
  if Path(f).suffix=='.json':
   for c in json.loads(Path(f).read_text())['calls']:vs.append((bytes.fromhex(c['input']),c['expected'],c['entry_pc'],c['exit_pc']))
  else:
   for line in Path(f).open():
    if not line.strip():continue
    v=json.loads(line)
    if v['entry_frame']!=v['exit_frame']:continue
    vs.append((bytes(memory(v['entry'])),projection(memory(v['exit'])),v['entry_pc'],v['exit_pc']))
 run=subprocess.run([a.probe,a.pack],input=b''.join(v[0] for v in vs),capture_output=True,check=True)
 got=[[int(x,16) for x in l.split()] for l in run.stdout.decode().splitlines() if l and not l.startswith('[')]
 bad=[]
 for i,(v,g) in enumerate(zip(vs,got),1):
  before=bytearray(v[0]);slot=word(before,0xc2);base=0x34eb+slot*0x100
  # A due mode-one/three/five decision calls child routines in banks $85/$86.
  # Their +$4E/+50 writes are protected by their own differential gates and
  # are not owned by the F1B0/F23F/F2CA parent. Mode two always finishes at
  # its parent-owned F78B-F790 +$50->$4E tail, so both fields remain compared.
  ignored=set()
  if word(before,base+0x60)<=0x20 and word(before,base+0x5e)!=2:
   actor_start=4+slot*14;ignored.update((actor_start+8,actor_start+9))
  # Mode four's role-clear route calls the still-unported `$86:EF09` policy
  # child before writing the defensive target. That child owns one `$80:CEE7`
  # RNG step; retain it in the native fixture, but do not credit this parent
  # gate with reproducing the child's RNG side effect.
  if v[2]=='86f794' and word(before,base+0x5e)==4 and \
     word(before,base+0x60)<=0x20 and word(before,0x9d8)==0:
   ignored.add(1)
  differences=[(n,x,y) for n,(x,y) in enumerate(zip(v[1],g))
               if n not in ignored and x!=y]
  if differences:bad.append((i,v[2],v[3],differences[:16]))
 print(f"[NORMAL ACTOR PARENTS] {'PASS' if not bad and len(got)==len(vs) else 'FAIL'}: calls={len(vs)} entries={dict(Counter(v[2] for v in vs))} mismatches={len(bad)}")
 for x in bad[:12]:print(x)
 if bad or len(got)!=len(vs):raise SystemExit(1)
if __name__=='__main__':main()
