"""Compare actual AF1D pose/attachment children and AF30 commit; stop before stack epilogue."""
import argparse,collections,copy,hashlib,json,struct,subprocess
from pathlib import Path
import mesen_portable

ROM_SHA='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
MESEN_SHA='d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b'
RANGES=[[0,0x100],[0x500,0x500],[0x1600,0x300],[0x3400,0x1600]]
SOURCE_VERSION={'capture':'afa3f8f238bf5d048bbde481e8bacb27ccb4d83bcee6519d2c713ccfde52eb3c',
 'runner':'15fcf629f9700d460d194f0789234567bb3627b356e3c8e8c1ddfefec39d21de',
 'isolation_helper':'1bc6db2d68d836c7c6af180137a3d5e8e4ea454d7cb8a97e9e95cc6312ddc3bb'}
PCS={'player.entry':(0x81a489,),'court.entry':(0x87a47a,),'pass.entry':(0x84df7a,),
 'pass.directional':(0x84dfb2,),'pass.neutral':(0x84e0b5,),'candidate.directional':(0x84e016,),
 'candidate.neutral':(0x84e11e,),'pass.callsite':(0x84e098,),'pass.initialize':(0x86ab2d,),
 'pass.no_receiver':(0x84e09c,),'pass.resume':(0x84e09c,),'pass.return':(0x84e0b4,),
 'metric.entry':(0x85f1c1,),'metric.exit':(0x85f1f3,0x85f1ff,0x85f21c,0x85f228),
 'switch.observed':(0x84e141,),'init.cancel.entry':(0x87b538,),'init.cancel.exit':(0x87b554,),
 'init.prefix':(0x86ab83,),'init.geometry.entry':(0x86abe9,),'init.geometry.exit':(0x86abed,),
 'init.ready':(0x86ac50,),'init.revisit':(0x86ac50,),
 'action.offaxis':(0x86ad0e,),'action.normal':(0x86aca9,),'action.boost':(0x86afc4,),
 'action.ground.entry':(0x86b00b,),'action.ground.exit':(0x86b04b,),
 'action.upper.entry':(0x87b47a,),'action.upper.exit':(0x87b4da,),'action.ready':(0x86af1d,)}
RAW_TAGS={'player.entry','court.entry','pass.entry','pass.initialize','pass.no_receiver','pass.resume',
          'pass.return','metric.entry','metric.exit','init.cancel.entry','init.cancel.exit',
 'init.prefix','init.geometry.entry','init.geometry.exit','init.ready',
 'action.offaxis','action.normal','action.boost','action.ground.entry','action.ground.exit',
 'action.upper.entry','action.upper.exit','action.ready'}
PCS.update({'aligned.entry':(0x86ad0e,), 'aligned.catch':(0x86ad3d,),
 'aligned.choice.entry':(0x86ae10,), 'aligned.aligned':(0x86ae52,),
 'aligned.lane.entry':(0x85f473,), 'aligned.lane.exit':(0x85f5e3,),
 'aligned.chosen':(0x86aed9,), 'aligned.upper.entry':(0x87b47a,),
 'aligned.upper.exit':(0x87b4c0,0x87b4ce,0x87b4da,),
 'aligned.both':(0x87b3bd,), 'aligned.commit':(0x86af30,), 'aligned.ready':(0x86af1d,)})
RAW_TAGS.update(k for k in PCS if k.startswith('aligned.'))
PCS.update({'pose.entry':(0x86af1d,), 'pose.resolve.entry':(0x87aec3,), 'pose.resolve.exit':(0x87af74,),
 'pose.attach.entry':(0x87b649,), 'pose.offset.entry':(0x87b832,), 'pose.offset.exit':(0x87b8eb,0x87b952),
 'pose.attach.exit':(0x87b669,), 'pose.commit':(0x86af30,), 'pose.ready':(0x86af4d,)})
RAW_TAGS.update(k for k in PCS if k.startswith('pose.'))
GLOBALS=[0x90c,0x90e,0x910,0x936,0x93a,0x93e,0x944,0x946,0x978]
INIT_GLOBALS=[0x90c,0x90e,0x910,0x936,0x93a,0x93e,0x942,0x944,0x946,0x978,0x9b8,0x9c4,0x9da]
SAVED_CALLER=[0xb8,0xb6,0xbc,0xba,0xc0,0xbe,0x9c,0x9a]

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
 data=(capture/row['raw']).read_bytes();need(len(data)==7936,'incomplete sparse raw')
 memory={};offset=0
 for base,size in RANGES:
  memory.update((base+i,v)for i,v in enumerate(data[offset:offset+size]));offset+=size
 return memory

