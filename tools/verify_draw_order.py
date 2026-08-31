"""Strict typed component comparisons; no complete OAM/CPU/timing claim."""
import argparse,hashlib,json,struct,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
ROM_SHA='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
MESEN_SHA='d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b'
BUILD={'include/nba_draw_order.h','src/nba_draw_order.c','tools/draw_order_probe.c','tools/build_draw_order_probe.ps1'}
IDENTITY=[0x34eb+256*i for i in range(12)]
TAGS=[('init.caller',0x86da89),('init.entry',0x80fbe9),('init.terminal',0x80fbfe),('init.return',0x86da8d),('basket.before',0x86dbc2),('basket.after',0x86dbc5)]+[('depth.before',0x87a3b1),('pass.caller',0x87a43e),('pass.entry',0x80fc80),('pass.terminal',0x80fca1),('pass.return',0x87a442)]*12
def check(ok,message):
 if not ok:raise ValueError(message)
def integer(v,lo,hi):return type(v)is int and lo<=v<=hi
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def pairs(items):
 result={}
 for k,v in items:check(k not in result,'duplicate JSON key');result[k]=v
 return result
def loads(s):return json.loads(s,object_pairs_hook=pairs)
def exact(actual,expected):check(type(actual)is type(expected)and actual==expected,'exact metadata value/type')
def word(m,p):return struct.unpack_from('<H',m,p)[0]
def projection(m):return dict(order=[word(m,0x7e44+i*2)for i in range(12)],depth=[word(m,p+0x68)for p in IDENTITY])
def inputs(m):return dict(xs=[word(m,p+4)for p in IDENTITY],ys=[word(m,p+8)for p in IDENTITY],camera=word(m,0x860))
def binary(op,state,input):return b'DOR1'+struct.pack('<50H',op,*state['order'],*state['depth'],*input['xs'],*input['ys'],input['camera'])
def source(rom):
 raw=rom.read_bytes();check(sha(rom)==ROM_SHA,'original ROM identity')
 ranges={}
 for pc,data in [(0x80fbe9,'a20c00a9eb34a0000099447ec8c818690001cad0f46b'),(0x80fc80,'a21600caca301abc467eb96800bc447ed9680010eebd467e9d447e989d467e80e26b'),(0x86da89,'22e9fb80'),(0x86dbc2,'9cf33f'),(0x87a3b1,'a916008596'),(0x87a3b6,'a496be447ebd080038fd0400c900806ac900806a38ed60089d6800'),(0x87a43e,'2280fc80')]:
  offset=((pc>>16)&127)*32768+(pc&32767);expected=bytes.fromhex(data)
  check(raw[offset:offset+len(expected)]==expected,f'original source bytes {pc:06x}')
  ranges[f'{pc:06x}']=hashlib.sha256(expected).hexdigest()
 return ranges
def check_build(exe):
 m=loads((exe.parent/'build-manifest.json').read_text(encoding='utf-8-sig'))
 check(set(m)=={'schema','compiler_exit','sources','executable'},'build keys')
 exact(m['schema'],1);exact(m['compiler_exit'],0)
 check(type(m['sources'])is dict and set(m['sources'])==BUILD,'build required source closure')
 for name,item in m['sources'].items():
  check(set(item)=={'path','sha256'}and sha(item['path'])==item['sha256']==sha(ROOT/name),'build source identity')
 e=m['executable'];check(set(e)=={'path','sha256'}and Path(e['path']).resolve()==exe.resolve()and sha(exe)==e['sha256'],'build executable identity')
 return m
def run_probe(exe,out,name,records):
 data=b''.join(records);check(data and len(data)%104==0,'diagnostic records')
 inp=out/(name+'.input');inp.write_bytes(data)
 r=subprocess.run([str(exe.resolve()),str(inp.resolve())],capture_output=True,text=True)
 check(type(r.returncode)is int and r.returncode==0 and type(r.stdout)is str and type(r.stderr)is str and r.stderr=='','C process status/stderr')
 (out/(name+'.jsonl')).write_text(r.stdout);(out/(name+'.stderr')).write_text(r.stderr)
 rows=[loads(s)for s in r.stdout.splitlines()]
 check(len(rows)==len(records),'C row count')
 for i,(row,record)in enumerate(zip(rows,records)):
  check(type(row)is dict and set(row)=={'index','operation','ok','order','depth'},'C row schema')
  exact(row['index'],i+1);exact(row['operation'],word(record,4));check(type(row['ok'])is bool,'C ok type')
  for key in('order','depth'):check(type(row[key])is list and len(row[key])==12 and all(integer(v,0,65535)for v in row[key]),'C word domain')
 return rows
