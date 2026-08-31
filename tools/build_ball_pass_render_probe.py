"""Fresh actual-source ball render diagnostic; optional exact base comparison."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
BASE='8f5b90382b112c298cac9c02ef035cb0ab848f00'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);p.add_argument('--baseline',action='store_true');a=p.parse_args()
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 names=[ROOT/n.strip()for n in(ROOT/'nba95_sources.txt').read_text().splitlines()if n.strip()and not n.startswith('#')and n.strip()not in('src/main.c','src/nba_tipoff.c')]
 source=ROOT/'src/nba_tipoff.c';probe=ROOT/'tools/ball_pass_render_probe.c'
 if a.baseline:
  source=out/'nba_tipoff_baseline.c';source.write_bytes(subprocess.check_output(['git','show',BASE+':src/nba_tipoff.c'],cwd=ROOT))
  probe=out/'probe_baseline.c';probe.write_text((ROOT/'tools/ball_pass_render_probe.c').read_text().replace('#include "../src/nba_tipoff.c"','#include "'+source.as_posix()+'"'))
 names.append(probe);inputs={str(q):sha(q)for q in[*names,source,ROOT/'nba95_sources.txt',*list((ROOT/'include').glob('*.h'))]}
 exe=out/'ball_pass_render_probe.exe'
 (out/'compile.rsp').write_text('/nologo /W4 /WX /O2 /MD /utf-8 /I "'+str(ROOT/'include')+'" /Fo"'+out.as_posix()+'/" /Fe"'+str(exe)+'"\n'+'\n'.join('"'+str(q)+'"'for q in names)+'\nuser32.lib gdi32.lib winmm.lib\n')
 (out/'compile.bat').write_text('@echo off\ncall "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul\nif errorlevel 1 exit /b %ERRORLEVEL%\ncl.exe @compile.rsp\nexit /b %ERRORLEVEL%\n')
 env={k:v for k,v in os.environ.items()if not k.startswith('NBA95')}
 r=subprocess.run(['cmd','/c',str(out/'compile.bat')],cwd=out,env=env,capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
 (out/'build.log').write_bytes(r.stdout+r.stderr);assert r.returncode==0,(out/'build.log').read_text()
 assert inputs=={n:sha(Path(n))for n in inputs}
 (out/'manifest.json').write_text(json.dumps({'baseline':a.baseline,'base_commit':BASE,'source_and_headers':inputs,'exe_sha256':sha(exe),'translation_units':len(names)},indent=2)+'\n');print(exe)
if __name__=='__main__':main()
