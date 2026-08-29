import argparse,json,subprocess
from pathlib import Path
def main():
 p=argparse.ArgumentParser();p.add_argument('--native',required=True);p.add_argument('--probe',required=True);p.add_argument('--pack',required=True);a=p.parse_args()
 rows=[json.loads(x) for x in Path(a.native).read_text().splitlines()]
 if not rows or any(len(r['input'])!=14 or len(r['output'])!=14 for r in rows):raise ValueError('invalid native graphics-scratch capture')
 run=subprocess.run([a.probe,a.pack],input=''.join(' '.join(map(str,r['input']))+'\n' for r in rows),text=True,capture_output=True,check=True)
 got=[list(map(int,x.split())) for x in run.stdout.splitlines() if not x.startswith('[ASSETS]')]
 expected=[r['output'] for r in rows]
 if got!=expected:
  i=next(i for i,(x,y) in enumerate(zip(got,expected)) if x!=y);raise ValueError({'first_mismatch':i,'native':expected[i],'port':got[i]})
 print(f'[GRAPHICS SCRATCH] {len(rows)} native three-slot calls match all mutable words and shared RNG')
if __name__=='__main__':main()
