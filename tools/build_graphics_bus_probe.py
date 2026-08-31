"""Build the canonical-WRAM graphics queue/alias probe."""
from pathlib import Path
import argparse,hashlib,json,os,subprocess
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
parser=argparse.ArgumentParser()
parser.add_argument('--output',type=Path,default=ROOT/'build/graphics-bus-probe')
args=parser.parse_args()
out=args.output if args.output.is_absolute() else ROOT/args.output
out.mkdir(parents=True,exist_ok=False)
sources=[ROOT/p for p in('src/nba_setup_scheduler.c','src/nba_graphics_bus.c','tools/graphics_bus_probe.c')]
headers=[ROOT/'include/nba_setup_scheduler.h',ROOT/'include/nba_graphics_bus.h'];before={str(p):sha(p)for p in sources+headers}
vswhere=Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe';vs=subprocess.check_output([str(vswhere),'-latest','-products','*','-requires','Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath']).decode().strip();vcvars=Path(vs)/'VC/Auxiliary/Build/vcvars64.bat'
exe=out/'graphics_bus_probe.exe';(out/'compile.rsp').write_text(f'/nologo /W4 /WX /O2 /MD /utf-8 /I "{ROOT/"include"}" /Fo"{out.as_posix()}/" /Fe"{exe}"\n'+'\n'.join(f'"{p}"'for p in sources)+'\n')
(out/'compile.bat').write_text(f'@echo off\ncall "{vcvars}" >nul\ncl.exe @compile.rsp\nexit /b %ERRORLEVEL%\n')
r=subprocess.run(['cmd','/c',str(out/'compile.bat')],cwd=out,capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW);(out/'build.log').write_bytes(r.stdout+r.stderr);assert r.returncode==0,(out/'build.log').read_text();assert before=={p:sha(Path(p))for p in before}
(out/'manifest.json').write_text(json.dumps({'sources':before,'exe_sha256':sha(exe)},indent=2)+'\n');print(exe)
