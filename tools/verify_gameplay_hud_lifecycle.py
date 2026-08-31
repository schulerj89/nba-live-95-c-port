"""Strict frozen native HUD data projections, not CPU/NMI scanout timing parity."""
import argparse,hashlib,json,os,re,struct,subprocess
from pathlib import Path
BASE=Path(__file__).resolve().parent
PINS={'ac0da740300c8afc39d76e7620319cbb34b79420fae23927826b30dc29c0aa74'}
ROM='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
MESEN='d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b'
PACK='f564c29612928984002ed3f0389d317de639fff122baf61a7bc9ecaef2a6be09'
SIZE=0x30200
LEAVES={0x83d0ad:0x83d156,0x83d157:0x83d1b0,0x83d1b1:0x83d1fc,0x83d1fd:0x83d24f,0x83d2e0:0x83d332,0x87bbe9:0x87bd2e,0x87ba5e:0x87ba9e,0x87bacb:0x87baf4,0x83ebdb:0x83ed46}
META={'clock_snapshot':0x92a,'shotclock':0x92c,'gate':0x492b,'mode':0x960,'clear':0x8ee,'clock_mirror':0x8f6,'shot_frame':0x8f4,'period':0x926,'clock':0x928,'requester':0x95e,'timer':0x8de,'sequence':0x8e6,'kind':0x8e8,'phase':0x8e4,'counter':0x4941,'canvas_x':0x18c6,'canvas_y':0x18c8}
FIELDS={'tag','pc','frame','court','cpu_a','cpu_x','cpu_y','cpu_d','cpu_ps','cpu_sp','cpu_dbr','cpu_k','cpu_pc','sample','teams','scores',*META}
def sha(x):return hashlib.sha256(x).hexdigest()
def check(x,m):
 if not x:raise ValueError(m)
def js(raw):
 def pairs(xs):
  d={}
  for k,v in xs:check(k not in d,'duplicateJSONkey');d[k]=v
  return d
 return json.loads(raw,object_pairs_hook=pairs)
