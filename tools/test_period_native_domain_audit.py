"""Native parsed-domain mutations, leaving every original file unchanged."""
import argparse,importlib.util,json
from pathlib import Path
from unittest.mock import patch

def main():
 p=argparse.ArgumentParser()
 for k in('verifier','native','rom','exe','output'):p.add_argument('--'+k,type=Path,required=True)
 a=p.parse_args()
 for k in vars(a):setattr(a,k,getattr(a,k).resolve())
 a.output.mkdir(parents=True,exist_ok=False);spec=importlib.util.spec_from_file_location('period_domain',a.verifier);v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
 def invoke(name):return v.main(argparse.Namespace(native=[a.native],rom=a.rom,exe=a.exe,output=a.output/name))
 invoke('baseline');checks=[];decoder=v.loads
 for tag in('formation.table','appearance.first.before','appearance.second.before','ball.initialize','cancel.before','target.before','target.after','possession.after'):
  hits=[]
  def altered(text):
   obj=decoder(text)
   if isinstance(obj,dict)and obj.get('tag')==tag and not hits:assert obj['ps']&8==0;obj['ps']|=8;hits.append(True)
   return obj
  try:
   with patch.object(v,'loads',altered):invoke('D-'+tag)
  except(ValueError,AssertionError,TypeError,KeyError)as e:rejected=True;reason=str(e)
  else:rejected=False;reason='ACCEPTED'
  assert hits;checks.append(dict(name='unsupported D at '+tag,rejected=rejected,reason=reason))
 for field,value in(('x',1),('y',0)):
  hits=[]
  def altered(text):
   obj=decoder(text)
   if isinstance(obj,dict)and obj.get('tag')=='formation.table':assert obj[field]!=value;obj[field]=value;hits.append(True)
   return obj
  try:
   with patch.object(v,'loads',altered):invoke('entry-'+field)
  except(ValueError,AssertionError,TypeError,KeyError)as e:rejected=True;reason=str(e)
  else:rejected=False;reason='ACCEPTED'
  assert hits;checks.append(dict(name='wrong source entry '+field,rejected=rejected,reason=reason))
 report=dict(passed=all(c['rejected']for c in checks),checks=checks,verifier_sha256=v.sha(a.verifier));(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(sum(c['rejected']for c in checks),'/',len(checks),'rejected');return not report['passed']
if __name__=='__main__':raise SystemExit(main())
