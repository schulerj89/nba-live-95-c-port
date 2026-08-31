"""Independent return output/domain mutations; no original evidence writes."""
import argparse,copy,importlib.util,json,subprocess,sys
from pathlib import Path
from unittest.mock import patch
def main():
 p=argparse.ArgumentParser()
 for key in ('verifier','capture','probe','rom','output'):p.add_argument('--'+key,type=Path,required=True)
 a=p.parse_args();a.output.mkdir(exist_ok=False);sys.path.insert(0,str(a.verifier.resolve().parent))
 spec=importlib.util.spec_from_file_location('return_independent',a.verifier);v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v);capture=a.capture.resolve()
 call=lambda name:v.verify(capture,a.probe.resolve(),a.rom.resolve(),a.output/(name+'.json'))
 baseline=call('baseline');assert baseline['passed']and baseline['compared_values']==121024
 stdout=(a.output/'baseline.probe-stdout.txt').read_text();stderr=(a.output/'baseline.probe-stderr.txt').read_text();assert stderr==''
 lines=stdout.splitlines();first=json.loads(lines[0]);checks=[]
 def run(name,context):
  try:
   with context:r=call('case-'+str(len(checks)))
   rejected=not r['passed'];reason='C mismatch'if rejected else'accepted'
  except (ValueError,TypeError,KeyError,AssertionError,IndexError)as e:rejected=True;reason=str(e)
  checks.append(dict(name=name,rejected=rejected,reason=reason))
 for name,change in [('missing DP',lambda r:r.pop('dp_words')),('short actor vector',lambda r:r['actor_words'].pop()),('extra field',lambda r:r.update(extra=0)),('high leaf result',lambda r:r.update(result=65536)),('bool scratch',lambda r:r['dp_words'].__setitem__(0,False)),('high global',lambda r:r['global_words'].__setitem__(0,65536)),('short saved words',lambda r:r['saved_words'].pop()),('bool saved word',lambda r:r['saved_words'].__setitem__(0,False))]:
  row=copy.deepcopy(first);change(row);text='\n'.join([json.dumps(row),*lines[1:]])+'\n';run(name,patch.object(v.subprocess,'run',return_value=subprocess.CompletedProcess([],0,text,stderr)))
 run('duplicate result key',patch.object(v.subprocess,'run',return_value=subprocess.CompletedProcess([],0,stdout.replace('"result":','"result":1,"result":',1),stderr)))
 for name,code,text in [('bool exit',False,''),('float exit',0.0,''),('failure stderr',0,'ERROR\n'),('blank stderr',0,'\n'),('asset loader stderr',0,'[ASSETS] Loaded asset pack\n')]:run(name,patch.object(v.subprocess,'run',return_value=subprocess.CompletedProcess([],code,stdout,text)))
 reader=Path.read_text;native=[json.loads(line)for line in reader(capture/'boundaries.jsonl').splitlines()]
 for tag in sorted({row['tag']for row in native[2:]}):
  rows=copy.deepcopy(native);next(row for row in rows if row['tag']==tag)['cpu_ps']|=8;text='\n'.join(json.dumps(row)for row in rows)+'\n';touched=[]
  def read(path,*args,**kw):
   if path==capture/'boundaries.jsonl':touched.append(True);return text
   return reader(path,*args,**kw)
  run('unsupported route D '+tag,patch.object(Path,'read_text',read));assert touched
 report=dict(passed=all(c['rejected']for c in checks),checks=checks,verifier_sha256=v.sha(a.verifier));(a.output/'report.json').write_text(json.dumps(report,indent=2));print(len(checks),'checks',report['passed']);return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
