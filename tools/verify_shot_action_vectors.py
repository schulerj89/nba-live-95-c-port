"""Owned shot-action slices, with exact ROM exit selection and no exclusions."""
import argparse
from collections import Counter
import json
from pathlib import Path
import subprocess
from verify_action_animation_vectors import memory,word,OFFSETS

OPS={'86b6d3':0,'86b8ca':1,'869846':2,'86b8c0':3,'86b84c':4,'86b7cd':5}
EXITS={0:{'86b744'},1:{'86b951','86b971','86b978','86b886'},2:{'86986c'},
       3:{'86b8c8'},4:{'86b866'},5:{'86b8ca','86b7e4','86b7f7'}}

def convert(v):
    a,b=memory(v['entry']),memory(v['exit'])
    op=OPS[v['entry_pc']]
    if v['exit_pc'] not in EXITS[op]:raise ValueError('bad call boundary')
    base=word(a,0x96)
    def read(m):
        return ([word(m,base+o) for o in OFFSETS]+
                [word(m,base+o) for o in (0xe,0x10,0x12,0x4a,0x5e,0x7e,0x60,0x28,0x64)]+
                [word(m,o) for o in (0x948,0x920,0x91c)])
    gate=[0]*11
    if op==1:
        ft=word(a,0x978)
        controller=word(a,base+0x16)
        lower=word(a,base+0x44)
        # Read only the context/input words that this entry actually consumes.
        anchor=word(a,word(a,0x9e)+0xa) if lower>=0x600 else 0
        buttons=word(a,word(a,0x9a)+8) if controller<0x8000 and not ft else 0
        gate=[word(a,base+o) for o in (4,8,0xc,0x12,0x16,0x44)]+[
            ft,word(a,0x7f6),buttons,word(a,base+0x4e),anchor]
    row=[op,word(a,base+0xa8),word(a,base+0x72),word(a,0x978),
         word(a,base+0x6e),word(a,0x93a)]+read(a)+gate+[word(a,0xc6)]
    # The resolver/cancel helpers' transitive channel writes are owned by
    # start/jump. Other operations leave channels and unowned words intact.
    expected=read(b)
    if op==1:
        # The gate stops before launch or latch writes; only facing is owned.
        if read(b)!=read(a) or word(a,0x7f6)!=word(b,0x7f6):
            raise ValueError('release gate changed non-facing actor state or RNG')
        expected=read(a)
    result=1 if v['exit_pc'] in {'86b951','86b971'} else 2 if v['exit_pc']=='86b886' else 0
    stage={'86b8ca':2,'86b7e4':0,'86b7f7':1}.get(v['exit_pc'],0) if op==5 else 0
    expected += [result,word(b,base+0x4e) if op==1 else 0,stage]
    return dict(call=v['call'],entry_pc=v['entry_pc'],exit_pc=v['exit_pc'],input=row,expected=expected)

def main():
    p=argparse.ArgumentParser()
    for name in ('vectors','probe','pack'):p.add_argument('--'+name,required=True)
    p.add_argument('--normalized',action='store_true')
    args=p.parse_args();raw=Path(args.vectors).read_text()
    rows=json.loads(raw) if args.normalized else [convert(json.loads(l)) for l in raw.splitlines() if l]
    if not rows:raise SystemExit('no live calls')
    run=subprocess.run([args.probe,args.pack],input=''.join(' '.join(f'{x:x}' for x in r['input'])+'\n' for r in rows),
                       text=True,capture_output=True,check=True)
    lines=[l for l in run.stdout.splitlines() if l and not l.startswith('[')]
    if len(lines)!=len(rows):raise SystemExit('missing probe output')
    bad=[]
    for r,l in zip(rows,lines):
        got=[int(x,16) for x in l.split()]
        if got!=r['expected']:bad.append((r['call'],[(i,x,y) for i,(x,y) in enumerate(zip(r['expected'],got)) if x!=y]))
    for x in bad[:12]:print(x)
    print(f'[SHOT ACTION] calls={len(rows)} mismatches={len(bad)} entries={dict(Counter(r["entry_pc"] for r in rows))}')
    if bad:raise SystemExit(1)
if __name__=='__main__':main()
