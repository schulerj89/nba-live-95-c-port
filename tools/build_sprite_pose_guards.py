"""Fresh 40-source atomic refusal diagnostic, never reusing probe objects."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args()
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 names=[ROOT/n.strip()for n in(ROOT/'nba95_sources.txt').read_text().splitlines()if n.strip()and not n.startswith('#')and n.strip()!='src/main.c']+[ROOT/'tools/sprite_pose_guard_probe.c']
 inputs={str(q):sha(q)for q in[*names,ROOT/'nba95_sources.txt',Path(__file__),*list((ROOT/'include').glob('*.h'))]}
 exe=out/'sprite_pose_guard_probe.exe'
 (out/'compile.rsp').write_text('/nologo /W4 /WX /O2 /MD /utf-8 /I "'+str(ROOT/'include')+'" /Fo"'+out.as_posix()+'/" /Fe"'+str(exe)+'"\n'+'\n'.join('"'+str(q)+'"'for q in names)+'\nuser32.lib gdi32.lib winmm.lib\n')
 (out/'compile.bat').write_text('@echo off\ncall "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul\nif errorlevel 1 exit /b %ERRORLEVEL%\ncl.exe @compile.rsp\nexit /b %ERRORLEVEL%\n')
 env={k:v for k,v in os.environ.items()if not k.startswith('NBA95')}
 run=subprocess.run(['cmd','/c',str(out/'compile.bat')],cwd=out,env=env,capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
 (out/'build.log').write_bytes(run.stdout+run.stderr);assert run.returncode==0,(out/'build.log').read_text()
 assert inputs=={n:sha(Path(n))for n in inputs}
 (out/'manifest.json').write_text(json.dumps({'schema':1,'source_and_headers':inputs,'exe_sha256':sha(exe),'translation_units':len(names),'objects':{p.name:sha(p)for p in out.glob('*.obj')}},indent=2)+'\n');print(exe)
if __name__=='__main__':main()
