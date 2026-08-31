"""Compare independent native route PCs and raw exits; never fake action children."""
import argparse,collections,json,struct,subprocess
from pathlib import Path
import mesen_portable
from verify_controller_contract import read_json,sha,unique,ROM_SHA,MESEN_SHA

RANGES=[[0,0x100],[0x500,0x500],[0x1600,0x300],[0x3400,0x1600]]
PCS={'player.entry':0x81a489,'initialize.entry':0x86e208,'initialize.exit':0x86e24b,'court.entry':0x87a47a,
 'gate.entry':0x879138,'gate.publish':0x87915d,'gate.skip':0x87922e,
 'b.entry':0x84e2ac,'b.pass':0x84e2e4,'b.switch':0x84e2eb,'b.other':0x84e2f2,'b.return':0x84e3e6,
 'motion.entry':0x8791c3,'motion.accelerate':0x87922a,'motion.exit':0x87922e,'offense.inputs.begin':0}

def raw(capture,row,manifest):
 name=row['raw']
 if name!=f"raw_{row['index']:05d}.bin" or name not in manifest['artifacts']:raise ValueError('missing attested sparse raw boundary')
 data=(capture/name).read_bytes()
 if len(data)!=sum(n for _,n in RANGES):raise ValueError('incomplete sparse raw snapshot')
 memory={};offset=0
 for base,size in RANGES:
  memory.update((base+i,v)for i,v in enumerate(data[offset:offset+size]));offset+=size
 return memory

def word(m,a):return m[a]|(m[a+1]<<8)
def words(m,a,n):return [word(m,a+i*2)for i in range(n)]

