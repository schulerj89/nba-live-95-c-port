import argparse,json,subprocess
from pathlib import Path
def main():
 p=argparse.ArgumentParser();p.add_argument('--vectors',required=True);p.add_argument('--probe',required=True);a=p.parse_args();calls=json.loads(Path(a.vectors).read_text())['calls']
 if len(calls)!=2 or {c['input'][1] for c in calls}!={2,65535}:raise AssertionError('dead-ball reset census changed')
 payload='\n'.join(' '.join(f'{v:x}' for v in c['input']) for c in calls)+'\n';r=subprocess.run([a.probe],input=payload,text=True,capture_output=True,check=True);actual=[[int(v,16) for v in line.split()] for line in r.stdout.splitlines()]
 bad=[(c['call'],c['expected'],x) for c,x in zip(calls,actual) if c['expected']!=x]
 print(f"[DEAD BALL RESET] {'PASS' if not bad else 'FAIL'}: cases={len(calls)} outputs={sum(map(len,actual))} mismatches={len(bad)}")
 for x in bad:print(' ',x)
 if len(actual)!=len(calls) or bad:raise SystemExit(1)
if __name__=='__main__':main()
