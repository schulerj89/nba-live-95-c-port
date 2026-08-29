import argparse,json,subprocess
from pathlib import Path
def main():
 p=argparse.ArgumentParser();p.add_argument('--vectors',required=True);p.add_argument('--probe',required=True);a=p.parse_args();calls=json.loads(Path(a.vectors).read_text())['calls']
 if len(calls)!=4:raise AssertionError('foul consume census changed')
 payload='\n'.join(' '.join(f'{v&0xffff:x}' for v in c['input']) for c in calls)+'\n';r=subprocess.run([a.probe],input=payload,text=True,capture_output=True,check=True);actual=[[int(v,16) for v in x.split()] for x in r.stdout.splitlines()]
 bad=[(c['call'],c['expected'],x) for c,x in zip(calls,actual) if c['expected']!=x]
 print(f"[FOUL CONSUMER] {'PASS' if not bad else 'FAIL'}: cases={len(calls)} mismatches={len(bad)}")
 for x in bad:print('  call=%d rom=%s port=%s'%x)
 if len(actual)!=len(calls) or bad:raise SystemExit(1)
if __name__=='__main__':main()
