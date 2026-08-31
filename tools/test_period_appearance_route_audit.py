"""Independent parsed-corpus/command-route checks; immutable raw files stay intact."""
import argparse,copy,json,runpy,types
from pathlib import Path
from unittest.mock import patch
def main():
 p=argparse.ArgumentParser();p.add_argument('--checker',type=Path,required=True);p.add_argument('--verifier',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 loads0=json.loads;results=[]
 for name,edit in [('hide-all-child-tags',lambda d:d.update(tag='appearance.hidden')),('rename-to-existing-tag',lambda d:d.update(tag='formation.entry'))]:
  hits=[0]
  def loads(s,*args,**kwargs):
   d=loads0(s,*args,**kwargs)
   if type(d)is dict and d.get('tag','').startswith('appearance.'):
    d=copy.deepcopy(d);edit(d);hits[0]+=1
   return d
  try:
   with patch('json.loads',loads),patch.object(Path,'write_text',lambda path,text,*args,**kwargs:len(text)):
    runpy.run_path(str(a.checker.resolve()),run_name='__main__')
  except(AssertionError,ValueError,TypeError,KeyError)as e:accepted=False;detail=repr(e)
  else:accepted=True;detail='zero validated appearance calls accepted'
  assert hits[0];results.append(dict(name=name,accepted=accepted,reached=hits[0],detail=detail))
 # Exercise the actual comparison loop with controlled parsed report records.
 # Native/capture checks and original SHA checks still run. Returned process
 # text is the immutable successful output, isolating route validation itself.
 source=a.verifier.read_text();prefix=source.split('\ncases=[]\n',1)[0]
 assert prefix!=source
 prefix=prefix.replace("OUT=V2/'strict-v3';OUT.mkdir(exist_ok=False)","OUT=AUDIT_OUT;OUT.mkdir(exist_ok=False)")
 owner=a.verifier.resolve().parents[2];cases_path=owner/'build/period-appearance-attribution-v1/attempt-v3/report.json';normal=loads0(cases_path.read_text())
 mutations=[('omit-39-calls',lambda d:d.update(cases=d['cases'][:1])),('duplicate-first-call-40-times',lambda d:d.update(cases=[copy.deepcopy(d['cases'][0])for _ in range(40)])),('unattested-executable',lambda d:[r['command'].__setitem__(0,'unattested-probe.exe')for r in d['cases']]),('entry-afterstate-substitution',lambda d:[r['command'].__setitem__(2,str(Path(r['command'][2]).parent/r['exit']))for r in d['cases']])]
 for name,edit in mutations:
  hit=[0];altered=copy.deepcopy(normal);edit(altered)
  def loads(s,*args,**kwargs):
   d=loads0(s,*args,**kwargs)
   if type(d)is dict and d.get('calls')==40 and type(d.get('cases'))is list and d.get('values_per_call')==130:hit[0]+=1;return copy.deepcopy(altered)
   return d
  def process(cmd,*args,**kwargs):
   case=next((c for c in normal['cases']if Path(c['command'][2]).parent==Path(cmd[2]).parent and c['command'][3]==cmd[3]),normal['cases'][0]);i=normal['cases'].index(case);directory=owner/'build/period-appearance-attribution-v1/strict-v1'
   return types.SimpleNamespace(returncode=0,stdout=(directory/f'{i}.stdout').read_text(),stderr=(directory/f'{i}.stderr').read_text())
  originalwrite=Path.write_text
  def write(path,text,*args,**kwargs):
   if path.name=='capture-validation.json':return len(text)
   return originalwrite(path,text,*args,**kwargs)
  try:
   with patch('json.loads',loads),patch('subprocess.run',process),patch.object(Path,'write_text',write):
    env={'__file__':str(a.verifier.resolve()),'AUDIT_OUT':a.output/name};exec(compile(prefix,str(a.verifier),'exec'),env)
  except(AssertionError,ValueError,TypeError,KeyError)as e:accepted=False;detail=repr(e)
  else:accepted=True;detail=f"comparison accepted {env['count']} words"
  assert hit[0];results.append(dict(name=name,accepted=accepted,reached=hit[0],detail=detail))
 report=dict(passed=not any(r['accepted']for r in results),cases=results,scope='parsed after-hash metadata and C-route fault injection; unchanged fixtures, no permission or filesystem bypass claim')
 (a.output/'report.json').write_text(json.dumps(report,indent=2));print(report)
if __name__=='__main__':main()
