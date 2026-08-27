"""Compare every owned channel word against live Mesen entry/exit calls."""
import argparse
from collections import Counter
import json
from pathlib import Path
import subprocess

OFFSETS = (0x18,0x1a,0x30,0x32,0x38,0x3a,0x3c,0x42,0x44,0x46,0x48,
           0x1c,0x1e,0x20,0x22,0x24,0x26)
COMMANDS = {'87b3bd':0,'87b47a':1,'87b4db':2,'87b538':3,'87b555':4,'87b37c':5}
EXITS = {0:{'87b459','87b464','87b479'},1:{'87b4c0','87b4ce','87b4da'},
         2:{'87b52b','87b537'},3:{'87b554'},4:{'87b571'},5:{'87b3bc'},
         6:{'87ac98','87ad5a'}}

def memory(snapshot):
    return {int(base,16)+i:b for base,payload in snapshot['mem'].items()
            for i,b in enumerate(bytes.fromhex(payload))}

def word(mem, address):
    # Missing bytes must fail, not turn incomplete captures into zero inputs.
    return mem[address] | mem[address+1] << 8

def convert(v):
    a,b=memory(v['entry']),memory(v['exit'])
    base=word(a,0x96)
    if v['entry_pc'] not in COMMANDS and v['entry_pc'] != '87ab38':
        raise ValueError(f"unsupported capture entry: {v['entry_pc']}")
    op=COMMANDS.get(v['entry_pc'],6)
    if v['exit_pc'] not in EXITS[op]:
        raise ValueError(f"wrong entry/exit pairing: {v['call']}")
    read=lambda m: [word(m,base+o) for o in OFFSETS]
    if op==6:
        dt=word(a,0xc6); delta=((dt << 8)|(dt >> 8)) & 0xffff
        row=[op,0,0,word(a,base+0xa8),word(a,base+0x52),word(a,base+0x4a),
             delta,word(a,base+0x6c),word(a,0x7f6)]+read(a)
        want=read(b)+[0,word(b,0x7f6),word(b,base+0x2a),word(b,base+0x2c)]
    else:
        row=[op,word(a,0),word(a,base+0x72),word(a,base+0xa8),0,0,0,0,0]+read(a)
        want=read(b)+[word(b,0),0,0,0]
    return {'call':v['call'],'entry_pc':v['entry_pc'],'input':row,'expected':want}

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--vectors',required=True)
    p.add_argument('--probe',required=True)
    p.add_argument('--pack',required=True)
    p.add_argument('--normalized',action='store_true')
    args=p.parse_args()
    path=Path(args.vectors)
    rows=json.loads(path.read_text()) if args.normalized else [convert(json.loads(l))
        for l in path.read_text().splitlines() if l.strip()]
    run=subprocess.run([args.probe,args.pack],input=''.join(
        ' '.join(f'{x:x}' for x in row['input'])+'\n' for row in rows),
        capture_output=True,text=True,check=True)
    lines=[l for l in run.stdout.splitlines() if l=='unsupported' or
           (len(l.split())==21 and all(len(x)==4 for x in l.split()))]
    if len(lines)!=len(rows): raise SystemExit('missing probe output')
    bad=[]; excluded=[]; counts=Counter(); changed=0
    for row,line in zip(rows,lines):
        if line=='unsupported':
            # Explicitly defer upper mode-2 states other than idle state 7.
            excluded.append((row['call'],row['input'][11],row['input'][12]))
            continue
        got=[int(x,16) for x in line.split()]
        if got!=row['expected']: bad.append((row['call'],row['expected'],got))
        counts[row['entry_pc']]+=1
        changed+=row['input'][9:26] != row['expected'][:17]
    for item in bad[:10]: print(item)
    print(f'[ACTION ANIMATION] checked={sum(counts.values())} changed={changed} '
          f'mismatches={len(bad)} unsupported={len(excluded)} entries={dict(counts)}')
    if excluded: print('Excluded states:',dict(Counter((u,l) for _,u,l in excluded)))
    if bad: raise SystemExit(1)
    if args.normalized and excluded: raise SystemExit('durable witnesses must all be supported')

if __name__=='__main__': main()
