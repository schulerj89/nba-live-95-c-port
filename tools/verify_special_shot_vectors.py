"""Compare selector and complete mode-17 state up to its shared launch call."""
import argparse
from collections import Counter
import json
from pathlib import Path
import subprocess
from verify_action_animation_vectors import memory, word, OFFSETS

EXITS={0:{'86b744','86b6d2'},1:{'86b9f9','86b9fe','86ba52','86ba53','86baa1','869da6'}}

def convert(v):
    a,b=memory(v['entry']),memory(v['exit'])
    op={'86b629':0,'86b979':1}[v['entry_pc']]
    if v['exit_pc'] not in EXITS[op]: raise ValueError('incorrect entry/exit boundary')
    base=word(a,0x96)
    def state(m):
        return ([word(m,base+o) for o in OFFSETS] +
                [word(m,base+o) for o in (0xe,0x10,0x12,0x4a,0x5e,0x7e,0x60,0x28,0x64)] +
                [word(m,o) for o in (0x948,0x920,0x91c)])
    def ball(m):
        return [word(m,o) for o in (0x3eef,0x3ef3,0x3ef7,0x922,0x936,0x9f6,0x968,0x3efd)]
    controller=word(a,base+0x16)
    buttons=word(a,word(a,0x90c)+8) if op==1 and controller<0x8000 else 0
    owns=word(a,0xc2)==word(a,0x93e)
    row=[op,word(a,base+0x72),word(a,base+0xa8),word(a,0xaa)] + [
        word(a,base+o) for o in (0x4c,0x8c,0x88,0x6c,4,8,0xc,0x16,0x4e,0x66,0x2a,0x2c)] + [
        word(a,0xc6),buttons,word(a,base+0x6e),word(a,0x93a),int(owns)] + state(a)+ball(a)
    result=int(v['exit_pc']=='86b6d2') if op==0 else {
        '86b9f9':1,'86b9fe':2,'86ba52':3,'86ba53':0,'86baa1':0,'869da6':4}[v['exit_pc']]
    expected=state(b)+[word(b,base+0x4e),word(b,base+0x66)]+ball(b)+[result]
    for address in (0x7f6,0x3eed,0x3ef1,0x3ef5,0x93e):
        if word(a,address)!=word(b,address):raise ValueError(f'unowned word changed: {address:04x}')
    return dict(call=v['call'],entry_pc=v['entry_pc'],exit_pc=v['exit_pc'],input=row,
                expected=expected,provenance=v.get('provenance','live-gameplay'))

def main():
    p=argparse.ArgumentParser()
    for name in ('vectors','probe','pack'):p.add_argument('--'+name,required=True)
    p.add_argument('--normalized',action='store_true')
    args=p.parse_args();raw=Path(args.vectors).read_text()
    rows=json.loads(raw) if args.normalized else [convert(json.loads(l)) for l in raw.splitlines() if l]
    if not rows:raise SystemExit('no captured calls')
    run=subprocess.run([args.probe,args.pack],input=''.join(' '.join(f'{x:x}' for x in r['input'])+'\n' for r in rows),
                       capture_output=True,text=True,check=True)
    outputs=[l for l in run.stdout.splitlines() if l and not l.startswith('[')]
    if len(outputs)!=len(rows):raise SystemExit('missing probe outputs')
    bad=[]
    for r,line in zip(rows,outputs):
        actual=[int(x,16) for x in line.split()]
        if actual!=r['expected']:
            bad.append((r['call'],r['provenance'],[(i,x,y) for i,(x,y) in enumerate(zip(r['expected'],actual)) if x!=y]))
    for item in bad[:20]:print(item)
    print(f'[SPECIAL SHOT] calls={len(rows)} mismatches={len(bad)} exits={dict(Counter(r["exit_pc"] for r in rows))}')
    if bad:raise SystemExit(1)

if __name__=='__main__':main()
