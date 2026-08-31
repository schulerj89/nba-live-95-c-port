"""C2 normal controller input capture; no injected CPU/WRAM/ROM state."""
import argparse,json,os,shutil,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
sys.path.insert(0,str(Path(__file__).resolve().parent))
import mesen_portable as m
ROM='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
MESEN='d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b'
def main():
 p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);p.add_argument('--selection',type=int,choices=[0,2],required=True);p.add_argument('--frames',type=int,default=6000);p.add_argument('--rom',type=Path,required=True);p.add_argument('--mesen',type=Path,required=True);a=p.parse_args()
 assert 600<=a.frames<=14000 and m.sha(a.rom)==ROM and m.sha(a.mesen)==MESEN
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);exe,iso=m.prepare(out,a.mesen)
 for source,name in [(Path(__file__),'capture.py'),(Path(__file__).with_name('mesen_receiver_prepare.lua'),'capture.lua'),(Path(m.__file__),'mesen_portable.py')]:shutil.copyfile(source,out/name)
 env={k:v for k,v in os.environ.items()if not k.startswith('NBA95')};env.update(NBA95_CAPTURE_DIR=str(out),NBA95_C2_SELECTION=str(a.selection),NBA95_C2_FRAMES=str(a.frames))
 command=[str(exe),'--testrunner','--timeout=900',str(a.rom.resolve()),str(out/'capture.lua')]
 def identity(q):return dict(path=str(q.resolve()),size=q.stat().st_size,sha256=m.sha(q))
 metadata=dict(schema=1,kind='C2 ordinary controller AF66/B468 inheritance',state_injection=False,rom_patch=False,selection=a.selection,requested_frames=a.frames,command=command,environment={k:v for k,v in env.items()if k.startswith('NBA95')},isolation=iso,sources={name:identity(q)for name,q in [('rom',a.rom),('mesen',exe),('capture',out/'capture.lua'),('runner',out/'capture.py'),('isolation_helper',out/'mesen_portable.py')]},accepted_capture=False)
 def save():(out/'manifest.json').write_text(json.dumps(metadata,indent=2)+'\n')
 save()
 try:
  with(out/'stdout.log').open('wb')as stdout,(out/'stderr.log').open('wb')as stderr:
   proc=subprocess.Popen(command,cwd=exe.parent,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW);metadata['pid']=proc.pid;save()
   try:code=proc.wait(timeout=920)
   except subprocess.TimeoutExpired:proc.kill();proc.wait();raise
  metadata['exit_code']=code;assert code==0
  metadata['recorded_post_settings_sha256']=m.sha(out/'portable-mesen/settings.json')
  metadata['isolation']=m.verify(out,iso)
  metadata['completion']=(out/'capture_complete.txt').read_text();assert metadata['completion'].startswith('C2 ordinary controller capture;')
  metadata['accepted_capture']=True
 finally:
  metadata['artifacts']={str(q.relative_to(out)):identity(q)for q in out.rglob('*')if q.is_file()and q.name!='manifest.json'};save()
 print(metadata['completion'],str(out),flush=True)
if __name__=='__main__':main()
