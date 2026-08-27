"""Replay shot-state writer inputs; expected words come only from Mesen exits."""
import argparse
from collections import Counter
import json
from pathlib import Path
import subprocess
from verify_action_animation_vectors import memory,word

KINDS={'make':0,'fatigue':1,'recovery':2,'grant':3,'fixed_grant':4,'init':5,'reset':6,'clock':7,'timer_init':8}
CLOCK=(0x936,0x926,0x928,0x92c,0x9c6,0xa04,0x9c2,0x930,0x17e1,0x13f9,0x13f7)

def convert(v,rom):
    a,b=memory(v['entry']),memory(v['exit']);op=KINDS[v['kind']]
    def state(m):
        momentum=[word(m,0x34eb+i*256+o) for o in (0xb2,0xb4,0x6e) for i in range(10)]
        momentum += [word(m,0x9c0),word(m,0x93e)]
        stamina=[word(m,0x40eb+i*64+0x18) for i in range(24)]
        seconds=[word(m,0x40eb+i*64+0x1a) for i in range(24)]
        active=[(word(m,0x3435+i*2)-0x40eb)//64 for i in range(10)]
        boosts=[word(m,0x34eb+i*256+0x72) for i in range(10)]
        ratings=[]
        for i in range(24):
            address=word(m,0x3471+i*4)|(word(m,0x3473+i*4)<<16)
            offset=((address>>16)&0x7f)*0x8000+(address&0x7fff)+0x35
            ratings.append(rom[offset] if offset<len(rom) else 0)
        return momentum+stamina+seconds+active+boosts+ratings+[
            word(m,0x9c2),word(m,0x936),word(m,0x17e7),word(m,0x17b1)]+[word(m,o) for o in CLOCK]
    amount=v['entry'].get('cpu',{}).get('a',0)
    params=[op,amount,word(a,0x9c8),word(a,0x17c1),word(a,0x928),word(a,0x4711),word(a,0x4791)]
    before=params+state(a);after=params+state(b)
    # Each typed helper owns only its named view. Other views alias the same
    # WRAM (timer, owner, live state) but are not extra helper side effects.
    expected=list(before)
    slices={0:(7,39),1:(39,135),2:(39,135),3:(39,135),4:(39,135),5:(39,63),6:(7,39),7:(135,146),8:(131,132)}
    first,last=slices[op];expected[first:last]=after[first:last]
    if op==1 and 'timer_writes' in v:
        # Mesen's write callback PC points after the writing instruction.
        # Reconstruct every timer byte, validating the intervening NMI's INC
        # independently. Only $87:9900 belongs to the fatigue helper.
        events=v['timer_writes'];timer=owned=word(a,0x9c2);i=0
        while i<len(events):
            event=events[i];pair=events[i:i+2]
            if len(pair)!=2:raise ValueError('Incomplete timer write')
            if event[2:]==[0x87,0x9903]:
                if pair!=[[0x9c2,0,0x87,0x9903],[0x9c3,0,0x87,0x9903]]:
                    raise ValueError('Unexpected fatigue timer store')
                timer=owned=0
            elif event[2]==0x85 and event[3] in (0xee12,0xee2a):
                timer=(timer+1)&65535;pc=event[3]
                if pair!=[[0x9c3,timer>>8,0x85,pc],[0x9c2,timer&255,0x85,pc]]:
                    raise ValueError('Unexpected NMI timer increment')
            else:raise ValueError(f'Unknown timer writer: {event}')
            i+=2
        if timer!=word(b,0x9c2):raise ValueError('Unexplained final timer')
        expected[131]=owned
    for pc in (0x7f6,):
        if word(a,pc)!=word(b,pc):raise ValueError('Unexpected RNG writer')
    return dict(call=v['call'],kind=v['kind'],provenance=v['provenance'],
                timer_writes=v.get('timer_writes',[]),input=before,expected=expected)

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--vectors',required=True,nargs='+')
    for n in ('probe','pack'):p.add_argument('--'+n,required=True)
    p.add_argument('--rom');p.add_argument('--normalized',action='store_true')
    p.add_argument('--write-normalized')
    p.add_argument('--require-complete',action='store_true')
    a=p.parse_args()
    if a.normalized:rows=[r for path in a.vectors for r in json.loads(Path(path).read_text())]
    else:
        rom=Path(a.rom).read_bytes()
        rows=[]
        for path in a.vectors:
            path=Path(path)
            if a.require_complete and not (path.parent/'capture_complete.txt').is_file():
                raise ValueError(f'Capture did not complete: {path}')
            for line in path.read_text().splitlines():
                if not line:continue
                row=convert(json.loads(line),rom);row['capture']=path.parent.name
                rows.append(row)
    if not rows:raise SystemExit('No shot-state calls')
    stream=''.join(' '.join(f'{v&65535:x}' for v in r['input'])+'\n' for r in rows)
    run=subprocess.run([a.probe,a.pack],input=stream,text=True,capture_output=True)
    if run.returncode:raise SystemExit(run.stderr or f'probe failed {run.returncode}')
    lines=[l for l in run.stdout.splitlines() if l and not l.startswith('[')]
    if len(lines)!=len(rows):raise SystemExit('Missing probe outputs')
    bad=[]
    for row,line in zip(rows,lines):
        actual=[int(x,16) for x in line.split()]
        changes=[(i,w,actual[i]) for i,w in enumerate(row['expected']) if (w&65535)!=actual[i]]
        if changes:bad.append((row['kind'],row['call'],row['provenance'],changes))
    for item in bad[:15]:print(item)
    print(f'[SHOT STATE] calls={len(rows)} kinds={dict(Counter(r["kind"] for r in rows))} mismatches={len(bad)}')
    if bad:raise SystemExit(1)
    if a.write_normalized:Path(a.write_normalized).write_text(json.dumps(rows,indent=2)+'\n')

if __name__=='__main__':main()
