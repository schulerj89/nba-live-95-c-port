"""A captured 8F F1 direct write requires SPC PS.P=0; commit API has no PS."""
import argparse,importlib.util,json,sys
from pathlib import Path
from unittest.mock import patch
def main():
 p=argparse.ArgumentParser()
 for k in('verifier','native','rom','exe','output'):p.add_argument('--'+k,type=Path,required=True)
 a=p.parse_args()
 for k,v in vars(a).items():setattr(a,k,v.resolve())
 a.output.mkdir(parents=True,exist_ok=False);sys.path.insert(0,str(a.verifier.parent));s=importlib.util.spec_from_file_location('control_audit',a.verifier);v=importlib.util.module_from_spec(s);s.loader.exec_module(v)
 def run(name):return v.main(argparse.Namespace(native=a.native,rom=a.rom,exe=a.exe,output=a.output/name))
 run('baseline');rows=[];original=v.state
 for publication in(1,2):
  hits=[]
  def state(path):
   result=original(path)
   if path.name.startswith(f'spc_control_{publication}_'):result['spc.ps']=str(int(result['spc.ps'])|32);hits.append(path.name)
   return result
  try:
   with patch.object(v,'state',state):run(f'publication{publication}-directpage1')
  except(ValueError,AssertionError,KeyError,TypeError)as e:rejected=True;detail=str(e)
  else:rejected=False;detail='accepted F1 callback with PS.P=1 (source addresses01F1)'
  assert hits;rows.append(dict(publication=publication,rejected=rejected,hits=hits,detail=detail))
 report=dict(passed=all(r['rejected']for r in rows),cases=rows,scope='parsed CPU callback metadata only; raw/native/source files unchanged; F1 commit C API not restricted')
 (a.output/'report.json').write_text(json.dumps(report,indent=2));print(report)
if __name__=='__main__':main()
