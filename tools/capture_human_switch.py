"""Capture normal native caller boundaries without modifying frozen controller evidence."""
import argparse,json,os,shutil,subprocess
from pathlib import Path
import mesen_portable

def main():
 p=argparse.ArgumentParser()
 p.add_argument('--output',type=Path,required=True)
 p.add_argument('--selection',type=int,choices=[0,2],required=True)
 p.add_argument('--frames',type=int,default=1800)
 p.add_argument('--rom',type=Path,required=True)
 p.add_argument('--mesen',type=Path,required=True)
 a=p.parse_args()
 if not 400<=a.frames<=3000: p.error('frames must be400..3000')
 if mesen_portable.sha(a.rom)!='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870':raise ValueError('unexpected ROM')
 if mesen_portable.sha(a.mesen)!='d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b':raise ValueError('unexpected Mesen')
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 exe,isolation=mesen_portable.prepare(out,a.mesen)
 script=out/'capture.lua';shutil.copyfile(Path(__file__).with_name('mesen_human_switch.lua'),script)
 shutil.copyfile(__file__,out/Path(__file__).name)
 shutil.copyfile(mesen_portable.__file__,out/'mesen_portable.py')
 env=os.environ.copy();env.update(NBA95_CAPTURE_DIR=out.as_posix(),NBA95_SWITCH_SELECTION=str(a.selection),NBA95_SWITCH_FRAMES=str(a.frames))
 args=[str(exe),'--testrunner','--timeout=240',str(a.rom.resolve()),str(script)]
 m=dict(schema=1,kind='native human switch boundaries',selection=a.selection,state_injection=False,rom_patch=False,
  sparse_ranges=[[0,0x100],[0x500,0x500],[0x1600,0x300],[0x3400,0x1600]],requested_frames=a.frames,
  arguments=args,environment={k:v for k,v in env.items()if k.startswith('NBA95_SWITCH')or k=='NBA95_CAPTURE_DIR'},isolation=isolation,
  sources={k:dict(path=str(v),sha256=mesen_portable.sha(v))for k,v in dict(rom=a.rom.resolve(),mesen=exe,capture=script,runner=out/Path(__file__).name,isolation_helper=out/'mesen_portable.py').items()})
 def save():(out/'manifest.json').write_text(json.dumps(m,indent=2)+'\n')
 save()
 with(out/'stdout.log').open('wb')as stdout,(out/'stderr.log').open('wb')as stderr:
  r=subprocess.run(args,cwd=exe.parent,env=env,stdout=stdout,stderr=stderr,timeout=270,creationflags=subprocess.CREATE_NO_WINDOW)
 m['exit_code']=r.returncode;save()
 if r.returncode or not(out/'capture_complete.txt').is_file():raise RuntimeError('capture failed; preserve '+str(out))
 m['isolation']=mesen_portable.verify(out,isolation)
 m['completion']=(out/'capture_complete.txt').read_text()
 m['artifacts']={v.name:dict(bytes=v.stat().st_size,sha256=mesen_portable.sha(v))for v in out.iterdir()if v.is_file()and v.name!='manifest.json'}
 save();print(m['completion'],flush=True)

if __name__=='__main__':main()
