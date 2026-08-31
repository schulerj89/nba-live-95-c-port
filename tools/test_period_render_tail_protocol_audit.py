"""Exercise the frozen render verifier's actual comparison loop after hashing."""
import argparse,copy,json,re,subprocess,sys
from pathlib import Path
from unittest.mock import patch
def main():
 p=argparse.ArgumentParser();p.add_argument('--verifier',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);a.verifier=a.verifier.resolve();owner=a.verifier.parents[2]
 source=a.verifier.read_text().split('\nresult,raw,command=first;rejected=[]',1)[0]
 source,n=re.subn(r"OUT=BASE/'strict-v\d+';OUT.mkdir\(exist_ok=False\)","OUT=AUDIT_OUT;OUT.mkdir(exist_ok=False)",source);assert n==1
 read0=Path.read_text;run0=subprocess.run;results=[]
 tests=['baseline','duplicate-before','duplicate-after','reverse-tail','nonadjacent-before','wrong-before-pc','wrong-after-sp','decimal-before','field-not-raw','wrong-clock']
 for kind in tests:
  hits=[0];calls=[]
  def read(path,*args,**kwargs):
   text=read0(path,*args,**kwargs)
   if path.name!='boundaries.jsonl' or 'period-0-ready1-children-v2'not in str(path) or kind=='baseline':return text
   rows=[json.loads(line)for line in text.splitlines()];b=next(i for i,r in enumerate(rows)if r['tag']=='roles.after');e=next(i for i,r in enumerate(rows)if r['tag']=='formation.return');hits[0]+=1
   if kind=='duplicate-before':rows.insert(b,copy.deepcopy(rows[b]))
   elif kind=='duplicate-after':rows.insert(e,copy.deepcopy(rows[e]))
   elif kind=='reverse-tail':rows[b],rows[e]=rows[e],rows[b]
   elif kind=='nonadjacent-before':rows.insert(e,copy.deepcopy(next(r for r in rows if r['tag']=='roles.before')))
   elif kind=='wrong-before-pc':rows[b]['pc']=0
   elif kind=='wrong-after-sp':rows[e]['sp']^=1
   elif kind=='decimal-before':rows[b]['ps']|=8
   elif kind=='field-not-raw':rows[b]['fields']['093e']^=1
   elif kind=='wrong-clock':rows[b]['frame']+=1;rows[b]['court']+=1
   return '\n'.join(json.dumps(r)for r in rows)
  def run(cmd,*args,**kwargs):
   i=len(calls);directory=owner/'build/period-restart-attribution-v1'/f'period-{i}-ready1-children-v{3 if i==3 else 2}';rows=[json.loads(s)for s in read0(directory/'boundaries.jsonl').splitlines()];before=next(r for r in rows if r['tag']=='roles.after')
   assert cmd[:2]==[str(owner/'build/period-render-tail-attribution-v1/attempt-v2/probe.exe'),str(directory/before['raw'])];calls.append(cmd);return run0(cmd,*args,**kwargs)
  try:
   with patch.object(Path,'read_text',read),patch('subprocess.run',run):exec(compile(source,str(a.verifier),'exec'),{'__file__':str(a.verifier),'AUDIT_OUT':a.output/kind})
  except (AssertionError,ValueError,KeyError,TypeError,StopIteration)as e:accepted=False;detail=repr(e)
  else:accepted=True;detail=f'{len(calls)} complete canonical C comparisons'
  assert kind=='baseline'or hits[0]>0
  results.append(dict(name=kind,accepted=accepted,expected=kind=='baseline',mutated_reads=hits[0],calls=len(calls),detail=detail))
 report=dict(passed=all(r['accepted']==r['expected']for r in results),cases=results)
 (a.output/'report.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
if __name__=='__main__':main()
