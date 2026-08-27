"""Native D365 state/ownership branching; tip child is independently replayed."""
import argparse,json,subprocess
from pathlib import Path
from verify_tip_contact import memory

def row(v):
    a,b=memory(v['entry']),memory(v['exit'])
    def w(m,p):
        assert m[p] is not None and m[p+1] is not None
        return m[p]|m[p+1]<<8
    fields=(0x936,0x9b8,0x946,0x9b6,0x996,0x994,0x942,0x944,0x3efd)
    inputs=[w(a,p) for p in fields];tip=int(0x86d3c6 in v['executed'])
    outputs=[tip]+(inputs if tip else [w(b,p) for p in fields])
    return dict(inputs=inputs,outputs=outputs,controlled=v.get('controlled',False),executed=v['executed'])

def main():
    p=argparse.ArgumentParser();p.add_argument('--vectors',nargs='+',required=True);p.add_argument('--probe',required=True)
    p.add_argument('--normalized',action='store_true');p.add_argument('--export');a=p.parse_args()
    if a.normalized:rows=json.loads(Path(a.vectors[0]).read_text())['witnesses']
    else:rows=[row(v) for f in a.vectors for s in Path(f).read_text().splitlines() if (v:=json.loads(s))['kind']=='completion']
    assert rows
    run=subprocess.run([a.probe],input='\n'.join(' '.join(map(str,r['inputs'])) for r in rows)+'\n',capture_output=True,text=True,check=True)
    got=[[int(x) for x in s.split()] for s in run.stdout.splitlines()]
    assert len(got)==len(rows)
    bad=[(i,r['outputs'],g) for i,(r,g) in enumerate(zip(rows,got)) if r['outputs']!=g]
    for item in bad[:8]:print(item)
    pcs={x for r in rows for x in r['executed'] if 0x86d365<=x<=0x86d3b0}
    print(f'TIP COMPLETION: {len(rows)} native calls, {len(bad)} mismatches, {len(pcs)} instruction starts')
    assert len(pcs)==28,'completion fixture lost native branch coverage'
    assert not bad
    if a.export:Path(a.export).write_text(json.dumps(dict(sources=a.vectors,witnesses=rows),separators=(',',':'))+'\n')
if __name__=='__main__':main()
