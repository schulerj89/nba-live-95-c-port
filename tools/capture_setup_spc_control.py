"""Cold reset to two resident F1 control publications, observation only."""
import argparse, hashlib, json, os, shutil, subprocess
from pathlib import Path
ROM_SHA='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
MESEN_SHA='d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);p.add_argument('--rom',type=Path,required=True);p.add_argument('--mesen',type=Path,required=True);a=p.parse_args()
 assert sha(a.rom)==ROM_SHA and sha(a.mesen)==MESEN_SHA
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);home=out/'portable-mesen';home.mkdir();saves=out/'saves';saves.mkdir()
 exe=home/'Mesen.exe';shutil.copyfile(a.mesen,exe)
 settings={'Debug':{'ScriptWindow':{'AllowIoOsAccess':True,'ScriptTimeout':60}},'Preferences':{'SingleInstance':False,'PauseWhenInBackground':False,'AutoLoadPatches':False,'OverrideSaveDataFolder':True,'SaveDataFolder':str(saves)},'Snes':{'Port1':{'Type':'SnesController'},'Port2':{'Type':'None'},'EnableRandomPowerOnState':False,'RamPowerOnState':'AllZeros','DisableFrameSkipping':True}}
 initial=out/'initial-settings.json';initial.write_text(json.dumps(settings,indent=2)+'\n');shutil.copyfile(initial,home/'settings.json')
 script=out/'capture.lua';shutil.copyfile(Path(__file__).with_name('mesen_setup_spc_control.lua'),script)
 runner=out/'runner.py';shutil.copyfile(__file__,runner)
 manifest={'schema':1,'kind':'cold-reset SPC F1 control observation; isolated component witness only','state_injection':False,'rom_patch':False,'accepted':False,'settings':settings,'sources':{},'initial_saves':[]}
 for name,path in [('rom',a.rom.resolve()),('mesen',exe),('script',script),('runner',runner),('settings',initial)]:manifest['sources'][name]={'path':str(path),'sha256':sha(path)}
 args=[str(exe),'--testrunner','--timeout=60',str(a.rom.resolve()),str(script)];manifest['arguments']=args
 env={k:v for k,v in os.environ.items()if not k.startswith('NBA95_')};env['NBA95_SPC_CONTROL_DIR']=str(out)
 startup=subprocess.STARTUPINFO();startup.dwFlags|=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=0
 try:
  with (out/'mesen.log').open('w')as f:r=subprocess.run(args,env=env,stdout=f,stderr=subprocess.STDOUT,startupinfo=startup,timeout=80)
  manifest['exit_code']=r.returncode
  assert r.returncode==0 and (out/'spc_control_complete.txt').read_text()=='ok; publications=2\n'
  post=json.loads((home/'settings.json').read_text(encoding='utf-8-sig'))
  def check(g,w):
   for k,v in w.items():
    if isinstance(v,dict):check(g[k],v)
    else:assert type(g[k])is type(v)and g[k]==v,k
  check(post,settings);manifest['post_settings_sha256']=sha(home/'settings.json')
  manifest['artifacts']={f.name:{'bytes':f.stat().st_size,'sha256':sha(f)}for f in out.iterdir()if f.is_file()and f.name!='manifest.json'}
  manifest['accepted']=True
 finally:(out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
 print('PASS',out)
if __name__=='__main__':main()
