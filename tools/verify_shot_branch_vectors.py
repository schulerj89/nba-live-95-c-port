"""Replay stationary/sidestep and shot cancellation ROM entry/exit witnesses."""
import argparse
from collections import Counter
import json
from pathlib import Path
import subprocess
from verify_action_animation_vectors import memory, word, OFFSETS

OPS = {'86b7f7': 0, '86b867': 1, '86b890': 2, '86b86c': 3, '86b769': 4, '869d7a': 5}
EXITS = {0: {'86b84c'}, 1: {'86b86b'}, 2: {'86b8c8'},
         3: {'86b8c9', '86b88f'}, 4: {'86b867', '86b791', '86b790', '86b890'},
         5: {'869d9b'}}

def convert(v):
    a, b = memory(v['entry']), memory(v['exit'])
    op = OPS[v['entry_pc']]
    if v['exit_pc'] not in EXITS[op]:
        raise ValueError(f"invalid boundary: {v['call']} {v['entry_pc']}->{v['exit_pc']}")
    base = word(a, 0x96)
    def outputs(m):
        return ([word(m, base+o) for o in OFFSETS] +
                [word(m, base+o) for o in (0xe,0x10,0x12,0x5e,0x7e,0x60,0x28,0x64)] +
                [word(m, o) for o in (0x948,0x936,0x3ef7,0x9f6,0x968,0x3efd)])
    anchor = word(a, word(a,0x9e)+0xa) if op in (0,5) else 0
    controller = word(a,base+0x16)
    buttons = word(a,word(a,0x9a)+8) if op==3 and controller<0x8000 and not word(a,0x978) else 0
    row = [op,word(a,base+0xa8),word(a,base+0x6e),word(a,0x93a),
           int(word(a,0xc2)==word(a,0x93e))] + outputs(a) + [
           word(a,base+0x4c),word(a,base+0x8c),word(a,base+4),word(a,base+8),
           anchor,word(a,0x978),word(a,0x7f6),controller,buttons]
    decision = {'86b867':1,'86b791':0,'86b790':2,'86b890':3}.get(v['exit_pc'],0) if op==4 else 0
    if op==5: decision=word(b,base+0x4e)
    if word(a,0x7f6)!=word(b,0x7f6): raise ValueError('unexpected RNG mutation')
    # Also protect the ball's unowned fractional Z (cancellation writes integer only).
    if word(a,0x3ef5)!=word(b,0x3ef5): raise ValueError('unexpected fractional ball Z mutation')
    return dict(call=v['call'],entry_pc=v['entry_pc'],exit_pc=v['exit_pc'],
                input=row,expected=outputs(b)+[decision],
                provenance=v.get('provenance','live-gameplay'))

def main():
    p=argparse.ArgumentParser()
    for key in ('vectors','probe','pack'):p.add_argument('--'+key,required=True)
    p.add_argument('--normalized',action='store_true')
    args=p.parse_args()
    raw=Path(args.vectors).read_text()
    rows=json.loads(raw) if args.normalized else [convert(json.loads(s)) for s in raw.splitlines() if s]
    if not rows:raise SystemExit('no captured calls')
    run=subprocess.run([args.probe,args.pack],input=''.join(' '.join(f'{v:x}' for v in r['input'])+'\n' for r in rows),
                       capture_output=True,text=True,check=True)
    lines=[s for s in run.stdout.splitlines() if s and not s.startswith('[')]
    if len(lines)!=len(rows):raise SystemExit('missing output')
    bad=[]
    for r,s in zip(rows,lines):
        values=[int(v,16) for v in s.split()]
        if values!=r['expected']:bad.append((r['call'],r['entry_pc'],r['expected'],values))
    for item in bad[:6]:print(item)
    print(f'[SHOT BRANCHES] calls={len(rows)} mismatches={len(bad)} entries={dict(Counter(r["entry_pc"] for r in rows))}')
    if bad:raise SystemExit(1)

if __name__=='__main__':main()
