"""Build the source-bound NBPDRAW1/runtime input probe with /W4 /WX."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
BASE='744809a9d2ad548f83dedd9dffabce09e3cbda11'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 names=[ROOT/n.strip()for n in(ROOT/'nba95_sources.txt').read_text().splitlines()if n.strip()and not n.startswith('#')and n.strip()!='src/main.c']
 names.append(ROOT/'tools/sprite_pose_runtime_probe.c')
 inputs={str(q):sha(q)for q in[*names,ROOT/'nba95_sources.txt',Path(__file__),*list((ROOT/'include').glob('*.h'))]}
 exe=out/'sprite_pose_runtime_probe.exe'
 (out/'compile.rsp').write_text('/nologo /W4 /WX /O2 /MD /utf-8 /I "'+str(ROOT/'include')+'" /Fo"'+out.as_posix()+'/" /Fe"'+str(exe)+'"\n'+'\n'.join('"'+str(q)+'"'for q in names)+'\nuser32.lib gdi32.lib winmm.lib\n')
 (out/'compile.bat').write_text('@echo off\ncall "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul\nif errorlevel 1 exit /b %ERRORLEVEL%\ncl.exe @compile.rsp\nexit /b %ERRORLEVEL%\n')
 env={k:v for k,v in os.environ.items()if not k.startswith('NBA95')}
 r=subprocess.run(['cmd','/c',str(out/'compile.bat')],cwd=out,env=env,capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
 (out/'build.log').write_bytes(r.stdout+r.stderr)
 if r.returncode:raise RuntimeError((out/'build.log').read_text())
 if inputs!={n:sha(Path(n))for n in inputs}:raise RuntimeError('build inputs changed')
 (out/'manifest.json').write_text(json.dumps({'schema':1,'base':BASE,'source_and_headers':inputs,'exe_sha256':sha(exe),'translation_units':len(names),'objects':{p.name:sha(p)for p in out.glob('*.obj')}},indent=2)+'\n')
 print(f'Built {exe}; {len(names)} translation units')
if __name__=='__main__':main()
