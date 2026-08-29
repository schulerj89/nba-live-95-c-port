"""Same-entry EAA8 child replay. Captures are emulator outputs, never fixtures made by C."""
import argparse,json,subprocess
from pathlib import Path
def main():
    p=argparse.ArgumentParser();p.add_argument('--native',required=True);p.add_argument('--probe',required=True);a=p.parse_args()
    rows=[json.loads(x) for x in Path(a.native).read_text().splitlines()]
    if not rows:raise ValueError('empty EAA8 capture')
    for r in rows:
        if len(r['input'])!=14 or len(r['output'])!=14:raise ValueError('bad EAA8 schema')
    run=subprocess.run([a.probe],input=''.join(' '.join(map(str,r['input']))+'\n' for r in rows),
                       text=True,capture_output=True,check=True)
    got=[list(map(int,x.split())) for x in run.stdout.splitlines()]
    if got!=[r['output'] for r in rows]:
        i=next(i for i,(x,y) in enumerate(zip(got,rows)) if x!=y['output'])
        raise ValueError(json.dumps({'first_mismatch':i,'native':rows[i]['output'],'port':got[i]}))
    # This natural/controlled set exercises the near-subject branch only.
    if any(r['input'][6]>=32 and r['input'][6]<32768 for r in rows):
        raise ValueError('unexpected far-branch claim without a controlled child capture')
    print(f'[REACH CHILD] {len(rows)} native EAA8 near-branch calls replay exactly; far branch remains unverified')
if __name__=='__main__':main()
