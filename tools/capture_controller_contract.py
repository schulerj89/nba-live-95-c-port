"""Natural controller journey with a per-process environment and immutable inputs."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess

import mesen_portable


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--selection', type=int, choices=range(3), required=True)
    p.add_argument('--court-frames', type=int, default=400)
    p.add_argument('--live-pass', action='store_true')
    p.add_argument('--rom', type=Path, required=True)
    p.add_argument('--mesen', type=Path, required=True)
    a = p.parse_args()
    if not 400 <= a.court_frames <= 2000:
        p.error('court frames must be 400..2000')
    out, rom = a.output.resolve(), a.rom.resolve()
    if mesen_portable.sha(rom) != '2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870':
        raise ValueError('Unexpected original ROM')
    if mesen_portable.sha(a.mesen) != 'd2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b':
        raise ValueError('Unexpected Mesen executable')
    out.mkdir(parents=True, exist_ok=False)
    mesen, isolation = mesen_portable.prepare(out, a.mesen)
    script = out / 'capture.lua'
    shutil.copyfile(Path(__file__).with_name('mesen_controller_ownership.lua'), script)
    shutil.copyfile(__file__, out / Path(__file__).name)
    shutil.copyfile(mesen_portable.__file__, out / 'mesen_portable.py')
    environment = os.environ.copy()
    environment.update(NBA95_CAPTURE_DIR=out.as_posix(),
                       NBA95_CONTROL_SELECTION=str(a.selection),
                       NBA95_CONTROL_TEAM_VARIANT='0', NBA95_CONTROL_PAUSE_AT='-1',
                       NBA95_CONTROL_COURT_FRAMES=str(a.court_frames),
                       NBA95_CONTROL_LIVE_PASS='1' if a.live_pass else '0')
    command = [str(mesen), '--testrunner', '--timeout=180', str(rom), str(script)]
    manifest = dict(schema=2, kind='natural controller-only Player Setup to gameplay journey',
                    selection=a.selection, court_frames=a.court_frames,live_pass=a.live_pass,
                    state_injection=False, rom_patch=False,
                    isolation=isolation, arguments=command,
                    environment={k: v for k, v in environment.items() if k.startswith('NBA95_CONTROL') or k == 'NBA95_CAPTURE_DIR'},
                    sources={k: dict(path=str(v), sha256=mesen_portable.sha(v)) for k, v in
                             dict(rom=rom, mesen=mesen, capture=script,
                                  runner=out / Path(__file__).name,
                                  isolation_helper=out / 'mesen_portable.py').items()})
    def save():
        (out / 'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
    save()
    with (out / 'stdout.log').open('wb') as stdout, (out / 'stderr.log').open('wb') as stderr:
        result = subprocess.run(command, cwd=mesen.parent, env=environment,
                                stdout=stdout, stderr=stderr, timeout=210,
                                creationflags=subprocess.CREATE_NO_WINDOW)
    manifest['exit_code'] = result.returncode
    save()
    if result.returncode != 0 or not (out / 'capture_complete.txt').is_file():
        raise RuntimeError('Incomplete native capture; artifacts retained at '+str(out))
    manifest['isolation'] = mesen_portable.verify(out, isolation)
    manifest['artifacts'] = {v.name: dict(bytes=v.stat().st_size, sha256=mesen_portable.sha(v))
                             for v in out.iterdir() if v.is_file() and v.name != 'manifest.json'}
    manifest['completion'] = (out / 'capture_complete.txt').read_text()
    save()
    print(manifest['completion'].strip())


if __name__ == '__main__':
    main()
