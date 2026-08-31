"""Reachable local protocol mutations, leaving native/source files untouched."""
import argparse,copy,json
from pathlib import Path
from unittest.mock import patch
import verify_period_formation as v

def main(a):
 for k,value in vars(a).items():
  if isinstance(value,Path):setattr(a,k,value.resolve())
 a.output.mkdir(parents=True,exist_ok=False);v.check_build(a.exe);tests=[]
 def bad(name,manager,hits,build=False):
  out=a.output/name;out.mkdir();rejected=False
  try:
   with manager:
    if build:v.check_build(a.exe)
    else:v.case(a.native,a.rom,a.exe,a.pack,out)
  except (ValueError,AssertionError,KeyError,TypeError,IndexError):rejected=True
  v.check(hits and rejected,name+' reachable rejection');tests.append(name)
 runner=v.subprocess.run
 for name,key,value in [('status-bool','returncode',False),('status-float','returncode',0.0),('stderr-extra','stderr','bad'),('stderr-missing','stderr',''),('stdout-type','stdout',b'')]:
  hits=[]
  def changed(*args,**kwargs):r=runner(*args,**kwargs);setattr(r,key,value);hits.append(1);return r
  bad(name,patch.object(v.subprocess,'run',changed),hits)
 fields=v.mapping();first_byte=next(i for i,(_,_,w)in enumerate(fields)if w==1);first_long=next(i for i,(_,_,w)in enumerate(fields)if w==4)
 changes=[('missing-terminal',lambda rows:rows.pop()),('extra-terminal',lambda rows:rows.append(copy.deepcopy(rows[-1]))),('reordered-boundaries',lambda rows:rows.reverse()),('wrong-PC',lambda rows:rows[0].update(pc=0)),('wrong-actor',lambda rows:rows[0].update(actor=5)),('bool-kind',lambda rows:rows[0].update(kind=True)),('extra-key',lambda rows:rows[0].update(extra=0)),('wrong-refusal',lambda rows:rows[-1].update(refusal=1)),('wrong-role',lambda rows:rows[-1].update(role_kind=3)),('missing-values',lambda rows:rows[0]['values'].pop()),('byte-wide',lambda rows:rows[0]['values'].__setitem__(first_byte,256)),('word-wide',lambda rows:rows[0]['values'].__setitem__(0,65536)),('long-wide',lambda rows:rows[0]['values'].__setitem__(first_long,4294967296)),('negative-value',lambda rows:rows[0]['values'].__setitem__(0,-1)),('bool-value',lambda rows:rows[0]['values'].__setitem__(0,False)),('owned-field',lambda rows:rows[-1]['values'].__setitem__(10,rows[-1]['values'][10]^1))]
 for name,change in changes:
  hits=[]
  def changed(*args,**kwargs):
   r=runner(*args,**kwargs);rows=[json.loads(s)for s in r.stdout.splitlines()];old=json.dumps(rows);change(rows);v.check(old!=json.dumps(rows),'mutation changes value');r.stdout=''.join(json.dumps(row)+'\n'for row in rows);hits.append(1);return r
  bad(name,patch.object(v.subprocess,'run',changed),hits)
 hits=[]
 def changed(*args,**kwargs):r=runner(*args,**kwargs);r.stdout=r.stdout.replace('"kind":1','"kind":1,"kind":1',1);hits.append(1);return r
 bad('duplicate-JSON',patch.object(v.subprocess,'run',changed),hits)
 native_reader=v.native_contract.read_native
 for name,key,value in [('native-decimal','ps',8),('native-M','ps',32),('native-X','ps',16),('native-DP','d',1),('native-PC','pc',0)]:
  hits=[]
  def changed(*args,**kwargs):
   m,rows=native_reader(*args,**kwargs);r=next(r for r in rows if r['tag']=='formation.table');r[key]=(r[key]|value)if key=='ps'else value;hits.append(1);return m,rows
  bad(name,patch.object(v.native_contract,'read_native',changed),hits)
 loader=v.loads
 for name,change in [('empty-sources',lambda m:m.update(sources={})),('missing-source',lambda m:m['sources'].pop(next(iter(m['sources'])))),('bool-build-status',lambda m:m.update(compiler_exit=False)),('extra-build-key',lambda m:m.update(extra=0)),('wrong-exe-sha',lambda m:m['executable'].update(sha256='0'*64))]:
  hits=[]
  def changed(text):
   m=loader(text)
   if type(m)is dict and 'compiler_exit'in m:change(m);hits.append(1)
   return m
  bad(name,patch.object(v,'loads',changed),hits,True)
 (a.output/'report.json').write_text(json.dumps(dict(passed=True,tests=tests,scope='local verifier failures reachable through fresh actual C/native paths; no fixtures or source mutated'),indent=2)+'\n');print('PASS',len(tests),'protocol refusals')
if __name__=='__main__':
 p=argparse.ArgumentParser()
 for n in ('rom','pack','exe','native','output'):p.add_argument('--'+n,type=Path,required=True)
 main(p.parse_args())
