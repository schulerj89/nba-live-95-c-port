"""Fresh /W4 /WX compositor probe, with exact pre-change compatibility build."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
BASE='c172877f378a102a174f58e6eae936abc8e5c781'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--baseline',action='store_true');a=p.parse_args()
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 source=ROOT/'src/nba_player_lab.c'
 if a.baseline:
  # Keep runtime-only APIs required by the current tipoff translation unit,
  # while restoring exactly the pre-component compatibility function.
  current=(ROOT/'src/nba_player_lab.c').read_text()
  old=subprocess.check_output(['git','show',BASE+':src/nba_player_lab.c'],cwd=ROOT,text=True)
  begin='bool nba_player_compose_sprite_parts('
  end='static bool draw_player_resources_at('
  cb=current.index(begin);ce=current.index(end,cb)
  ob=old.index(begin);oe=old.index(end,ob)
  source=out/'nba_player_lab.c';source.write_text(current[:cb]+old[ob:oe]+current[ce:])
 names=[ROOT/n.strip()for n in(ROOT/'nba95_sources.txt').read_text().splitlines()if n.strip()and not n.startswith('#')and n.strip()not in('src/main.c','src/nba_player_lab.c')]
 names += [source,ROOT/'tools/sprite_pose_probe.c']
 inputs={str(q):sha(q)for q in[*names,ROOT/'nba95_sources.txt',Path(__file__),*list((ROOT/'include').glob('*.h'))]}
 exe=out/'sprite_pose_probe.exe'
 (out/'compile.rsp').write_text('/nologo /W4 /WX /O2 /MD /utf-8 '+('/DLEGACY_ONLY 'if a.baseline else'')+'/I "'+str(ROOT/'include')+'" /Fo"'+out.as_posix()+'/" /Fe"'+str(exe)+'"\n'+'\n'.join('"'+str(q)+'"'for q in names)+'\nuser32.lib gdi32.lib winmm.lib\n')
 (out/'compile.bat').write_text('@echo off\ncall "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul\nif errorlevel 1 exit /b %ERRORLEVEL%\ncl.exe @compile.rsp\nexit /b %ERRORLEVEL%\n')
 env={k:v for k,v in os.environ.items()if not k.startswith('NBA95')}
 r=subprocess.run(['cmd','/c',str(out/'compile.bat')],cwd=out,env=env,capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
 (out/'build.log').write_bytes(r.stdout+r.stderr);assert r.returncode==0,(out/'build.log').read_text()
 assert inputs=={n:sha(Path(n))for n in inputs}
 (out/'manifest.json').write_text(json.dumps({'schema':1,'baseline':a.baseline,'base_commit':BASE,'source_and_headers':inputs,'exe_sha256':sha(exe),'translation_units':len(names),'objects':{p.name:sha(p)for p in out.glob('*.obj')}},indent=2)+'\n');print(exe)
if __name__=='__main__':main()
