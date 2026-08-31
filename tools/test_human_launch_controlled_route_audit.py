"""Controlled-multiply companion gate must also bind actual sign-return path."""
import argparse,json,sys
from pathlib import Path
from unittest.mock import patch
def main():
 p=argparse.ArgumentParser()
 for k in('verifier','probe','output'):p.add_argument('--'+k,type=Path,required=True)
 a=p.parse_args();a.verifier=a.verifier.resolve();a.probe=a.probe.resolve();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);source0=a.verifier.read_text();read0=Path.read_text;write0=Path.write_text;results=[]
 for kind in('baseline','positive-to-signed','negative-to-unsigned'):
  out=a.output/kind;out.mkdir();source=source0.replace("probe=base/'human_pass_launch_probe.exe'",f'probe=Path({str(a.probe)!r})').replace("out=base/'controlled-math-report-v7.json'",f'out=Path({str(out/"report.json")!r})');hits=[0]
  def read(path,*args,**kwargs):
   text=read0(path,*args,**kwargs);target='controlled-positive-low-ffff-v2'if kind=='positive-to-signed'else'controlled-negative-low-ffff-v2'
   if kind!='baseline'and path.name=='boundaries.jsonl'and path.parent.name==target:
    rows=[json.loads(s)for s in text.splitlines()];row=rows[-1];row['tag']='mul.exit.signed'if kind=='positive-to-signed'else'mul.exit.unsigned';row['pc']=0x85f7ae if kind=='positive-to-signed'else 0x85f820;row['cpu_pc']=row['pc']&65535;row['cpu_k']=0x85;hits[0]+=1;return '\n'.join(json.dumps(r)for r in rows)
   return text
  def write(path,text,*args,**kwargs):return write0(out/path.name,text,*args,**kwargs)
  try:
   with patch.object(Path,'read_text',read),patch.object(Path,'write_text',write):exec(compile(source,str(a.verifier),'exec'),{'__file__':str(a.verifier)})
  except (AssertionError,ValueError,KeyError,TypeError,IndexError)as e:accepted=False;detail=repr(e)
  else:accepted=True;detail='all5670controlledvalues compared'
  assert kind=='baseline'or hits[0]>0;results.append(dict(name=kind,accepted=accepted,expected=kind=='baseline',parsed_reads=hits[0],detail=detail))
 report=dict(passed=all(r['accepted']==r['expected']for r in results),cases=results,scope='controlled metadata only afterhash; all writesredirectedtoownaudit; rawsource/Cunchanged')
 (a.output/'report.json').write_text(json.dumps(report,indent=2));print(report)
if __name__=='__main__':main()
