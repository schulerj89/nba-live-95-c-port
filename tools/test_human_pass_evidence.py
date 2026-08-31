"""Attestation rejection checks using actual pass capture files unchanged."""
import argparse,copy,json,subprocess
from pathlib import Path
from unittest.mock import patch
import verify_human_pass as verifier

def main():
 p=argparse.ArgumentParser()
 for name in('capture','probe','rom','output'):p.add_argument('--'+name,type=Path,required=True)
 a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 capture,probe,rom=a.capture.resolve(),a.probe.resolve(),a.rom.resolve()
 actual_read=verifier.read_json;original=actual_read(capture/'manifest.json')
 baseline=verifier.verify(capture,probe,rom,out/'baseline.json')
 assert baseline['passed']and baseline['calls']>0 and baseline['compared_values']>0
 (out/'baseline.json').write_text(json.dumps(baseline,indent=2)+'\n')
 checks=[]
 def manifest_check(name,change):
  altered=copy.deepcopy(original);change(altered)
  def read(path):return altered if Path(path)==capture/'manifest.json'else actual_read(path)
  rejected=False
  try:
   with patch.object(verifier,'read_json',side_effect=read):verifier.attest(capture,rom)
  except(ValueError,KeyError)as error:rejected=True;reason=str(error)
  assert rejected,name
  checks.append(dict(name=name,rejected=True,reason=reason))
 for source in('capture','runner','isolation_helper','rom','mesen'):
  manifest_check('missing source '+source,lambda m,key=source:m['sources'].pop(key))
 for artifact in('boundaries.jsonl','capture.lua','capture_complete.txt','capture_human_pass.py','mesen_portable.py',
                  'observed-script-data-folder.txt','initial-mesen-settings.json','stdout.log','stderr.log'):
  manifest_check('missing artifact '+artifact,lambda m,key=artifact:m['artifacts'].pop(key))
 manifest_check('schema float',lambda m:m.update(schema=1.0))
 manifest_check('schema bool',lambda m:m.update(schema=True))
 manifest_check('forged settings hash',lambda m:m['isolation'].update(post_settings_sha256='0'*64))
 manifest_check('missing settings hash',lambda m:m['isolation'].pop('post_settings_sha256'))
 manifest_check('forged final save hash',lambda m:m['isolation'].update(final_saves={}))
 manifest_check('missing arguments',lambda m:m.pop('arguments'))
 manifest_check('missing environment',lambda m:m.pop('environment'))
 manifest_check('wrong selection environment',lambda m:m['environment'].update(NBA95_PASS_SELECTION='1'))
 manifest_check('zero native calls',lambda m:m.update(completion=m['completion'].replace('calls='+str(baseline['calls']),'calls=0')))
 manifest_check('failed process bool',lambda m:m.update(exit_code=False))
 manifest_check('float sparse range',lambda m:m['sparse_ranges'][0].__setitem__(0,0.0))
 manifest_check('empty settings recipe',lambda m:m['isolation'].update(settings={}))
 manifest_check('missing settings key',lambda m:m['isolation']['settings']['Snes'].pop('EnableRandomPowerOnState'))
 manifest_check('boolean setting as integer',lambda m:m['isolation']['settings']['Preferences'].update(SingleInstance=0))
 manifest_check('changed observed home',lambda m:m['isolation'].update(observed_script_data_folder=str(out)))
 manifest_check('changed declared save directory',lambda m:m['isolation'].update(save_folder=str(out)))
 manifest_check('wrong runner frame bound',lambda m:m.update(requested_frames=3001))
 manifest_check('missing source version binding',lambda m:m['sources']['runner'].update(sha256='0'*64))
 stdout=(out/'baseline.probe-stdout.txt').read_text();actual_lines=stdout.splitlines()
 altered_lines=list(actual_lines);pass_index=next(i for i,line in enumerate(actual_lines)if 'route'in json.loads(line));first=json.loads(altered_lines[pass_index]);first['route']=True;altered_lines[pass_index]=json.dumps(first)
 changes=[('missing C result','\n'.join(actual_lines[:-1])+'\n'),('extra C result',stdout+actual_lines[0]+'\n'),
          ('boolean C route','\n'.join(altered_lines)+'\n')]
 metric_index=next(i for i,line in enumerate(actual_lines)if 'distance'in json.loads(line))
 mutated_metric=list(actual_lines);item=json.loads(mutated_metric[metric_index]);item['distance']=True;mutated_metric[metric_index]=json.dumps(item)
 changes.extend([('boolean distance','\n'.join(mutated_metric)+'\n'),
  ('missing metric result','\n'.join(x for i,x in enumerate(actual_lines)if i!=metric_index)+'\n'),
  ('missing all pass prefix results','\n'.join(x for x in actual_lines if 'route'not in json.loads(x))+'\n')])
 for index,(name,text)in enumerate(changes):
  rejected=False;reason=''
  try:
   with patch.object(verifier.subprocess,'run',return_value=subprocess.CompletedProcess([str(probe)],0,text,'')):
    result=verifier.verify(capture,probe,rom,out/f'mutation-{index}.json')
   rejected=not result['passed'];reason='typed C mismatch'if rejected else'accepted'
  except ValueError as error:rejected=True;reason=str(error)
  assert rejected,name;checks.append(dict(name=name,rejected=True,reason=reason))
 report=dict(passed=True,manifest_sha256=verifier.sha(capture/'manifest.json'),probe_sha256=verifier.sha(probe),
  verifier_sha256=verifier.sha(verifier.__file__),test_source_sha256=verifier.sha(__file__),checks=checks)
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(dict(passed=True,rejected_mutations=len(checks))))

if __name__=='__main__':main()
