import argparse,json,subprocess
from pathlib import Path
def main():
 p=argparse.ArgumentParser();p.add_argument('--vectors',required=True);p.add_argument('--probe',required=True);a=p.parse_args();f=json.loads(Path(a.vectors).read_text());calls=f['calls']
 if len(calls)!=6:raise AssertionError('foul-classifier population changed')
 payload='\n'.join(' '.join(f'{v&0xffff:x}' for v in c['input']) for c in calls)+'\n'
 r=subprocess.run([a.probe],input=payload,text=True,capture_output=True,check=True);actual=[[int(v,16) for v in x.split()] for x in r.stdout.splitlines()]
 bad=[(c['call'],c['expected'],x) for c,x in zip(calls,actual) if c['expected']!=x]
 print(f"[FOUL CLASSIFIER] {'PASS' if not bad else 'FAIL'}: calls={len(calls)} mismatches={len(bad)}")
 for x in bad:print('  call=%d rom=%s port=%s'%x)
 if len(actual)!=len(calls) or bad:raise SystemExit(1)
if __name__=='__main__':main()
