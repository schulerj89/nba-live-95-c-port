"""Private original-game neutral CPU draw capture; no state injection."""
import argparse,json,os,shutil,subprocess
from pathlib import Path
from mesen_portable import prepare,verify,sha
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);p.add_argument('--rom',required=True,type=Path);p.add_argument('--mesen',required=True,type=Path);a=p.parse_args()
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);rom=a.rom.resolve()
 assert sha(rom)=='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
 exe,iso=prepare(out,a.mesen);assert sha(exe)=='d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b'
 for source,target in [(Path(__file__),'runner.py'),(Path(__file__).with_name('mesen_ball_pass_draw.lua'),'capture.lua'),(Path(__file__).with_name('mesen_portable.py'),'mesen_portable.py')]:shutil.copyfile(source,out/target)
 env={k:v for k,v in os.environ.items()if not k.startswith('NBA95')};env['NBA95_CAPTURE_DIR']=str(out)
 command=[str(exe),'--testrunner','--timeout=300',str(rom),str(out/'capture.lua')]
 m={'schema':1,'rom_sha256':sha(rom),'mesen_sha256':sha(exe),'command':command,'environment':{'NBA95_CAPTURE_DIR':str(out)},'isolation':iso,'state_injection':False,'accepted_capture':False}
 try:
  with(out/'mesen.log').open('wb')as log:
   run=subprocess.Popen(command,env=env,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW);m['pid']=run.pid;m['exit_code']=run.wait(timeout=320)
  assert m['exit_code']==0
  verify(out,iso);m['summary']=(out/'capture_complete.txt').read_text();assert m['summary'].startswith('Ball ordinary CPU draw;')
  m['artifacts']={str(q.relative_to(out)):{'size':q.stat().st_size,'sha256':sha(q)}for q in out.rglob('*')if q.is_file()and q.name!='manifest.json'};m['accepted_capture']=True
 finally:(out/'manifest.json').write_text(json.dumps(m,indent=2)+'\n')
 print(m['summary'])
if __name__=='__main__':main()
