"""Adaptive full-comparison gate test: derive commands, never trust old reports."""
import argparse,copy,json,re,struct,subprocess,sys
from pathlib import Path
from unittest.mock import patch
def main():
 p=argparse.ArgumentParser();p.add_argument('--verifier',type=Path,required=True);p.add_argument('--component',choices=('appearance','support'),required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);a.verifier=a.verifier.resolve();a.output=a.output.resolve();owner=a.verifier.parents[2]
 text=a.verifier.read_text();marker='\ncases=[]\n'if a.component=='appearance'else'\nr,actual,row,entry,pc,mode=first;negative=[]\n';prefix=text.split(marker,1)[0];assert len(prefix)<len(text)
 prefix,n=re.subn(r"OUT=(V2|BASE)/'strict-v\d+';OUT.mkdir\(exist_ok=False\)","OUT=AUDIT_OUT;OUT.mkdir(exist_ok=False)",prefix);assert n==1
 sys.path.insert(0,str(a.verifier.parent));oldloads=json.loads;oldread=Path.read_text;oldwrite=Path.write_text;run0=subprocess.run
 canonical=[]
 for period in range(4):
  d=owner/'build/period-restart-attribution-v1'/f'period-{period}-ready1-children-v{3 if period==3 else 2}';rows=[json.loads(s)for s in(d/'boundaries.jsonl').read_text().splitlines()]
  if a.component=='appearance':
   for row in rows:
    if row['tag'] in('appearance.first.before','appearance.second.before'):
     actor=(struct.unpack_from('<H',(d/row['raw']).read_bytes(),0x96)[0]-0x34eb)//256;canonical.append([str(owner/'build/period-appearance-attribution-v1/attempt-v3/probe.exe'),str(owner/'build/full-extraction-v1/nba95_assets.pak'),str(d/row['raw']),str(actor)])
  else:
   for mode,tag in [('assignment','assignment.before'),('sort','assignment.after'),('attachment','possession.after')]:
    if period==3 and mode=='attachment':continue
    row=next(r for r in rows if r['tag']==tag);canonical.append([str(owner/'build/period-support-attribution-v1/attempt-v2/probe.exe'),str(owner/'build/full-extraction-v1/nba95_assets.pak'),str(d/row['raw']),mode])
 target='appearance.first.before'if a.component=='appearance'else'assignment.before';results=[]
 tests=[('baseline-old-report-unread',None),('hide-children','hide'),('rename-children','rename'),('extra-metadata','extra'),('wrong-field-value','field'),('wrong-raw-path','path'),('wrong-court-clock','clock'),('decimal-entry','decimal')]
 for name,kind in tests:
  calls=[];hits=[0]
  def edit(row):
   if kind in('hide','rename')and row.get('tag','').startswith('appearance.'):
    row['tag']='appearance.hidden'if kind=='hide'else'formation.entry';hits[0]+=1
   elif row.get('tag')==target and kind:
    hits[0]+=1
    if kind=='extra':row['extra']=0
    elif kind=='field':row['fields']['093e']^=1
    elif kind=='path':row['raw']='../outside.bin'
    elif kind=='clock':row['court']=0
    elif kind=='decimal':row['ps']|=8
   return row
  def loads(s,*args,**kwargs):
   d=oldloads(s,*args,**kwargs)
   if type(d)is dict and 'tag'in d:d=edit(copy.deepcopy(d))
   return d
  def read(path,*args,**kwargs):
   if path.name=='report.json'and path.parent.name in('attempt-v2','attempt-v3'):raise AssertionError('old C report consumed')
   return oldread(path,*args,**kwargs)
  def write(path,txt,*args,**kwargs):
   if path.name=='capture-validation.json':return oldwrite(a.output/f'{name}-capture-validation.json',txt,*args,**kwargs)
   assert path.is_relative_to(a.output),path;return oldwrite(path,txt,*args,**kwargs)
  def run(cmd,*args,**kwargs):
   index=len(calls);assert index<len(canonical);assert cmd[:4]==canonical[index],(index,cmd,canonical[index]);calls.append(cmd)
   if kind:raise RuntimeError('invalid input reached C')
   return run0(cmd,*args,**kwargs)
  try:
   with patch('json.loads',loads),patch.object(Path,'read_text',read),patch.object(Path,'write_text',write),patch('subprocess.run',run):
    env={'__file__':str(a.verifier),'AUDIT_OUT':a.output/name};exec(compile(prefix,str(a.verifier),'exec'),env)
  except (AssertionError,ValueError,KeyError,TypeError)as e:accepted=False;detail=repr(e)
  except RuntimeError as e:accepted=True;detail=str(e)
  else:accepted=True;detail=f'{len(calls)} canonical calls; old report never read'
  if kind:assert hits[0]>0
  else:assert accepted and len(calls)==len(canonical),detail
  results.append(dict(name=name,accepted=accepted,expected_accept=kind is None,mutations=hits[0],calls=len(calls),detail=detail))
 report=dict(passed=all(r['accepted']==r['expected_accept']for r in results),cases=results,scope='actual verifier comparison path; old report access forbidden; C commands independently derived and checked; bad parsed inputs must fail before C')
 (a.output/'report.json').write_text(json.dumps(report,indent=2));print(report)
if __name__=='__main__':main()
