"""Compare complete switch persistent effects with original entry/return captures."""
import argparse,collections,copy,hashlib,json,struct,subprocess
from pathlib import Path
import mesen_portable

ROM_SHA='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
MESEN_SHA='d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b'
RANGES=[[0,0x100],[0x500,0x500],[0x1600,0x300],[0x3400,0x1600]]
SOURCE_VERSION={'capture':'7d4402650c0ba112fe8e273f73375927c2784918ea887a8b42aa0144c21b8f9d',
 'runner':'33d71c2d8228befea2f83adea4e4070b5a31c58ea8767051e7d2bf9caff03707',
 'isolation_helper':'1bc6db2d68d836c7c6af180137a3d5e8e4ea454d7cb8a97e9e95cc6312ddc3bb'}
PCS={'player.entry':0x81a489,'court.entry':0x87a47a,'switch.entry':0x84e141,
 'switch.full':0x84e14b,'switch.exit':0x84e230,'scan.directional':0x84e175,
 'scan.neutral':0x84e231,'candidate.directional':0x84e1d9,'candidate.neutral':0x84e28c,
 'switch.transfer':0x84e1fc,'switch.restore':0x84e21e,'pass.observed':0x84df7a}
RAW_TAGS={'player.entry','court.entry','switch.entry','switch.full','switch.exit'}
GLOBALS=[0x90c,0x90e,0x910,0x936,0x93a,0x93e,0x944,0x946,0x978,0xa6,0x9a,0xc2,0xbe,0xba,0xb6]

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
 need(integer(m.get('schema'),1,1)and m.get('kind')=='native human switch boundaries','wrong capture schema')
 need(m.get('state_injection')is False and m.get('rom_patch')is False,'capture injected state')
 need(integer(m.get('exit_code'),0,0),'native process failed')
 need(integer(m.get('selection'),0,2)and m['selection']!=1,'invalid selection')
 need(integer(m.get('requested_frames'),400,3000),'invalid requested frames')
 need(m.get('sparse_ranges')==RANGES and all(type(v)is int for r in m['sparse_ranges']for v in r),'changed sparse schema')
 need(set(m.get('sources',{}))=={'rom','mesen','capture','runner','isolation_helper'},'missing source attestation')
 private={'mesen':'portable-mesen/Mesen.exe','capture':'capture.lua','runner':'capture_human_switch.py','isolation_helper':'mesen_portable.py'}
 for key,name in private.items():need(Path(m['sources'][key]['path']).resolve()==capture/name,'nonprivate executed source')
 need(sha(rom)==ROM_SHA and m['sources']['rom']['sha256']==ROM_SHA and m['sources']['mesen']['sha256']==MESEN_SHA,'wrong original ROM/Mesen')
 need(m.get('arguments')==[str(capture/'portable-mesen/Mesen.exe'),'--testrunner','--timeout=240',
                         str(Path(m['sources']['rom']['path']).resolve()),str(capture/'capture.lua')],'changed executed arguments')
 need(m.get('environment')=={'NBA95_CAPTURE_DIR':capture.as_posix(),'NBA95_SWITCH_SELECTION':str(m['selection']),
                            'NBA95_SWITCH_FRAMES':str(m['requested_frames'])},'changed executed environment')
 for key,source in m['sources'].items():need(sha(source['path'])==source['sha256'],'changed source '+key)
 for key,digest in SOURCE_VERSION.items():need(m['sources'][key]['sha256']==digest,'unsupported source version '+key)
 required={'boundaries.jsonl','capture.lua','capture_complete.txt','capture_human_switch.py','mesen_portable.py',
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
 used_raw=set();fields={'index','tag','pc','frame','court','raw','cpu_d','actor','owner','live','offense','direction','candidate','score'}
 last_frame=last_court=-1
 for index,row in enumerate(rows,1):
  need(set(row)==fields,'invalid native event fields')
  need(type(row['tag'])is str and row['tag']in PCS,'unknown native event')
  for field in fields-{'tag','raw'}:need(integer(row[field],-1 if field=='court'else 0,0xffffff),'invalid event numeric type')
  need(row['index']==index and row['pc']==PCS[row['tag']]and row['cpu_d']==0,'invalid boundary/order/DP')
  need(row['frame']>=last_frame and row['court']>=last_court,'native time reversed');last_frame=row['frame'];last_court=row['court']
  if row['tag']in RAW_TAGS:
   name=f'raw_{index:05d}.bin';need(row['raw']==name and name in artifacts,'missing attested raw boundary');used_raw.add(name)
  else:need(row['raw']=='','unexpected raw boundary')
 need(set(artifacts)==required|used_raw,'unexpected or unbound artifacts')
 return m,rows,int(complete['calls'])

def verify(capture,probe,rom,diagnostics):
 m,rows,native_calls=attest(capture,rom)
 pending=None;special=collections.Counter();coverage=collections.Counter();routes=collections.Counter()
 inputs=[];expected=[];contexts=[];crossings=[];ties=[];pass_count=0
 for row in rows:
  tag=row['tag']
  if tag in('player.entry','court.entry'):
   need(pending is None,'initialization inside switch');special[tag]+=1;state=raw(capture,row)
   if tag=='player.entry':need(words(state,0x166d,5)==[2,1,1,1,1],'wrong fresh selection state')
   else:need(word(state,0x166d)==m['selection'],'wrong native court selection')
   continue
  if tag=='pass.observed':need(pending is None,'pass nested inside switch');pass_count+=1;continue
  if tag=='switch.entry':
   need(pending is None and special['court.entry']==1,'nested or precourt switch')
   pending=dict(entry=row,state=raw(capture,row),scan=None,transfer=False,restore=False,score=0x640,candidates=0)
   continue
  need(pending is not None,'switch event without entry')
  if tag.startswith('scan.'):
   need(pending['scan']is None and not pending['restore'],'repeated/misordered scan');pending['scan']=tag;continue
  if tag.startswith('candidate.'):
   need(pending['scan']=='scan.'+tag.split('.')[1]and not pending['restore']and not pending['transfer'],'misordered candidate')
   if row['score']==pending['score']:ties.append(dict(entry=pending['entry']['index'],event=row['index'],scan=pending['scan']))
   pending['score']=row['score'];pending['candidates']+=1;continue
  if tag=='switch.transfer':need(pending['scan']is not None and not pending['transfer']and not pending['restore'],'misordered transfer');pending['transfer']=True;continue
  if tag=='switch.restore':need(pending['scan']is not None and not pending['restore'],'misordered restore');pending['restore']=True;continue
  need(tag in('switch.full','switch.exit'),'unexpected exit')
  entry,state=pending['entry'],pending['state'];end=raw(capture,row)
  if tag=='switch.full':need(pending['scan']is None and not pending['restore']and not pending['transfer'],'invalid count-gate exit');route=0
  else:need(pending['scan']is not None and pending['restore'],'incomplete switch body');route=2 if pending['transfer']else 1
  actor=word(state,0x96);need(0x34eb<=actor<0x3eeb and(actor-0x34eb)%0x100==0,'invalid actor pointer')
  expected.append(dict(route=route,actor_words=words(end,0x34eb,11*128),controller_words=words(end,0x47eb,160),
                       context_words=words(end,0x46eb,128),preserved_words=[word(end,a)for a in GLOBALS]))
  inputs.append(str(capture/entry['raw']));contexts.append(dict(entry=entry['index'],exit=row['index'],court=entry['court']))
  if entry['court']!=row['court']:crossings.append(contexts[-1])
  phase='tip'if word(state,0x936)==0x81 else'inbound'if word(state,0x936)==0x82 else'live'
  side='offense'if word(state,actor+0x6e)==word(state,0x93a)else'defense'
  coverage[(pending['scan'],phase,side,entry['direction'])]+=1;routes[route]+=1;pending=None
 need(pending is None and len(inputs)==native_calls,'missing switch calls')
 need(special==collections.Counter({'player.entry':1,'court.entry':1}),'missing native initialization')
 need(all(any(k[0]==scan for k in coverage)for scan in('scan.neutral','scan.directional')),'missing neutral/directional coverage')
 run=subprocess.run([str(probe)],input='\n'.join(inputs)+'\n',text=True,capture_output=True,timeout=60)
 diagnostics.with_suffix('.probe-stdout.txt').write_text(run.stdout);diagnostics.with_suffix('.probe-stderr.txt').write_text(run.stderr)
 need(run.returncode==0,f'probe failed {run.returncode}: {run.stderr}')
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
 return dict(kind='full84E141 persistent effects, natural native prestate replay; not production human play',passed=not failures,
  manifest_sha256=sha(capture/'manifest.json'),probe_sha256=sha(probe),rom_sha256=sha(rom),verifier_sha256=sha(__file__),
  calls=len(inputs),compared_values=compared,routes=dict(routes),frame_crossings=crossings,native_equal_score_replacements=ties,
  pass_calls_observed_not_replayed=pass_count,
  coverage=[dict(scan=k[0],phase=k[1],side=k[2],direction=k[3],calls=v)for k,v in coverage.items()],failures=failures)

def main():
 p=argparse.ArgumentParser()
 for name in('capture','probe','rom','output'):p.add_argument('--'+name,type=Path,required=True)
 a=p.parse_args();need(not a.output.exists(),'preserve earlier report')
 need(all(not a.output.with_suffix(s).exists()for s in('.probe-stdout.txt','.probe-stderr.txt')),'preserve earlier probe output')
 try:report=verify(a.capture.resolve(),a.probe.resolve(),a.rom.resolve(),a.output)
 except Exception as error:a.output.write_text(json.dumps(dict(passed=False,error=str(error)),indent=2)+'\n');raise
 a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k not in('coverage','failures')}))
 if report['failures']:print(json.dumps(report['failures'][:10]));raise SystemExit(1)

if __name__=='__main__':main()