def settings(out):
 return {'Debug':{'ScriptWindow':{'AllowIoOsAccess':True,'ScriptTimeout':60,'SaveScriptBeforeRun':False}},'Preferences':{'SingleInstance':False,'PauseWhenInBackground':False,'AutoLoadPatches':False,'OverrideSaveDataFolder':True,'SaveDataFolder':str(out/'isolated-saves')},'Snes':{'Port1':{'Type':'SnesController'},'Port2':{'Type':'None'},'DisableFrameSkipping':True,'EnableRandomPowerOnState':False,'RamPowerOnState':'AllZeros','ForceFixedResolution':False,'Overscan':{'Top':7,'Bottom':8,'Left':0,'Right':0}},'Video':{'VideoFilter':'None','AspectRatio':'NoStretching','Brightness':0,'Contrast':0,'Hue':0,'Saturation':0,'ScanlineIntensity':0,'UseBilinearInterpolation':False,'ScreenRotation':'None'}}
def typed_tree(a,b):
 check(type(a)is type(b),'settings scalar type')
 if type(b)is dict:
  check(set(a)==set(b),'settings keys')
  for k in b:typed_tree(a[k],b[k])
 else:check(a==b,'settings value')
def native(out,rom):
 out=out.resolve();m=loads((out/'manifest.json').read_text())
 check(set(m)=={'schema','kind','state_injection','rom_patch','arguments','environment','isolation','sources','exit_code','artifacts'},'capture manifest keys')
 for k,v in dict(schema=1,kind='normal coldboot CPU menu draw order',state_injection=False,rom_patch=False,exit_code=0).items():exact(m[k],v)
 exe=out/'portable-mesen/Mesen.exe'
 exact(m['arguments'],[str(exe),'--testrunner','--timeout=180',str(rom),str(out/'capture_draw_order.lua')])
 exact(m['environment'],{'NBA95_CAPTURE_DIR':out.as_posix()})
 expected={str(rom):ROM_SHA,str(exe):MESEN_SHA,**{str(out/name):sha(ROOT/'tools'/name)for name in('capture_draw_order.lua','capture_draw_order.py','mesen_portable.py')}}
 exact(m['sources'],expected)
 for p,h in expected.items():check(sha(p)==h,'capture source rehash')
 artifacts={f'raw_{i:04d}.bin'for i in range(1,67)}|{'boundaries.jsonl','capture_complete.txt','initial-mesen-settings.json','observed-script-data-folder.txt','capture_draw_order.lua','capture_draw_order.py','mesen_portable.py','stdout.log','stderr.log'}
 check(type(m['artifacts'])is dict and set(m['artifacts'])==artifacts,'capture artifact closure')
 check({p.name for p in out.iterdir()if p.is_file()}==artifacts|{'manifest.json'},'capture undeclared files')
 for name,item in m['artifacts'].items():
  check(set(item)=={'bytes','sha256'}and integer(item['bytes'],0,10000000)and(out/name).stat().st_size==item['bytes']and sha(out/name)==item['sha256'],'capture artifact identity')
 check((out/'stderr.log').read_bytes()==b'','native stderr')
 iso=m['isolation'];check(set(iso)=={'method','home','save_folder','initial_saves','settings','initial_settings_sha256','post_settings_verified','observed_script_data_folder','post_settings_sha256','final_saves'},'isolation keys')
 exact(iso['method'],'private portable executable/settings');exact(iso['home'],str(exe.parent));exact(iso['save_folder'],str(out/'isolated-saves'));exact(iso['initial_saves'],[]);exact(iso['post_settings_verified'],True)
 expected_settings=settings(out);typed_tree(iso['settings'],expected_settings)
 for p,k in [(out/'initial-mesen-settings.json','initial_settings_sha256'),(exe.parent/'settings.json','post_settings_sha256')]:
  check(sha(p)==iso[k],'settings hash');typed_tree(loads(p.read_text(encoding='utf-8-sig')),expected_settings)
 observed=(out/'observed-script-data-folder.txt').read_text().strip();exact(iso['observed_script_data_folder'],observed)
 check(Path(observed).resolve()==(exe.parent/'LuaScriptData/capture_draw_order').resolve(),'observed private script home')
 exact(iso['final_saves'],{p.name:sha(p)for p in(out/'isolated-saves').iterdir()if p.is_file()})
 rows=[loads(s)for s in(out/'boundaries.jsonl').read_text().splitlines()];check(len(rows)==66,'native row count')
 keys={'index','frame','court','tag','pc','raw','a','x','y','sp','d','dbr','ps','cycle'}
 for i,(row,(tag,pc))in enumerate(zip(rows,TAGS)):
  check(type(row)is dict and set(row)==keys,'native row keys')
  exact(row['index'],i+1);exact(row['tag'],tag);exact(row['pc'],pc);exact(row['raw'],f'raw_{i+1:04d}.bin')
  check(all(integer(row[k],0,65535)for k in('a','x','y','sp','d'))and integer(row['dbr'],0,255)and integer(row['ps'],0,255),'native registers')
  check(integer(row['frame'],0,11999)and integer(row['court'],-1,11999)and integer(row['cycle'],1,2**53-1),'native clocks')
  check(row['d']==0 and row['dbr']==0x7e and not(row['ps']&0x38),'native source binary16 DP/DBR domain')
  if i:check(row['cycle']>rows[i-1]['cycle']and row['frame']>=rows[i-1]['frame']and row['court']>=rows[i-1]['court'],'native chronological rows')
  row['memory']=(out/row['raw']).read_bytes();check(len(row['memory'])==131072,'native full WRAM')
 for indices in [(0,1,2,3)]+[(i+1,i+2,i+3,i+4)for i in range(6,66,5)]:
  caller,entry,terminal,ret=[rows[i]for i in indices]
  check(all(r['frame']==caller['frame']and r['court']==caller['court']for r in(entry,terminal,ret)),'native bounded call frame')
  check(entry['sp']==(caller['sp']-3)&65535 and terminal['sp']==entry['sp']and ret['sp']==caller['sp'],'native JSL stack relation')
 for i in range(6,66,5):check(rows[i]['court']>=240 and rows[i]['frame']==rows[i+4]['frame'],'scheduled projection frame')
 exact((out/'capture_complete.txt').read_text(),f"frames={rows[-1]['frame']}\nboundaries=66\npasses=12\n")
 return m,rows
