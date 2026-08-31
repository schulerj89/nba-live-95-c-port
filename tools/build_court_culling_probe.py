"""Build the actual culling caller's module; optionally retain its old body."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--baseline', action='store_true')
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    names = [ROOT/line.strip() for line in (ROOT/'nba95_sources.txt').read_text().splitlines()
             if line.strip() and not line.startswith('#') and line.strip() != 'src/main.c']
    names.append(ROOT/'tools/court_culling_probe.c')
    if args.baseline:
        old = out/'nba_court_presentation_baseline.c'
        old.write_bytes(subprocess.check_output(['git','show','facd818:src/nba_court_presentation.c'], cwd=ROOT))
        names[names.index(ROOT/'src/nba_court_presentation.c')] = old
    sources = {str(path):sha(path) for path in names}
    headers = {str(path):sha(path) for path in (ROOT/'include').glob('*.h')}
    vswhere = Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
    vs = subprocess.check_output([str(vswhere),'-latest','-products','*','-requires',
                                  'Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath']).decode().strip()
    vcvars = Path(vs)/'VC/Auxiliary/Build/vcvars64.bat'
    exe = out/'court_culling_probe.exe'
    (out/'compile.rsp').write_text(f'/nologo /W4 /WX /O2 /MD /utf-8 /I "{ROOT / "include"}" /Fo"{out.as_posix()}/" /Fe"{exe}"\n'
                                  +'\n'.join(f'"{path}"' for path in names)+'\nuser32.lib gdi32.lib winmm.lib\n')
    batch = out/'compile.bat'
    batch.write_text(f'@echo off\ncall "{vcvars}" >nul\nif errorlevel 1 exit /b %ERRORLEVEL%\ncl.exe @compile.rsp\nexit /b %ERRORLEVEL%\n')
    run = subprocess.run(['cmd.exe','/c',str(batch)], cwd=out,
                         env={k:v for k,v in os.environ.items() if not k.startswith('NBA95')},
                         capture_output=True, creationflags=subprocess.CREATE_NO_WINDOW)
    (out/'build.log').write_bytes(run.stdout+run.stderr)
    if run.returncode:
        raise RuntimeError('Retained failed build: '+str(out/'build.log'))
    assert sources == {path:sha(Path(path)) for path in sources}
    assert headers == {str(path):sha(path) for path in (ROOT/'include').glob('*.h')}
    (out/'manifest.json').write_text(json.dumps({'baseline':args.baseline,'sources':sources,
                'headers':headers,'exe_sha256':sha(exe),'translation_units':len(names)},indent=2)+'\n')
    print(exe)


if __name__ == '__main__':
    main()
