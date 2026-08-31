"""Mutation checks for the deliberately closed C2 capture/probe protocol."""
import argparse,contextlib,copy,io,json,os,subprocess,sys
from pathlib import Path
from unittest.mock import patch
import verify_receiver_prepare as v

def main():
 p=argparse.ArgumentParser();p.add_argument('--left',type=Path,required=True);p.add_argument('--right',type=Path,required=True);p.add_argument('--exe',type=Path,required=True);p.add_argument('--pack',type=Path,required=True);p.add_argument('--rom',type=Path,required=True);p.add_argument('--baseline',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);a.left=a.left.resolve();records=[]
 original=v.read_json;manifest=original(a.left/'manifest.json')
 cases={
  'schema_bool':lambda m:m.update(schema=True),'schema_float':lambda m:m.update(schema=1.0),
  'state_injection':lambda m:m.update(state_injection=True),'selection_float':lambda m:m.update(selection=0.0),
  'frames':lambda m:m.update(requested_frames=6001),'pid':lambda m:m.update(pid=-1),
  'exit_bool':lambda m:m.update(exit_code=False),'extra_field':lambda m:m.update(extra=1),
  'missing_source':lambda m:m['sources'].pop('capture'),'missing_artifact':lambda m:m['artifacts'].pop('boundaries.jsonl'),
  'changed_command':lambda m:m['command'].append('--different'),
  'extra_environment':lambda m:m['environment'].update(NBA95_SEED='1'),
  'post_hash':lambda m:m['isolation'].update(post_settings_sha256='0'*64),
  'post_verified_int':lambda m:m['isolation'].update(post_settings_verified=1),
  'home':lambda m:m['isolation'].update(home='C:/other'),
  'save_attestation':lambda m:m['isolation'].update(final_saves={}),
  'completion':lambda m:m.update(completion='success')}
 for name,mutate in cases.items():
  altered=copy.deepcopy(manifest);mutate(altered)
  # Empty saves may be the genuine recorded state; use a contradictory entry.
  if name=='save_attestation'and altered==manifest:altered['isolation']['final_saves']={'forged.srm':'0'*64}
  def reader(path):return altered if path==a.left/'manifest.json'else original(path)
  rejected=False
  try:
   with patch.object(v,'read_json',side_effect=reader):v.attest(a.left,a.rom)
  except (ValueError,AssertionError):rejected=True
  records.append(dict(case=name,rejected=rejected))
 baseline_stdout=(a.baseline/'stdout.txt').read_bytes();baseline_stderr=(a.baseline/'stderr.txt').read_bytes()
 variants={
  'extra_stdout':(baseline_stdout+b'ERROR\r\n',baseline_stderr,0),
  'missing_word':(baseline_stdout.split(b'\n',1)[1],baseline_stderr,0),
  'extra_stderr':(baseline_stdout,baseline_stderr+b'ERROR\r\n',0),
  'missing_stderr':(baseline_stdout,b'',0),
  'different_pack_stderr':(baseline_stdout,baseline_stderr.replace(b'nba95_assets',b'forged_assets'),0),
  'exit_boolean':(baseline_stdout,baseline_stderr,False),
  'exit_failure':(baseline_stdout,baseline_stderr,1)}
 first=baseline_stdout.splitlines()[0].split()
 for name,replacement in [('wrong_index',b'99'),('bad_address',b'0'),('negative_word',b'-1'),('large_word',b'65536'),('float_word',b'0.0')]:
  line=first.copy();line[1 if name=='wrong_index'else 2 if name=='bad_address'else 3]=replacement
  lines=baseline_stdout.splitlines();lines[0]=b' '.join(line);variants[name]=(b'\r\n'.join(lines)+b'\r\n',baseline_stderr,0)
 for name,(stdout,stderr,code)in variants.items():
  argv=['verify_receiver_prepare.py','--capture',str(a.left),'--capture',str(a.right),'--exe',str(a.exe),'--pack',str(a.pack),'--rom',str(a.rom),'--output',str(out/name)]
  rejected=False
  try:
   with patch.object(sys,'argv',argv),patch.object(v.subprocess,'run',return_value=subprocess.CompletedProcess([],code,stdout,stderr)),contextlib.redirect_stdout(io.StringIO()):
    rejected=v.main()!=0
  except (ValueError,AssertionError):rejected=True
  records.append(dict(case=name,rejected=rejected))
 command=[str(a.exe.resolve()),str(a.pack.resolve()),str(a.rom.resolve()),str((a.baseline/'before.bin').resolve()),'--guards']
 run=subprocess.run(command,env={k:v for k,v in os.environ.items()if not k.startswith('NBA95')},capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
 (out/'guard-stdout.txt').write_bytes(run.stdout);(out/'guard-stderr.txt').write_bytes(run.stderr)
 assert run.returncode==0 and run.stdout==b'GUARDS 42\r\n'and run.stderr==baseline_stderr
 report=dict(verifier_sha256=v.sha(Path(v.__file__)),cases=records,guard_command=command,guard_count=42,passed=all(r['rejected']for r in records))
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
