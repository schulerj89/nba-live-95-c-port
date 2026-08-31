"""Natural header pre-wait source-work capture; read-only instructions, writes and call boundaries."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

ROM_SHA='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
MESEN_SHA='d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b'
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--output',required=True,type=Path)
 p.add_argument('--rom',required=True,type=Path)
 p.add_argument('--mesen',type=Path)
 a=p.parse_args();out=a.output.resolve();rom=a.rom.resolve()
 if sha(rom)!=ROM_SHA:raise ValueError('canonical original ROM required')
 installed=a.mesen or Path(shutil.which('Mesen.exe'))
 if sha(installed)!=MESEN_SHA:raise ValueError('verified Mesen binary required')
 out.mkdir(parents=True,exist_ok=False)
 home=out/'portable-mesen';home.mkdir();saves=out/'isolated-saves';saves.mkdir()
 exe=home/'Mesen.exe';shutil.copyfile(installed,exe)
 settings={
  'Debug':{'ScriptWindow':{'AllowIoOsAccess':True,'ScriptTimeout':60,'SaveScriptBeforeRun':False}},
  'Preferences':{'SingleInstance':False,'PauseWhenInBackground':False,'AutoLoadPatches':False,
                 'OverrideSaveDataFolder':True,'SaveDataFolder':str(saves)},
  'Snes':{'Port1':{'Type':'SnesController'},'Port2':{'Type':'None'},'DisableFrameSkipping':True,
          'EnableRandomPowerOnState':False,'RamPowerOnState':'AllZeros','ForceFixedResolution':False,
          'Overscan':{'Top':7,'Bottom':8,'Left':0,'Right':0}},
  'Video':{'VideoFilter':'None','AspectRatio':'NoStretching','Brightness':0,'Contrast':0,'Hue':0,'Saturation':0}}
 initial=out/'initial-settings.json';initial.write_text(json.dumps(settings,indent=2)+'\n')
 shutil.copyfile(initial,home/'settings.json')
 script=out/'capture.lua';shutil.copyfile(Path(__file__).with_name('mesen_setup_header_work.lua'),script)
 base=out/'scheduler_base.lua';shutil.copyfile(Path(__file__).with_name('mesen_setup_scheduler.lua'),base)
 codec=out/'codec_base.lua';shutil.copyfile(Path(__file__).with_name('mesen_setup_codec_work.lua'),codec)
 producer=out/'producer_base.lua';shutil.copyfile(Path(__file__).with_name('mesen_setup_producer_work.lua'),producer)
 runner=out/'capture_runner.py';shutil.copyfile(__file__,runner)
 args=[str(exe),'--testrunner','--timeout=300',str(rom),str(script)]
 manifest=dict(schema=1,kind='natural controller-only Setup resource scheduler observation',
  state_injection=False,rom_patch=False,accepted=False,sources={},arguments=args,
  isolation=dict(home=str(home),save_folder=str(saves),initial_saves=[],settings=settings),
  schedule='Canonical Simulation/3min normalization; Rules A470, row2 Right640, Start830; reentry A1100, row2 Right1270, Start1460. Evidence labels rebase after normalization; emulation uninterrupted.')
 for name,path in [('rom',rom),('mesen',exe),('script',script),('runner',runner),('settings',initial),('base_script',base),('codec_script',codec),('producer_script',producer)]:
  manifest['sources'][name]=dict(path=str(path),sha256=sha(path))
 env={k:v for k,v in os.environ.items()if not k.startswith('NBA95_')}
 env['NBA95_SCHEDULER_DIR']=str(out)
 startup=subprocess.STARTUPINFO();startup.dwFlags|=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=0
 try:
  with (out/'mesen.log').open('w')as log:
   run=subprocess.run(args,env=env,stdout=log,stderr=subprocess.STDOUT,startupinfo=startup,timeout=320)
  manifest['exit_code']=run.returncode
  if run.returncode:raise RuntimeError('Mesen capture did not complete')
  observed=dict(line.split('=',1)for line in (out/'observed_environment.txt').read_text().splitlines())
  if Path(observed['output']).resolve()!=out or not Path(observed['home']).resolve().is_relative_to(home):
   raise ValueError('actual Lua output/home differs from private launch')
  post=json.loads((home/'settings.json').read_text(encoding='utf-8-sig'))
  def subset(got,wanted):
   for k,v in wanted.items():
    if isinstance(v,dict):subset(got[k],v)
    elif type(got[k])is not type(v)or got[k]!=v:raise ValueError('persisted setting changed: '+k)
  subset(post,settings)
  summary=(out/'capture_complete.txt').read_text()
  if summary!='ok; headers=4; normal controller-only Rules repeat journey\n':raise ValueError('missing native completion')
  tail_summary=(out/'codec_complete.txt').read_text()
  if not tail_summary.startswith('ok; scopes=4; calls=20; instructions='):raise ValueError('incomplete codec observation')
  manifest['codec_summary']=tail_summary
  producer_summary=(out/'producer_complete.txt').read_text()
  if not producer_summary.startswith('ok; scopes=4; boundaries=48; instructions='):raise ValueError('incomplete producer observation')
  manifest['producer_summary']=producer_summary
  header_summary=(out/'header_work_complete.txt').read_text()
  if not header_summary.startswith('ok; scopes=4; boundaries=8; instructions='):raise ValueError('incomplete header work observation')
  manifest['header_work_summary']=header_summary
  manifest['isolation'].update(observed=observed,post_settings_sha256=sha(home/'settings.json'))
  manifest['artifacts']={f.name:dict(bytes=f.stat().st_size,sha256=sha(f))for f in out.iterdir()if f.is_file()and f.name!='manifest.json'}
  manifest['accepted']=True
 finally:(out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
 print('PASS: natural scheduler capture',out)

if __name__=='__main__':main()
