"""Exact ROM replay for held-ball phase timing/resources and latched posing."""
import argparse
from collections import Counter
import json
import re
from pathlib import Path
import subprocess
from verify_action_animation_vectors import convert as animation_convert, memory, word


def convert(v):
    a,b=memory(v['entry']),memory(v['exit'])
    base=word(a,0x96)
    if v['kind']=='animation':
        row=animation_convert(v)
        row['input'].append(word(a,base+0xb0))
        row['expected'].append(word(b,base+0xb0))
    else:
        assert v['entry_pc']=='86e4f5'
        assert v['exit_pc'] in ('86e518','86e51f','86e534','86e544')
        inputs=[7,word(a,base+0x16),word(a,0x9f6),word(a,0x968),
                word(a,base+0x8a),word(a,base+0x50),word(a,base+0x4e)]
        row={'call':v['call'],'entry_pc':v['entry_pc'],
             'input':inputs+[0]*20,'expected':[word(b,base+0x38),word(b,base+0x4e)]}
        # Selector must preserve all other actor fields, RNG and its inputs.
        for address in range(base,base+0xc0):
            if address not in (base+0x38,base+0x39,base+0x4e,base+0x4f):
                assert a[address]==b[address],(v['call'],hex(address))
        for address in (0x7f6,0x968,0x9f6):assert word(a,address)==word(b,address)
    row.update(provenance=v['provenance'],exit_pc=v['exit_pc'],executed=v.get('executed',[]))
    return row


def main():
    p=argparse.ArgumentParser()
    p.add_argument('--vectors',required=True,nargs='+')
    p.add_argument('--probe',required=True)
    p.add_argument('--pack',required=True)
    p.add_argument('--normalized',action='store_true')
    p.add_argument('--save-fixture')
    p.add_argument('--listing-dir',help='fresh Ghidra census/PC coverage check')
    args=p.parse_args()
    rows=[]
    for name in args.vectors:
        path=Path(name)
        rows.extend(json.loads(path.read_text()) if args.normalized else
                    [convert(json.loads(l)) for l in path.read_text().splitlines() if l.strip()])
    run=subprocess.run([args.probe,args.pack],input=''.join(
        ' '.join(f'{x:x}' for x in row['input'])+'\n' for row in rows),
        capture_output=True,text=True,check=True)
    lines=[l for l in run.stdout.splitlines() if l=='unsupported' or
           (len(l.split()) in (2,22) and all(len(x)==4 for x in l.split()))]
    assert len(lines)==len(rows),(len(lines),len(rows))
    bad=[];counts=Counter()
    for row,line in zip(rows,lines):
        got=None if line=='unsupported' else [int(x,16) for x in line.split()]
        if got!=row['expected']:bad.append((row['call'],row['provenance'],row['input'],row['expected'],got))
        counts[('pose' if row['input'][0]==7 else 'upper-'+str(row['input'][11]),
                row['provenance'].split(':')[0])]+=1
    for item in bad[:12]:print(item)
    print(f'[OWNER POSE ANIMATION] calls={len(rows)} mismatches={len(bad)} counts={dict(counts)}')
    if bad:raise SystemExit(1)
    if not rows:raise SystemExit('empty oracle')
    executed={int(pc,16) for row in rows for pc in row.get('executed',[])}
    spans=((0x87AE89,0x87AEBC,20),(0x87ADBE,0x87AE88,78),(0x86E4F5,0x86E544,31))
    if args.normalized or args.listing_dir:
        for lo,hi,count in spans:
            actual={pc for pc in executed if lo<=pc<=hi}
            assert len(actual)==count,(hex(lo),'missing captured instruction paths',len(actual),count)
            if args.listing_dir:
                listing=Path(args.listing_dir,f'owner_pose_bank{lo>>16:02x}.txt').read_text()
                instructions={int(bank+pc,16):int(size) for bank,pc,size in
                              re.findall(r'\$(8[67]):([0-9A-F]{4}) \[(\d+)\]',listing)}
                expected={pc for pc in instructions if lo<=pc<=hi}
                assert actual==expected,(hex(lo),'Ghidra/captured PC mismatch')
                pc=lo
                while pc<=hi:pc+=instructions[pc]
                assert pc==hi+1,'Ghidra census has a gap or truncated instruction'
        print('[OWNER POSE CENSUS] 20 + 78 + 31 = 129 instruction starts captured and replayed')
    if args.save_fixture:
        # Every controlled branch plus all naturally observed new states,
        # and a small cross-section of ordinary callers for preservation.
        selected=[];seen=Counter()
        for row in rows:
            key=row['input'][0],row['input'][11]
            if row['provenance'].startswith('controlled-ROM:') or key[0]==7 or key[1] in (13,18) or seen[key]<8:
                selected.append(row);seen[key]+=1
        Path(args.save_fixture).write_text(json.dumps(selected,indent=2)+'\n')
        print(f'Saved {len(selected)} independent ROM witnesses')


if __name__=='__main__':main()
