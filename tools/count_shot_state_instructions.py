"""Count decoded Ghidra instruction starts, not byte lengths or exec intervals."""
import argparse
from pathlib import Path
import re

SLICES = [
    ('natural selector (already ported)', 0x86, 0xB625, 0xB6D2, False),
    ('made-shot modifier update/reset', 0x85, 0xA081, 0xA0B7, True),
    ('late-game assistance selection', 0x85, 0xA0B8, 0xA0EA, True),
    ('period assistance reset', 0x86, 0xDD80, 0xDD88, True),
    ('fatigue active-player update', 0x87, 0x98EA, 0x9969, True),
    ('fatigue 24-player recovery', 0x87, 0x996A, 0x99C2, True),
    ('timeout/period stamina grant', 0x87, 0x985D, 0x987D, True),
    ('fixed stamina grant', 0x86, 0x8468, 0x8495, True),
    ('24-player stamina initialization', 0x86, 0xDA49, 0xDA60, True),
    ('fatigue/clock cadence block', 0x85, 0xEDC6, 0xEE3D, True),
    ('fatigue timer initialization', 0x87, 0x8DF3, 0x8DF8, True),
    ('fatigue scheduler call', 0x87, 0x8EF3, 0x8EF6, True),
]

def census(directory):
    instructions={}
    for path in Path(directory).glob('shot_state_bank*.txt'):
        for line in path.read_text().splitlines():
            match=re.match(r'^.([0-9A-Fa-f]{2}):([0-9A-Fa-f]{4}) \[(\d+)\] (.+)$',line)
            if match:
                bank,pc,length,text=match.groups()
                instructions[(int(bank,16)<<16)|int(pc,16)]=(int(length),text)
    pending=set()
    rows=[]
    for name,bank,first,last,is_pending in SLICES:
        begin=(bank<<16)|first;end=(bank<<16)|last
        pcs={pc for pc in instructions if begin<=pc<=end}
        if begin not in pcs:raise ValueError(f'Missing start for {name}')
        for pc in pcs:
            length,text=instructions[pc]
            if text.startswith(('BRK','COP')):raise ValueError(f'Suspect decoding at {pc:06X}')
            if pc+length-1>end:raise ValueError(f'Truncated instruction at {pc:06X}')
        rows.append((name,f'{begin:06X}-{end:06X}',len(pcs),len(pcs) if is_pending else 0))
        if is_pending:pending.update(pcs)
    return rows,len(pending)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--listing-dir',required=True)
    rows,pending=census(p.parse_args().listing_dir)
    for row in rows:print(' | '.join(map(str,row)))
    print(f'UNIQUE PENDING BASELINE INSTRUCTIONS: {pending}')
