"""Replay native camera state, never derive expected outputs from the C port."""
import argparse,json,re,subprocess
from pathlib import Path

SLICES=[(0x858B98,0x858BBE),(0x859192,0x8591CA),(0x859349,0x859351),
        (0x8591DF,0x8591FA),(0x859219,0x85922F),(0x8592C0,0x8592C9),
        (0x8592E4,0x8592F8),(0x87A9D0,0x87A9E2),(0x8795AC,0x8795BA),(0x8795BB,0x8795DE)]

def main():
    p=argparse.ArgumentParser();p.add_argument('--vectors',nargs='+',required=True);p.add_argument('--probe',required=True)
    p.add_argument('--ghidra');p.add_argument('--fixture-out');p.add_argument('--require-census',action='store_true');a=p.parse_args()
    root=Path(__file__).resolve().parents[1];vectors=[]
    for path in a.vectors:
        text=Path(path).read_text();vectors.extend(json.loads(text) if text.lstrip().startswith('[') else [json.loads(s) for s in text.splitlines() if s])
    kinds={'core':0,'init':1,'resolve':2,'copy':3,'cadence':4}
    rows=[' '.join(map(str,[kinds[v['kind']],len(v['input'])]+v['input'])) for v in vectors]
    run=subprocess.run([a.probe],input='\n'.join(rows)+'\n',text=True,capture_output=True)
    assert run.returncode==0,(run.returncode,run.stderr)
    results=[[int(n) for n in line.split()] for line in run.stdout.splitlines()]
    assert len(results)==len(vectors),'missing probe output'
    errors=[(i,v['kind'],v['frame'],v['input'],v['expected'],r) for i,(v,r) in enumerate(zip(vectors,results)) if r!=v['expected']]
    for e in errors[:6]:print('MISMATCH',e)
    pcs={pc for v in vectors for pc in v['executed']};requested={pc for pc in pcs if any(lo<=pc<=hi for lo,hi in SLICES)}
    print(f'CAMERA HANDOFF: {len(vectors)} native calls, {len(errors)} mismatches; requested PCs={len(requested)}/99; kinds='+str({k:sum(v['kind']==k for v in vectors) for k in kinds}))
    assert not errors,'ROM output mismatch'
    census_path=root/'tests/fixtures/camera-handoff-census.json'
    if a.ghidra:
        decoded=set()
        for f in Path(a.ghidra).glob('camera_bank*.txt'):
            decoded.update(int(b+x,16) for b,x in re.findall(r'^\$([0-9A-F]{2}):([0-9A-F]{4}) ',f.read_text(),re.M))
        census=sorted(pc for pc in decoded if any(lo<=pc<=hi for lo,hi in SLICES))
        assert len(census)==99
        if a.fixture_out:census_path.write_text(json.dumps(census)+'\n')
    else:census=json.loads(census_path.read_text())
    if a.require_census:assert requested==set(census),'incomplete requested-PC coverage'
    if a.fixture_out:
        kept=[];seen=set();per_kind={k:0 for k in kinds}
        for v in vectors:
            new=set(v['executed'])-seen;k=v['kind']
            early=(k=='core' and v['frame']<=360) or (k in ('copy','cadence') and 195<=v['frame']<=225)
            if v['controlled'] or early or new or per_kind[k]<40:
                kept.append(v);seen.update(v['executed']);per_kind[k]+=1
        Path(a.fixture_out).write_text('[\n'+',\n'.join(json.dumps(v,separators=(',',':')) for v in kept)+'\n]\n')
        print(f'Retained {len(kept)} independent witnesses')

if __name__=='__main__':main()
