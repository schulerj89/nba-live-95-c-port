"""Build frozen C regression revisions and retain the complete closure diagnostics.

This is not a ROM parity tool. No expected digest is changed. A legacy digest
failure is retained alongside exact output so it can be independently audited.
"""
import argparse
import hashlib
import io
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import zipfile

ROOT=Path(__file__).resolve().parents[1]


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def snapshot_sources(target,baseline):
    target.mkdir(parents=True)
    if baseline:
        archive=subprocess.run(['git','archive','--format=zip',baseline,
            'src','include','nba95_sources.txt'],cwd=ROOT,capture_output=True,check=True).stdout
        with zipfile.ZipFile(io.BytesIO(archive)) as entries:
            for entry in entries.infolist():
                if Path(entry.filename).is_absolute() or '..' in Path(entry.filename).parts:
                    raise ValueError('unsafe git archive entry')
            entries.extractall(target)
    else:
        for name in ('src','include'):shutil.copytree(ROOT/name,target/name)
        shutil.copy2(ROOT/'nba95_sources.txt',target/'nba95_sources.txt')
    shutil.copy2(ROOT/'tools/gameplay100_closure_probe.c',target/'closure_probe.c')
    return {str(path.relative_to(target)).replace('\\','/'):sha(path)
            for path in sorted(target.rglob('*')) if path.is_file()}


def compile_probe(directory,vcvars,expected_override=None):
    source=directory/'source';objects=directory/'obj';objects.mkdir()
    sources=[source/line for line in (source/'nba95_sources.txt').read_text().splitlines()
             if line.startswith('src/') and Path(line).name not in ('main.c','win32_game_main.c')]
    batch=directory/'compile.bat';exe=directory/'closure.exe'
    command=' '.join('"'+str(path)+'"' for path in [source/'closure_probe.c',*sources])
    batch.write_text('@echo off\ncall "'+str(vcvars)+'" >nul\n'
        'cl.exe /nologo /W4 /O2 /MD /utf-8 '+
        (('/DNBA_CLOSURE_EXPECTED_DIGEST=0x'+expected_override+
          'ull /DNBA_CLOSURE_HISTORICAL_NAVIGATION=1 ') if expected_override else '')+
        '/I "'+str(source/'include')+'" /Fe"'+
        str(exe)+'" /Fo"'+objects.as_posix()+'/" '+command+
        ' user32.lib gdi32.lib winmm.lib\nexit /b %ERRORLEVEL%\n',encoding='ascii')
    with (directory/'build.log').open('wb') as log:
        subprocess.run(['cmd.exe','/c',str(batch)],cwd=directory,stdout=log,
                       stderr=subprocess.STDOUT,check=True,timeout=180)
    return exe


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--baseline',required=True);p.add_argument('--output',required=True)
    p.add_argument('--before-pack',required=True);p.add_argument('--pack',required=True)
    a=p.parse_args();output=Path(a.output).resolve()
    if output.exists():p.error('diagnostic output directory must be new')
    output.mkdir(parents=True)
    baseline=subprocess.run(['git','rev-parse',a.baseline+'^{commit}'],cwd=ROOT,
        capture_output=True,text=True,check=True).stdout.strip()
    historical_probe=subprocess.run(['git','show',baseline+':tools/gameplay100_closure_probe.c'],
        cwd=ROOT,capture_output=True,text=True,check=True).stdout
    expected=re.findall(r'static const uint64_t expected_digest = 0x([0-9a-f]{16})ull;',historical_probe)
    if len(expected)!=1:raise ValueError('cannot identify historical checked-in closure golden')
    vswhere=Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
    vs=subprocess.run([str(vswhere),'-latest','-products','*','-requires',
        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath'],
        capture_output=True,text=True,check=True).stdout.strip()
    vcvars=Path(vs)/'VC/Auxiliary/Build/vcvars64.bat'
    report={'classification':'C-versus-C closure regression attribution; not ROM equivalence',
            'baseline':baseline,'historical_expected_digest':expected[0],'runs':{}}
    # Freeze both before compiling either. Shared repository builds/edits after
    # this point cannot alter the retained source snapshots.
    for label,ref in (('before',baseline),('after',None)):
        report['runs'][label]={'source_sha256':snapshot_sources(output/label/'source',ref)}
    for label,pack in (('before',a.before_pack),('after',a.pack)):
        directory=output/label;pack=Path(pack).resolve();pack_hash=sha(pack)
        exe=compile_probe(directory,vcvars,expected[0] if label=='before' else None)
        captures=directory/'captures';captures.mkdir()
        process=subprocess.run([str(exe),str(pack),'--diagnostics',str(captures)],
            capture_output=True,text=True,timeout=180)
        (directory/'stdout.txt').write_text(process.stdout)
        (directory/'stderr.txt').write_text(process.stderr)
        if sha(pack)!=pack_hash:raise ValueError('asset pack changed during closure')
        report['runs'][label].update(executable_sha256=sha(exe),
            pack={'path':str(pack),'sha256':pack_hash},exit_code=process.returncode,
            stdout=process.stdout,stderr=process.stderr,
            outputs={str(path.relative_to(captures)):sha(path)
                     for path in captures.iterdir() if path.is_file()})
        (output/'manifest.json').write_text(json.dumps(report,indent=2)+'\n')
        print(label,process.returncode,process.stdout.strip(),flush=True)
        if process.returncode not in (0,1) or 'CLOSURE_COMPONENTS' not in process.stdout:
            raise ValueError('closure diagnostic execution incomplete')
    print(output/'manifest.json')


if __name__=='__main__':main()
