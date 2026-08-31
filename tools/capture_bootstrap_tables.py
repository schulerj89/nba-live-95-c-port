"""Observe unpatched NBA95 normal cold power-on through pre-NMI-enable8145."""
import argparse,hashlib,json,os,shutil,subprocess
from pathlib import Path
ROM_SHA='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
MESEN_SHA='d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def subset(got,want):
    for k,v in want.items():
        if isinstance(v,dict):subset(got[k],v)
        elif type(got[k])is not type(v)or got[k]!=v:raise ValueError('settings changed: '+k)
def main():
    p=argparse.ArgumentParser();p.add_argument('--rom',type=Path,required=True);p.add_argument('--mesen',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    if sha(a.rom)!=ROM_SHA or sha(a.mesen)!=MESEN_SHA:raise ValueError('canonical ROM/pinned emulator required')
    out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);home=out/'portable-mesen';home.mkdir();saves=out/'saves';saves.mkdir()
    exe=home/'Mesen.exe';shutil.copyfile(a.mesen,exe)
    settings={'Debug':{'ScriptWindow':{'AllowIoOsAccess':True,'ScriptTimeout':60}},'Preferences':{'SingleInstance':False,'PauseWhenInBackground':False,'AutoLoadPatches':False,'OverrideSaveDataFolder':True,'SaveDataFolder':str(saves)},'Snes':{'Port1':{'Type':'SnesController'},'Port2':{'Type':'None'},'EnableRandomPowerOnState':False,'RamPowerOnState':'AllZeros','DisableFrameSkipping':True,'SpcClockSpeedAdjustment':40,'Region':'Ntsc'}}
    initial=out/'initial-settings.json';initial.write_text(json.dumps(settings,indent=2)+'\n');shutil.copyfile(initial,home/'settings.json')
    script=out/'capture.lua';shutil.copyfile(Path(__file__).with_name('mesen_bootstrap_tables.lua'),script);runner=out/'runner.py';shutil.copyfile(__file__,runner)
    args=[str(exe),'--testrunner','--timeout=180',str(a.rom.resolve()),str(script)]
    m={'schema':1,'kind':'normal reset table initialization observation','state_injection':False,'rom_patch':False,'inputs':False,'accepted':False,'sources':{},'settings':settings,'initial_saves':[],'arguments':args}
    for name,path in [('rom',a.rom.resolve()),('mesen',exe),('script',script),('runner',runner),('settings',initial)]:m['sources'][name]={'path':str(path),'sha256':sha(path)}
    env={k:v for k,v in os.environ.items()if not k.startswith('NBA95_')};env['NBA95_BOOTSTRAP_DIR']=str(out)
    startup=subprocess.STARTUPINFO();startup.dwFlags|=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=0
    try:
        with (out/'stdout.log').open('w')as stdout,(out/'stderr.log').open('w')as stderr:
            r=subprocess.run(args,env=env,stdout=stdout,stderr=stderr,startupinfo=startup,timeout=200)
        m['exit_code']=r.returncode
        if type(r.returncode)is not int or r.returncode!=0:raise RuntimeError('capture process failed')
        if (out/'complete.txt').read_text()!='ok; normal power-on through pre-NMI-enable8145\n':raise ValueError('incomplete capture')
        post=json.loads((home/'settings.json').read_text(encoding='utf-8-sig'));subset(post,settings)
        m['post_settings_sha256']=sha(home/'settings.json')
        observed=dict(line.split('=',1)for line in(out/'environment.txt').read_text().splitlines())
        if Path(observed['output']).resolve()!=out or not Path(observed['home']).resolve().is_relative_to(home):raise ValueError('private environment mismatch')
        m['observed_environment']=observed
        m['artifacts']={f.name:{'bytes':f.stat().st_size,'sha256':sha(f)}for f in out.iterdir()if f.is_file()and f.name!='manifest.json'};m['accepted']=True
    finally:(out/'manifest.json').write_text(json.dumps(m,indent=2)+'\n')
    print('PASS capture',out)
if __name__=='__main__':main()