def w(b,a):return struct.unpack_from('<H',b,a)[0]
def native(path):
 data=(path/'manifest.json').read_bytes();check(sha(data)in PINS,'unreviewed native manifest identity');m=js(data)
 check(type(m['schema'])is int and m['schema']==2 and m['accepted_capture']is True and m['state_injection']is False and m['rom_patch']is False,'native schema/acceptance')
 check(type(m['selection'])is int and m['selection']==1 and m['alternate_teams']is False and type(m['exit_code'])is int and m['exit_code']==0,'native route')
 check(m['rom_sha256']==ROM and m['mesen_sha256']==MESEN,'originalversions')
 check(set(m)=={'schema','kind','state_injection','rom_patch','selection','alternate_teams','rom_sha256','mesen_sha256','script_sha256','isolation','input_schedule','accepted_capture','command','NBA95_environment','exit_code','summary','artifacts'},'manifest field closure')
 check(m['kind']=='natural neutral-controller CPU match three HUD requests','capture kind')
 check(m['input_schedule']=='Title/setup/team Start; PlayerSetup Left400 then Start700. All gameplay controllers neutral. AlternateTeams uses Setup650Right,700L,750Right,850Start.','input route')
 iso=m['isolation'];home=Path(iso['home']);check(sha((home/'Mesen.exe').read_bytes())==MESEN,'Mesen executable')
 check(m['command']==[str(home/'Mesen.exe'),'--testrunner','--timeout=600',str(Path(r'F:\Games\SNES\NBA Live 95 (USA).sfc')),str(path/'capture.lua')],'executed command')
 check(m['NBA95_environment']=={'NBA95_CAPTURE_DIR':str(path),'NBA95_CONTROL_TEAM_VARIANT':'0'},'isolated environment')
 check(set(iso)=={'method','home','save_folder','initial_saves','settings','initial_settings_sha256','post_settings_verified','observed_script_data_folder','post_settings_sha256','final_saves'},'isolation schema')
 check(iso['method']=='private portable executable/settings' and home.resolve()==path/'portable-mesen' and Path(iso['save_folder']).resolve()==path/'isolated-saves','private paths')
 final={p.name:sha(p.read_bytes())for p in(path/'isolated-saves').glob('*')if p.is_file()}
 check(iso['final_saves']==final,'actual private saves')
 check(iso['initial_saves']==[] and iso['post_settings_verified']is True,'isolation')
 settings=home/'settings.json'
 if not settings.exists():settings=home/'Settings.json'
 check(sha(settings.read_bytes())==iso['post_settings_sha256'],'private postsettings changed')
 check(json.dumps(js((path/'initial-mesen-settings.json').read_bytes()),sort_keys=True,allow_nan=False)==json.dumps(iso['settings'],sort_keys=True,allow_nan=False),'initialsettings exact types')
 check(sha((path/'initial-mesen-settings.json').read_bytes())==iso['initial_settings_sha256'],'initialsettingssha')
 check((path/'observed-script-data-folder.txt').read_text().strip()==iso['observed_script_data_folder'],'privatehome attestation')
 blobs={}
 check(set(m['artifacts'])=={p.name for p in path.iterdir()if p.is_file()and p.name!='manifest.json'},'artifactclosure')
 for name,rec in m['artifacts'].items():
  check(set(rec)=={'size','sha256'}and type(rec['size'])is int and type(rec['sha256'])is str,'artifact schema')
  b=(path/name).read_bytes();check(len(b)==rec['size']and sha(b)==rec['sha256'],'artifact '+name);blobs[name]=b
 check(m['script_sha256']==sha(blobs['capture.lua']) and m['summary']==blobs['capture_complete.txt'].decode(),'capture script/completion')
 rows=[js(line)for line in blobs['hud.jsonl'].splitlines()];first=rows[0];check(first['tag']=='first_court'and first['frame']==4390 and first['court']==0,'absolute initialclock')
 previous=-1
 for row in rows:
  check(set(row)==FIELDS,'row schema')
  check(type(row['tag'])is str and row['tag']in['first_court','end_frame','publisher','timer_before','timer_after'],'rowtag')
  for k,v in row.items():
   if k in ['tag','teams','scores']:continue
   check(type(v)is int and v>=0,'row integer '+k)
   limit=0xffffff if k=='pc'else 0xff if k in ['cpu_ps','cpu_dbr','cpu_k']else 1000000 if k in ['frame','court','sample']else 65535
   check(v<=limit,'row range '+k)
  for k in ['teams','scores']:check(type(row[k])is list and len(row[k])==2 and all(type(v)is int and 0<=v<=65535 for v in row[k]),'wordlist')
  check(row['frame']>=previous,'clockorder');previous=row['frame']
  check(row['frame']-row['court']==4390+(row['tag']=='end_frame'),'absolute clock/court mapping')
  if row['pc']:check(row['cpu_k']==row['pc']>>16 and row['cpu_pc']==row['pc']&65535,'CPU/sourcePC')
  name=f"publisher_{row['sample']:05d}_{row['pc']:06x}.wram"
  if row['tag']=='publisher'and name in blobs:
   raw=blobs[name];check(len(raw)==131072,'rawsize')
   for k,a in META.items():check(row[k]==w(raw,a),'row/raw '+k)
   check(row['teams']==[w(raw,0x46eb),w(raw,0x476b)]and row['scores']==[w(raw,0x4711),w(raw,0x4791)],'row/raw teams/scores')
 return m,rows,blobs

