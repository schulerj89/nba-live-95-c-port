"""Compare natural99C4 and arithmetic helper entry states, with captured NMI effects separate."""
import argparse,collections,copy,hashlib,json,struct,subprocess
from pathlib import Path
import mesen_portable

ROM_SHA='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
MESEN_SHA='d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b'
RANGES=[[0,0x2000],[0x3400,0x1600]]
SOURCE_VERSION={'capture': 'b71ecca6cf1a8952c8b2387ee17e5c2136b017759d29e8ea19068eadeb375d0c', 'runner': '0a281da9bab1f994c5962546d037a35e4e9c232f97f3cacd4519681c97218020', 'isolation_helper': '1bc6db2d68d836c7c6af180137a3d5e8e4ea454d7cb8a97e9e95cc6312ddc3bb'}
PCS={'dispatch.call': 8884824, 'wrapper.entry': 8887379, 'mode.entry': 8824499, 'mul.exit.signed': 8779694, 'mul.exit.unsigned': 8779808, 'prediction.ready': 8821387, 'clamp.x.upper': 8821681, 'clamp.x.lower': 8821688, 'clamp.y.upper': 8821755, 'clamp.y.lower': 8821762, 'target.ready': 8821423, 'velocity.ready': 8821523, 'launch.restore': 8821650, 'launch.exit': 8821680, 'launch.return': 8824671, 'wrapper.exit': 8887383, 'nmi.exit.reentrant': 8421745, 'nmi.exit.normal': 8422811, 'player.entry': 8496265, 'court.entry': 8889466, 'human.entry': 8708780, 'pass.entry': 8707962, 'init.return': 8826701, 'human.return': 8884675, 'dispatch.entry': 8884804, 'launch.entry': 8821188, 'mul.entry': 8779659, 'divide.entry': 8779993, 'divide.exit': 8780072, 'nmi.entry': 8421722, 'mode.return': 8884828}
GLOBALS=[0x7f6,0x904,0x90c,0x90e,0x910,0x914,0x916,0x922,0x936,0x93a,0x93e,0x942,0x944,0x946,0x94a,0x978,0x9b8,0x9c4,0x9da]

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
 need(integer(m.get('schema'),1,1)and m.get('kind')=='native human99C4 launch boundaries','wrong capture schema')
 need(m.get('state_injection')is False and m.get('rom_patch')is False,'capture injected state')
 need(integer(m.get('exit_code'),0,0),'native process failed')
 need(integer(m.get('selection'),0,2)and m['selection']!=1,'invalid selection')
 need(integer(m.get('requested_frames'),400,3000),'invalid requested frames')
 need(m.get('sparse_ranges')==RANGES and all(type(v)is int for r in m['sparse_ranges']for v in r),'changed sparse schema')
 need(set(m.get('sources',{}))=={'rom','mesen','capture','runner','isolation_helper'},'missing source attestation')
 private={'mesen':'portable-mesen/Mesen.exe','capture':'capture.lua','runner':'capture_human_pass_launch.py','isolation_helper':'mesen_portable.py'}
 for key,name in private.items():need(Path(m['sources'][key]['path']).resolve()==capture/name,'nonprivate executed source')
 need(sha(rom)==ROM_SHA and m['sources']['rom']['sha256']==ROM_SHA and m['sources']['mesen']['sha256']==MESEN_SHA,'wrong original ROM/Mesen')
 need(m.get('arguments')==[str(capture/'portable-mesen/Mesen.exe'),'--testrunner','--timeout=240',
                         str(Path(m['sources']['rom']['path']).resolve()),str(capture/'capture.lua')],'changed executed arguments')
 need(m.get('environment')=={'NBA95_CAPTURE_DIR':capture.as_posix(),'NBA95_PASS_LAUNCH_SELECTION':str(m['selection']),
                            'NBA95_PASS_LAUNCH_FRAMES':str(m['requested_frames'])},'changed executed environment')
 for key,source in m['sources'].items():
  need(set(source)=={'path','sha256'}and type(source['path'])is str and type(source['sha256'])is str,'invalid source identity')
  need(sha(source['path'])==source['sha256'],'changed source '+key)
 for key,digest in SOURCE_VERSION.items():need(m['sources'][key]['sha256']==digest,'unsupported source version '+key)
 required={'boundaries.jsonl','capture.lua','capture_complete.txt','capture_human_pass_launch.py','mesen_portable.py',
           'observed-script-data-folder.txt','initial-mesen-settings.json','stdout.log','stderr.log'}
 artifacts=m.get('artifacts',{})
 need(required<=set(artifacts),'missing artifact attestation')
 actual_files={p.name for p in capture.iterdir()if p.is_file()and p.name!='manifest.json'}
 need(set(artifacts)==actual_files,'unattested or missing capture file')
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
 used_raw=set();fields={'index','tag','pc','frame','court','raw','cpu_a','cpu_x','cpu_y','cpu_ps','cpu_d','cpu_sp','cpu_dbr','cpu_k','cpu_pc','actor','actor_pointer','owner','live','offense','stack','origin','call','component','nmi_depth'}
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

