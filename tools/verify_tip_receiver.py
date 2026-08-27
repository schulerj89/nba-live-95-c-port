"""B04C-owned outputs, excluding separately tested 99C4 launch child."""
import argparse,json,subprocess
from pathlib import Path
from verify_tip_contact import memory

def row(v):
    a,b=memory(v['entry']),memory(v['exit'])
    def w(m,p):
        assert m[p] is not None and m[p+1] is not None
        return m[p]|m[p+1]<<8
    actor=w(a,0x96);receiver=0x34eb+w(b,0x946)*256
    inputs=[w(a,0x7f6),w(a,0xc2),w(a,actor+0x6e),w(a,0x13e9)]
    outputs=[w(b,p) for p in (0x7f6,0x942,0x946,actor+0xc0,actor+0x62,receiver+0x5e,
                              0x13e9,0x140f,0x148f,0x14a7,0x1477,0x14bf)]+[b[0x1430],b[0x1448]]
    return dict(inputs=inputs,outputs=outputs,controlled=v.get('controlled',False),executed=v['executed'])

def main():
    p=argparse.ArgumentParser();p.add_argument('--vectors',nargs='+',required=True)
    p.add_argument('--probe',required=True);p.add_argument('--normalized',action='store_true');p.add_argument('--export');a=p.parse_args()
    if a.normalized: rows=json.loads(Path(a.vectors[0]).read_text())['witnesses']
    else: rows=[row(v) for f in a.vectors for s in Path(f).read_text().splitlines() if (v:=json.loads(s))['kind']=='receiver']
    assert rows
    if a.export:Path(a.export).write_text(json.dumps(dict(sources=a.vectors,witnesses=rows),separators=(',',':'))+'\n')
    result=subprocess.run([a.probe],input='\n'.join(' '.join(map(str,r['inputs'])) for r in rows)+'\n',capture_output=True,text=True,check=True)
    got=[[int(x) for x in s.split()] for s in result.stdout.splitlines()]
    assert len(got)==len(rows)
    bad=[(i,r['outputs'],g) for i,(r,g) in enumerate(zip(rows,got)) if r['outputs']!=g]
    for item in bad:print(item)
    pcs={x for r in rows for x in r['executed'] if 0x86b04c<=x<=0x86b0e1}
    print(f'TIP RECEIVER: {len(rows)} native calls, {len(bad)} mismatches, {len(pcs)}/59 instruction starts')
    assert not bad
if __name__=='__main__':main()
