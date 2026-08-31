"""Independent mode15 verifier mutations; immutable raw memory cache only."""
import argparse,copy,functools,importlib.util,json,subprocess,sys
from pathlib import Path
from unittest.mock import patch

def main():
 p=argparse.ArgumentParser()
 for k in('verifier','capture','probe','rom','output'):p.add_argument('--'+k,type=Path,required=True)
 a=p.parse_args()
 for k in vars(a):setattr(a,k,getattr(a,k).resolve())
 a.output.mkdir(parents=True,exist_ok=False);sys.path.insert(0,str(a.verifier.parent))
 spec=importlib.util.spec_from_file_location('release_independent',a.verifier);v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
 # Repeated projections reread the same immutable raw snapshot. Cache only
 # those bytes' decoded view, never identities, metadata, expectations or C.
 raw=v.raw
 @functools.lru_cache(maxsize=64)
 def cached(capture,name):return raw(capture,{'raw':name})
 v.raw=lambda capture,row:dict(cached(capture,row['raw']))
 invoke=lambda name:v.verify(a.capture,a.probe,a.rom,a.output/(name+'.json'))
 baseline=invoke('baseline');assert baseline['passed']and baseline['compared_values']==1071928
 stdout=(a.output/'baseline.probe-stdout.txt').read_text();stderr=(a.output/'baseline.probe-stderr.txt').read_text()
 lines=stdout.splitlines();first=json.loads(lines[0]);checks=[]
 def run(name,context):
  try:
   with context:r=invoke('case-'+str(len(checks)))
   rejected=not r['passed'];reason='C mismatch'if rejected else'accepted'
  except(ValueError,TypeError,KeyError,AssertionError,IndexError)as e:rejected=True;reason=str(e)
  checks.append(dict(name=name,rejected=rejected,reason=reason))
 for name,change in [('missing DP',lambda x:x.pop('dp_words')),('short actor',lambda x:x['actor_words'].pop()),('extra field',lambda x:x.update(extra=0)),('bool result',lambda x:x.update(result=True)),('float scratch',lambda x:x['dp_words'].__setitem__(0,float(x['dp_words'][0]))),('huge global',lambda x:x['global_words'].__setitem__(0,65536))]:
  row=copy.deepcopy(first);change(row);out='\n'.join([json.dumps(row),*lines[1:]])+'\n';run(name,patch.object(v.subprocess,'run',return_value=subprocess.CompletedProcess([],0,out,stderr)))
 for name,code,err in [('bool exit',False,stderr),('float exit',0.0,stderr),('missing stderr',0,''),('extra stderr',0,stderr+'ERROR\n')]:
  run(name,patch.object(v.subprocess,'run',return_value=subprocess.CompletedProcess([],code,stdout,err)))
 read=Path.read_text;rows=[json.loads(line)for line in read(a.capture/'boundaries.jsonl').splitlines()]
 for tag in sorted({row['tag']for row in rows[2:]}):
  altered=copy.deepcopy(rows);next(r for r in altered if r['tag']==tag)['cpu_ps']|=8;text='\n'.join(json.dumps(row)for row in altered)+'\n';hits=[]
  def changed(path,*args,**kwargs):
   if path==a.capture/'boundaries.jsonl':hits.append(True);return text
   return read(path,*args,**kwargs)
  run('D arithmetic unsupported '+tag,patch.object(Path,'read_text',changed));assert hits
 # Caller frames must remain checked even when discarded stack bytes can
 # differ due to native interrupt activity.
 for tag in ('dispatch.entry','dispatch.call','wrapper.entry','wrapper.exit','mode.return'):
  altered=copy.deepcopy(rows);row=next(r for r in altered if r['tag']==tag);row['stack'][0]^=1;text='\n'.join(json.dumps(r)for r in altered)+'\n';hits=[]
  def changed(path,*args,**kwargs):
   if path==a.capture/'boundaries.jsonl':hits.append(True);return text
   return read(path,*args,**kwargs)
  run('active stack '+tag,patch.object(Path,'read_text',changed));assert hits
 report=dict(passed=all(c['rejected']for c in checks),checks=checks,verifier_sha256=v.sha(a.verifier),raw_cache='immutable snapshots only; hashes/attestation rerun')
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(len(checks),'checks',report['passed'])
 return not report['passed']
if __name__=='__main__':raise SystemExit(main())
