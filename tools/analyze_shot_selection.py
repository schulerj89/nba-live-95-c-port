"""Explain actual selector outcomes without treating forced inputs as natural."""
import argparse
from collections import Counter
import json
from verify_action_animation_vectors import memory,word

def reason(v):
    lane,movement,distance,direction,facing,variant,mode,actor=v
    if not lane:return 'lane'
    if movement:return 'moving'
    if distance>=96:return 'distance'
    relative=((direction>>1)-facing)&7
    if relative<2 or relative==7:return 'facing'
    pose=0x14 if relative>=5 or (relative==4 and variant) else 0x15
    if not ((variant^1)&pose):return 'appearance'
    return 'special'

def analyze(path,rom=False):
    counts=Counter();last=0
    with open(path) as f:
        for line in f:
            row=json.loads(line)
            if rom:
                if row['entry_pc']!='86b629':continue
                a=memory(row['entry']);base=word(a,0x96)
                v=[word(a,0xaa)]+[word(a,base+o) for o in (0x4c,0x8c,0x88,0x4e,0x6c)]
                v += [17 if row['exit_pc']=='86b6d2' else 12,word(a,0xc2)]
            else:
                event=row['shot_selection']
                if event['serial']==last:continue
                if event['serial']!=last+1:raise ValueError('Dropped selector event')
                last=event['serial'];v=event['input']
            why=reason(v)
            if (why=='special')!=(v[6]==17):raise ValueError(f'Selector contract mismatch: {v}')
            counts[why]+=1
    if not counts:raise ValueError('No selector calls')
    print(f'[SELECTOR {"ROM" if rom else "C"}] calls={sum(counts.values())} gates={dict(counts)}')
    return counts

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('trace');p.add_argument('--rom',action='store_true')
    a=p.parse_args();analyze(a.trace,a.rom)
