"""Mutate parsed metadata after immutable file hashing; never rewrite native files."""
import argparse,copy,json,runpy
from pathlib import Path
from unittest.mock import patch
def main():
 p=argparse.ArgumentParser();p.add_argument('--checker',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 original=json.loads;results=[]
 cases=[('decimal-before','appearance.first.before','ps',8),('decimal-after','appearance.first.after','ps',8),('M8','appearance.first.before','ps',32),('X8','appearance.second.before','ps',16),('DP-nonzero','appearance.first.before','d',1),('wrong-call-PC','appearance.first.before','pc',0),('wrong-return-PC','appearance.second.after','pc',0),('wrong-SP-return','appearance.second.after','sp',0),('wrong-entry-Y','appearance.first.before','y',0),('missing-status','appearance.second.before','ps',None),('boolean-status','appearance.first.before','ps',False),('wrong-court-clock','appearance.first.after','court',0)]
 for name,tag,key,value in cases:
  hit=[0]
  def loads(s,*args,**kwargs):
   d=original(s,*args,**kwargs)
   if type(d)is dict and d.get('tag')==tag:
    d=copy.deepcopy(d);hit[0]+=1
    if value is None:d.pop(key)
    elif name.startswith('decimal'):d[key]|=value
    else:d[key]=value
   return d
  def write(path,text,*args,**kwargs):
   assert path.name=='capture-validation.json';return len(text)
  try:
   with patch('json.loads',loads),patch.object(Path,'write_text',write):runpy.run_path(str(a.checker.resolve()),run_name='__main__')
  except (AssertionError,ValueError,TypeError,KeyError) as e:accepted=False;detail=repr(e)
  else:accepted=True;detail='checker accepted changed parsed CPU/caller metadata'
  assert hit[0]>0
  results.append(dict(name=name,accepted=accepted,mutated_rows=hit[0],detail=detail))
 report=dict(passed=not any(c['accepted']for c in results),cases=results,scope='parsed-boundary validation; raw files/hash checks unchanged; original checker supplies no CPU-domain guarantee')
 (a.output/'report.json').write_text(json.dumps(report,indent=2));print(report)
if __name__=='__main__':main()
