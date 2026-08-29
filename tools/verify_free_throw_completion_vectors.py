import argparse, json, pathlib, subprocess, sys

p=argparse.ArgumentParser()
p.add_argument('--vectors',required=True)
p.add_argument('--probe',required=True)
a=p.parse_args()
doc=json.loads(pathlib.Path(a.vectors).read_text())
cases=doc.get('cases',[])
if len(cases)<8:
    raise SystemExit('[FREE THROW COMPLETION] FAIL: expected at least 8 cases')
payload=''.join(' '.join(f'{v & 0xffff:x}' for v in case['input'])+'\n' for case in cases)
run=subprocess.run([a.probe],input=payload,text=True,capture_output=True)
if run.returncode:
    raise SystemExit(f'[FREE THROW COMPLETION] FAIL: probe exit {run.returncode}: {run.stderr}')
lines=[line for line in run.stdout.splitlines() if line.strip()]
bad=[]
for case,line in zip(cases,lines):
    got=[int(x,16) for x in line.split()]
    expected=case['expected']
    if got!=expected: bad.append((case['name'],expected,got))
if len(lines)!=len(cases): bad.append(('line-count',len(cases),len(lines)))
if bad:
    for name,expected,got in bad: print(f'{name}: expected={expected} got={got}',file=sys.stderr)
    raise SystemExit(f'[FREE THROW COMPLETION] FAIL: mismatches={len(bad)}')
print(f'[FREE THROW COMPLETION] PASS: cases={len(cases)} mismatches=0')
