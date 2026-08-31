"""Compare source-owned nested return frames fromAF4D to human caller87:91C3."""
import argparse,collections,copy,hashlib,json,struct,subprocess
from pathlib import Path
import mesen_portable

ROM_SHA='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
MESEN_SHA='d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b'
RANGES=[[0,0x2000],[0x3400,0x1600]]
SOURCE_VERSION={'capture':'ce0054920a543da176db7b7023f59e17f3c52dd3287f142cfa9c96d633bf2785',
 'runner':'71a7f63eee91ac3d9138f677f2bfade75a9b098aa067c3960d8d512fd6604267',
 'isolation_helper':'1bc6db2d68d836c7c6af180137a3d5e8e4ea454d7cb8a97e9e95cc6312ddc3bb'}
PCS={'player.entry':0x81a489,'court.entry':0x87a47a,'human.entry':0x84e2ac,'pass.entry':0x84df7a,
 'init.entry':0x86ab2d,'init.restore':0x86af4d,'init.rtl':0x86af65,'pass.restore':0x84e09c,
 'pass.rtl':0x84e0b4,'human.resume':0x84e2e8,'human.restore':0x84e3e6,'human.rtl':0x84e3e9,'human.return':0x8791c3}
SAVED=[0xb8,0xb6,0xbc,0xba,0xc0,0xbe,0x9c,0x9a]
GLOBALS=[0x7f6,0x904,0x90c,0x90e,0x910,0x922,0x936,0x93a,0x93e,0x942,0x944,0x946,0x978,0x9b8,0x9c4,0x9da]

def need(test,message):
 if not test:raise ValueError(message)
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def unique(pairs):
 d={}
 for k,v in pairs:
  need(k not in d,'duplicate JSON field '+k);d[k]=v
 return d
def read_json(path):return json.loads(Path(path).read_text(),object_pairs_hook=unique)
def integer(value,lo,hi):return type(value)is int and lo<=value<=hi
def strict_equal(a,b):
 if type(a)is not type(b):return False
 if isinstance(a,dict):return a.keys()==b.keys()and all(strict_equal(a[k],b[k])for k in a)
 if isinstance(a,list):return len(a)==len(b)and all(strict_equal(x,y)for x,y in zip(a,b))
 return a==b
def expected_settings(capture):
 return {'Debug':{'ScriptWindow':{'AllowIoOsAccess':True,'ScriptTimeout':60,'SaveScriptBeforeRun':False}},
  'Preferences':{'SingleInstance':False,'PauseWhenInBackground':False,'AutoLoadPatches':False,
   'OverrideSaveDataFolder':True,'SaveDataFolder':str(capture/'isolated-saves')},
  'Snes':{'Port1':{'Type':'SnesController'},'Port2':{'Type':'None'},'DisableFrameSkipping':True,
   'EnableRandomPowerOnState':False,'RamPowerOnState':'AllZeros','ForceFixedResolution':False,
   'Overscan':{'Top':7,'Bottom':8,'Left':0,'Right':0}},
  'Video':{'VideoFilter':'None','AspectRatio':'NoStretching','Brightness':0,'Contrast':0,'Hue':0,'Saturation':0,
   'ScanlineIntensity':0,'UseBilinearInterpolation':False,'ScreenRotation':'None'}}
def word(m,a):return m[a]|(m[a+1]<<8)
def words(m,a,n):return [word(m,a+2*i)for i in range(n)]
def raw(capture,row):
 data=(capture/row['raw']).read_bytes();need(len(data)==13824,'incomplete sparse raw')
 memory={};offset=0
 for base,size in RANGES:
  memory.update((base+i,v)for i,v in enumerate(data[offset:offset+size]));offset+=size
 return memory

