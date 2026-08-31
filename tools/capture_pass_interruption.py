"""Record original ordinary CPU knockdown/pass interruption, without state writes."""
import argparse,os,shutil,subprocess,json
from pathlib import Path
from mesen_portable import prepare,verify,sha
ROM='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
MESEN='d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b'
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--rom',type=Path,required=True);p.add_argument('--mesen',type=Path);a=p.parse_args()
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);rom=a.rom.resolve();assert sha(rom)==ROM
 exe,iso=prepare(out,a.mesen or Path(shutil.which('Mesen.exe')));assert sha(exe)==MESEN
 for source,target in[(Path(__file__),'runner.py'),(Path(__file__).with_name('mesen_pass_interruption.lua'),'capture.lua'),(Path(__file__).with_name('mesen_portable.py'),'mesen_portable.py')]:shutil.copyfile(source,out/target)
 env={k:v for k,v in os.environ.items()if not k.startswith('NBA95')};env['NBA95_CAPTURE_DIR']=str(out)
 command=[str(exe),'--testrunner','--timeout=900',str(rom),str(out/'capture.lua')]
 m={'schema':1,'kind':'C1 normal neutral CPU contact/knockdown source route','rom_sha256':ROM,'mesen_sha256':MESEN,'script_sha256':sha(out/'capture.lua'),'runner_sha256':sha(out/'runner.py'),'command':command,'environment':{'NBA95_CAPTURE_DIR':str(out)},'isolation':iso,'state_injection':False,'rom_patch':False,'accepted_capture':False}
 try:
  with(out/'mesen.log').open('wb')as log:r=subprocess.run(command,env=env,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW,timeout=920)
  m['exit_code']=r.returncode;assert r.returncode==0
  verify(out,iso);m['summary']=(out/'capture_complete.txt').read_text();assert m['summary'].startswith('C1 natural CPU capture;')
  m['artifacts']={str(q.relative_to(out)):dict(size=q.stat().st_size,sha256=sha(q))for q in out.rglob('*')if q.is_file()and q.name!='manifest.json'};m['accepted_capture']=True
 finally:(out/'manifest.json').write_text(json.dumps(m,indent=2)+'\n')
 print(m['summary'],out)
if __name__=='__main__':main()