def verify_case(out,rom,exe,dest):
 m,rows=native(out,rom);records=[];wanted=[]
 def add(op,before,after):
  records.append(binary(op,projection(before['memory']),inputs(before['memory'])))
  wanted.append(projection(after['memory']))
 add(0,rows[1],rows[2]);check(projection(rows[2]['memory'])['order']==IDENTITY,'native initializer identity')
 for a,b in[(0,1),(2,3)]:exact(projection(rows[a]['memory']),projection(rows[b]['memory']))
 check(word(rows[5]['memory'],0x3ff3)==0,'native basket zero write')
 check(rows[4]['memory'][:0x3ff3]+rows[4]['memory'][0x3ff5:]==rows[5]['memory'][:0x3ff3]+rows[5]['memory'][0x3ff5:],'basket isolated source store')
 for i in range(6,66,5):
  before,caller,entry,terminal,ret=rows[i:i+5]
  check(sorted(projection(before['memory'])['order'])==IDENTITY,'native carried permutation')
  for r in(caller,entry,terminal,ret):exact(inputs(r['memory']),inputs(before['memory']))
  exact(projection(caller['memory']),projection(entry['memory']));exact(projection(terminal['memory']),projection(ret['memory']))
  add(1,before,caller);add(2,entry,terminal);add(3,before,ret)
 got=run_probe(exe,dest,'native',records)
 for row,expected in zip(got,wanted):check(row['ok'],'native C refusal');exact({k:row[k]for k in('order','depth')},expected)
 # A second proof carries C's own order/depth across all12 scheduled passes.
 # Only first before-state initializes this isolated sequence. Subsequent
 # records supply actual changing XY/camera, never native order/depth outputs.
 carried=projection(rows[6]['memory'])
 for step,i in enumerate(range(6,66,5)):
  row=run_probe(exe,dest,f'chain-{step:02}',[binary(3,carried,inputs(rows[i]['memory']))])[0]
  check(row['ok'],'persistent C chain refusal');carried={k:row[k]for k in('order','depth')}
  exact(carried,projection(rows[i+4]['memory']))
 return dict(passed=True,rows=len(rows),component_cases=len(records),words_compared=24*len(records),persistent_steps=12,persistent_words_compared=288,native_manifest_sha256=sha(out/'manifest.json'),basket_y_before=word(rows[4]['memory'],0x3ff3),basket_y_after=0,scope='normal input12 scheduled passes; isolated first-prestate chain, typed data only, no elapsed/CPU/OAM parity')
def main(a):
 a.rom=a.rom.resolve();a.exe=a.exe.resolve();a.native=a.native.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 check_build(a.exe);ranges=source(a.rom);report=verify_case(a.native,a.rom,a.exe,out)
 report.update(source_intervals=ranges,verifier_sha256=sha(__file__),build_manifest_sha256=sha(a.exe.parent/'build-manifest.json'))
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2));return report
if __name__=='__main__':
 p=argparse.ArgumentParser()
 for key in('rom','exe','native','output'):p.add_argument('--'+key,type=Path,required=True)
 main(p.parse_args())
