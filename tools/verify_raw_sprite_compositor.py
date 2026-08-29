import argparse,json,subprocess
from pathlib import Path
def main():
 p=argparse.ArgumentParser();p.add_argument('--vectors',required=True);p.add_argument('--probe',required=True);p.add_argument('--assets',required=True);a=p.parse_args();calls=json.loads(Path(a.vectors).read_text())['calls']
 if len(calls)!=2000:raise AssertionError('raw sprite call census changed')
 payload='\n'.join(' '.join(f'{v:x}' for v in c['input']) for c in calls)+'\n';r=subprocess.run([a.probe,a.assets],input=payload,text=True,capture_output=True,check=True)
 lines=[line for line in r.stdout.splitlines() if not line.startswith('[')]
 actual=[]
 for line in lines:
  q=line.split();actual.append([[int(q[i][0:4],16),int(q[i][4:6],16),int(q[i][6:8],16),int(q[i][8:10],16),int(q[i][10:12],16)] for i in range(1,len(q))])
 supported=[(c,x) for c,x,line in zip(calls,actual,lines) if line.strip()!='-']
 def portable(entries):return [[e[0],e[1],e[3]&0xfe,e[4]] for e in entries]
 bad=[(c['call'],c['input'],portable(c['expected']),portable(x)) for c,x in supported if portable(c['expected'])!=portable(x)]
 resources={c['input'][0] for c,_ in supported};flips={bool(c['input'][1]&0x4000) for c,_ in supported}
 print(f'[RAW SPRITE CENSUS] supported={len(supported)} resources={len(resources)} flips={flips}')
 if len(supported)!=2000 or len(resources)<6 or flips!={False,True} or len({tuple(c['input']) for c in calls})<40:raise AssertionError('native sprite-call diversity collapsed')
 print(f"[RAW SPRITE COMPOSITOR] {'PASS' if not bad else 'FAIL'}: native_calls={len(calls)} supported={len(supported)} entries={sum(len(x) for _,x in supported)} resources={len(resources)} flips={len(flips)} mismatches={len(bad)}")
 for q in bad[:8]:print(' ',q)
 if len(actual)!=len(calls) or bad:raise SystemExit(1)
if __name__=='__main__':main()