MATH=[0x806,0x808,0x80a,0x80c,0x80e,0x810,0x820,0x822,0x824,0x85a]
SAVED=[0x9e,0xa0,0x9a,0x9c,0xbe,0xc0,0xba,0xbc,0xb6,0xb8]
OWNED_DP={a+i for a in SAVED+[0x92,0xb2,0xcc,0xce,0xd0]for i in(0,1)}

def verify(capture,probe,rom,diagnostics):
 manifest,rows,native_calls,native_passes=attest(capture,rom)
 inputs=[];expected=[];contexts=[];stages=collections.Counter();calls=[];nmis=[];origins={}
 def state(row):return raw(capture,row)
 def w(row,a):return word(state(row),a)
 def nz(value):return (0x80 if value&0x8000 else 0)|(2 if value==0 else 0)
 def same(a,b,keys):need(all(a[k]==b[k]for k in keys),'native register contract '+a['tag'])
 def rtl(a,b):
  need(a['cpu_sp']+3==b['cpu_sp'],'invalid RTL stack depth')
  need(len(a['stack'])>=3 and ((a['stack'][2]<<16)|(((a['stack'][1]<<8|a['stack'][0])+1)&65535))==b['pc'],'wrong RTL target')
  same(a,b,('cpu_a','cpu_x','cpu_y','cpu_ps','cpu_d','cpu_dbr'))
 def jsl(a,b,target):
  need(a['cpu_sp']-3==b['cpu_sp']and b['stack'][:3]==[(target-1)&255,((target-1)>>8)&255,target>>16],'wrong native JSL frame')
  same(a,b,('cpu_a','cpu_x','cpu_y','cpu_ps','cpu_d','cpu_dbr'))
 def append(mode,a,b):
  before=state(a);after=state(b);projected=after.copy();removed=[]
  # The C input is ALWAYS its original routine entry. NMI bytes are not fed
  # back into C. Separately prove all observed changes to unowned DP words
  # are exactly the captured NMI transitions, then test C's preservation of
  # the corresponding original entry bytes. Owned launch/math DP is never
  # exempted. Unknown unowned changes still fail the complete C comparison.
  changes={}
  for n in nmis:
   if a['index']<=n['entry']['index']and n['exit']['index']<=b['index']:
    first=state(n['entry']);last=state(n['exit'])
    for address in range(256):
     if first[address]==last[address]:continue
     need(address not in OWNED_DP,'NMI changed launch-owned arithmetic scratch')
     need(first[address]==changes.get(address,before[address]),'unowned DP changed before captured NMI')
     changes[address]=last[address]
  for address,value in changes.items():
   need(after[address]==value,'unowned DP changed after captured NMI')
   projected[address]=before[address];removed.append(dict(address=address,entry=before[address],native_after=value))
  regs=[]if mode=='launch'else[b['cpu_a'],b['cpu_x'],b['cpu_y']]
  path=str(capture/a['raw']);need('|'not in path and '\n'not in path,'unsupported protocol path')
  args=(0,0,0)if mode=='launch'else(a['cpu_a'],a['cpu_x'],a['cpu_y'])
  inputs.append(mode+'|'+path+'|'+'|'.join(map(str,args)));stages[mode]+=1
  expected.append(dict(result=1,return_words=regs,dp_words=words(projected,0,128),actor_words=words(projected,0x34eb,1408),
   controller_words=words(projected,0x47eb,160),context_words=words(projected,0x46eb,128),profile_words=words(projected,0x3449,20),
   order_words=words(projected,0x34d1,13),global_words=[word(projected,a)for a in GLOBALS],math_words=[word(projected,a)for a in MATH]))
  contexts.append(dict(mode=mode,entry=a['index'],exit=b['index'],court=a['court'],nmi_bytes_separately_attributed=removed))
 need(words(state(rows[0]),0x166d,5)==[2,1,1,1,1]and w(rows[1],0x166d)==manifest['selection'],'wrong native selection route')
 human=None;group=None;helper=None;nmi_stack=[];completed=passes=0
 for row in rows[2:]:
  tag=row['tag']
  if tag=='human.entry':
   need(human is None and group is None,'nested human origin');passes+=1
   need(row['origin']==passes and row['call']==completed and row['component']==0 and row['nmi_depth']==0,'wrong human origin counter')
   m=state(row);p=word(m,0x90c)
   need(p==0x47eb and word(m,0xae)&0x8000 and word(m,p+14)&0x8000 and word(m,p+2)==row['actor']==row['owner'],'human origin lacks native B/ownership')
   human=[row];continue
  if human is not None:
   need(tag==['human.entry','pass.entry','init.return','human.return'][len(human)]and row['origin']==passes and row['call']==completed and row['component']==0 and row['nmi_depth']==0,'misordered human origin')
   need(row['actor']==human[0]['actor']and row['actor_pointer']==human[0]['actor_pointer'],'human origin actor changed');human.append(row)
   if tag!='human.return':continue
   need(w(human[2],row['actor_pointer']+0x5e)==15 and w(row,row['actor_pointer']+0x5e)==15,'native initializer lacks mode15')
   origins[passes]=row['actor_pointer'];human=None;continue
  if tag=='dispatch.entry':
   need(group is None,'nested launch dispatch');completed+=1
   need(row['call']==completed and origins.get(row['origin'])==row['actor_pointer']and row['component']==0 and row['nmi_depth']==0,'launch lacks human origin')
   need(row['actor']<10 and row['actor_pointer']==0x34eb+row['actor']*256,'wrong source actor pointer')
   group=[row];continue
  need(group is not None and row['call']==completed and row['origin']==group[0]['origin'],'orphan launch boundary')
  need(row['actor']==group[0]['actor']and row['actor_pointer']==group[0]['actor_pointer'],'launch actor changed')
  previous=group[-1]
  need(row['court']>=previous['court']and row['court']<=previous['court']+1,'launch clock jump')
  if row['court']!=previous['court']:need(tag=='nmi.entry','unattributed launch frame crossing')
  group.append(row)
  if tag=='nmi.entry':
   need(row['component']==previous['component'],'NMI changed arithmetic component identity')
   need(len(row['stack'])>=4 and row['cpu_ps']==((row['stack'][0]&~8)|4),'NMI hardware status/frame contradiction')
   nmi_stack.append(row);need(row['nmi_depth']==len(nmi_stack),'wrong NMI entry depth');continue
  if tag.startswith('nmi.exit.'):
   need(nmi_stack and row['nmi_depth']==len(nmi_stack),'orphan NMI return');start=nmi_stack.pop()
   need(row['component']==start['component'],'NMI return changed arithmetic component identity')
   same(start,row,('cpu_a','cpu_x','cpu_y','cpu_d','cpu_dbr','cpu_sp'))
   need(start['stack'][:4]==row['stack'][:4]and len(row['stack'])>=4,'NMI changed native RTI frame')
   nmis.append(dict(entry=start,exit=row));continue
  need(row['nmi_depth']==0 and not nmi_stack,'ordinary launch boundary inside unclosed NMI')
  if tag in('mul.entry','divide.entry'):
   need(helper is None,'nested math helper');helper=row;continue
  if tag.startswith('mul.exit.')or tag=='divide.exit':
   need(helper is not None and tag.startswith(helper['tag'].split('.')[0]+'.exit')and row['component']==helper['component'],'orphan math exit')
   need(helper['cpu_sp']==row['cpu_sp']and helper['stack'][:3]==row['stack'][:3],'unbalanced math helper frame')
   append('mul'if tag.startswith('mul.')else'divide',helper,row);helper=None;continue
  if tag!='mode.return':continue
  need(helper is None and not nmi_stack,'unfinished arithmetic/interrupt')
  regular=[r for r in group if not r['tag'].startswith('nmi.')];tags=[r['tag']for r in regular]
  # Current natural routes do not clamp. Preserve capture/code for the
  # separate source-derived clamp children, but require a new evidence
  # contract before admitting a different native sequence here.
  want=['dispatch.entry','dispatch.call','wrapper.entry','mode.entry','launch.entry','mul.entry',None,'mul.entry',None,
   'prediction.ready','target.ready','divide.entry','divide.exit','divide.entry','divide.exit','velocity.ready',
   'launch.restore','launch.exit','launch.return','wrapper.exit','mode.return']
  need(len(tags)==len(want)and all(t in('mul.exit.signed','mul.exit.unsigned')if wanted is None else t==wanted for t,wanted in zip(tags,want)),'unsupported natural launch branch/order')
  g={r['tag']:r for r in regular if r['tag']not in('mul.entry','mul.exit.signed','mul.exit.unsigned','divide.entry','divide.exit')}
  for i,r in enumerate(regular):
   number=0 if i<5 else 1 if i<7 else 2 if i<11 else 3 if i<13 else 4
   need(r['component']==number,'wrong arithmetic component sequence')
  jsl(g['dispatch.call'],g['wrapper.entry'],0x87925c);jsl(g['wrapper.entry'],g['mode.entry'],0x879c57)
  need(w(g['dispatch.call'],0x8e)==0x9c53 and w(g['dispatch.call'],0x90)==0x87 and g['dispatch.call']['cpu_a']==0x87 and g['dispatch.call']['cpu_x']==60,'wrong native mode table')
  source=g['launch.entry']['actor_pointer'];receiver=w(g['launch.entry'],0x8e)
  need(source==w(g['launch.entry'],0x96)and receiver in range(0x34eb,0x3eeb,256),'unresolved receiver pointer')
  need(w(g['launch.entry'],0x946)==(receiver-0x34eb)//256 and w(g['launch.entry'],0xaa)==(receiver-0x34eb)//256,'receiver table/index contradiction')
  # Source load contracts bind actual CPU arithmetic inputs to raw WRAM.
  for position,lo,hi in((5,0xb6,None),(7,0xba,None),(11,0xb6,0xb8),(13,0xba,0xbc)):
   r=regular[position]
   if hi is None:need(r['cpu_a']==w(r,0xb2)and r['cpu_x']==w(r,lo),'mul operand/raw contradiction')
   else:need(r['cpu_a']==w(r,lo)and r['cpu_x']==w(r,hi)and r['cpu_y']==w(r,0xb2),'divide operand/raw contradiction')
  need(regular[5]['cpu_y']==source and regular[7]['cpu_y']==0,'mul carriedY differs from observed normal caller contract')
  # Bind caller status bits too: the last LDX/LDY sets N/Z while preserving
  # carry/overflow from the source ADC/SBC (or previous divide return).
  irq_flag=g['launch.entry']['cpu_ps']&4
  need(regular[5]['cpu_ps']==irq_flag|nz(regular[5]['cpu_x']),'first multiply source-load flags')
  product=(regular[6]['cpu_x']<<16)|regular[6]['cpu_a'];delta=(product>>8)&65535
  position=w(g['launch.entry'],receiver+4);summed=(position+delta)&65535
  cv=(1 if position+delta>65535 else 0)|(0x40 if (~(position^delta)&(position^summed)&0x8000)else 0)
  need(regular[7]['cpu_ps']==irq_flag|cv|nz(regular[7]['cpu_x']),'second multiply source-load flags')
  target_y=w(g['target.ready'],0xba);ball_y=w(g['launch.entry'],0x3ef3);difference=(target_y-ball_y)&65535
  cv=(1 if target_y>=ball_y else 0)|(0x40 if ((target_y^ball_y)&(target_y^difference)&0x8000)else 0)
  need(regular[11]['cpu_ps']==irq_flag|cv|nz(regular[11]['cpu_y']),'first divide source-load flags')
  need(regular[13]['cpu_ps']==(regular[12]['cpu_ps']&~0x82)|nz(regular[13]['cpu_y']),'second divide source-load flags')
  for start,end in((regular[5],regular[6]),(regular[7],regular[8])):
   flag_word=end['cpu_x']if end['tag']=='mul.exit.signed'and w(end,0x820)==0 else end['cpu_a']
   # F81F PLB sets8-bit N/Z from restoredDBR7E on the unsigned return.
   # SignedF7AE has later EOR/INC/INX flag writes instead.
   flag_nz=0 if end['tag']=='mul.exit.unsigned'else nz(flag_word)
   need(end['cpu_ps']==(start['cpu_ps']&4)|flag_nz,'multiply return status contract')
  for start,end in((regular[11],regular[12]),(regular[13],regular[14])):
   # These naturally observed shifted coordinate dividends have magnitude
   # below2^24 and positive duration<=30: high-word subtractions cannot
   # overflow. FinalF8C9 supplies denominator bit0 asC, F928 suppliesN/Z.
   magnitude=(start['cpu_x']<<16)|start['cpu_a']
   if magnitude&0x80000000:magnitude=(-magnitude)&0xffffffff
   need(magnitude<0x1000000 and 0<start['cpu_y']<=30,'unproven divide status domain')
   need(end['cpu_ps']==(start['cpu_ps']&4)|(start['cpu_y']&1)|nz(end['cpu_a']),'divide return status contract')
  entry=g['launch.entry'];restore=g['launch.restore'];end=g['launch.exit']
  saved=[w(entry,a)for a in SAVED];saved_bytes=[b for value in saved for b in(value&255,value>>8)]
  need(restore['cpu_sp']==entry['cpu_sp']-20 and restore['stack'][:20]==saved_bytes,'ten PEI words lack original entry frame')
  need(end['cpu_sp']==entry['cpu_sp']and all(w(end,a)==w(entry,a)for a in SAVED),'ten original scratch words not restored')
  need(end['cpu_a']==saved[-1]and end['cpu_x']==source and end['cpu_y']==w(entry,source+0x30),'launch return register contract')
  same(restore,end,('cpu_x','cpu_y','cpu_d','cpu_dbr'))
  flags=(restore['cpu_ps']&~0x82)|(0x80 if saved[-1]&0x8000 else 0)|(2 if saved[-1]==0 else 0)
  need(end['cpu_ps']==flags,'launch final PLA flags mismatch')
  rtl(end,g['launch.return']);rtl(g['wrapper.exit'],g['mode.return'])
  append('launch',entry,end)
  calls.append(dict(origin=entry['origin'],entry=entry['index'],exit=end['index'],court=entry['court'],return_court=end['court'],
   family=w(entry,source+0xc0),band=w(entry,source+0x62),receiver_pointer=receiver,held=w(entry,0x47f3)))
  group=None
 need(human is None and group is None and completed==native_calls==native_passes==passes,'incomplete natural launch journey')
 need(len({c['origin']for c in calls})==passes and stages==collections.Counter(launch=passes,mul=2*passes,divide=2*passes),'missing positive launch/helper coverage')
 run=subprocess.run([str(probe),str(rom)],input='\n'.join(inputs)+'\n',text=True,capture_output=True,timeout=60)
 diagnostics.with_suffix('.probe-stdout.txt').write_text(run.stdout);diagnostics.with_suffix('.probe-stderr.txt').write_text(run.stderr)
 need(type(run.returncode)is int and run.returncode==0,'launch C probe failed')
 need(type(run.stderr)is str and run.stderr=='','unexpected C diagnostic protocol; no asset loader')
 lines=run.stdout.splitlines();need(len(lines)==len(expected),'wrong C row count');failures=[];compared=0
 for i,(line,want)in enumerate(zip(lines,expected)):
  actual=json.loads(line,object_pairs_hook=unique);need(type(actual)is dict and actual.keys()==want.keys(),'wrong C fields')
  for key,value in want.items():
   if type(value)is list:
    need(type(actual[key])is list and len(actual[key])==len(value),'wrong C vector shape');pairs=enumerate(zip(actual[key],value))
   else:pairs=[(None,(actual[key],value))]
   for slot,(got,wanted)in pairs:
    compared+=1
    if type(got)is not int or got!=wanted:failures.append(dict(**contexts[i],field=key,slot=slot,actual=got,expected=wanted))
 return dict(kind='Natural99C4 and F78B/F8D9 complete typed memory/result contracts; NMI unowned DP effects separately witnessed',passed=not failures,
  manifest_sha256=sha(capture/'manifest.json'),verifier_sha256=sha(__file__),probe_sha256=sha(probe),rom_sha256=sha(rom),
  probe_arguments=[str(probe),str(rom)],stages=dict(stages),calls=calls,compared_values=compared,
  nmi_calls=[dict(entry=n['entry']['index'],exit=n['exit']['index'],court=n['entry']['court'])for n in nmis],
  comparisons_with_nmi_attribution=[c for c in contexts if c['nmi_bytes_separately_attributed']],failures=failures)

def main():
 p=argparse.ArgumentParser()
 for name in('capture','probe','rom','output'):p.add_argument('--'+name,type=Path,required=True)
 a=p.parse_args();need(not a.output.exists(),'preserve old report')
 need(all(not a.output.with_suffix(s).exists()for s in('.probe-stdout.txt','.probe-stderr.txt')),'preserve earlier C output')
 try:report=verify(a.capture.resolve(),a.probe.resolve(),a.rom.resolve(),a.output)
 except Exception as error:a.output.write_text(json.dumps(dict(passed=False,error=str(error)),indent=2)+'\n');raise
 a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k not in('calls','comparisons_with_nmi_attribution','failures')}))
 if report['failures']:print(json.dumps(report['failures'][:10]));raise SystemExit(1)
if __name__=='__main__':main()