def attest(capture,rom):
 m=read_json(capture/'manifest.json')
 need(set(m)=={'schema','kind','selection','state_injection','rom_patch','sparse_ranges','requested_frames',
  'arguments','environment','isolation','sources','exit_code','completion','artifacts'},'wrong manifest fields')
 need(integer(m.get('schema'),1,1)and m.get('kind')=='native human pass return boundaries','wrong capture schema')
 need(m.get('state_injection')is False and m.get('rom_patch')is False,'capture injected state')
 need(integer(m.get('exit_code'),0,0),'native process failed')
 need(integer(m.get('selection'),0,2)and m['selection']!=1,'invalid selection')
 need(integer(m.get('requested_frames'),400,3000),'invalid requested frames')
 need(m.get('sparse_ranges')==RANGES and all(type(v)is int for r in m['sparse_ranges']for v in r),'changed sparse schema')
 need(set(m.get('sources',{}))=={'rom','mesen','capture','runner','isolation_helper'},'missing source attestation')
 private={'mesen':'portable-mesen/Mesen.exe','capture':'capture.lua','runner':'capture_human_pass_return.py','isolation_helper':'mesen_portable.py'}
 for key,name in private.items():need(Path(m['sources'][key]['path']).resolve()==capture/name,'nonprivate executed source')
 need(sha(rom)==ROM_SHA and m['sources']['rom']['sha256']==ROM_SHA and m['sources']['mesen']['sha256']==MESEN_SHA,'wrong original ROM/Mesen')
 need(m.get('arguments')==[str(capture/'portable-mesen/Mesen.exe'),'--testrunner','--timeout=240',
                         str(Path(m['sources']['rom']['path']).resolve()),str(capture/'capture.lua')],'changed executed arguments')
 need(m.get('environment')=={'NBA95_CAPTURE_DIR':capture.as_posix(),'NBA95_PASS_RETURN_SELECTION':str(m['selection']),
                            'NBA95_PASS_RETURN_FRAMES':str(m['requested_frames'])},'changed executed environment')
 for key,source in m['sources'].items():
  need(set(source)=={'path','sha256'}and type(source['path'])is str and type(source['sha256'])is str,'invalid source identity')
  need(sha(source['path'])==source['sha256'],'changed source '+key)
 for key,digest in SOURCE_VERSION.items():need(m['sources'][key]['sha256']==digest,'unsupported source version '+key)
 required={'boundaries.jsonl','capture.lua','capture_complete.txt','capture_human_pass_return.py','mesen_portable.py',
           'observed-script-data-folder.txt','initial-mesen-settings.json','stdout.log','stderr.log'}
 artifacts=m.get('artifacts',{})
 need(required<=set(artifacts),'missing artifact attestation')
 for name,a in artifacts.items():
  need(Path(name).name==name and set(a)=={'bytes','sha256'},'invalid artifact metadata')
  need(integer(a['bytes'],0,1<<30)and(capture/name).stat().st_size==a['bytes']and sha(capture/name)==a['sha256'],'changed artifact '+name)
 isolation=m['isolation']
 need(set(isolation)=={'method','home','save_folder','initial_saves','settings','initial_settings_sha256',
  'post_settings_verified','observed_script_data_folder','post_settings_sha256','final_saves'},'wrong isolation fields')
 need(isolation['method']=='private portable executable/settings','wrong isolation method')
 need(Path(isolation['home']).resolve()==capture/'portable-mesen'and Path(isolation['save_folder']).resolve()==capture/'isolated-saves','nonprivate home/saves')
 need(isolation['initial_saves']==[]and isolation['post_settings_verified']is True,'missing process isolation')
 need(sha(capture/'initial-mesen-settings.json')==isolation['initial_settings_sha256'],'changed initial settings')
 need(sha(capture/'portable-mesen/settings.json')==isolation['post_settings_sha256'],'changed persisted settings')
 initial=read_json(capture/'initial-mesen-settings.json')
 need(strict_equal(initial,expected_settings(capture))and strict_equal(initial,isolation['settings']),'changed settings recipe')
 observed=(capture/'observed-script-data-folder.txt').read_text().strip()
 need(observed==isolation['observed_script_data_folder']and Path(observed).resolve()==capture/'portable-mesen/LuaScriptData/capture','changed observed Lua home')
 saves={p.name:sha(p)for p in(capture/'isolated-saves').iterdir()if p.is_file()}
 need(saves==isolation['final_saves'],'changed final saves')
 # Verify on a copy: the shared helper records hashes and must not silently
 # repair the attestation currently under review.
 checked=mesen_portable.verify(capture,copy.deepcopy(isolation))
 need(strict_equal(checked,isolation),'isolation verifier changed recorded evidence')
 need((capture/'capture_complete.txt').read_text()==m['completion'],'changed completion')
 rows=[json.loads(s,object_pairs_hook=unique)for s in(capture/'boundaries.jsonl').read_text().splitlines()]
 complete=unique([s.split('=',1)for s in m['completion'].splitlines()])
 need(set(complete)=={'selection','frames','boundaries','calls'},'invalid completion fields')
 need(all(v.isdecimal()for v in complete.values()),'nonnumeric completion')
 need(int(complete['selection'])==m['selection']and int(complete['boundaries'])==len(rows)and
      m['requested_frames']<=int(complete['frames'])<=m['requested_frames']+1 and int(complete['calls'])>0,'incomplete normal journey')
 used_raw=set();fields={'index','tag','pc','frame','court','raw','cpu_a','cpu_x','cpu_y','cpu_ps','cpu_d','cpu_sp','cpu_dbr','cpu_k','cpu_pc','actor','owner','live','offense','stack'}
 last_frame=last_court=-1;court_frame=4590 if m['selection']==0 else 4390
 for index,row in enumerate(rows,1):
  need(set(row)==fields,'invalid native event fields')
  need(type(row['tag'])is str and row['tag']in PCS,'unknown native event')
  limits={'index':(1,len(rows)),'pc':(0,0xffffff),'frame':(0,17999),'court':(-1,int(complete['frames'])-1),
   'cpu_ps':(0,255),'cpu_dbr':(0,255),'cpu_k':(0,255),'cpu_sp':(0,0x1fff)}
  for key in fields-{'tag','raw','stack'}:
   lo,hi=limits.get(key,(0,65535));need(integer(row[key],lo,hi),'invalid native numeric type/range '+key)
  need(row['index']==index and row['pc']==PCS[row['tag']]and row['cpu_d']==0,'wrong index/PC/DP')
  need((row['cpu_k']<<16)|row['cpu_pc']==row['pc'],'CPU program-bank/PC disagrees with boundary')
  if index==1:need(row['tag']=='player.entry'and row['frame']==2697 and row['court']==-1,'changed Player clock')
  elif index==2:need(row['tag']=='court.entry'and row['frame']==court_frame and row['court']==0,'changed court clock')
  else:need(row['court']>=0 and row['frame']-row['court']==court_frame,'changed relative court clock')
  need(row['frame']>=last_frame and row['court']>=last_court,'native time reversed');last_frame=row['frame'];last_court=row['court']
  name=f'raw_{index:05d}.bin';need(row['raw']==name and name in artifacts,'unbound raw snapshot');used_raw.add(name)
  state=raw(capture,row)
  for key,address in {'actor':0xc2,'owner':0x93e,'live':0x936,'offense':0x93a}.items():need(row[key]==word(state,address),'raw/metadata disagreement '+key)
  stack=row['stack'];need(type(stack)is list and len(stack)==min(19,0x1fff-row['cpu_sp']),'wrong bounded stack preview')
  need(all(integer(x,0,255)for x in stack),'invalid stack byte type/range')
  need(stack==[state[row['cpu_sp']+1+i]for i in range(len(stack))],'stack preview/raw disagreement')
  if index>2:need(row['cpu_ps']&0x38==0 and row['cpu_dbr']==0x7e,'unsupported binary 16-bit runtime/WRAM bank domain')
 need(set(artifacts)==required|used_raw,'unexpected/unbound artifact')
 return m,rows,int(complete['calls'])