def main():
 p=argparse.ArgumentParser();p.add_argument('--bounded-shared-clock-read',action='store_true')
 for k in ['native','probe','pack','build','source','output']:p.add_argument('--'+k,required=True,type=Path)
 a=p.parse_args();a.native=a.native.resolve();out=a.output.resolve();out.mkdir(exist_ok=False)
 m,rows,blobs=native(a.native);check(sha(a.pack.read_bytes())==PACK,'pack identity')
 build=js(a.build.read_bytes());check(set(build)=={'compiled_sources','headers','exe_sha256'},'buildschema')
 names={'tools/hud_native_lifecycle_probe.c','src/nba_gameplay_hud.c','src/nba_rom_font.c','src/nba_assets.c','src/nba_gameplay_ai.c','src/nba_ea_intro.c','src/nba_intro_text.c','src/nba_renderer.c','src/nba_snes_ppu.c'}
 check(set(build['compiled_sources'])==names and set(build['headers'])=={p.name for p in(a.source/'include').glob('*.h')},'sourceclosure')
 for n,d in build['compiled_sources'].items():check(sha((a.source/n).read_bytes())==d,'compiledsource '+n)
 for n,d in build['headers'].items():check(sha((a.source/'include'/n).read_bytes())==d,'compiledheader '+n)
 check(sha(a.probe.read_bytes())==build['exe_sha256'],'executableidentity')
 pairs=[]
 for i,row in enumerate(rows):
  pc=row['pc'];name=f"publisher_{row['sample']:05d}_{pc:06x}"
  if row['tag']!='publisher'or name+'.wram'not in blobs or pc not in {*LEAVES,0x83cc10,0x83ce36}:continue
  pre=blobs[name+'.wram'];ret={LEAVES[pc]}if pc in LEAVES else {0x83cc53,0x83cc7a}if pc==0x83cc10 else {0x83cfe7,0x83cf79,0x83cebf}
  end=next((q for q in rows[i+1:]if q['tag']=='publisher'and q['pc']in ret and q['cpu_sp']==row['cpu_sp']),None)
  check(end is not None,'return absent')
  postname=f"publisher_{end['sample']:05d}_{end['pc']:06x}"
  if postname+'.wram'not in blobs:continue
  # Only fully translated dispatcher children; unsupported overlays are reported separately.
  if pc==0x83cc10 and w(pre,0x8de)>0 and w(pre,0x8de)<0x8000 and 6<=w(pre,0x8e6)<0x8000:continue
  if pc==0x83ebdb and w(pre,0x8e8)in[17,22]:continue
  check(row['cpu_ps']&0x38==0 and row['cpu_d']==0,'D0/M16/X16 source domain')
  pairs.append((row,end,name,postname))
 check(len(pairs)==746 and sum(r['pc']==0x83ce36 for r,_,_,_ in pairs)==3,'complete frozen route coverage')
 inp=bytearray(struct.pack('<I',len(pairs)))
 for r,e,n,pn in pairs:inp+=struct.pack('<I',r['pc'])+blobs[n+'.wram']+blobs[n+'.vram']+blobs[n+'.cgram']
 (out/'before.bin').write_bytes(inp);cmd=[str(a.probe.resolve()),str(a.pack.resolve()),str(out/'before.bin'),str(out/'actual.bin')]
 env={k:v for k,v in os.environ.items()if not k.startswith('NBA95')};run=subprocess.run(cmd,capture_output=True,env=env,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
 (out/'stdout.txt').write_bytes(run.stdout);(out/'stderr.txt').write_bytes(run.stderr)
 expected=f"[ASSETS] Loaded asset pack: '{a.pack.resolve()}' (89442736 bytes, 264 assets)\nHUD_NATIVE {len(pairs)}\n"
 check(run.returncode==0 and run.stdout.decode().replace('\r\n','\n')==expected and run.stderr==b'','complete C response protocol')
 result=(out/'actual.bin').read_bytes();check(len(result)==4+len(pairs)*(SIZE+8) and struct.unpack_from('<I',result)[0]==len(pairs),'C outputclosure')
 failures=[];counts={};nbytes=0;crossings=[];shared=[];visible_differences=[]
 common=[0x8f6,0x8ee,0x8f4,0x8e6,0x9b4]
 for i,(r,e,n,pn)in enumerate(pairs):
  off=4+i*(SIZE+8);ok,pending=struct.unpack_from('<II',result,off);actual=result[off+8:off+8+SIZE];post=blobs[pn+'.wram'];pre=blobs[n+'.wram'];pc=r['pc'];check(ok==1 and pending==0,'C incomplete boundedcase')
  counts[hex(pc)]=counts.get(hex(pc),0)+1
  expectedret=LEAVES.get(pc)
  if pc==0x83ce36:expectedret=0x83cebf if w(pre,0x8de)<0x8000 else 0x83cfe7 if w(actual,0x8e8)==1 else 0x83cf79
  if pc==0x83cc10:expectedret=0x83cc53 if w(pre,0x960)<0x8000 or (0<w(pre,0x8de)<0x8000 and (w(pre,0x8e6)<0x8000 or w(pre,0x8e8)!=1))else 0x83cc7a
  check(e['pc']==expectedret,'return tag/source route')
  addresses=common.copy()
  if pc==0x83ce36:addresses += [0x8de,0x8e2,0x8e4,0x8e8,0x8ea,0x8ec,0x4931,0x493d,0x4941,0x7f6,0xaa]
  # NMI may tick08DE/0928/RNG during long renderer children; do not rewrite
  # C after-state to hide those independent shared-owner differences.
  if pc==0x83cc10 and r['frame']==e['frame']:addresses += [0x8de]
  if r['frame']!=e['frame']:crossings.append([i,r['frame'],e['frame'],hex(pc)])
  for address in addresses:
   nbytes+=2
   if w(actual,address)!=w(post,address):
    mismatch=dict(index=i,pc=hex(pc),field=hex(address),actual=w(actual,address),expected=w(post,address))
    inner=next((q for q in rows if r['sample']<q['sample']<e['sample'] and q['tag']=='publisher'and q['pc']==0x87bbe9),None)
    dependency=False
    if address==0x8f6 and pc in [0x83cc10,0x83d1fd]and e['frame']>r['frame'] and inner:
     child=blobs[f"publisher_{inner['sample']:05d}_{inner['pc']:06x}.wram"]
     dependency=w(child,0x8f6)==65535 and w(actual,address)==w(pre,0x928) and w(post,address)==w(child,0x928) and w(child,0x928)!=w(pre,0x928)
     if dependency:mismatch.update(before_parent_clock=w(pre,0x928),before_inner_BBE9_clock=w(child,0x928),inner_sample=inner['sample'],scope='unowned NMI changes a later shared clock read; no C afterstate input')
    if dependency and a.bounded_shared_clock_read:shared.append(mismatch)
    else:failures.append(mismatch)
  native_vram=blobs[pn+'.vram'];actual_vram=actual[0x20000:0x30000]
  visible_bad=sum(x!=y for x,y in zip(actual_vram[0x800:0xf00],native_vram[0x800:0xf00]))
  if visible_bad:visible_differences.append(dict(index=i,pc=hex(pc),different_map_bytes=visible_bad))
  for start,end in [(0x4a60,0x4a68),(0x4a70,0x58c0)]:
   nbytes+=end-start
   bad=[j for j in range(start,end)if actual[j]!=post[j]]
   if bad:failures.append(dict(index=i,pc=hex(pc),buffer=hex(start),different=len(bad),first=hex(bad[0])))
 check(counts=={'0x83ce36':3,'0x87bacb':6,'0x83cc10':552,'0x83d0ad':2,'0x83d157':2,'0x83d1b1':2,'0x83d1fd':2,'0x87bbe9':172,'0x83d2e0':2,'0x83ebdb':3},'complete routine coverage')
 (out/'report.json').write_text(json.dumps(dict(command=cmd,compared_bytes=nbytes,pairs=len(pairs),counts=counts,failures=failures,shared_clock_dependency_differences=shared,bounded_shared_clock_read_requested=a.bounded_shared_clock_read,full_atomic_parent_pass=not failures and not shared,visible_map_scanout_differences=visible_differences,crossing_calls=crossings,scope='before-only owned data; native CPU/DP/VRAM scanout timing and unsupportedstatisticschildren excluded',manifest_sha256=sha((a.native/'manifest.json').read_bytes()),exe_sha256=sha(a.probe.read_bytes()),pack_sha256=PACK),indent=2)+'\n')
 check(not failures,'native differences: '+str(failures[:6]));print('PASS',len(pairs),nbytes,counts)
if __name__=='__main__':main()