def attest(capture,rom):
 m=read_json(capture/'manifest.json')
 need(set(m)=={'schema','kind','selection','state_injection','rom_patch','sparse_ranges','requested_frames',
  'arguments','environment','isolation','sources','exit_code','completion','artifacts'},'wrong manifest fields')
 need(integer(m.get('schema'),1,1)and m.get('kind')=='native human pass pose boundaries','wrong capture schema')
 need(m.get('state_injection')is False and m.get('rom_patch')is False,'capture injected state')
 need(integer(m.get('exit_code'),0,0),'native process failed')
 need(integer(m.get('selection'),0,2)and m['selection']!=1,'invalid selection')
 need(integer(m.get('requested_frames'),400,3000),'invalid requested frames')
 need(m.get('sparse_ranges')==RANGES and all(type(v)is int for r in m['sparse_ranges']for v in r),'changed sparse schema')
 need(set(m.get('sources',{}))=={'rom','mesen','capture','runner','isolation_helper'},'missing source attestation')
 private={'mesen':'portable-mesen/Mesen.exe','capture':'capture.lua','runner':'capture_human_pass_pose.py','isolation_helper':'mesen_portable.py'}
 for key,name in private.items():need(Path(m['sources'][key]['path']).resolve()==capture/name,'nonprivate executed source')
 need(sha(rom)==ROM_SHA and m['sources']['rom']['sha256']==ROM_SHA and m['sources']['mesen']['sha256']==MESEN_SHA,'wrong original ROM/Mesen')
 need(m.get('arguments')==[str(capture/'portable-mesen/Mesen.exe'),'--testrunner','--timeout=240',
                         str(Path(m['sources']['rom']['path']).resolve()),str(capture/'capture.lua')],'changed executed arguments')
 need(m.get('environment')=={'NBA95_CAPTURE_DIR':capture.as_posix(),'NBA95_PASS_POSE_SELECTION':str(m['selection']),
                            'NBA95_PASS_POSE_FRAMES':str(m['requested_frames'])},'changed executed environment')
 for key,source in m['sources'].items():
  need(set(source)=={'path','sha256'}and type(source['path'])is str and type(source['sha256'])is str,'invalid source identity')
  need(sha(source['path'])==source['sha256'],'changed source '+key)
 for key,digest in SOURCE_VERSION.items():need(m['sources'][key]['sha256']==digest,'unsupported source version '+key)
 required={'boundaries.jsonl','capture.lua','capture_complete.txt','capture_human_pass_pose.py','mesen_portable.py',
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
 used_raw=set();fields={'index','tag','pc','frame','court','raw','cpu_d','cpu_x','cpu_y','cpu_ps','actor','owner','live','offense','direction','candidate','score'}
 last_frame=last_court=-1
 court_frame=4590 if m['selection']==0 else 4390
 for index,row in enumerate(rows,1):
  need(set(row)==fields,'invalid native event fields')
  need(type(row['tag'])is str and row['tag']in PCS,'unknown native event')
  bounds={'index':(1,len(rows)),'pc':(0,0xffffff),'frame':(0,17999),
          'court':(-1,int(complete['frames'])-1),'cpu_ps':(0,255)}
  for field in fields-{'tag','raw'}:
   lo,hi=bounds.get(field,(0,0xffff))
   need(integer(row[field],lo,hi),'invalid event numeric type/range '+field)
  need(row['index']==index and row['pc']in PCS[row['tag']]and row['cpu_d']==0,'invalid boundary/order/DP')
  # These exact initial clock anchors belong to the supported script/runner
  # versions and both fresh deterministic routes, not to arbitrary inputs.
  if index==1:need(row['tag']=='player.entry'and row['court']==-1 and row['frame']==2697,'changed Player route clock')
  elif index==2:need(row['tag']=='court.entry'and row['court']==0 and row['frame']==court_frame,'changed court route clock')
  else:need(row['court']>=0 and row['frame']-row['court']==court_frame,'changed relative court clock')
  need(row['frame']>=last_frame and row['court']>=last_court,'native time reversed');last_frame=row['frame'];last_court=row['court']
  if row['tag']in RAW_TAGS:
   name=f'raw_{index:05d}.bin';need(row['raw']==name and name in artifacts,'missing attested raw boundary');used_raw.add(name)
   state=raw(capture,row)
   for field,address in {'actor':0xc2,'owner':0x93e,'live':0x936,'offense':0x93a,'candidate':0x92,'score':0xaa}.items():
    need(row[field]==word(state,address),'raw/event disagreement '+field)
   pointer=word(state,0x90c)
   need(row['direction']==(word(state,pointer+6)if pointer>=0x47eb else 0xffff),'raw/event disagreement direction')
  else:need(row['raw']=='','unexpected raw boundary')
 need(set(artifacts)==required|used_raw,'unexpected or unbound artifacts')
 return m,rows,int(complete['calls'])

DEFAULT_ASSETS=Path(r'C:\Users\joshs\Projects\nba-live-95-c-port\.analysis\frontend-integration-20260830\nba95_assets_candidate.pak')
ASSETS_SHA='951f82331c4bb6ce8f381da519ee8bfdf517bf8c13f2cd6f20cfa9c34d5ed4df'

def verify(capture,probe,rom,diagnostics,assets=DEFAULT_ASSETS):
 m,rows,native_calls=attest(capture,rom)
 need(sha(assets)==ASSETS_SHA,'wrong immutable asset pack');rom_data=rom.read_bytes()
 pose_stages=collections.Counter();pose_calls=[];
 aligned_stages=collections.Counter();aligned_routes=collections.Counter();aligned_calls=[];lane_calls=[];aligned_requests=[]
 action_stages=collections.Counter();action_routes=collections.Counter();upper_requests=[];ground_calls=[]
 pending=metric=None;special=collections.Counter();coverage=collections.Counter();routes=collections.Counter()
 metric_exits=collections.Counter();inputs=[];expected=[];contexts=[];crossings=[];ties=[];switches=0
 completed=0;child_boundaries=[];stages=collections.Counter();cancel_locks=collections.Counter();bands=collections.Counter();revisits=[]
 def observe_append(mode,entry,end,want):
  # Earlier DF7A/AB2D stages are checked structurally, not replayed by this probe.
  if mode not in('gate','choice','install','lane','upper'):return
  aligned_stages[mode]+=1
  # Earlier aligned stages are observed only by this AF1D probe.
 def init_expected(end,mode,route=1,fine=None):
  return dict(route=route,
   prefix_words=[word(end,a)for a in(0xe6,0xe8,0x9a)]if route and mode in('prefix','chain')else[],
   geometry_words=([word(fine,a)for a in(0xaa,0xae)]+[word(end,a)for a in(0xc0,0xbe,0xaa,0xb2,0x4f,0xba,0x51,0x8e)])if route and mode in('geometry','chain')else[],
   actor_words=words(end,0x34eb,11*128),controller_words=words(end,0x47eb,160),
   context_words=words(end,0x46eb,128),profile_words=words(end,0x3449,20),global_words=[word(end,a)for a in INIT_GLOBALS])
 def action_expected(entry,end,route):
  pre=raw(capture,entry);address=word(pre,0xe6);bank=word(pre,0xe8)&255
  offset=(bank&0x7f)*0x8000+(address&0x7fff)+0x3e
  need(address>=0x8000 and offset<len(rom_data),'invalid native profile pointer')
  return dict(route=route,profile_byte=rom_data[offset],dp_words=[word(end,a)for a in(0,0x47,0x49,0x4f,0x51,0xbe)],
   actor_words=words(end,0x34eb,11*128),controller_words=words(end,0x47eb,160),context_words=words(end,0x46eb,128),
   profile_words=words(end,0x3449,20),global_words=[word(end,a)for a in INIT_GLOBALS])
 def aligned_expected(entry,end,mode,route=1):
  pre=raw(capture,entry);address=word(pre,0xe6);bank=word(pre,0xe8)&255
  offset=(bank&0x7f)*0x8000+(address&0x7fff)+0x3e
  need(address>=0x8000 and offset<len(rom_data),'invalid native profile pointer')
  want=dict(profile_byte=rom_data[offset],selection_words=[word(end,a)for a in(0x7f6,0x956)],
   order_words=words(end,0x34d1,13),actor_words=words(end,0x34eb,11*128),
   controller_words=words(end,0x47eb,160),context_words=words(end,0x46eb,128),
   profile_words=words(end,0x3449,20),global_words=[word(end,a)for a in INIT_GLOBALS])
  if mode=='lane':
   want.update(obstructed=word(end,0xaa),saved_words=[word(end,a)for a in(0xb6,0xba,0xbe,0xc2,0x9a,0x9e,0xa6)])
  else:want.update(route=route,dp_words=[word(end,a)for a in(0,0x47,0x49,0x4f,0x51,0xbe,0xc0,0xb2,0xaa,0xae)])
  return want
 def append_pose(mode,entry,row):
  end=raw(capture,row)
  want=dict(route=1,dp_words=[word(end,a)for a in(0,2,4,6,0x47,0x49,0xac)],
   actor_words=words(end,0x34eb,11*128),controller_words=words(end,0x47eb,160),context_words=words(end,0x46eb,128),
   profile_words=words(end,0x3449,20),order_words=words(end,0x34d1,13),
   global_words=[word(end,a)for a in(0x90c,0x90e,0x910,0x922,0x936,0x93a,0x93e,0x942,0x944,0x946,0x978,0x9b8,0x9c4,0x9da)])
  inputs.append(mode+' '+str(capture/entry['raw']));expected.append(want);pose_stages[mode]+=1
  context=dict(mode=mode,entry=entry['index'],exit=row['index'],court=entry['court']);contexts.append(context)
  if entry['court']!=row['court']:crossings.append(context)
 for row in rows:
  tag=row['tag']
  if tag in('player.entry','court.entry'):
   need(pending is None,'initialization inside pass');special[tag]+=1;state=raw(capture,row)
   if tag=='player.entry':need(words(state,0x166d,5)==[2,1,1,1,1],'wrong fresh selection state')
   else:need(word(state,0x166d)==m['selection'],'wrong native court selection')
   continue
  if tag=='switch.observed':need(pending is None,'switch nested inside pass');switches+=1;continue
  if tag=='pass.entry':
   need(pending is None and special['court.entry']==1,'nested or precourt pass')
   pending=dict(entry=row,state=raw(capture,row),phase='selection',scan=None,scan_count=0,score=0x640)
   continue
  need(pending is not None,'pass event without entry')
  phase=pending['phase']
  if tag in('pass.directional','pass.neutral'):
   need(phase=='selection'and metric is None and pending['scan']in(None,tag),'misordered scan')
   need(row['candidate']==word(pending['state'],word(pending['state'],0x9e)+4)+pending['scan_count']*0x100,'changed native scan sequence')
   pending['scan_count']+=1;need(pending['scan_count']<=5,'too many candidate scans');pending['scan']=tag;continue
  if tag=='metric.entry':
   need(phase=='selection'and metric is None and pending['scan']=='pass.neutral','misordered metric entry')
   metric=(row,raw(capture,row));continue
  if tag=='metric.exit':
   need(phase=='selection'and metric is not None,'unpaired metric exit')
   observe_append('metric',metric[0],row,dict(distance=word(raw(capture,row),0xaa)))
   metric_exits[row['pc']]+=1;metric=None;continue
  if tag.startswith('candidate.'):
   need(phase=='selection'and metric is None and pending['scan']=='pass.'+tag.split('.')[1],'misordered candidate')
   if row['score']==pending['score']:ties.append(dict(entry=pending['entry']['index'],event=row['index'],scan=pending['scan']))
   pending['score']=row['score'];continue
  if tag=='pass.callsite':
   need(phase=='selection'and metric is None and pending['scan_count']==5,'incomplete pass scan')
   pending['phase']='callsite';continue
  if tag in('pass.initialize','pass.no_receiver'):
   route=int(tag=='pass.initialize')
   need(phase==('callsite'if route else'selection')and metric is None and pending['scan_count']==5,'invalid selection boundary')
   entry,state=pending['entry'],pending['state'];end=raw(capture,row)
   want=dict(route=route,score=word(end,0xba),handoff_words=[word(end,0xaa),word(end,0x8e)]if route else[],
    actor_words=words(end,0x34eb,11*128),controller_words=words(end,0x47eb,160),
    context_words=words(end,0x46eb,128),global_words=[word(end,a)for a in GLOBALS])
   observe_append('pass',entry,row,want);routes[route]+=1
   actor=word(state,0x96);need(0x34eb<=actor<0x3eeb and(actor-0x34eb)%0x100==0,'invalid actor pointer')
   gamephase='tip'if word(state,0x936)==0x81 else'inbound'if word(state,0x936)==0x82 else'live'
   side='offense'if word(state,actor+0x6e)==word(state,0x93a)else'defense'
   coverage[(pending['scan'],gamephase,side,entry['direction'])]+=1
   pending['phase']='child'if route else'restoring'
   if route:
    pending['child_entry']=row['index'];pending['init_entry']=row;pending['init_step']='cancel_entry'
   else:
    observe_append('chain',entry,row,init_expected(end,'chain',0));stages['chain']+=1
   continue
  if tag.startswith('init.'):
   need(phase=='child'and metric is None,'initializer outside child')
   step=pending['init_step']
   if tag=='init.cancel.entry':
    need(step=='cancel_entry','misordered cancel entry');pending['cancel_entry']=row
    end=raw(capture,row);cancel_locks[word(end,word(end,0x96)+0x46)]+=1;pending['init_step']='cancel_exit'
   elif tag=='init.cancel.exit':
    need(step=='cancel_exit','misordered cancel exit')
    observe_append('cancel',pending['cancel_entry'],row,init_expected(raw(capture,row),'cancel'));stages['cancel']+=1
    pending['init_step']='prefix'
   elif tag=='init.prefix':
    need(step=='prefix','misordered prefix')
    observe_append('prefix',pending['init_entry'],row,init_expected(raw(capture,row),'prefix'));stages['prefix']+=1
    pending['prefix_entry']=row;pending['init_step']='geometry_entry'
   elif tag=='init.geometry.entry':
    need(step=='geometry_entry','misordered fine entry');pending['fine_entry']=row;pending['init_step']='geometry_exit'
   elif tag=='init.geometry.exit':
    need(step=='geometry_exit','misordered fine exit');end=raw(capture,row)
    observe_append('fine',pending['fine_entry'],row,dict(distance=word(end,0xaa),fine=word(end,0xb2)));stages['fine']+=1
    pending['init_step']='ready'
   elif tag=='init.ready':
    need(step=='ready','misordered first AC50');end=raw(capture,row)
    fine=raw(capture,pending['fine_entry'])
    observe_append('geometry',pending['prefix_entry'],row,init_expected(end,'geometry',fine=fine));stages['geometry']+=1
    observe_append('chain',pending['entry'],row,init_expected(end,'chain',fine=fine));stages['chain']+=1
    bands[word(end,0xb2)]+=1;pending['ready_entry']=row;pending['init_step']='continuation'
    pending['action_entry']=row;pending['action_step']='gate';pending['upper_entry']=None
   else:
    need(tag=='init.revisit'and step=='continuation','unexpected AC50 revisit')
    revisits.append(dict(entry=pending['entry']['index'],event=row['index'],replayed=False))
   continue
  if tag.startswith('action.'):
   need(phase=='child'and pending['init_step']=='continuation','action outside first AC50')
   step=pending['action_step']
   if tag in('action.offaxis','action.normal','action.boost'):
    need(step=='gate','misordered action continuation')
    route={'action.offaxis':0,'action.normal':1,'action.boost':2}[tag]
    observe_append('observed_gate',pending['action_entry'],row,action_expected(pending['action_entry'],raw(capture,row),route))
    action_stages['gate']+=1;action_routes[route]+=1;pending['action_step']='done';pending['action_stop']=row
   elif tag=='action.ground.entry':
    need(step=='gate','misordered ground entry');pending['ground_entry']=row;pending['ground_upper']=[];pending['action_step']='ground'
   elif tag=='action.upper.entry':
    need(step=='ground'and pending['upper_entry']is None,'misordered upper entry')
    pending['upper_entry']=row;pre=raw(capture,row);actor=word(pre,0x96)
    pending['ground_upper'].append(word(pre,0));upper_requests.append(dict(event=row['index'],request=word(pre,0),state=word(pre,actor+0x30),lock=word(pre,actor+0x46)))
   elif tag=='action.upper.exit':
    need(step=='ground'and pending['upper_entry']is not None,'misordered upper exit')
    observe_append('observed_upper',pending['upper_entry'],row,action_expected(pending['upper_entry'],raw(capture,row),3));action_stages['upper']+=1
    pending['upper_entry']=None
   elif tag=='action.ground.exit':
    need(step=='ground'and pending['upper_entry']is None,'misordered ground exit')
    pre=raw(capture,pending['ground_entry']);commands=[0x2c,0x2f]if word(pre,0x4f)>=0xf1 else[0x2f]
    need(pending['ground_upper']==commands,'incomplete native upper sequence')
    observe_append('observed_ground',pending['ground_entry'],row,action_expected(pending['ground_entry'],raw(capture,row),3));action_stages['ground']+=1
    ground_calls.append(dict(entry=pending['ground_entry']['index'],exit=row['index'],requests=commands));pending['action_step']='ground_return'
   else:
    need(tag=='action.ready'and step=='ground_return','misordered pose continuation')
    observe_append('observed_gate',pending['action_entry'],row,action_expected(pending['action_entry'],raw(capture,row),3));action_stages['gate']+=1;action_routes[3]+=1
    pending['action_step']='done';pending['action_stop']=row
   continue
  if tag.startswith('aligned.'):
   need(phase=='child'and pending['action_step']=='done'and pending['action_stop']['tag']=='action.offaxis','aligned outside AD0E continuation')
   step=pending.get('aligned_step')
   if tag=='aligned.entry':
    need(step is None,'duplicate aligned entry')
    previous=pending['action_stop']
    need(row['index']==previous['index']+1 and row['frame']==previous['frame']and row['court']==previous['court']and raw(capture,row)==raw(capture,previous),'changed shared AD0E boundary')
    pending['aligned_entry']=row;pending['aligned_step']='gates';pending['aligned_lane']=None
   elif tag=='aligned.choice.entry':
    need(step=='gates','misordered AE10 choice');pending['choice_entry']=row;pending['aligned_step']='choice'
   elif tag=='aligned.aligned':
    need(step=='choice','misordered AE52 alignment');pending['alignment_entry']=row;pending['aligned_step']='aligned'
   elif tag=='aligned.lane.entry':
    need(step=='aligned'and pending['aligned_lane']is None,'misordered lane entry')
    pre=raw(capture,pending['alignment_entry'])
    need(word(pre,word(pre,0x8e)+0x5e)!=14,'unexpected lane for receiver mode14')
    pending['lane_entry']=row;pending['aligned_step']='lane'
   elif tag=='aligned.lane.exit':
    need(step=='lane','unpaired lane exit');end=raw(capture,row)
    observe_append('lane',pending['lane_entry'],row,aligned_expected(pending['lane_entry'],end,'lane'))
    lane_calls.append(dict(entry=pending['lane_entry']['index'],exit=row['index'],court=row['court'],obstructed=word(end,0xaa)))
    pending['aligned_lane']=row;pending['aligned_step']='aligned'
   elif tag=='aligned.chosen':
    need(step in('choice','aligned'),'misordered selected upper/family')
    if step=='aligned':
     pre=raw(capture,pending['alignment_entry'])
     need((pending['aligned_lane']is not None)==(word(pre,word(pre,0x8e)+0x5e)!=14),'missing lane child')
    end=raw(capture,row)
    observe_append('choice',pending['choice_entry'],row,aligned_expected(pending['choice_entry'],end,'choice'))
    pending['install_entry']=row;pending['aligned_step']='install'
    aligned_requests.append(dict(event=row['index'],court=row['court'],request=word(end,0),family=word(end,0xae)))
   elif tag=='aligned.upper.entry':
    need(step=='install','misordered aligned upper entry');pending['aligned_upper_entry']=row;pending['aligned_step']='upper'
   elif tag=='aligned.upper.exit':
    need(step=='upper','unpaired aligned upper exit')
    observe_append('upper',pending['aligned_upper_entry'],row,aligned_expected(pending['aligned_upper_entry'],raw(capture,row),'upper'))
    pending['aligned_step']='upper_done'
   else:
    need(tag in('aligned.catch','aligned.both','aligned.commit','aligned.ready'),'unknown aligned terminal')
    required_step={'aligned.catch':'gates','aligned.both':'install','aligned.commit':'install','aligned.ready':'upper_done'}[tag]
    need(step==required_step,'misordered aligned terminal')
    route={'aligned.catch':0,'aligned.ready':1,'aligned.both':2,'aligned.commit':3}[tag];end=raw(capture,row)
    if tag!='aligned.catch':observe_append('install',pending['install_entry'],row,aligned_expected(pending['install_entry'],end,'install',route))
    observe_append('gate',pending['aligned_entry'],row,aligned_expected(pending['aligned_entry'],end,'gate',route))
    aligned_routes[route]+=1;aligned_calls.append(dict(entry=pending['aligned_entry']['index'],exit=row['index'],court=pending['aligned_entry']['court'],route=route,pc=row['pc']))
    pending['aligned_step']='done';pending['aligned_stop']=row
   continue
  if tag.startswith('pose.'):
   need(phase=='child'and pending['init_step']=='continuation','pose outside actual pass child')
   step=pending.get('pose_step');state=raw(capture,row)
   need((row['cpu_ps']&0x30)==0,'pose requires native 16-bit A/X')
   if tag=='pose.entry':
    need(step is None,'duplicate AF1D pose');pending['pose_entry']=row;pending['pose_step']='resolve_wait'
   elif tag=='pose.resolve.entry':
    need(step=='resolve_wait','misordered pose resolver');pending['resolve_entry']=row;pending['pose_step']='resolve'
    need(raw(capture,pending['pose_entry'])==state,'AF1D JSL changed input memory')
   elif tag=='pose.resolve.exit':
    need(step=='resolve','misordered pose resolver exit');need(row['cpu_x']==word(state,0x96),'pose resolver lost actor X')
    append_pose('resolve',pending['resolve_entry'],row);pending['pose_step']='attach_wait'
   elif tag=='pose.attach.entry':
    need(step=='attach_wait','misordered B649');pending['attach_entry']=row;pending['pose_step']='offset_wait'
   elif tag=='pose.offset.entry':
    need(step=='offset_wait','misordered B832');need(row['cpu_x']==word(state,0x96)and word(state,0)==0,'wrong B649 point/actor input')
    pending['offset_entry']=row;pending['pose_step']='offset'
   elif tag=='pose.offset.exit':
    need(step=='offset','misordered B832 exit')
    entry=pending['offset_entry'];need(row['cpu_x']==entry['cpu_x']and row['cpu_y']==entry['cpu_y'],'B832 did not restore X/Y')
    append_pose('offset',entry,row);pending['pose_step']='attach_return'
   elif tag=='pose.attach.exit':
    need(step=='attach_return','misordered B649 return');need(row['cpu_x']==word(state,0x96),'B649 actor X changed')
    append_pose('attach',pending['attach_entry'],row);pending['pose_step']='commit_wait'
   elif tag=='pose.commit':
    need(step=='commit_wait','misordered AF30 commit');need(row['cpu_x']==word(state,0x96),'AF30 actor X disagrees with native caller')
    append_pose('prefix',pending['pose_entry'],row);pending['commit_entry']=row;pending['pose_step']='commit'
   else:
    need(tag=='pose.ready'and step=='commit','misordered AF4D stack continuation')
    append_pose('commit',pending['commit_entry'],row);append_pose('chain',pending['pose_entry'],row)
    pose_calls.append(dict(entry=pending['pose_entry']['index'],exit=row['index'],court=pending['pose_entry']['court']))
    pending['pose_step']='done';pending['pose_stop']=row
   continue
  if tag=='pass.resume':
   need(phase=='child'and metric is None and pending['init_step']=='continuation'and pending['action_step']=='done','unexpected initializer resume');raw(capture,row)
   if pending['action_stop']['tag']=='action.offaxis':need(pending.get('aligned_step')=='done','missing aligned completion')
   need(pending.get('pose_step')in(None,'done'),'unfinished pose continuation')
   stop=pending.get('pose_stop',pending.get('aligned_stop',pending['action_stop']))
   child_boundaries.append(dict(entry=stop['index'],pc=stop['pc'],exit=row['index'],replayed=False))
   pending['phase']='restoring';continue
  need(tag=='pass.return'and phase=='restoring'and metric is None,'bad pass return')
  returned=raw(capture,row)
  need(all(word(pending['state'],a)==word(returned,a)for a in SAVED_CALLER),'original pass epilogue did not restore saved words')
  completed+=1;pending=None
 need(pending is None and metric is None and completed==native_calls and sum(routes.values())==native_calls,'missing pass calls')
 need(stages==collections.Counter(cancel=routes[1],prefix=routes[1],fine=routes[1],geometry=routes[1],chain=native_calls)and routes[1]>0,'missing initializer stages')
 need(special==collections.Counter({'player.entry':1,'court.entry':1}),'missing native initialization')
 need(all(any(k[0]==scan for k in coverage)for scan in('pass.neutral','pass.directional'))and sum(metric_exits.values())>0,'missing pass/metric coverage')
 need(action_stages['gate']==routes[1]and action_stages['ground']==action_routes[3]and action_stages['upper']>0,'missing action coverage')
 # This right-side route never enters AE52/F473. Report that zero explicitly;
 # require actual AD0E comparisons without inventing a leaf witness per route.
 need(aligned_stages['gate']==action_routes[0]and aligned_stages['gate']>0 and aligned_stages['choice']==aligned_stages['install'],'missing aligned coverage')
 need(pose_stages['chain']>0 and all(pose_stages[k]==pose_stages['chain']for k in('resolve','offset','attach','prefix','commit')),'missing pose comparisons')
 run=subprocess.run([str(probe),str(assets),str(rom)],input='\n'.join(inputs)+'\n',text=True,capture_output=True,timeout=60)
 diagnostics.with_suffix('.probe-stdout.txt').write_text(run.stdout);diagnostics.with_suffix('.probe-stderr.txt').write_text(run.stderr)
 need(type(run.returncode)is int and run.returncode==0,f'probe failed {run.returncode}: {run.stderr}')
 # This SHA-pinned pack contains exactly263 assets. The unmodified probe
 # redirects the source loader's one success line to stderr; no other text
 # is a valid success protocol, even if every stdout word happens to match.
 expected_stderr=f"[ASSETS] Loaded asset pack: '{assets}' ({assets.stat().st_size} bytes, 263 assets)\n"
 need(type(run.stderr)is str and run.stderr==expected_stderr,'unexpected C diagnostic protocol')
 lines=run.stdout.splitlines();need(len(lines)==len(expected),'wrong C result count')
 failures=[];compared=0
 for i,(line,want)in enumerate(zip(lines,expected)):
  actual=json.loads(line,object_pairs_hook=unique);need(actual.keys()==want.keys(),'wrong C fields')
  for key,value in want.items():
   if isinstance(value,list):
    need(type(actual[key])is list and len(actual[key])==len(value),'wrong C vector shape');pairs=enumerate(zip(actual[key],value))
   else:pairs=[(None,(actual[key],value))]
   for slot,(got,v)in pairs:
    compared+=1
    if type(got)is not int or got!=v:failures.append(dict(**contexts[i],field=key,slot=slot,expected=v,actual=got))
 return dict(kind='AF1D pose/attachment children and AF30 state commit; stop before AF4D stack epilogue',passed=not failures,
  manifest_sha256=sha(capture/'manifest.json'),probe_sha256=sha(probe),rom_sha256=sha(rom),verifier_sha256=sha(__file__),
  calls=native_calls,pose_stages=dict(pose_stages),native_pose_calls=pose_calls,earlier_aligned_stages_observed=dict(aligned_stages),aligned_routes=dict(aligned_routes),native_aligned_calls=aligned_calls,native_lane_calls=lane_calls,native_aligned_requests=aligned_requests,earlier_action_stages_observed=dict(action_stages),earlier_action_routes_observed=dict(action_routes),earlier_upper_requests_observed=upper_requests,earlier_ground_calls_observed=ground_calls,assets_sha256=sha(assets),probe_arguments=[str(probe),str(assets),str(rom)],earlier_initializer_stages_observed_not_replayed=dict(stages),earlier_cancel_locks_observed=dict(cancel_locks),earlier_bands_observed=dict(bands),unreplayed_AC50_revisits=revisits,earlier_metric_calls_observed=sum(metric_exits.values()),earlier_metric_exits_observed=dict(metric_exits),compared_values=compared,
  earlier_selection_routes_observed=dict(routes),frame_crossings=crossings,earlier_selection_ties_observed=ties,
  initializer_continuations_observed_not_replayed=child_boundaries,switch_calls_observed_not_replayed=switches,
  earlier_selection_coverage_observed=[dict(scan=k[0],phase=k[1],side=k[2],direction=k[3],calls=v)for k,v in coverage.items()],failures=failures)

def main():
 p=argparse.ArgumentParser()
 for name in('capture','probe','rom','output'):p.add_argument('--'+name,type=Path,required=True)
 p.add_argument('--assets',type=Path,default=DEFAULT_ASSETS)
 a=p.parse_args();need(not a.output.exists(),'preserve earlier report')
 need(all(not a.output.with_suffix(s).exists()for s in('.probe-stdout.txt','.probe-stderr.txt')),'preserve earlier probe output')
 try:report=verify(a.capture.resolve(),a.probe.resolve(),a.rom.resolve(),a.output,a.assets.resolve())
 except Exception as error:a.output.write_text(json.dumps(dict(passed=False,error=str(error)),indent=2)+'\n');raise
 a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k not in('earlier_selection_coverage_observed','failures','initializer_continuations_observed_not_replayed')}))
 if report['failures']:print(json.dumps(report['failures'][:10]));raise SystemExit(1)

if __name__=='__main__':main()

