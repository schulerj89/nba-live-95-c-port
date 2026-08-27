"""Replay caller-owned words AND every child-call boundary, full unlatched
channels, and idle RNG/resources. Captured child results are only an external
boundary oracle: this does not reclassify any child body as verified C."""
import argparse
from collections import Counter
import json
import re
from pathlib import Path
import subprocess

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--vectors',nargs='+',required=True)
    p.add_argument('--probe',required=True);p.add_argument('--pack',required=True)
    p.add_argument('--normalized',action='store_true');p.add_argument('--save-fixture')
    p.add_argument('--require-census',action='store_true')
    p.add_argument('--listing-dir',help='fresh DumpOwnerFlow Ghidra listing directory')
    a=p.parse_args();rows=[]
    for path in a.vectors:
        t=Path(path).read_text()
        rows.extend(json.loads(t) if a.normalized else [json.loads(l) for l in t.splitlines() if l.strip()])
    for i,r in enumerate(rows):
        if 'kind' not in r and 'dx' in r:
            assert (r['dx'],r['dy'])==(r['vx'],r['vy']),'contact inputs are not actor velocity'
            rows[i]={'kind':'contact','call':i,'provenance':'natural-ROM','entry_pc':r['entry'],
                'input':[r['dx'],r['dy']]+[0]*25,'expected':[r['dx'],r['dy'],r['facing']]+[0]*24,
                'children':[],'executed':[],'exit_pc':r['entry']+12}
    rows=[r for r in rows if r['kind']!='idle' or r['input'][2]==7]
    data=[]
    for r in rows:
        if r['kind']=='unlatched':
            if r['input'][9]&0x8000 or r['input'][10]&0x8000:
                assert r['descriptor_bank']==0x84,'rejected B3BD requires a valid descriptor bank'
            if 'actor_before' in r:
                words=(0x18,0x1a,0x30,0x32,0x38,0x3a,0x3c,0x42,0x44,0x46,0x48,0x1c,0x1e,0x20,0x22,0x24,0x26,0xb0,0x4e)
                owned={b for o in words for b in (o,o+1)}
                assert all(x==y or i in owned for i,(x,y) in enumerate(zip(r['actor_before'],r['actor_after']))),('unrepresented write',r['call'])
        v=[{'flow':0,'unlatched':1,'idle':2,'contact':3}[r['kind']]]+r['input']
        if r['kind']=='flow':
            v.append(len(r['children']))
            for c in r['children']:v.extend([c['id']]+c['input']+c['output'])
        data.append(' '.join(f'{x:x}' for x in v))
    run=subprocess.run([a.probe,a.pack],input='\n'.join(data)+'\n',capture_output=True,text=True,check=True)
    lines=[l for l in run.stdout.splitlines() if l=='unsupported' or len(l.split()) in (26,27)]
    assert len(lines)==len(rows),(len(lines),len(rows),run.stdout[:1000])
    bad=[]
    for r,line in zip(rows,lines):
        got=None if line=='unsupported' else [int(x,16) for x in line.split()]
        if got!=r['expected']:bad.append((r['kind'],r['call'],r['provenance'],[(i,x,None if got is None else got[i]) for i,x in enumerate(r['expected']) if got is None or x!=got[i]]))
    for b in bad[:20]:print(b)
    if run.stderr:print(run.stderr[:5000])
    print('[OWNER FLOW]',len(rows),'calls;',len(bad),'mismatches;',dict(Counter((r['kind'],r['provenance'].split(':')[0]) for r in rows)))
    if bad or not rows:raise SystemExit(1)
    if a.require_census:
        executed={pc for r in rows for pc in r['executed']}
        for lo,hi,n in ((0x86e545,0x86e592,31),(0x87ad86,0x87adbd,22),(0x86f34f,0x86f439,92)):
            actual={pc for pc in executed if lo<=pc<=hi}
            print(f'[OWNER CENSUS] {lo:06x}-{hi:06x}: {len(actual)}/{n}')
            assert len(actual)==n,(hex(lo),actual)
            if a.listing_dir:
                listing=Path(a.listing_dir,f'owner_flow_bank{lo>>16:02x}.txt').read_text()
                decoded={int(bank+pc,16):int(size) for bank,pc,size in
                         re.findall(r'\$(8[67]):([0-9A-F]{4}) \[(\d+)\]',listing)}
                assert actual=={pc for pc in decoded if lo<=pc<=hi},'Ghidra/capture PC disagreement'
                pc=lo
                while pc<=hi:pc+=decoded[pc]
                assert pc==hi+1,'truncated instruction or decode gap'
    if a.save_fixture:
        counts=Counter();selected=[]
        for r in rows:
            key=(r['kind'],r['input'][2] if r['kind']!='flow' else r['exit_pc'],
                 r['provenance'].split(':')[0])
            if r['provenance'].startswith('controlled-') or counts[key]<24:
                selected.append(r);counts[key]+=1
        # Preserve instruction coverage even where a later natural call is
        # the only witness of a branch.
        covered={p for r in selected for p in r['executed']}
        for r in rows:
            if set(r['executed'])-covered:selected.append(r);covered.update(r['executed'])
        Path(a.save_fixture).write_text(json.dumps(selected,indent=2)+'\n')
        print('Saved',len(selected),'ROM witnesses')
if __name__=='__main__':main()
