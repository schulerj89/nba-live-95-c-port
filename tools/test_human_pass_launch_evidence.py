"""Strict launch/helper/NMI native fields and asset-free complete C protocol mutations."""
import argparse,copy,importlib.util,json,subprocess,sys
from pathlib import Path
from unittest.mock import patch

def main():
 p=argparse.ArgumentParser()
 for key in('verifier','capture','probe','rom','output'):p.add_argument('--'+key,type=Path,required=True)
 a=p.parse_args()
 for key in vars(a):setattr(a,key,getattr(a,key).resolve())
 a.output.mkdir(exist_ok=False);sys.path.insert(0,str(a.verifier.parent))
 spec=importlib.util.spec_from_file_location('launch_verifier',a.verifier);v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
 baseline=v.verify(a.capture,a.probe,a.rom,a.output/'baseline.json');assert baseline['passed']and len(baseline['calls'])>0
 (a.output/'baseline.json').write_text(json.dumps(baseline,indent=2)+'\n')
 original_read=v.read_json;original_text=Path.read_text;original_raw=v.raw;original_iter=Path.iterdir;original_is_file=Path.is_file
 manifest=v.read_json(a.capture/'manifest.json');rows=[json.loads(s)for s in(a.capture/'boundaries.jsonl').read_text().splitlines()]
 stdout=(a.output/'baseline.probe-stdout.txt').read_text();stderr=(a.output/'baseline.probe-stderr.txt').read_text();assert stderr==''
 checks=[]
 def run(name,context):
  try:
   with context:result=v.verify(a.capture,a.probe,a.rom,a.output/f'case-{len(checks):03d}.json')
   rejected=not result['passed'];reason='C mismatch'if rejected else'accepted'
  except(ValueError,KeyError,TypeError,IndexError)as error:rejected=True;reason=str(error)
  checks.append(dict(name=name,rejected=rejected,reason=reason))
 def metadata(name,change):
  altered=copy.deepcopy(manifest);change(altered)
  run(name,patch.object(v,'read_json',side_effect=lambda p:altered if Path(p)==a.capture/'manifest.json'else original_read(p)))
 for key,value in(('schema',1.0),('requested_frames',True),('requested_frames',399),('selection',1),('exit_code',False),('state_injection',0),('rom_patch',0)):
  metadata('manifest '+key+'='+repr(value),lambda m,k=key,x=value:m.update({k:x}))
 for key in('rom','mesen','capture','runner','isolation_helper'):metadata('missing source '+key,lambda m,k=key:m['sources'].pop(k))
 for name in('boundaries.jsonl','raw_00003.bin'):metadata('missing artifact '+name,lambda m,k=name:m['artifacts'].pop(k))
 metadata('boolean artifact size',lambda m:m['artifacts']['stderr.log'].update(bytes=False))
 metadata('wrong command',lambda m:m['arguments'].__setitem__(3,'wrong.sfc'))
 metadata('wrong environment route',lambda m:m['environment'].update(NBA95_PASS_LAUNCH_SELECTION='2'))
 metadata('extra environment',lambda m:m['environment'].update(EXTRA='1'))
 metadata('forged settings hash',lambda m:m['isolation'].update(post_settings_sha256='0'*64))
 metadata('forged final saves',lambda m:m['isolation'].update(final_saves={'extra.srm':'0'*64}))
 metadata('integer private verification flag',lambda m:m['isolation'].update(post_settings_verified=1))
 for extra in('capture_error.txt','raw_99999.bin'):
  fake=a.capture/extra
  def listing(path,f=fake):return iter([*original_iter(path),f])if path==a.capture else original_iter(path)
  from contextlib import ExitStack
  with ExitStack()as context:
   context.enter_context(patch.object(Path,'iterdir',listing))
   context.enter_context(patch.object(Path,'is_file',lambda path,f=fake:True if path==f else original_is_file(path)))
   run('unattested file '+extra,context)
 def events(name,change):
  altered=copy.deepcopy(rows);change(altered);text='\n'.join(json.dumps(r)for r in altered)+'\n'
  def read(path,*args,**kwargs):return text if path==a.capture/'boundaries.jsonl'else original_text(path,*args,**kwargs)
  run(name,patch.object(Path,'read_text',read))
 entry=next(i for i,r in enumerate(rows)if r['tag']=='launch.entry')
 for key,value in(('actor',True),('actor_pointer',65536),('origin',True),('call',65536),('component',True),('nmi_depth',65536),('owner',65536),('live',65536),('offense',65536),('cpu_a',65536),('cpu_x',65536),('cpu_y',False),('cpu_sp',8192),('cpu_k',256),('cpu_dbr',256),('cpu_pc',65536),('cpu_ps',256)):
  events('numeric domain '+key,lambda rs,k=key,x=value:rs[entry].update({k:x}))
 for key in('actor','actor_pointer','owner','live','offense'):events('raw metadata '+key,lambda rs,k=key:rs[entry].update({k:rs[entry][k]^1}))
 for key in('cpu_pc','cpu_k','pc','cpu_sp','cpu_d'):
  events('CPU boundary '+key,lambda rs,k=key:rs[entry].update({k:rs[entry][k]^1}))
 for value in(0,0x80):events('wrong runtime DBR '+str(value),lambda rs,x=value:rs[entry].update(cpu_dbr=x))
 for bit in(8,16,32):events('runtime CPU mode '+str(bit),lambda rs,x=bit:rs[entry].update(cpu_ps=rs[entry]['cpu_ps']|x))
 events('stack wrong value',lambda rs:rs[entry]['stack'].__setitem__(0,rs[entry]['stack'][0]^1))
 events('stack bool',lambda rs:rs[entry]['stack'].__setitem__(0,False))
 events('stack high byte',lambda rs:rs[entry]['stack'].__setitem__(0,256))
 events('stack short',lambda rs:rs[entry]['stack'].pop())
 events('stack extra',lambda rs:rs[entry]['stack'].append(0))
 events('uniform frame shift',lambda rs:[r.update(frame=r['frame']+1)for r in rs])
 events('uniform court shift',lambda rs:[r.update(court=r['court']+1)for r in rs])
 events('huge clock shift',lambda rs:[r.update(frame=r['frame']+100000,court=r['court']+100000)for r in rs])
 for tag in('dispatch.call','wrapper.entry','launch.entry','mul.entry','divide.entry','launch.restore','launch.exit','wrapper.exit','mode.return','nmi.entry','nmi.exit.normal'):
  events('missing boundary '+tag,lambda rs,t=tag:rs.pop(next(i for i,r in enumerate(rs)if r['tag']==t)))
 for tag in('dispatch.call','wrapper.entry','mode.entry','mul.entry','divide.entry','launch.exit','launch.return','wrapper.exit','mode.return'):
  index=next(i for i,r in enumerate(rows)if r['tag']==tag)
  for key in('cpu_a','cpu_x','cpu_y','cpu_ps'):
   events('CPU contract '+tag+' '+key,lambda rs,i=index,k=key:rs[i].update({k:rs[i][k]^1}))
 for i,r in enumerate(rows):
  if r['tag']in('mul.entry','divide.entry','mul.exit.signed','mul.exit.unsigned','divide.exit')and r['call']==1:
   events('all first-call helper status '+str(i),lambda rs,n=i:rs[n].update(cpu_ps=rs[n]['cpu_ps']^1))
 for tag in('launch.entry','mul.entry','divide.entry','launch.exit','nmi.entry','nmi.exit.normal'):
  index=next(i for i,r in enumerate(rows)if r['tag']==tag)
  events('binary domain '+tag,lambda rs,i=index:rs[i].update(cpu_ps=rs[i]['cpu_ps']|8))
 for key in('origin','call','component','nmi_depth'):
  events('forged launch '+key,lambda rs,k=key:rs[entry].update({k:rs[entry][k]+1}))
 for tag in('nmi.entry','nmi.exit.normal'):
  index=next(i for i,r in enumerate(rows)if r['tag']==tag)
  for key in('cpu_a','cpu_x','cpu_y','cpu_dbr','cpu_sp','component','nmi_depth'):
   events('NMI field '+tag+' '+key,lambda rs,i=index,k=key:rs[i].update({k:rs[i][k]^1}))
 for address in(0xcc,0xb6,0x9e):
  def corrupted_owned(capture,row,addr=address):
   memory=original_raw(capture,row)
   if row['tag']=='nmi.exit.normal':memory[addr]^=1
   return memory
  run('NMI cannot exempt owned DP '+hex(address),patch.object(v,'raw',side_effect=corrupted_owned))
 for address in v.SAVED:
  def corrupted_saved(capture,row,addr=address):
   memory=original_raw(capture,row)
   if row['tag']=='launch.entry':memory[addr]^=1
   return memory
  run('saved original frame '+hex(address),patch.object(v,'raw',side_effect=corrupted_saved))
 lines=stdout.splitlines();first=json.loads(lines[0]);outputs=[('missing row','\n'.join(lines[:-1])+'\n'),('extra row',stdout+lines[0]+'\n'),('noise','noise\n'+stdout)]
 for key in first:
  altered=copy.deepcopy(first)
  if type(altered[key])is list:altered[key][0]=float(altered[key][0])
  else:altered[key]=float(altered[key])
  outputs.append(('float C '+key,'\n'.join([json.dumps(altered),*lines[1:]])+'\n'))
 for name,change in(('missing actor output',lambda x:x.pop('actor_words')),('short DP',lambda x:x['dp_words'].pop()),('extra field',lambda x:x.update(extra=0))):
  altered=copy.deepcopy(first);change(altered);outputs.append((name,'\n'.join([json.dumps(altered),*lines[1:]])+'\n'))
 outputs.append(('duplicate result',stdout.replace('"result":','"result":0,"result":',1)))
 for name,text in outputs:run(name,patch.object(v.subprocess,'run',return_value=subprocess.CompletedProcess([],0,text,stderr)))
 for text in('ERROR\n','\n','[ASSETS] Loaded asset pack: forged.pak\n'):
  run('unexpected stderr '+repr(text),patch.object(v.subprocess,'run',return_value=subprocess.CompletedProcess([],0,stdout,text)))
 for code in(False,0.0):run('noninteger C process '+repr(code),patch.object(v.subprocess,'run',return_value=subprocess.CompletedProcess([],code,stdout,stderr)))
 report=dict(passed=all(c['rejected']for c in checks),test_source_sha256=v.sha(__file__),verifier_sha256=v.sha(a.verifier),probe_sha256=v.sha(a.probe),manifest_sha256=v.sha(a.capture/'manifest.json'),checks=checks)
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
