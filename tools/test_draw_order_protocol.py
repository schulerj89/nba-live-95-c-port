"""Reachable parsed-evidence corruption checks; original files stay immutable."""
import argparse,copy,json,subprocess
from pathlib import Path
from types import SimpleNamespace
import verify_draw_order as v
def main(a):
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);a.exe=a.exe.resolve();a.rom=a.rom.resolve();a.native=a.native.resolve()
 original_loads=v.loads;original_run=subprocess.run;results=[]
 def case(name,where,mutate):
  hit=0;dest=out/name;dest.mkdir()
  def alter(obj):
   nonlocal hit
   if not hit and where(obj):hit+=1;return mutate(obj)
   return obj
  def loads(s):return alter(original_loads(s))
  def run(*args,**kwargs):
   r=original_run(*args,**kwargs)
   if name.startswith('process.'):
    return alter(SimpleNamespace(returncode=r.returncode,stdout=r.stdout,stderr=r.stderr))
   return r
  try:
   v.loads=loads;v.subprocess.run=run
   try:v.check_build(a.exe);v.verify_case(a.native,a.rom,a.exe,dest)
   except ValueError as e:results.append(dict(name=name,rejected=True,reason=str(e)))
   else:raise AssertionError('corruption accepted: '+name)
  finally:v.loads=original_loads;v.subprocess.run=original_run
  v.check(hit==1,'mutation not reached exactly once: '+name)
 def setkey(k,val):
  def f(o):o[k]=val;return o
  return f
 def drop(k):
  def f(o):del o[k];return o
  return f
 c=lambda o:type(o)is dict and 'operation'in o
 n=lambda tag:lambda o:type(o)is dict and o.get('tag')==tag
 m=lambda o:type(o)is dict and 'kind'in o and 'artifacts'in o
 b=lambda o:type(o)is dict and 'compiler_exit'in o
 for name,k,val in [('index-bool','index',True),('op-float','operation',0.0),('ok-int','ok',1),('wrong-op','operation',3),('wrong-index','index',2),('extra','invented',0)]:case('C.'+name,c,setkey(k,val))
 for k in('order','depth'):
  for name,val in [('wide',65536),('negative',-1),('bool',False)]:
   def mutation(o,k=k,val=val):o[k][0]=val;return o
   case('C.'+k+'-'+name,c,mutation)
 case('C.missing-depth',c,drop('depth'))
 case('C.wrong-valid-word',c,lambda o:setkey('depth',[1]+o['depth'][1:])(o))
 for number,(k,val)in enumerate([('returncode',False),('returncode',0.0),('stderr','unexpected\n'),('stdout','')]):
  def mutation(o,k=k,val=val):setattr(o,k,val);return o
  case('process.'+k+'-'+str(number),lambda o:isinstance(o,SimpleNamespace),mutation)
 def duplicate(o):o.stdout=o.stdout.replace('{"index":','{"index":1,"index":',1);return o
 case('process.duplicate-json',lambda o:isinstance(o,SimpleNamespace),duplicate)
 def reverse(o):o.stdout='\n'.join(reversed(o.stdout.splitlines()))+'\n';return o
 case('process.reverse-rows',lambda o:isinstance(o,SimpleNamespace),reverse)
 for name,k,val in [('decimal','ps',8),('M8','ps',32),('X8','ps',16),('DP','d',1),('DBR','dbr',0),('bool-status','ps',False),('clock','cycle',0),('pc','pc',0),('raw','raw','../raw.bin'),('extra','fake',0)]:case('native.'+name,n('pass.entry'),setkey(k,val))
 case('native.missing-register',n('pass.entry'),drop('x'))
 case('native.stack',n('pass.return'),setkey('sp',0))
 case('native.frame',n('pass.return'),setkey('frame',0))
 case('native.wrong-source-tag',n('pass.entry'),setkey('tag','depth.before'))
 for name,k,val in [('schema-bool','schema',True),('status-float','exit_code',0.0),('kind','kind','unknown'),('seed','state_injection',True)]:case('manifest.'+name,m,setkey(k,val))
 case('manifest.missing-source',m,lambda o:(o['sources'].pop(str(a.native/'capture_draw_order.lua')),o)[1])
 case('manifest.missing-raw',m,lambda o:(o['artifacts'].pop('raw_0001.bin'),o)[1])
 case('manifest.settings',m,lambda o:(o['isolation']['settings']['Snes'].__setitem__('EnableRandomPowerOnState',True),o)[1])
 case('manifest.posthash',m,lambda o:(o['isolation'].__setitem__('post_settings_sha256','0'*64),o)[1])
 case('manifest.args',m,lambda o:(o['arguments'].__setitem__(2,'--timeout=1'),o)[1])
 for name,k,val in [('empty-source','sources',{}),('status-bool','compiler_exit',False),('schema-float','schema',1.0)]:case('build.'+name,b,setkey(k,val))
 v.check_build(a.exe);dest=out/'baseline';dest.mkdir();v.verify_case(a.native,a.rom,a.exe,dest)
 report=dict(passed=True,rejections=len(results),cases=results,scope='in-memory parsed mutations after immutable-file reads; no fixture rewriting')
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS',len(results),'reachable rejections');return report
if __name__=='__main__':
 p=argparse.ArgumentParser()
 for key in('rom','exe','native','output'):p.add_argument('--'+key,type=Path,required=True)
 main(p.parse_args())
