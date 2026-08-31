"""Local reachable verifier corruptions; originals and fresh C files untouched."""
import argparse,copy,json
from pathlib import Path
from unittest.mock import patch
import verify_period_roles_v2 as v

def main(a):
 a.native=a.native.resolve();a.exe=a.exe.resolve();a.rom=a.rom.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 tests=[]
 def negative(name,manager,hits):
  directory=out/name;directory.mkdir()
  rejected=False
  try:
   with manager:v.verify_case(a.native,a.rom,a.exe,directory)
  except (ValueError,AssertionError,KeyError,TypeError,IndexError):rejected=True
  v.check(hits and rejected,name+' must reach changed value and reject');tests.append(name)
 runner=v.subprocess.run
 for name,key,value in [('status-bool','returncode',False),('status-float','returncode',0.0),('stderr','stderr','unexpected')]:
  hits=[]
  def changed(*args,**kwargs):
   result=runner(*args,**kwargs);setattr(result,key,value);hits.append(True);return result
  negative(name,patch.object(v.subprocess,'run',changed),hits)
 for name,change in [('extra-key',lambda r:r[0].update(extra=1)),('bool-kind',lambda r:r[0].update(kind=True)),('wide-word',lambda r:r[-1]['words'].__setitem__(0,65536)),('bool-word',lambda r:r[-1]['words'].__setitem__(0,False)),('missing-last',lambda r:r.pop()),('extra-last',lambda r:r.append(copy.deepcopy(r[-1]))),('record-pointer',lambda r:r[-1].update(record_pointer=1)),('calls',lambda r:r[-1].update(completed_calls=1)),('PC',lambda r:r[-1].update(pc=0)),('source-field',lambda r:r[-1]['words'].__setitem__(200,r[-1]['words'][200]^1))]:
  hits=[]
  def changed(*args,**kwargs):
   result=runner(*args,**kwargs);rows=[json.loads(l)for l in result.stdout.splitlines()];old=copy.deepcopy(rows);change(rows);assert json.dumps(old)!=json.dumps(rows);result.stdout=''.join(json.dumps(r)+'\n'for r in rows);hits.append(True);return result
  negative(name,patch.object(v.subprocess,'run',changed),hits)
 hits=[]
 def changed(*args,**kwargs):
  result=runner(*args,**kwargs);result.stdout=result.stdout.replace('"kind":1','"kind":1,"kind":1',1);hits.append(True);return result
 negative('duplicate-JSON-key',patch.object(v.subprocess,'run',changed),hits)
 reader=v.capture_contract.read_native
 for name,bit in [('native-decimal',8),('native-M',32),('native-X',16)]:
  hits=[]
  def changed(*args,**kwargs):
   m,rows=reader(*args,**kwargs);r=next(r for r in rows if r['tag']=='roles.before');r['ps']|=bit;hits.append(True);return m,rows
  negative(name,patch.object(v.capture_contract,'read_native',changed),hits)
 (out/'report.json').write_text(json.dumps(dict(passed=True,tests=tests,scope='local verifier rejection; original fixtures/executable unmodified'),indent=2)+'\n');print('PASS',len(tests),'reachable protocol corruptions')
if __name__=='__main__':
 p=argparse.ArgumentParser()
 for name in ('native','rom','exe','output'):p.add_argument('--'+name,type=Path,required=True)
 main(p.parse_args())