def verify(capture,probe,rom,diagnostics):
 m=read_json(capture/'manifest.json')
 if type(m.get('schema'))is not int or m['schema']!=1 or m.get('kind')!='native human dispatch boundaries' or m.get('state_injection')is not False or m.get('rom_patch')is not False:raise ValueError('wrong capture kind')
 if type(m.get('exit_code'))is not int or m['exit_code']!=0:raise ValueError('failed native process')
 if m.get('sparse_ranges')!=RANGES:raise ValueError('raw memory schema changed')
 if set(m['sources'])!={'rom','mesen','capture','runner','isolation_helper'}:raise ValueError('missing or changed source attestation fields')
 required={'boundaries.jsonl','capture.lua','capture_complete.txt','capture_human_dispatch.py','mesen_portable.py',
           'observed-script-data-folder.txt','initial-mesen-settings.json','stdout.log','stderr.log'}
 if not required<=set(m['artifacts']):raise ValueError('missing required artifact attestation')
 for key,name in [('mesen','portable-mesen/Mesen.exe'),('capture','capture.lua'),('runner','capture_human_dispatch.py'),('isolation_helper','mesen_portable.py')]:
  if Path(m['sources'][key]['path']).resolve()!=(capture/name).resolve():raise ValueError('source points outside private executed copy')
 if sha(rom)!=ROM_SHA or m['sources']['rom']['sha256']!=ROM_SHA or m['sources']['mesen']['sha256']!=MESEN_SHA:raise ValueError('wrong original source')
 for name,source in m['sources'].items():
  if sha(source['path'])!=source['sha256']:raise ValueError('changed source '+name)
 for name,a in m['artifacts'].items():
  if Path(name).name!=name or type(a['bytes'])is not int or (capture/name).stat().st_size!=a['bytes'] or sha(capture/name)!=a['sha256']:raise ValueError('changed native artifact '+name)
 if m['isolation']['initial_saves']!=[] or m['isolation']['post_settings_verified']is not True:raise ValueError('missing process isolation')
 if sha(capture/'portable-mesen/settings.json')!=m['isolation']['post_settings_sha256']:raise ValueError('changed persisted settings')
 saves={v.name:sha(v)for v in(capture/'isolated-saves').iterdir()if v.is_file()}
 if saves!=m['isolation']['final_saves']:raise ValueError('changed private final saves')
 mesen_portable.verify(capture,m['isolation'])
 if(capture/'capture_complete.txt').read_text()!=m['completion']:raise ValueError('changed completion')
 rows=[json.loads(s,object_pairs_hook=unique)for s in(capture/'boundaries.jsonl').read_text().splitlines()]
 complete=unique([s.split('=',1)for s in m['completion'].splitlines()])
 if set(complete)!=set(('selection','frames','boundaries','offense_start'))or int(complete['selection'])!=m['selection']or int(complete['boundaries'])!=len(rows)or int(complete['frames'])<m['requested_frames']:raise ValueError('incomplete normal journey')
 pending={};inputs=[];expected=[];context=[];counts=collections.Counter();coverage=collections.Counter();crossings=[]
 special=collections.Counter()
 for index,row in enumerate(rows,1):
  if type(row['index'])is not int or row['index']!=index or row['pc']!=PCS[row['tag']]:raise ValueError('bad native event order/boundary')
  tag=row['tag'];mode=tag.split('.')[0]
  if tag in ('player.entry','initialize.entry','initialize.exit','court.entry'):
   state=raw(capture,row,m);special[tag]+=1
   if tag=='player.entry' and words(state,0x166d,5)!=[2,1,1,1,1]:raise ValueError('wrong fresh Player Setup state')
   if tag=='court.entry' and word(state,0x166d)!=m['selection']:raise ValueError('wrong court selection')
   continue
  if mode not in ('gate','b','motion'):continue
  if tag.endswith('.entry'):
   if mode in pending:raise ValueError('nested native stage')
   pending[mode]=(row,raw(capture,row,m),False)
   continue
  if tag=='motion.accelerate':
   entry,state,called=pending['motion']
   if called:raise ValueError('repeated accelerator call')
   pending['motion']=(entry,state,True);continue
  entry,state,called=pending.pop(mode)
  if entry['court']!=row['court']:crossings.append(dict(mode=mode,entry=entry['index'],exit=row['index']))
  actor=word(state,0x96)
  if actor<0x34eb or actor>=0x3eeb or (actor-0x34eb)%0x100:raise ValueError('bad native actor')
  if mode=='motion':
   end=raw(capture,row,m)
   want=dict(accelerator_call=0x85a82c if called else 0,actor_words=words(end,actor,128),
             controller_words=words(end,0x47eb,160),context_words=words(end,0x46eb,128))
  else:want=dict(next_pc=row['pc'])
  inputs.append(mode+' '+str(capture/entry['raw']))
  expected.append(want);context.append(dict(mode=mode,entry=entry['index'],exit=row['index'],court=entry['court']))
  counts[mode]+=1
  if mode=='b':
   phase='tip'if word(state,0x936)==0x81 else'inbound'if word(state,0x936)==0x82 else'live'
   side='offense'if word(state,actor+0x6e)==word(state,0x93a)else'defense'
   coverage[(tag,phase,side,word(state,0xae))]+=1
 if pending:raise ValueError('unfinished native stage')
 if special!=collections.Counter({'player.entry':1,'initialize.entry':1,'initialize.exit':1,'court.entry':1}):raise ValueError('missing native initialization')
 if any(not counts[k]for k in ('gate','b','motion')):raise ValueError('missing bounded stage')
 run=subprocess.run([str(probe),str(rom)],input='\n'.join(inputs)+'\n',text=True,capture_output=True,timeout=120)
 diagnostics.with_suffix('.probe-stdout.txt').write_text(run.stdout)
 diagnostics.with_suffix('.probe-stderr.txt').write_text(run.stderr)
 if run.returncode:raise ValueError(f'probe failed {run.returncode}: {run.stderr}')
 lines=run.stdout.splitlines()
 # The existing ROM loader prints one fixed, attested-ROM diagnostic. Do not
 # silently filter arbitrary non-JSON output or accept missing C records.
 if not lines or lines.pop(0)!='[ROM] Loaded successfully: "NBA Live \'95         " (Reset: 0x800D, Headered: No, Size: 1536 KiB)':raise ValueError('unexpected loader diagnostic')
 if len(lines)!=len(expected):raise ValueError(f'wrong C result count {len(lines)} vs {len(expected)}')
 failures=[];compared=0
 for i,(line,want)in enumerate(zip(lines,expected)):
  actual=json.loads(line,object_pairs_hook=unique)
  if actual.keys()!=want.keys():raise ValueError('wrong C field set')
  for key,value in want.items():
   if isinstance(value,list):
    if not isinstance(actual[key],list)or len(value)!=len(actual[key]):raise ValueError('wrong C vector shape')
    pairs=enumerate(zip(actual[key],value))
   else:pairs=[(None,(actual[key],value))]
   for slot,(got,v)in pairs:
    compared+=1
    if type(got)is not int or got!=v:failures.append(dict(**context[i],field=key,slot=slot,expected=v,actual=got))
 return dict(kind='bounded human caller stages, native prestate replay; not production human play',
  manifest_sha256=sha(capture/'manifest.json'),probe_sha256=sha(probe),rom_sha256=sha(rom),
  calls=dict(counts),compared_values=compared,frame_crossings=crossings,
  action_coverage=[dict(tag=k[0],phase=k[1],side=k[2],pressed=k[3],calls=v)for k,v in coverage.items()],
  failures=failures,passed=not failures)

def main():
 p=argparse.ArgumentParser();p.add_argument('--capture',type=Path,required=True);p.add_argument('--probe',type=Path,required=True)
 p.add_argument('--rom',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 if a.output.exists():raise ValueError('keep previous verification immutable')
 if any(a.output.with_suffix(s).exists()for s in('.probe-stdout.txt','.probe-stderr.txt')):raise ValueError('keep previous probe diagnostics immutable')
 try:report=verify(a.capture.resolve(),a.probe.resolve(),a.rom.resolve(),a.output)
 except Exception as error:
  a.output.write_text(json.dumps(dict(passed=False,error=str(error)),indent=2)+'\n');raise
 a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k not in('action_coverage','failures')}))
 if report['failures']:print(json.dumps(report['failures'][:10]));raise SystemExit(1)

if __name__=='__main__':main()
