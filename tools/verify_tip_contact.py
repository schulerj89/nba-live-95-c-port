"""Replay geometric gates using native pose-child outputs as input witnesses."""
import argparse
import json
import subprocess
from pathlib import Path


def memory(parts):
    raw=[None]*0x5000
    for base,payload in parts.items():
        start=int(base,16);data=bytes.fromhex(payload);raw[start:start+len(data)]=data
    return raw


def row(vector):
    raw=memory(vector['entry'])
    def w(a):
        assert raw[a] is not None and raw[a+1] is not None,f'missing native input {a:04x}'
        return raw[a]|raw[a+1]<<8
    actor=w(0x9a)
    fields=[actor+x for x in (0,0x5a,0x6e,0x30,0x46,0xaa)]
    fields += [0x978,0x492f,0x936,0x952,0x93e,0x946,0x93a,0x3fef,0x948]
    fields += [actor+x for x in (4,8,0xc)] + [0x3eef,0x3ef3,0x3ef7,0x3efd]
    points=vector['points'];values=[w(a) for a in fields]+[len(points)//3]+points+[0]*(6-len(points))+[0]
    expected=[{0x86cf9f:0,0x86cfa0:1,0x86d43e:2}[vector['exit_pc']],
              int(0x86cf49 in vector['executed']),int(0x86ceae in vector['executed'])]
    return values,expected


def main():
    p=argparse.ArgumentParser();p.add_argument('--vectors',required=True);p.add_argument('--probe',required=True)
    p.add_argument('--normalized',action='store_true');p.add_argument('--export');a=p.parse_args()
    if a.normalized:
        vectors=json.loads(Path(a.vectors).read_text())['witnesses']
        rows=[(v['inputs'],v['outputs']) for v in vectors]
    else:
        vectors=[json.loads(s) for s in Path(a.vectors).read_text().splitlines() if s]
        vectors=[v for v in vectors if v['kind']=='contact']
        rows=[row(v) for v in vectors]
    if a.export:
        witnesses=[dict(frame=v['frame'],controlled=v.get('controlled',False),inputs=i,outputs=o)
                   for v,(i,o) in zip(vectors,rows)]
        Path(a.export).write_text(json.dumps(dict(source=a.vectors,witnesses=witnesses),separators=(',',':'))+'\n')
    run=subprocess.run([a.probe],input='\n'.join(' '.join(map(str,r[0])) for r in rows)+'\n',text=True,capture_output=True,check=True)
    outputs=[[int(x) for x in s.split()] for s in run.stdout.splitlines()]
    assert len(outputs)==len(rows)
    errors=[(i,vectors[i]['frame'],want,got) for i,((_,want),got) in enumerate(zip(rows,outputs)) if want!=got]
    for error in errors[:12]:print('MISMATCH',error)
    print(f'TIP CONTACT: {len(rows)} native calls, {len(errors)} mismatches')
    assert not errors


if __name__=='__main__':main()
