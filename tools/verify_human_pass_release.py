"""Compare natural mode15 dispatch/gate/attachment/cleanup; launch99C4 observed only."""
import argparse,collections,copy,hashlib,json,struct,subprocess
from pathlib import Path
import mesen_portable

ROM_SHA='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
MESEN_SHA='d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b'
RANGES=[[0,0x2000],[0x3400,0x1600]]
SOURCE_VERSION={'capture': '5d9e5e8976fb4367cbb3e0ca3083f0c889abc0b236c474d11c06d9b9551b176e', 'runner': 'fb1d8ddf2d8b7b70a740d0314c992cd96b196fb11037d1317e22a1d0e8d32cda', 'isolation_helper': '1bc6db2d68d836c7c6af180137a3d5e8e4ea454d7cb8a97e9e95cc6312ddc3bb'}
PCS={'wrapper.entry': 8887379, 'wrapper.exit': 8887383, 'mode.entry': 8824499, 'mode.expired': 8824690, 'mode.launch': 8824649, 'mode.wait': 8824720, 'mode.airborne': 8824361, 'mode.special': 8824744, 'mode.steer': 8760683, 'mode.cancel': 8824695, 'normalize.entry': 8820806, 'normalize.shared': 8820833, 'normalize.exit': 8820844, 'pose.entry': 8892099, 'pose.return': 8824572, 'attach.entry': 8894025, 'attach.exit': 8894057, 'offset.entry': 8894514, 'offset.exit.point0': 8894699, 'offset.exit.point1': 8894802, 'special.exit': 8824793, 'launch.entry': 8821188, 'launch.exit': 8821680, 'launch.return': 8824671, 'mode.exit.timer': 8824522, 'mode.exit.airborne': 8824567, 'mode.exit.pose': 8824587, 'mode.exit.launch': 8824687, 'mode.exit.expired': 8824694, 'mode.exit.cancel': 8824719, 'mode.exit.wait': 8824735, 'player.entry': 8496265, 'court.entry': 8889466, 'human.entry': 8708780, 'pass.entry': 8707962, 'init.return': 8826701, 'human.return': 8884675, 'dispatch.entry': 8884804, 'dispatch.call': 8884824, 'mode.return': 8884828}
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
 need(integer(m.get('schema'),1,1)and m.get('kind')=='native native human mode15 release boundaries','wrong capture schema')
 need(m.get('state_injection')is False and m.get('rom_patch')is False,'capture injected state')
 need(integer(m.get('exit_code'),0,0),'native process failed')
 need(integer(m.get('selection'),0,2)and m['selection']!=1,'invalid selection')
 need(integer(m.get('requested_frames'),400,3000),'invalid requested frames')
 need(m.get('sparse_ranges')==RANGES and all(type(v)is int for r in m['sparse_ranges']for v in r),'changed sparse schema')
 need(set(m.get('sources',{}))=={'rom','mesen','capture','runner','isolation_helper'},'missing source attestation')
 private={'mesen':'portable-mesen/Mesen.exe','capture':'capture.lua','runner':'capture_human_pass_release.py','isolation_helper':'mesen_portable.py'}
 for key,name in private.items():need(Path(m['sources'][key]['path']).resolve()==capture/name,'nonprivate executed source')
 need(sha(rom)==ROM_SHA and m['sources']['rom']['sha256']==ROM_SHA and m['sources']['mesen']['sha256']==MESEN_SHA,'wrong original ROM/Mesen')
 need(m.get('arguments')==[str(capture/'portable-mesen/Mesen.exe'),'--testrunner','--timeout=240',
                         str(Path(m['sources']['rom']['path']).resolve()),str(capture/'capture.lua')],'changed executed arguments')
 need(m.get('environment')=={'NBA95_CAPTURE_DIR':capture.as_posix(),'NBA95_PASS_RELEASE_SELECTION':str(m['selection']),
                            'NBA95_PASS_RELEASE_FRAMES':str(m['requested_frames'])},'changed executed environment')
 for key,source in m['sources'].items():
  need(set(source)=={'path','sha256'}and type(source['path'])is str and type(source['sha256'])is str,'invalid source identity')
  need(sha(source['path'])==source['sha256'],'changed source '+key)
 for key,digest in SOURCE_VERSION.items():need(m['sources'][key]['sha256']==digest,'unsupported source version '+key)
 required={'boundaries.jsonl','capture.lua','capture_complete.txt','capture_human_pass_release.py','mesen_portable.py',
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
 need(set(complete)=={'selection','frames','boundaries','calls','passes'},'invalid completion fields')
 need(all(v.isdecimal()for v in complete.values()),'nonnumeric completion')
 need(int(complete['selection'])==m['selection']and int(complete['boundaries'])==len(rows)and
      m['requested_frames']<=int(complete['frames'])<=m['requested_frames']+1 and int(complete['calls'])>0 and int(complete['passes'])>0,'incomplete normal journey')
 used_raw=set();fields={'index','tag','pc','frame','court','raw','cpu_a','cpu_x','cpu_y','cpu_ps','cpu_d','cpu_sp','cpu_dbr','cpu_k','cpu_pc','actor','actor_pointer','owner','live','offense','stack','origin','call'}
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
  for key,address in {'actor':0xc2,'actor_pointer':0x96,'owner':0x93e,'live':0x936,'offense':0x93a}.items():need(row[key]==word(state,address),'raw/metadata disagreement '+key)
  stack=row['stack'];need(type(stack)is list and len(stack)==min(23,0x1fff-row['cpu_sp']),'wrong bounded stack preview')
  need(all(integer(x,0,255)for x in stack),'invalid stack byte type/range')
  need(stack==[state[row['cpu_sp']+1+i]for i in range(len(stack))],'stack preview/raw disagreement')
  if index>2:need(row['cpu_ps']&0x38==0 and row['cpu_dbr']==0x7e,'unsupported binary 16-bit runtime/WRAM bank domain')
 need(set(artifacts)==required|used_raw,'unexpected/unbound artifact')
 return m,rows,int(complete['calls']),int(complete['passes'])

DEFAULT_ASSETS=Path(r'C:\Users\joshs\Projects\nba-live-95-c-port\.analysis\frontend-integration-20260830\nba95_assets_candidate.pak')
ASSET_SHA='951f82331c4bb6ce8f381da519ee8bfdf517bf8c13f2cd6f20cfa9c34d5ed4df'

def verify(capture,probe,rom,diagnostics,assets=DEFAULT_ASSETS):
 manifest,rows,native_calls,native_passes=attest(capture,rom)
 need(sha(assets)==ASSET_SHA,'wrong proven animation pack')
 original=rom.read_bytes()
 need(original[0x39c0f:0x39c13]==bytes.fromhex('539c8700'),'wrong original mode15 wrapper table')
 pointers=[int.from_bytes(original[0x39c7b+2*i:0x39c7d+2*i],'little')for i in range(10)]
 inputs=[];expected=[];contexts=[];stages=collections.Counter();routes=collections.Counter();calls=[];passes=[];irq_observations=[]
 def state(row):return raw(capture,row)
 def w(row,address):return word(state(row),address)
 def same_registers(a,b,keys=('cpu_a','cpu_x','cpu_y','cpu_ps','cpu_d','cpu_dbr')):
  need(all(a[k]==b[k]for k in keys),'changed CPU contract '+a['tag']+' -> '+b['tag'])
 def same_memory(a,b,addresses=(),allow_stack=False):
  before=state(a);after=state(b);changed={n+i for n in addresses for i in(0,1)}
  need(all(before[n]==after[n]for n in before if n not in changed and not(allow_stack and 0x1f00<=n<=max(a['cpu_sp'],b['cpu_sp']))),
       'unexpected memory change '+a['tag']+' -> '+b['tag'])
 def rtl(a,b):
  need(len(a['stack'])>=3 and a['cpu_sp']+3==b['cpu_sp'],'invalid RTL depth')
  target=(a['stack'][2]<<16)|(((a['stack'][1]<<8|a['stack'][0])+1)&65535)
  need(target==b['pc'],'invalid RTL target');same_registers(a,b);same_memory(a,b)
 def jsl(a,b,target):
  need(a['cpu_sp']-3==b['cpu_sp'] and b['stack'][:3]==[(target-1)&255,((target-1)>>8)&255,target>>16],'invalid JSL frame')
  same_registers(a,b);same_memory(a,b,allow_stack=True)
 def append(mode,a,b,result=1):
  path=str(capture/a['raw']);need('\n'not in path,'unsupported protocol path')
  inputs.append(mode+' '+path);after=state(b);stages[mode]+=1
  expected.append(dict(result=result,dp_words=words(after,0,128),actor_words=words(after,0x34eb,1408),
   controller_words=words(after,0x47eb,160),context_words=words(after,0x46eb,128),profile_words=words(after,0x3449,20),
   order_words=words(after,0x34d1,13),global_words=[word(after,a)for a in GLOBALS]))
  contexts.append(dict(mode=mode,entry=a['index'],exit=b['index'],court=a['court']))
 need(words(state(rows[0]),0x166d,5)==[2,1,1,1,1]and w(rows[1],0x166d)==manifest['selection'],'changed native controller selection route')
 origins={};human=None;group=None;completed_calls=0;completed_passes=0
 for row in rows[2:]:
  tag=row['tag']
  if tag=='human.entry':
   need(human is None and group is None,'nested human origin');completed_passes+=1
   need(row['origin']==completed_passes and row['call']==completed_calls,'wrong human origin sequence')
   memory=state(row);controller=word(memory,0x90c)
   need(controller==0x47eb and word(memory,0xae)&0x8000 and word(memory,controller+14)&0x8000,'human origin lacks original B press')
   need(word(memory,controller+2)==row['actor']==row['owner']and word(memory,row['actor_pointer']+0x16)==0,'human origin lacks controller/owner identity')
   human=[row];continue
  if human is not None:
   sequence=['human.entry','pass.entry','init.return','human.return']
   need(tag==sequence[len(human)]and row['origin']==completed_passes and row['call']==completed_calls,'misordered human origin')
   need(row['actor_pointer']==human[0]['actor_pointer'] and row['actor']==human[0]['actor'],'human origin actor changed')
   human.append(row)
   if tag!='human.return':continue
   need(w(human[2],row['actor_pointer']+0x5e)==15 and w(row,row['actor_pointer']+0x5e)==15,'native initializer did not install mode15')
   need(row['cpu_sp']==human[0]['cpu_sp']+3 and human[0]['stack'][:3]==[0xc2,0x91,0x87],'human return stack lacks actual caller')
   origins[row['actor_pointer']]=row['origin']
   passes.append(dict(origin=row['origin'],entry=human[0]['index'],return_index=row['index'],court=human[0]['court'],
    held=w(human[0],0x47f3),pressed=w(human[0],0x47f9)))
   human=None;continue
  if tag=='dispatch.entry':
   need(group is None,'nested mode call');completed_calls+=1
   need(row['call']==completed_calls and row['origin']==origins.get(row['actor_pointer']),'mode call lacks human origin')
   need(row['actor']<10 and row['actor_pointer']==pointers[row['actor']]and w(row,row['actor_pointer']+0x5e)==15,'invalid native mode15 actor')
   group=[row];continue
  need(group is not None,'orphan mode boundary')
  need(row['call']==completed_calls and row['origin']==group[0]['origin'],'mode call identity mismatch')
  # Five naturally observed99C4 calls cross one frame. That child is NOT
  # replayed here. Every compared source segment remains within one frame.
  launch_exit=next((r for r in group if r['tag']=='launch.exit'),None)
  if tag=='launch.exit':need(group[0]['court']<=row['court']<=group[0]['court']+1,'launch observation clock jump')
  else:need(row['court']==(launch_exit or group[0])['court'],'compared mode segment crosses clock')
  need(row['actor_pointer']==group[0]['actor_pointer']and row['actor']==group[0]['actor'],'mode call actor mismatch')
  group.append(row)
  if tag!='mode.return':continue
  tags=[r['tag']for r in group];need(len(tags)==len(set(tags)),'duplicate mode boundary')
  g={r['tag']:r for r in group}
  prefix=['dispatch.entry','dispatch.call','wrapper.entry','mode.entry'];suffix=['wrapper.exit','mode.return']
  need(tags[:4]==prefix and tags[-2:]==suffix,'missing native dispatch wrapper')
  body=tags[4:-2];special=[]
  if body[:2]==['mode.special','special.exit']:
   special=body[:2];body=body[2:];append('turn',g['mode.special'],g['special.exit'])
   need(g['special.exit']['cpu_sp']==g['mode.special']['cpu_sp'],'A7A8 unbalanced local stack')
  path=None
  if body==['mode.exit.timer']:path=1;stop='mode.exit.timer'
  elif body==['mode.expired','normalize.entry','normalize.shared','normalize.exit','mode.exit.expired']:
   path=2;stop='mode.exit.expired';append('normalize',g['normalize.entry'],g['normalize.exit'])
   rtl(g['normalize.exit'],g[stop])
  elif body==['mode.wait','attach.entry','offset.entry','offset.exit.point0','attach.exit','mode.exit.wait']:
   path=3;stop='mode.exit.wait';append('offset',g['offset.entry'],g['offset.exit.point0']);append('attach',g['attach.entry'],g['attach.exit'])
   need(g['offset.entry']['cpu_x']==g[stop]['actor_pointer']and w(g['offset.entry'],0)==0,'attachment did not request point0')
   same_registers(g['offset.entry'],g['offset.exit.point0'],('cpu_x','cpu_y','cpu_d','cpu_dbr'))
   need(g['offset.entry']['cpu_sp']==g['offset.exit.point0']['cpu_sp'],'offset stack not balanced')
  elif body==['mode.launch','launch.entry','launch.exit','launch.return','mode.exit.launch']:
   path=4;stop='launch.entry';append('after',g['launch.return'],g['mode.exit.launch'])
   rtl(g['launch.exit'],g['launch.return'])
   receiver=w(g['launch.entry'],0x946)
   need(receiver<10 and w(g['launch.entry'],0x8e)==pointers[receiver]and w(g['launch.entry'],0xaa)==receiver,'launch pointer not original selected receiver')
  else:raise ValueError('unsupported or misordered observed mode branch '+repr(body))
  if path in(1,2):need(not special,'nonowner timer unexpectedly turned')
  append('dispatch',g['dispatch.entry'],g['dispatch.call']);append('step',g['mode.entry'],g[stop],path);routes[path]+=1
  jsl(g['dispatch.call'],g['wrapper.entry'],0x87925c);jsl(g['wrapper.entry'],g['mode.entry'],0x879c57)
  mode_exit=g[tags[-3]];need(mode_exit['tag'].startswith('mode.exit.'),'mode did not reach actual RTL')
  rtl(mode_exit,g['wrapper.exit']);rtl(g['wrapper.exit'],row)
  need(g['dispatch.call']['cpu_x']==60 and g['dispatch.call']['cpu_a']==0x87,'mode15 table register contract')
  # A right696 interrupt changes stale stack bytes belowSP during this
  # straight-line dispatch. Active caller frames and all nonstack memory
  # remain exact; popped interrupt scratch is outside the typed C contract.
  irq_words=()
  if w(g['dispatch.entry'],0x5c8)!=w(g['dispatch.call'],0x5c8):
   # At right1461, the actual raw IRQ target changes85:EEEE->85:EF14.
   # Original85:EF05-EF0E installs exactly that successor and preserves A/P.
   # The writer PC itself is not hooked: this is source attribution of an
   # observed interrupt effect, outside C's declared DP/actor/global state.
   need((w(g['dispatch.entry'],0x5c8),w(g['dispatch.call'],0x5c8),w(g['dispatch.entry'],0x5ca),w(g['dispatch.call'],0x5ca))==
        (0xeeee,0xef14,0x85,0x85),'unbounded IRQ target transition')
   irq_words=(0x5c8,);irq_observations.append(dict(entry=g['dispatch.entry']['index'],exit=g['dispatch.call']['index'],court=row['court'],
    before=0x85eeee,after=0x85ef14,source_writer=0x85ef0e,writer_pc_observed=False))
  same_memory(g['dispatch.entry'],g['dispatch.call'],(0x8e,0x90)+irq_words,allow_stack=True)
  if path==2:need(w(row,row['actor_pointer']+0x5e)in(1,2),'normalizer did not restore team mode');origins.pop(row['actor_pointer'])
  calls.append(dict(call=completed_calls,origin=row['origin'],court=row['court'],entry=g['mode.entry']['index'],stop=g[stop]['index'],route=path,
   family=w(g['mode.entry'],row['actor_pointer']+0xc0),upper=w(g['mode.entry'],row['actor_pointer']+0x30),phase=w(g['mode.entry'],row['actor_pointer']+0x3a),
   held=w(g['mode.entry'],0x47f3),pressed=w(g['mode.entry'],0x47f9),launch_observed_not_replayed=path==4,
   return_court=row['court'],entry_court=g['mode.entry']['court']))
  group=None
 need(group is None and human is None and completed_calls==native_calls and completed_passes==native_passes,'incomplete native journey')
 need(stages['step']==stages['dispatch']==native_calls and all(routes[k]>0 for k in(1,2,3,4)),'missing positive native comparison coverage')
 need(routes[4]==native_passes and len({c['origin']for c in calls if c['route']==4})==native_passes,'missing human launch observation')
 need(routes[2]+len(origins)==native_passes,'missing cleanup or bounded remainder')
 pending_cleanup=[];last=state(rows[-1])
 for pointer,origin in origins.items():
  origin_calls=[c for c in calls if c['origin']==origin]
  need(origin_calls[-1]['route']==1 and word(last,pointer+0x5e)==15 and word(last,pointer+0x60)<0x8000 and
       word(last,0x93e)!=(pointer-0x34eb)//256,'unexplained unfinished mode15 origin')
  pending_cleanup.append(dict(origin=origin,actor_pointer=pointer,timer=word(last,pointer+0x60),
   last_call=origin_calls[-1]['call'],last_court=origin_calls[-1]['return_court'],reason='fixed capture end after observed launch; nonnegative mode15 timer remains'))
 run=subprocess.run([str(probe),str(assets),str(rom)],input='\n'.join(inputs)+'\n',text=True,capture_output=True,timeout=60)
 diagnostics.with_suffix('.probe-stdout.txt').write_text(run.stdout);diagnostics.with_suffix('.probe-stderr.txt').write_text(run.stderr)
 need(type(run.returncode)is int and run.returncode==0,'C probe failed')
 need(type(run.stderr)is str and run.stderr==f"[ASSETS] Loaded asset pack: '{assets}' ({assets.stat().st_size} bytes, 263 assets)\n",'unexpected C diagnostic protocol')
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
 return dict(kind='Natural mode15 dispatch/gate/attachment/cleanup; first missing launch99C4 observed only',passed=not failures,
  manifest_sha256=sha(capture/'manifest.json'),verifier_sha256=sha(__file__),probe_sha256=sha(probe),rom_sha256=sha(rom),assets_sha256=sha(assets),
  probe_arguments=[str(probe),str(assets),str(rom)],stages=dict(stages),routes=dict(routes),passes=passes,calls=calls,
  interrupt_target_observations=irq_observations,pending_cleanup_at_capture_end=pending_cleanup,compared_values=compared,failures=failures)

def main():
 p=argparse.ArgumentParser()
 for name in('capture','probe','rom','output'):p.add_argument('--'+name,type=Path,required=True)
 p.add_argument('--assets',type=Path,default=DEFAULT_ASSETS)
 a=p.parse_args();need(not a.output.exists(),'preserve old report')
 need(all(not a.output.with_suffix(s).exists()for s in('.probe-stdout.txt','.probe-stderr.txt')),'preserve earlier C output')
 try:report=verify(a.capture.resolve(),a.probe.resolve(),a.rom.resolve(),a.output,a.assets.resolve())
 except Exception as error:a.output.write_text(json.dumps(dict(passed=False,error=str(error)),indent=2)+'\n');raise
 a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k not in('calls','passes','failures')}))
 if report['failures']:print(json.dumps(report['failures'][:10]));raise SystemExit(1)
if __name__=='__main__':main()
