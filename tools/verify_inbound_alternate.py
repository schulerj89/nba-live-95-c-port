import argparse,json,subprocess
from pathlib import Path
def main():
 p=argparse.ArgumentParser();p.add_argument('--vectors',required=True);p.add_argument('--probe',required=True);a=p.parse_args();calls=json.loads(Path(a.vectors).read_text())['calls']
 if len(calls)!=2 or {c['exit_pc'].lower() for c in calls}!={'86f60b','86f653'}:raise AssertionError('alternate selector branches changed')
 payload='\n'.join(' '.join(f'{v:x}' for v in c['input']) for c in calls)+'\n';r=subprocess.run([a.probe],input=payload,text=True,capture_output=True,check=True);actual=[int(x,16) for x in r.stdout.splitlines()]
 bad=[(c['call'],c['expected'],x) for c,x in zip(calls,actual) if c['expected']!=x]
 print(f"[INBOUND ALTERNATE] {'PASS' if not bad else 'FAIL'}: cases={len(calls)} second_selector=1 timeout_fallback=1 mismatches={len(bad)}")
 if len(actual)!=len(calls) or bad:raise SystemExit(1)
if __name__=='__main__':main()
