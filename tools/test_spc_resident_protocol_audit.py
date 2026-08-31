"""Independent SPC protocol/domain rejection checks; original files unchanged."""
import argparse,copy,importlib.util,json,subprocess
from pathlib import Path
from unittest.mock import patch

def main():
 p=argparse.ArgumentParser()
 for key in ('verifier','native','rom','exe','output'):p.add_argument('--'+key,type=Path,required=True)
 a=p.parse_args()
 for k in vars(a):setattr(a,k,getattr(a,k).resolve())
 a.output.mkdir(parents=True,exist_ok=False)
 spec=importlib.util.spec_from_file_location('spc_independent',a.verifier);v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
 def invoke(name):return v.main(argparse.Namespace(native=a.native,rom=a.rom,exe=a.exe,output=a.output/name))
 invoke('baseline');reader=v.json_lines;runner=v.subprocess.run;checks=[]
 def test(name,context,touched):
  try:
   with context:report=invoke(name)
  except (ValueError,TypeError,KeyError,AssertionError,json.JSONDecodeError,IndexError)as e:rejected=True;reason=str(e)
  else:rejected=not report['passed'];reason='accepted'
  assert touched,'unreachable mutation: '+name
  checks.append(dict(name=name,rejected=rejected,reason=reason));print(name,rejected,flush=True)
 for name,value in [('bool_process_exit',False),('float_process_exit',0.0),('extra_stderr','ERROR\n')]:
  touched=[]
  def run(*args,**kw):
   r=runner(*args,**kw)
   if not touched:
    if name=='extra_stderr':r.stderr=value
    else:r.returncode=value
    touched.append(True)
   return r
  test(name,patch.object(v.subprocess,'run',run),touched)
 def extra_idle(rows):
  stop=rows[-1];stop['cycles']+=1
  rows.insert(-1,dict(kind='cycle',pc=stop['pc'],cycles=stop['cycles'],bus=4,address=0,value=0,end=False))
 def extra_early_end(rows):next(r for r in rows if r['kind']=='cycle')['end']=True
 for name,change in [('missing_stop',lambda r:r.pop()),('extra_terminal_idle',extra_idle),
                     ('early_instruction_end',extra_early_end),
                     ('fetch_value',lambda r:r[1].update(value=r[1]['value']^1)),
                     ('fetch_address',lambda r:r[1].update(address=r[1]['address']+1)),
                     ('idle_address',lambda r:next(x for x in r if x.get('bus')==4).update(address=65535))]:
  touched=[]
  def read(path):
   rows=reader(path)
   if Path(path).name=='00.jsonl':before=copy.deepcopy(rows);change(rows);assert rows!=before;touched.append(True)
   return rows
  test(name,patch.object(v,'json_lines',read),touched)
 original=Path.read_text;manifest=a.native/'manifest.json'
 changes=[('missing_arguments',lambda m:m.pop('arguments')),('wrong_command_route',lambda m:m.update(arguments=['wrong.exe','wrong.rom','wrong.lua'])),
          ('wrong_capture_kind',lambda m:m.update(kind='other route')),('extra_manifest_field',lambda m:m.update(unknown_field=True)),
          ('extra_source_field',lambda m:m['sources']['script'].update(extra=0)),
          ('different_script_path',lambda m:m['sources']['script'].update(path=str(v.ROOT/'tools/mesen_setup_spc_resident.lua'))),
          ('extra_artifact_field',lambda m:m['artifacts']['capture.lua'].update(extra=0))]
 for name,change in changes:
  touched=[]
  def read_text(path,*args,**kw):
   text=original(path,*args,**kw)
   if path==manifest:
    m=json.loads(text);change(m);text=json.dumps(m);touched.append(True)
   return text
  test(name,patch.object(Path,'read_text',read_text),touched)
 for name,old,new in [('nondefault_internal_speed','spc.internalSpeed=0','spc.internalSpeed=1'),('nondefault_external_speed','spc.externalSpeed=0','spc.externalSpeed=1'),('ARAM_writes_disabled','spc.writeEnabled=true','spc.writeEnabled=false')]:
  touched=[]
  def read_text(path,*args,**kw):
   text=original(path,*args,**kw)
   if path==a.native/'spc_resident_poll_entry.state':assert old in text;text=text.replace(old,new);touched.append(True)
   return text
  test(name,patch.object(Path,'read_text',read_text),touched)
 for name,filename,change in [
  ('duplicate_manifest_schema','manifest.json',lambda t:t.replace('"schema":','"schema":0,"schema":',1)),
  ('duplicate_native_pc','spc_resident_instructions.jsonl',lambda t:t.replace('"pc":','"pc":0,"pc":',1)),
  ('duplicate_C_pc','00.jsonl',lambda t:t.replace('"pc":','"pc":0,"pc":',1))]:
  touched=[]
  def read_text(path,*args,**kw):
   text=original(path,*args,**kw)
   if path.name==filename:
    changed=change(text);assert changed!=text;text=changed;touched.append(True)
   return text
  test(name,patch.object(Path,'read_text',read_text),touched)
 for name,filename,change in [
  ('unknown_initialization_tag','spc_resident_boundaries.jsonl',lambda r:r[1].update(tag='unrecognized')),
  ('CPU_port_outside_hook_range','spc_resident_cpu_ports.jsonl',lambda r:r[0].update(address=0x7e1234))]:
  touched=[]
  def read(path):
   rows=reader(path)
   if Path(path).name==filename:change(rows);touched.append(True)
   return rows
  test(name,patch.object(v,'json_lines',read),touched)
 report=dict(passed=all(c['rejected']for c in checks),checks=checks,verifier_sha256=v.sha(a.verifier))
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');return 0 if report['passed']else 1

if __name__=='__main__':raise SystemExit(main())
