"""Private isolated capture, explicit environment; ROM is read-only."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path

def digest(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True)
    p.add_argument('--mode',choices=('natural','controlled'),required=True)
    p.add_argument('--mesen',type=Path,required=True)
    p.add_argument('--rom',type=Path,default=Path('F:/Games/SNES/NBA Live 95 (USA).sfc'))
    a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
    runtime=out/'portable-mesen';runtime.mkdir();saves=out/'isolated-saves';saves.mkdir()
    rom=a.rom.resolve();assert digest(rom)=='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
    mesen=runtime/'Mesen.exe';shutil.copy2(a.mesen,mesen)
    script=out/'capture.lua';shutil.copy2(Path(__file__).with_name('mesen_audio_contract.lua'),script)
    runner=out/'capture_audio_contract.py';shutil.copy2(__file__,runner)
    settings={'Debug':{'ScriptWindow':{'AllowIoOsAccess':True,'ScriptTimeout':60,'SaveScriptBeforeRun':False}},
      'Preferences':{'SingleInstance':False,'PauseWhenInBackground':False,'AutoLoadPatches':False,'OverrideSaveDataFolder':True,'SaveDataFolder':str(saves)},
      'Snes':{'Port1':{'Type':'SnesController'},'Port2':{'Type':'None'},'DisableFrameSkipping':True,'EnableRandomPowerOnState':False,'RamPowerOnState':'AllZeros','ForceFixedResolution':False,'Overscan':{'Top':7,'Bottom':8,'Left':0,'Right':0}},
      'Video':{'VideoFilter':'None','AspectRatio':'NoStretching','Brightness':0,'Contrast':0,'Hue':0,'Saturation':0,'ScanlineIntensity':0,'UseBilinearInterpolation':False,'ScreenRotation':'None'}}
    setting_file=runtime/'settings.json';setting_file.write_text(json.dumps(settings,indent=2))
    env={k:v for k,v in os.environ.items() if not k.startswith('NBA95_')}
    env.update(NBA95_CAPTURE_DIR=out.as_posix(),NBA95_AUDIO_CONTRACT=a.mode)
    args=[str(mesen),'--testrunner','--timeout=300',str(rom),str(script)]
    manifest={'schema':1,'mode':a.mode,'navigation':'normal controller-only title/Main/Options/Team/Player Setup, center selection1',
      'interventions':'none' if a.mode=='natural' else 'declared audio input word writes only at82FD65 after100courtframes; all before/after writes retained',
      'rom_patch':False,'cpu_register_injection':False,'initial_save_files':[],
      'isolation':{'home':str(runtime),'save_folder':str(saves),'settings':settings},'arguments':args,
      'sources':{k:{'path':str(v),'sha256':digest(v)} for k,v in {'rom':rom,'mesen':mesen,'script':script,'runner':runner,'settings':setting_file}.items()}}
    manifest_path=out/'manifest.json';manifest_path.write_text(json.dumps(manifest,indent=2))
    with (out/'stdout.log').open('wb') as stdout,(out/'stderr.log').open('wb') as stderr:
        result=subprocess.run(args,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=0x08000000 if os.name=='nt' else 0)
    manifest['exit_code']=result.returncode
    if (out/'capture_complete.json').exists():manifest['completion']=json.loads((out/'capture_complete.json').read_text())
    manifest['artifacts']={v.name:{'bytes':v.stat().st_size,'sha256':digest(v)} for v in out.iterdir() if v.is_file() and v!=manifest_path}
    manifest_path.write_text(json.dumps(manifest,indent=2))
    assert result.returncode==0 and 'completion' in manifest,f'incomplete capture: {out}'
    observed=Path((out/'observed-script-data-folder.txt').read_text().strip()).resolve()
    assert (runtime/'LuaScriptData').resolve() in observed.parents,f'wrong Mesen home {observed}'
    assert digest(setting_file)==manifest['sources']['settings']['sha256'],'settings changed'
    assert digest(rom)==manifest['sources']['rom']['sha256'],'ROM changed'
    manifest['isolation']['observed_script_data_folder']=str(observed)
    manifest['isolation']['post_settings_verified']=True
    manifest_path.write_text(json.dumps(manifest,indent=2));print(json.dumps(manifest['completion']))
if __name__=='__main__':main()
