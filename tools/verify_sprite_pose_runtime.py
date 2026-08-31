"""Fresh old-compositor CLI and ordinary 390-frame compatibility journey."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
BASE='c172877f378a102a174f58e6eae936abc8e5c781'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for name in('exe','pack','rom','output'):p.add_argument('--'+name,type=Path,required=True)
 a=p.parse_args();a=argparse.Namespace(**{k:v.resolve()for k,v in vars(a).items()});out=a.output;out.mkdir(parents=True,exist_ok=False)
 assert sha(a.rom)=='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'and sha(a.pack)=='f564c29612928984002ed3f0389d317de639fff122baf61a7bc9ecaef2a6be09'
 m=json.loads(a.exe.with_name('manifest.json').read_text());assert sha(a.exe)==m['exe_sha256']
 for n,h in m['compiled_sources'].items():assert sha(ROOT/n)==h
 for n,h in m['headers'].items():assert sha(ROOT/'include'/n)==h
 build=out/'baseline';build.mkdir();source=build/'nba_player_lab.c'
 source.write_bytes(subprocess.check_output(['git','show',BASE+':src/nba_player_lab.c'],cwd=ROOT))
 names=[source if n=='src/nba_player_lab.c'else ROOT/n for n in m['compiled_sources']]
 assert len(names)==40;exe=build/'baseline.exe'
 (build/'compile.rsp').write_text('/nologo /W4 /WX /O2 /MD /utf-8 /I "'+str(ROOT/'include')+'" /Fo"'+build.as_posix()+'/" /Fe"'+str(exe)+'"\n'+'\n'.join('"'+str(q)+'"'for q in names)+'\nuser32.lib gdi32.lib winmm.lib\n')
 (build/'compile.bat').write_text('@echo off\ncall "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul\nif errorlevel 1 exit /b %ERRORLEVEL%\ncl.exe @compile.rsp\nexit /b %ERRORLEVEL%\n')
 env={k:v for k,v in os.environ.items()if not k.startswith('NBA95')}
 r=subprocess.run(['cmd','/c',str(build/'compile.bat')],cwd=build,env=env,capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
 (build/'build.log').write_bytes(r.stdout+r.stderr);assert r.returncode==0
 (build/'manifest.json').write_text(json.dumps({'sources':{str(p):sha(p)for p in names},'headers':m['headers'],'exe_sha256':sha(exe),'objects':{p.name:sha(p)for p in build.glob('*.obj')}},indent=2)+'\n')
 results={}
 for label,binary in [('baseline',exe),('candidate',a.exe)]:
  runout=out/(label+'-runtime');runout.mkdir()
  cmd=[str(binary),'--headless','--rom',str(a.rom),'--assets',str(a.pack),'--tipoff-only','--tipoff-clock','43200','--frames','390','--gameplay-trace',str(runout/'trace.jsonl'),'--dump-sequence-dir',str(runout),'--dump-sequence-from','390','--debug-state']
  r=subprocess.run(cmd,capture_output=True,env=env,creationflags=subprocess.CREATE_NO_WINDOW)
  (runout/'stdout.txt').write_bytes(r.stdout);(runout/'stderr.txt').write_bytes(r.stderr);assert r.returncode==0
  results[label]={'command':cmd,'exit_code':0,'exe_sha256':sha(binary),'trace_sha256':sha(runout/'trace.jsonl'),'frames':len((runout/'trace.jsonl').read_text().splitlines()),'bmp_sha256':{p.name:sha(p)for p in runout.glob('*.bmp')}}
 assert results['candidate']['frames']==results['baseline']['frames']==390
 assert results['candidate']['trace_sha256']==results['baseline']['trace_sha256']
 assert results['candidate']['bmp_sha256']==results['baseline']['bmp_sha256'] and len(results['candidate']['bmp_sha256'])==1
 for n,h in m['compiled_sources'].items():assert sha(ROOT/n)==h
 for n,h in m['headers'].items():assert sha(ROOT/'include'/n)==h
 (out/'report.json').write_text(json.dumps({'passed':True,'runs':results,'limits':'Compatibility regression only. New literal DP compositor is not the gameplay caller yet; this does not claim repaired body/head flags in visible gameplay.'},indent=2)+'\n');print('390 full runtime snapshots and final rendered BMP byte-identical')
if __name__=='__main__':main()
