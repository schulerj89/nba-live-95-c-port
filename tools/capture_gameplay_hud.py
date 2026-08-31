"""Capture a natural CPU match's first HUD publisher with isolated Mesen."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess

from mesen_portable import prepare, verify, sha

ROM_SHA = '2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
MESEN_SHA = 'd2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--rom', required=True, type=Path)
    parser.add_argument('--mesen', type=Path)
    parser.add_argument('--alternate-teams', action='store_true')
    args = parser.parse_args()
    rom = args.rom.resolve()
    if sha(rom) != ROM_SHA:
        raise ValueError('unexpected original ROM identity')
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    mesen, isolation = prepare(out, args.mesen or Path(shutil.which('Mesen.exe')))
    if sha(mesen) != MESEN_SHA:
        raise ValueError('unexpected Mesen identity')
    script = out / 'capture.lua'
    shutil.copyfile(Path(__file__).with_name('mesen_gameplay_hud.lua'), script)
    shutil.copyfile(__file__, out / 'capture_runner.py')
    shutil.copyfile(Path(__file__).with_name('mesen_portable.py'), out / 'mesen_portable.py')
    manifest = dict(schema=2, kind='natural neutral-controller CPU match first HUD publisher',
                    state_injection=False, rom_patch=False, selection=1,
                    alternate_teams=args.alternate_teams, rom_sha256=ROM_SHA,
                    mesen_sha256=MESEN_SHA, script_sha256=sha(script), isolation=isolation,
                    input_schedule='Title/setup/team Start; PlayerSetup Left400 then Start700. All gameplay controllers neutral. AlternateTeams uses Setup650Right,700L,750Right,850Start.',
                    accepted_capture=False)
    env = {key: value for key, value in os.environ.items() if not key.startswith('NBA95_')}
    env.update(NBA95_CAPTURE_DIR=str(out), NBA95_CONTROL_TEAM_VARIANT='1' if args.alternate_teams else '0')
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0
    try:
        with (out / 'mesen.log').open('w') as log:
            result = subprocess.run([str(mesen), '--testrunner', '--timeout=600', str(rom), str(script)],
                                    env=env, stdout=log, stderr=subprocess.STDOUT,
                                    startupinfo=startup, timeout=620)
        manifest['exit_code'] = result.returncode
        if result.returncode:
            raise RuntimeError('Mesen capture failed')
        verify(out, isolation)
        summary = (out / 'capture_complete.txt').read_text()
        if not summary.startswith('first natural HUD publication observed; court='):
            raise ValueError('natural HUD publisher was not reached')
        manifest['summary'] = summary
        manifest['artifacts'] = {path.name: dict(size=path.stat().st_size, sha256=sha(path))
                                 for path in out.iterdir() if path.is_file() and path.name != 'manifest.json'}
        manifest['accepted_capture'] = True
    finally:
        (out / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print('PASS: natural first HUD publisher, no gameplay input/state injection:', out)


if __name__ == '__main__':
    main()
