"""Independent catch output/domain checks; preserve raw captures and frozen tools."""
import argparse,copy,importlib.util,json,subprocess,sys
from pathlib import Path
from unittest.mock import patch
def main():
 p=argparse.ArgumentParser()
 for key in ('verifier','capture','observation','probe','rom','output'):p.add_argument('--'+key,type=Path,required=True)
 a=p.parse_args();a.output.mkdir(exist_ok=False);sys.path.insert(0,str(a.verifier.resolve().parent))
 spec=importlib.util.spec_from_file_location('catch_independent',a.verifier);v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
 capture=a.capture.resolve();call=lambda name:v.verify(capture,a.probe.resolve(),a.rom.resolve(),a.output/(name+'.json'))
 baseline=call('baseline');assert baseline['passed']and baseline['compared_values']==31788
 stdout=(a.output/'baseline.probe-stdout.txt').read_text();stderr=(a.output/'baseline.probe-stderr.txt').read_text();lines=stdout.splitlines();first=json.loads(lines[0]);checks=[]
 def run(name,context):
  try:
   with context:r=call('case-'+str(len(checks)))
   rejected=not r['passed'];reason='C mismatch'if rejected else'accepted'
  except (ValueError,TypeError,KeyError,AssertionError,IndexError)as e:rejected=True;reason=str(e)
  checks.append(dict(name=name,rejected=rejected,reason=reason))
 for name,change in [('missing DP',lambda r:r.pop('dp_words')),('short actor vector',lambda r:r['actor_words'].pop()),('extra field',lambda r:r.update(extra=0)),('high leaf result',lambda r:r.update(result=65536)),('bool scratch',lambda r:r['dp_words'].__setitem__(0,False)),('high global',lambda r:r['global_words'].__setitem__(0,65536)),('short input words',lambda r:r['input_words'].pop())]:
  row=copy.deepcopy(first);change(row);text='\n'.join([json.dumps(row),*lines[1:]])+'\n';run(name,patch.object(v.subprocess,'run',return_value=subprocess.CompletedProcess([],0,text,stderr)))
 run('duplicate leaf key',patch.object(v.subprocess,'run',return_value=subprocess.CompletedProcess([],0,stdout.replace('"result":','"result":1,"result":',1),stderr)))
 for name,code,text in [('bool exit',False,stderr),('float exit',0.0,stderr),('extra failure',0,stderr+'ERROR\n'),('missing diagnostic',0,''),('forged diagnostic',0,'Loaded other pack\n')]:run(name,patch.object(v.subprocess,'run',return_value=subprocess.CompletedProcess([],code,stdout,text)))
 reader=Path.read_text;native=[json.loads(line)for line in reader(capture/'boundaries.jsonl').splitlines()]
 for tag in sorted({row['tag']for row in native if row['tag'].startswith('catch.')}):
  rows=copy.deepcopy(native);next(row for row in rows if row['tag']==tag)['cpu_ps']|=8;text='\n'.join(json.dumps(row)for row in rows)+'\n';touched=[]
  def read(path,*args,**kw):
   if path==capture/'boundaries.jsonl':touched.append(True);return text
   return reader(path,*args,**kw)
  run('decimal '+tag,patch.object(Path,'read_text',read));assert touched
 # Observation mode must attest the route and must never present zero C values as parity.
 with patch.object(v.subprocess,'run',side_effect=AssertionError('observation invoked C')):
  observation=v.verify(a.observation.resolve(),a.probe.resolve(),a.rom.resolve(),a.output/'observed.json',observe_only=True)
 assert observation['capture_attested']is True and observation['comparison_performed']is False and 'passed'not in observation and observation['compared_values']==0
 report=dict(passed=all(c['rejected']for c in checks),checks=checks,observation_no_C=True,verifier_sha256=v.sha(a.verifier));(a.output/'report.json').write_text(json.dumps(report,indent=2));print(len(checks),'checks',report['passed']);return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
