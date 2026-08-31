"""Natural F78B return must follow original operand-sign control flow."""
import argparse,copy,importlib.util,json,sys
from pathlib import Path
from unittest.mock import patch
def main():
 p=argparse.ArgumentParser()
 for k in('verifier','capture','probe','rom','output'):p.add_argument('--'+k,type=Path,required=True)
 a=p.parse_args()
 for k,value in vars(a).items():setattr(a,k,value.resolve())
 a.output.mkdir(parents=True,exist_ok=False);sys.path.insert(0,str(a.verifier.parent));spec=importlib.util.spec_from_file_location('audit_launch_return',a.verifier);v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
 original=Path.read_text;raw=original(a.capture/'boundaries.jsonl');rows=[json.loads(s)for s in raw.splitlines()];results=[]
 for kind in('baseline','positive-to-signed','negative-to-unsigned'):
  changed=copy.deepcopy(rows);hits=[0]
  if kind!='baseline':
   tag='mul.exit.unsigned'if kind=='positive-to-signed'else'mul.exit.signed';row=next(r for r in changed if r['tag']==tag and r['call']==1);entry=next(r for r in changed if r['tag']=='mul.entry'and r['call']==1 and r['component']==row['component']);opposite=bool((entry['cpu_a']^entry['cpu_x'])&0x8000);assert opposite==(tag=='mul.exit.signed')
   row['tag']='mul.exit.signed'if kind=='positive-to-signed'else'mul.exit.unsigned';row['pc']=v.PCS[row['tag']];row['cpu_k']=row['pc']>>16;row['cpu_pc']=row['pc']&65535
   # Both forged endpoints have valid internally consistent status metadata.
   row['cpu_ps']=(entry['cpu_ps']&4)|((0x80 if row['cpu_a']&0x8000 else 2 if row['cpu_a']==0 else 0)if row['tag']=='mul.exit.signed'else 0)
   text='\n'.join(json.dumps(r)for r in changed)+'\n'
  def read(path,*args,**kwargs):
   if kind!='baseline'and path==a.capture/'boundaries.jsonl':hits[0]+=1;return text
   return original(path,*args,**kwargs)
  try:
   with patch.object(Path,'read_text',read):report=v.verify(a.capture,a.probe,a.rom,a.output/(kind+'.json'))
   accepted=report['passed'];detail='complete natural values compared'
  except (ValueError,AssertionError,KeyError,TypeError,IndexError)as e:accepted=False;detail=repr(e)
  assert kind=='baseline'or hits[0]>0;results.append(dict(name=kind,accepted=accepted,expected=kind=='baseline',parsed_reads=hits[0],detail=detail))
 result=dict(passed=all(r['accepted']==r['expected']for r in results),cases=results,scope='parsed native route metadata after immutable hashing; originalraw/Cunchanged; F78B CPX/CMP sign route excludes forged return')
 (a.output/'report.json').write_text(json.dumps(result,indent=2));print(result)
if __name__=='__main__':main()
