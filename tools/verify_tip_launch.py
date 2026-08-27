"""Independent native 99C4/9C45 launch replay, including preserved fractions."""
import argparse,json,subprocess
from pathlib import Path
from verify_tip_contact import memory

def row(v):
    a,b=memory(v['entry']),memory(v['exit'])
    def w(m,p):
        assert m[p] is not None and m[p+1] is not None,f'missing input {p:04x}'
        return m[p]|m[p+1]<<8
    actor=w(a,0x96);receiver=w(a,0x8e)
    fields=[0x3eef,0x3ef3,0x3ef7,0x3ef9,0x3efb,0x3efd]
    fields += [receiver+p for p in (4,8,0xe,0x10)]+[actor+0xc,actor+0xc0]
    fields += [actor+0x62,actor+0x30,actor+0x5e,receiver+0x5e,receiver+0x60]
    fields += [actor+0x6e,0x93a,actor+0x60,actor+0x64,actor+0x7e,actor+0x28]
    fields += [0x936,0x93e,0x94a,actor+0x5a,0x910,0xe0,0xe2,0x914,0x916,0x3eed,0x3ef1,0x3ef5]
    return dict(inputs=[w(a,p) for p in fields],outputs=[w(b,p) for p in fields],controlled=v.get('controlled',False),executed=v['executed'])

def main():
    p=argparse.ArgumentParser();p.add_argument('--vectors',nargs='+',required=True);p.add_argument('--probe',required=True)
    p.add_argument('--pack',required=True);p.add_argument('--normalized',action='store_true');p.add_argument('--export');a=p.parse_args()
    if a.normalized:rows=json.loads(Path(a.vectors[0]).read_text())['witnesses']
    else:rows=[row(v) for f in a.vectors for line in Path(f).read_text().splitlines() if (v:=json.loads(line))['kind']=='deflection']
    assert rows
    run=subprocess.run([a.probe,a.pack],input='\n'.join(' '.join(map(str,r['inputs'])) for r in rows)+'\n',capture_output=True,text=True,check=True)
    got=[[int(x) for x in s.split()] for s in run.stdout.splitlines() if not s.startswith('[')]
    assert len(got)==len(rows),(len(got),len(rows))
    bad=[(i,[(j,x,y) for j,(x,y) in enumerate(zip(r['outputs'],g)) if x!=y]) for i,(r,g) in enumerate(zip(rows,got)) if r['outputs']!=g]
    for item in bad[:15]:print(item)
    pcs={x for r in rows for x in r['executed'] if 0x8699c4<=x<=0x869c6e}
    print(f'TIP LAUNCH: {len(rows)} native calls, {len(bad)} mismatches, {len(pcs)}/306 instruction starts')
    assert not bad
    if a.export:Path(a.export).write_text(json.dumps(dict(sources=a.vectors,witnesses=rows),separators=(',',':'))+'\n')
if __name__=='__main__':main()
