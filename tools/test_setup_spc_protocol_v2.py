"""Same parsed protocol corruptions across old/new SPC component verifiers."""
import argparse,copy,importlib.util,json,sys
from pathlib import Path
def main():
 p=argparse.ArgumentParser();p.add_argument('--kind',choices=('resident','init','control'),required=True)
 for n in ('verifier','native','rom','exe','output'):p.add_argument('--'+n,type=Path,required=True)
 a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);sys.path.insert(0,str(a.verifier.resolve().parent));spec=importlib.util.spec_from_file_location('checked_verifier',a.verifier);v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
 def invoke(name):return v.main(argparse.Namespace(native=a.native,rom=a.rom,exe=a.exe,output=a.output/name))
 invoke('baseline');runner=v.subprocess.run;checks=[]
 mutations=['bool_exit','float_exit','extra_stderr']+(['extra_stdout']if a.kind in ('init','control')else[])
 for name in mutations:
  touched=[0]
  def run(*args,**kwargs):
   r=runner(*args,**kwargs)
   if not touched[0]:
    if name=='bool_exit':r.returncode=False
    elif name=='float_exit':r.returncode=0.0
    elif name=='extra_stderr':r.stderr='ERROR\n'if kwargs.get('text')else b'ERROR\n'
    elif name=='extra_stdout':r.stdout=(r.stdout or '')+'ERROR\n'if kwargs.get('text')else(r.stdout or b'')+b'ERROR\n'
    touched[0]+=1
   return r
  v.subprocess.run=run;rejected=False;reason='accepted'
  try:invoke(name)
  except (ValueError,TypeError,AssertionError)as e:rejected=True;reason=str(e)
  finally:v.subprocess.run=runner
  checks.append({'name':name,'rejected':rejected,'touched':touched[0],'reason':reason})
 if a.kind=='resident':
  reader=v.json_lines
  for name in ('fetch_value','fetch_address','idle_address','idle_value'):
   touched=[0]
   def read(path):
    rows=copy.deepcopy(reader(path))
    if path.name=='00.jsonl':
     row=next(r for r in rows if r['kind']=='cycle'and r['bus']==(1 if name.startswith('fetch')else 4));field='address'if name.endswith('address')else'value';row[field]^=1;touched[0]+=1
    return rows
   v.json_lines=read;rejected=False;reason='accepted'
   try:invoke(name)
   except (ValueError,TypeError,AssertionError)as e:rejected=True;reason=str(e)
   finally:v.json_lines=reader
   checks.append({'name':name,'rejected':rejected,'touched':touched[0],'reason':reason})
 report={'passed':all(c['rejected']and c['touched']==1 for c in checks),'checks':checks,'verifier_sha256':v.sha(a.verifier),'test_sha256':v.sha(__file__)};(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS'if report['passed']else'REJECTED VERIFIER',[(c['name'],c['rejected'])for c in checks]);return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
