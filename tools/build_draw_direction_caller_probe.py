"""Fresh actual static draw caller, with an optional unchanged source control."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);p.add_argument('--baseline',action='store_true');a=p.parse_args()
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 names=[ROOT/n.strip() for n in (ROOT/'nba95_sources.txt').read_text().splitlines() if n.strip() and not n.startswith('#') and n.strip()not in('src/main.c','src/nba_tipoff.c')]
 probe=ROOT/'tools/draw_direction_caller_probe.c';dependency=ROOT/'src/nba_tipoff.c'
 if a.baseline:
  old=out/'baseline';(old/'src').mkdir(parents=True);(old/'tools').mkdir()
  for name in('nba_tipoff.c','nba_gameplay_ai.c'):
   (old/'src'/name).write_bytes(subprocess.check_output(['git','show','378900b:src/'+name],cwd=ROOT))
  (old/'tools'/probe.name).write_bytes(probe.read_bytes());probe=old/'tools'/probe.name
  dependency=old/'src/nba_tipoff.c';names[names.index(ROOT/'src/nba_gameplay_ai.c')]=old/'src/nba_gameplay_ai.c'
 names.append(probe)
 bound={str(p):sha(p)for p in names+[dependency,ROOT/'nba95_sources.txt',Path(__file__).resolve()]+list((ROOT/'include').glob('*.h'))}
 vswhere=Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
 vs=subprocess.check_output([str(vswhere),'-latest','-products','*','-requires','Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath']).decode().strip()
 exe=out/'draw_direction_caller_probe.exe'
 (out/'compile.rsp').write_text(f'/nologo /W4 /WX /O2 /MD /utf-8 /I "{ROOT / "include"}" /Fo"{out.as_posix()}/" /Fe"{exe}"\n'+'\n'.join(f'"{p}"'for p in names)+'\nuser32.lib gdi32.lib winmm.lib\n')
 batch=out/'compile.bat';batch.write_text(f'@echo off\ncall "{Path(vs)/"VC/Auxiliary/Build/vcvars64.bat"}" >nul\nif errorlevel 1 exit /b %ERRORLEVEL%\ncl.exe @compile.rsp\nexit /b %ERRORLEVEL%\n')
 run=subprocess.run(['cmd.exe','/c',str(batch)],cwd=out,capture_output=True,env={k:v for k,v in os.environ.items()if not k.startswith('NBA95')},creationflags=subprocess.CREATE_NO_WINDOW)
 (out/'build.log').write_bytes(run.stdout+run.stderr)
 assert run.returncode==0, out/'build.log'
 assert bound=={p:sha(Path(p))for p in bound}
 (out/'manifest.json').write_text(json.dumps({'baseline':a.baseline,'sources':bound,'translation_units':len(names),'exe_sha256':sha(exe)},indent=2)+'\n')
 print(exe)
if __name__=='__main__':main()
