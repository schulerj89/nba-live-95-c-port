"""Replay independently captured ROM outputs, not a duplicate C algorithm."""
import argparse
import json
import subprocess
from pathlib import Path

CORE_SPANS = [(0x8591CB,0x8591DE),(0x8591FB,0x859218),(0x859230,0x8592BF),
              (0x8592CA,0x8592E3),(0x8592F9,0x85932E),(0x85932F,0x859348),(0x859352,0x8593F4)]

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--vectors',required=True,nargs='+')
    p.add_argument('--probe',required=True)
    p.add_argument('--pack',required=True)
    p.add_argument('--fixture-out')
    p.add_argument('--require-census',action='store_true')
    args=p.parse_args()
    vectors=[]
    for name in args.vectors:
        text=Path(name).read_text()
        vectors.extend(json.loads(text) if text.lstrip().startswith('[') else
                       [json.loads(line) for line in text.splitlines() if line.strip()])
    lines=[];expected=[];seen={k:set() for k in ('wrapper','stream','core')}
    for v in vectors:
        kind=v['kind'];data=v['input']+v.get('buffer_before',[])
        lines.append(' '.join(map(str,[{'wrapper':0,'stream':1,'core':2}[kind],len(data)]+data)))
        result=v['expected']+v.get('buffer_after',[])
        if kind=='stream':result += [len(v['transfers'])]+[n for tx in v['transfers'] for n in tx]
        expected.append(result)
        seen[kind].update(pc for pc in v['executed'] if kind!='core' or any(a<=pc<=b for a,b in CORE_SPANS))
    run=subprocess.run([args.probe,args.pack],input='\n'.join(lines)+'\n',text=True,capture_output=True)
    produced=[[int(n) for n in line.split()] for line in run.stdout.splitlines()
              if line and line[0].isdigit()]
    if run.returncode:raise AssertionError(f'probe exit {run.returncode}; produced {len(produced)}/{len(vectors)} rows; {run.stderr}')
    assert len(produced)==len(expected),'missing/extra outputs'
    mismatches=[(i,vectors[i]['provenance'],e,a) for i,(e,a) in enumerate(zip(expected,produced)) if e!=a]
    for row in mismatches[:4]:print('MISMATCH',row)
    print(f'Camera/presentation ROM replay: {len(vectors)} calls, {len(mismatches)} mismatches; PCs '+str({k:len(s) for k,s in seen.items()}))
    if mismatches:raise SystemExit(1)
    if args.require_census:
        assert {k:len(s) for k,s in seen.items()}=={'wrapper':78,'stream':220,'core':212},'incomplete 510-instruction witness census'
        census=json.loads((Path(__file__).resolve().parents[1]/'tests/fixtures/camera-presentation-census.json').read_text())
        assert all(seen[k]==set(census[k]) for k in seen),'witness PCs differ from decoded Ghidra census'
    if args.fixture_out:
        # Keep every controlled case plus natural witnesses that add PCs or
        # distinct transfer/exit shapes. These are original emulator outputs.
        retained=[];covered={k:set() for k in seen};shapes=set()
        for v in vectors:
            k=v['kind'];pcs=set(v['executed'])
            shape=(k,len(v.get('transfers',[])),tuple(v['expected'][:3]))
            if v['provenance'].startswith('controlled') or pcs-covered[k] or (shape not in shapes and sum(x['kind']==k and x['provenance']=='natural-ROM' for x in retained)<60):
                retained.append(v);covered[k]|=pcs;shapes.add(shape)
        # One native call per line keeps large numeric buffers reviewable
        # without expanding a 480-call fixture into ninety thousand lines.
        Path(args.fixture_out).write_text('[\n'+',\n'.join(
            json.dumps(v,separators=(',',':')) for v in retained)+'\n]\n')
        print('Durable witnesses:',len(retained))

if __name__=='__main__':main()