def verify(capture,probe,rom,diagnostics):
 manifest,rows,native_calls=attest(capture,rom);inputs=[];expected=[];contexts=[];calls=[];stages=collections.Counter()
 def state(row):return raw(capture,row)
 def saved(row):return [word(state(row),a)for a in SAVED]
 def stack_return(row):
  need(len(row['stack'])>=3,'missing native RTL address')
  return (row['stack'][2]<<16)|(((row['stack'][1]<<8)|row['stack'][0])+1)&65535
 def stack_saved(row,values,offset=0):
  want=[byte for value in values for byte in(value&255,value>>8)]
  need(row['stack'][offset:offset+len(want)]==want,'native saved frame disagrees with original caller words')
 def same_registers(entry,end,keys):need(all(entry[k]==end[k]for k in keys),'unexpected register change '+entry['tag'])
 def same_memory(entry,end,addresses=()):
  before=state(entry);after=state(end);changed={a+i for a in addresses for i in(0,1)}
  need(all(before[a]==after[a]for a in before if a not in changed),'unexpected native memory effect '+entry['tag'])
 def pop(entry,end,values,addresses):
  stack_saved(entry,values);need(end['cpu_sp']==entry['cpu_sp']+2*len(values),'wrong native PLA stack depth')
  last=values[-1];need(end['cpu_a']==last,'wrong final PLA accumulator')
  flags=(entry['cpu_ps']&~0x82)|(0x80 if last&0x8000 else 0)|(2 if last==0 else 0)
  need(end['cpu_ps']==flags,'wrong final PLA N/Z flags')
  same_registers(entry,end,('cpu_x','cpu_y','cpu_d','cpu_dbr','cpu_k'))
  same_memory(entry,end,addresses)
 def rtl(entry,end):
  need(end['cpu_sp']==entry['cpu_sp']+3 and stack_return(entry)==end['pc'],'wrong native RTL depth/address')
  same_registers(entry,end,('cpu_a','cpu_x','cpu_y','cpu_ps','cpu_d','cpu_dbr'));same_memory(entry,end)
 def append(mode,entry,end,group):
  init=group.get('init.entry',group['pass.entry']);selector=group['pass.entry'];human=group['human.entry'];after=state(end)
  args=[mode,*[str(capture/r['raw'])for r in(entry,init,selector,human)]]
  need(all('|'not in a and '\n'not in a for a in args),'unsupported protocol path')
  inputs.append('|'.join(args));stages[mode]+=1
  expected.append(dict(result=1,saved_words=saved(init)+saved(selector)+[word(state(human),0xb6)],
   dp_words=words(after,0,128),actor_words=words(after,0x34eb,1408),controller_words=words(after,0x47eb,160),
   context_words=words(after,0x46eb,128),profile_words=words(after,0x3449,20),order_words=words(after,0x34d1,13),global_words=[word(after,a)for a in GLOBALS]))
  contexts.append(dict(mode=mode,entry=entry['index'],exit=end['index'],court=entry['court']))
 need([r['tag']for r in rows[:2]]==['player.entry','court.entry'],'missing initial route')
 need(words(state(rows[0]),0x166d,5)==[2,1,1,1,1]and word(state(rows[1]),0x166d)==manifest['selection'],'wrong native selection route')
 group=None
 for row in rows[2:]:
  tag=row['tag']
  if tag=='human.entry':need(group is None,'nested human return group');group={tag:row};continue
  need(group is not None and tag not in group,'orphan/duplicate return boundary');previous=list(group)[-1]
  allowed={'pass.entry':('human.entry',),'init.entry':('pass.entry',),'init.restore':('init.entry',),'init.rtl':('init.restore',),
   'pass.restore':('init.rtl','pass.entry'),'pass.rtl':('pass.restore',),'human.resume':('pass.rtl',),'human.restore':('human.resume',),
   'human.rtl':('human.restore',),'human.return':('human.rtl',)}
  need(previous in allowed.get(tag,()),'misordered return boundary')
  # Selection can cross a native frame before AB2D (observed left1471->1472).
  # Only the bounded return instructions are required to be uninterrupted.
  return_start=group.get('init.restore',group.get('pass.restore'))
  if return_start is not None:need(row['court']==return_start['court'],'bounded return crosses court clock')
  group[tag]=row
  if tag!='human.return':continue
  human=group['human.entry'];selector=group['pass.entry'];human_state=state(human)
  need(word(human_state,0xae)&0x8000 and word(human_state,0xc2)==word(human_state,0x93e),'pass did not originate at natural human B/owner gate')
  need(selector['cpu_sp']==human['cpu_sp']-5 and stack_return(selector)==0x84e2e8,'wrong human-to-selector frame')
  stack_saved(selector,[word(human_state,0xb6)],3)
  need(word(state(selector),0xb6)==word(human_state,0xae),'human B mask not carried into selector')
  need(stack_return(human)==0x8791c3,'wrong gameplay caller of human action')
  selector_restore=group['pass.restore']
  if 'init.entry'in group:
   init=group['init.entry'];restore=group['init.restore'];init_rtl=group['init.rtl']
   need(init['cpu_sp']==selector['cpu_sp']-19 and stack_return(init)==0x84e09c,'wrong selector-to-initializer frame')
   stack_saved(init,list(reversed(saved(selector))),3)
   need(restore['cpu_sp']==init['cpu_sp']-16,'unbalanced initializer body stack')
   pop(restore,init_rtl,list(reversed(saved(init))),SAVED);rtl(init_rtl,selector_restore)
   append('initializer',restore,init_rtl,group)
  else:need(selector_restore['cpu_sp']==selector['cpu_sp']-16,'unbalanced no-receiver selector stack')
  pop(selector_restore,group['pass.rtl'],list(reversed(saved(selector))),SAVED)
  rtl(group['pass.rtl'],group['human.resume'])
  same_registers(group['human.resume'],group['human.restore'],('cpu_a','cpu_x','cpu_y','cpu_ps','cpu_d','cpu_dbr','cpu_k','cpu_sp'))
  same_memory(group['human.resume'],group['human.restore'])
  pop(group['human.restore'],group['human.rtl'],[word(human_state,0xb6)],[0xb6]);rtl(group['human.rtl'],row)
  need(group['human.rtl']['cpu_sp']==human['cpu_sp'],'outer action frame did not balance')
  append('selector',selector_restore,group['pass.rtl'],group);append('human',group['human.restore'],group['human.rtl'],group)
  if 'init.entry'in group:append('chain',group['init.restore'],group['human.rtl'],group)
  calls.append(dict(entry=human['index'],exit=row['index'],court=human['court'],return_court=row['court'],initializer='init.entry'in group,stack_depths={k:r['cpu_sp']for k,r in group.items()}));group=None
 need(group is None and len(calls)==native_calls and native_calls>0,'incomplete return calls')
 need(stages['selector']==stages['human']==native_calls and stages['initializer']==stages['chain']>0,'missing native return comparisons')
 run=subprocess.run([str(probe)],input='\n'.join(inputs)+'\n',text=True,capture_output=True,timeout=60)
 diagnostics.with_suffix('.probe-stdout.txt').write_text(run.stdout);diagnostics.with_suffix('.probe-stderr.txt').write_text(run.stderr)
 need(type(run.returncode)is int and run.returncode==0,'C probe failed')
 need(type(run.stderr)is str and run.stderr=='','unexpected C diagnostic protocol; this asset-free probe is silent')
 lines=run.stdout.splitlines();need(len(lines)==len(expected),'wrong C response count');failures=[];compared=0
 for i,(line,want)in enumerate(zip(lines,expected)):
  actual=json.loads(line,object_pairs_hook=unique);need(type(actual)is dict and actual.keys()==want.keys(),'wrong C fields')
  for key,value in want.items():
   if type(value)is list:
    need(type(actual[key])is list and len(actual[key])==len(value),'wrong C vector shape');pairs=enumerate(zip(actual[key],value))
   else:pairs=[(None,(actual[key],value))]
   for slot,(got,wanted)in pairs:
    compared+=1
    if type(got)is not int or got!=wanted:failures.append(dict(**contexts[i],field=key,slot=slot,actual=got,expected=wanted))
 return dict(kind='AF4D nested frame restoration through E3E9; native RTL return to8791C3 observed, no launch or human enable',passed=not failures,
  manifest_sha256=sha(capture/'manifest.json'),verifier_sha256=sha(__file__),probe_sha256=sha(probe),rom_sha256=sha(rom),probe_arguments=[str(probe)],
  stages=dict(stages),calls=calls,compared_values=compared,failures=failures)

def main():
 p=argparse.ArgumentParser()
 for name in('capture','probe','rom','output'):p.add_argument('--'+name,type=Path,required=True)
 a=p.parse_args();need(not a.output.exists(),'preserve old report')
 need(all(not a.output.with_suffix(s).exists()for s in('.probe-stdout.txt','.probe-stderr.txt')),'preserve earlier C output')
 try:report=verify(a.capture.resolve(),a.probe.resolve(),a.rom.resolve(),a.output)
 except Exception as error:a.output.write_text(json.dumps(dict(passed=False,error=str(error)),indent=2)+'\n');raise
 a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k not in('calls','failures')}))
 if report['failures']:print(json.dumps(report['failures'][:10]));raise SystemExit(1)
if __name__=='__main__':main()
