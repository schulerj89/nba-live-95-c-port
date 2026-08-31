"""Isolated original-ROM draw-order observation; no state/ROM seed."""
import argparse,json,os,shutil,subprocess
from pathlib import Path
import mesen_portable
ROM_SHA='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
MESEN_SHA='d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b'
def main():
 p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 rom=Path('F:/Games/SNES/NBA Live 95 (USA).sfc')
 installed=Path('C:/Users/joshs/AppData/Local/Microsoft/WinGet/Packages/SourMesen.Mesen2_Microsoft.Winget.Source_8wekyb3d8bbwe/Mesen.exe')
 assert mesen_portable.sha(rom)==ROM_SHA and mesen_portable.sha(installed)==MESEN_SHA
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 exe,isolation=mesen_portable.prepare(out,installed)
 for src,name in [(Path(__file__).with_suffix('.lua'),'capture_draw_order.lua'),(Path(__file__),'capture_draw_order.py'),(Path(mesen_portable.__file__),'mesen_portable.py')]:shutil.copyfile(src,out/name)
 env={k:v for k,v in os.environ.items()if not k.startswith('NBA95')};env['NBA95_CAPTURE_DIR']=out.as_posix()
 args=[str(exe),'--testrunner','--timeout=180',str(rom),str(out/'capture_draw_order.lua')]
 m=dict(schema=1,kind='normal coldboot CPU menu draw order',state_injection=False,rom_patch=False,
        arguments=args,environment={'NBA95_CAPTURE_DIR':out.as_posix()},isolation=isolation,
        sources={str(x):mesen_portable.sha(x)for x in [rom,exe,out/'capture_draw_order.lua',out/'capture_draw_order.py',out/'mesen_portable.py']})
 def save():(out/'manifest.json').write_text(json.dumps(m,indent=2)+'\n')
 save()
 try:
  with(out/'stdout.log').open('xb')as stdout,(out/'stderr.log').open('xb')as stderr:
   r=subprocess.run(args,cwd=exe.parent,env=env,stdout=stdout,stderr=stderr,timeout=210,creationflags=subprocess.CREATE_NO_WINDOW)
  m['exit_code']=r.returncode
 except subprocess.TimeoutExpired:
  m['timeout']=True;save();raise
 m['isolation']=mesen_portable.verify(out,isolation)
 m['artifacts']={x.name:dict(bytes=x.stat().st_size,sha256=mesen_portable.sha(x))for x in out.iterdir()if x.is_file()and x.name!='manifest.json'}
 save();assert type(r.returncode)is int and r.returncode==0 and(out/'capture_complete.txt').is_file(),out
 print((out/'capture_complete.txt').read_text(),flush=True)
if __name__=='__main__':main()
