"""Independent parsed/protocol mutations; immutable capture and original traces untouched."""
import argparse,copy,hashlib,importlib.util,json,subprocess,sys
from pathlib import Path
from unittest.mock import patch

def main():
 p=argparse.ArgumentParser()
 for key in ('verifier','native','previous','rom','exe','output'):p.add_argument('--'+key,type=Path,required=True)
 a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 sys.path.insert(0,str(a.verifier.resolve().parent))
 spec=importlib.util.spec_from_file_location('sound_independent',a.verifier);v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
 argv=['verify','--native',str(a.native.resolve()),'--previous-native',str(a.previous.resolve()),'--rom',str(a.rom.resolve()),'--exe',str(a.exe.resolve())]
 def invoke(name):
  with patch.object(sys,'argv',argv+['--output',str((a.output/name).resolve())]):v.main()
 invoke('baseline');reader=v.json_lines;runner=v.subprocess.run;checks=[]
 def execute(name,context,touched):
  try:
   with context:invoke(name)
  except (ValueError,TypeError,KeyError,AssertionError,json.JSONDecodeError)as e:rejected=True;reason=str(e)
  else:rejected=False;reason='accepted'
  assert touched,'unreachable mutation'
  checks.append(dict(name=name,rejected=rejected,reason=reason));print(name,rejected,flush=True)
 for name,alter in [('bool_process_exit',lambda r:setattr(r,'returncode',False)),
                    ('float_process_exit',lambda r:setattr(r,'returncode',0.0)),
                    ('extra_stderr',lambda r:setattr(r,'stderr',r.stderr+'ERROR\n')),
                    ('extra_stdout',lambda r:setattr(r,'stdout',r.stdout+'ERROR\n')),
                    ('bool_report_cycle',lambda r:setattr(r,'stdout',json.dumps(dict(json.loads(r.stdout),cycles=True))))]:
  touched=[]
  def run(*args,**kw):
   r=runner(*args,**kw)
   if not touched:alter(r);touched.append(True)
   return r
  execute(name,patch.object(v.subprocess,'run',run),touched)
 def modify_fetch_value(rows):rows[1]['value']^=1
 def modify_fetch_address(rows):rows[1]['address']+=1
 def modify_idle_address(rows):next(r for r in rows if r['kind']=='bus'and r['access']==2)['address']=0x800001
 def duplicate_json_placeholder(rows):rows[1]['end']=True
 for name,alter in [('opcode_fetch_value',modify_fetch_value),('opcode_fetch_address',modify_fetch_address),('idle_bus_address',modify_idle_address),('bool_instruction_end',duplicate_json_placeholder)]:
  touched=[]
  def read(path):
   rows=reader(path)
   if path.name=='call_01.jsonl':
    before=copy.deepcopy(rows);alter(rows);assert before!=rows or any(type(r.get('end'))is bool for r in rows);touched.append(True)
   return rows
  execute(name,patch.object(v,'json_lines',read),touched)
 report=dict(passed=all(c['rejected']for c in checks),checks=checks,verifier_sha256=hashlib.sha256(a.verifier.read_bytes()).hexdigest())
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 return 0 if report['passed']else 1

if __name__=='__main__':raise SystemExit(main())
