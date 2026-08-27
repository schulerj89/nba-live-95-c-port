"""Replay exact pose refresh and active-lineup appearance initialization.

Only the named entry/exit boundaries count. Every captured call is checked;
missing WRAM, missing roster identities, and unsupported poses fail closed.
"""
import argparse
from collections import Counter
import json
from pathlib import Path
import subprocess
import struct

from verify_action_animation_vectors import memory, word
from test_player_lab import read_pack


def convert(vector, roster):
    a, b = memory(vector['entry']), memory(vector['exit'])
    entry, exit_pc = vector['entry_pc'], vector['exit_pc']
    if entry == '87aec3' and exit_pc == '87af74':
        base = word(a, 0x96)
        if base not in range(0x34eb, 0x3eeb, 0x100):
            raise ValueError('unexpected actor address')
        values = [0] + [word(a, base+o) for o in (0x30,0x32,0x3a,0x3c,0x4e,0xa8,0x6c,0x28,0x52)]
        expected = [word(b, base+o) for o in (0x28,0x2a,0x2c,0x34,0x36,0x3e,0x40,0x52)]
        # These are deliberately NOT owned by the snapshot resolver.
        for o in (0x30,0x32,0x3a,0x3c,0x42,0x44,0x46,0x48):
            if word(a,base+o) != word(b,base+o):
                raise ValueError('snapshot unexpectedly changed animation channels')
    elif entry == '87afa2' and exit_pc == '87b054':
        values = [1]
        expected = [word(b,0x180b) | b[0x180d] << 16]
        for i in range(10):
            p = 0x3449+i*4
            pointer = word(a,p) | a[p+2]<<16
            matches = [(t,r) for t in range(29) for r in range(12)
                       if struct.unpack_from('<I',roster,24+(t*12+r)*64)[0] == pointer]
            if not matches:
                raise ValueError(f'roster pointer absent from asset pack: {pointer:06x}')
            values += list(matches[0])
            base = 0x34eb+i*0x100
            expected += [word(b,base+o) for o in (0xac,0xa8,0x6c,0x2e)] + [word(b,0x8e10+i*2)]
    else:
        raise ValueError(f'wrong entry/exit pairing: {entry}/{exit_pc}')
    return dict(call=vector['call'],entry_pc=entry,input=values,expected=expected)


def main():
    p=argparse.ArgumentParser()
    p.add_argument('--vectors',required=True)
    p.add_argument('--probe',required=True)
    p.add_argument('--pack',required=True)
    p.add_argument('--normalized',action='store_true')
    args=p.parse_args()
    raw=Path(args.vectors).read_text()
    roster=read_pack(args.pack)[251][0]
    rows=json.loads(raw) if args.normalized else [convert(json.loads(l),roster)
                                                for l in raw.splitlines() if l.strip()]
    if not rows: raise SystemExit('empty replay is not verification')
    run=subprocess.run([args.probe,args.pack],input=''.join(
        ' '.join(f'{v:x}' for v in row['input'])+'\n' for row in rows),
        text=True,capture_output=True,check=True)
    lines=[l for l in run.stdout.splitlines() if l and not l.startswith('[')]
    if len(lines)!=len(rows): raise SystemExit('missing probe outputs')
    bad=[]
    for row,line in zip(rows,lines):
        got=[int(x,16) for x in line.split()]
        if got!=row['expected']: bad.append((row['call'],row['expected'],got))
    for error in bad[:10]: print(error)
    print(f'[ACTION POSE] calls={len(rows)} mismatches={len(bad)} entries={dict(Counter(r["entry_pc"] for r in rows))}')
    if bad: raise SystemExit(1)


if __name__=='__main__': main()
