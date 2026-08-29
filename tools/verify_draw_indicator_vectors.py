import argparse,json,subprocess
from pathlib import Path
def main():
 p=argparse.ArgumentParser();p.add_argument('--vectors',required=True);p.add_argument('--probe',required=True);a=p.parse_args();f=json.loads(Path(a.vectors).read_text());calls=f['calls']
 if f['natural_calls']!=200 or f['controlled_calls']!=32 or len(calls)!=232:raise AssertionError('indicator census changed')
 payload='\n'.join(' '.join(f'{v&0xffff:x}' for v in c['input']) for c in calls)+'\n';r=subprocess.run([a.probe],input=payload,text=True,capture_output=True,check=True);actual=[[int(v,16) for v in x.split()] for x in r.stdout.splitlines()]
 bad=[(c['call'],c['expected'],x) for c,x in zip(calls,actual) if c['expected']!=x]
 resources=sorted({x[0] for x in actual});attrs=sorted({x[1] for x in actual})
 print(f"[DRAW INDICATOR] {'PASS' if not bad else 'FAIL'}: calls={len(calls)} mismatches={len(bad)} resources={resources} attrs={len(attrs)}")
 for x in bad[:10]:print('  call=%d rom=%s port=%s'%x)
 if len(actual)!=len(calls) or bad:raise SystemExit(1)
if __name__=='__main__':main()
