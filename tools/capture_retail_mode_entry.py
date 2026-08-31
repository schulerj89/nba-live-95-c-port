"""Capture original mode entry using only normal controller input.

This records a bounded first entry, not a complete Season/Playoffs/save journey.
All emulator configuration, saves and observations live in a new private folder.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess

import mesen_portable


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--mode', type=int, choices=range(4), required=True)
    parser.add_argument('--rom', type=Path, required=True)
    parser.add_argument('--mesen', type=Path, required=True)
    args = parser.parse_args()
    rom, installed = args.rom.resolve(), args.mesen.resolve()
    if mesen_portable.sha(rom) != '2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870':
        raise ValueError('Unexpected original ROM')
    if mesen_portable.sha(installed) != 'd2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b':
        raise ValueError('Unexpected Mesen executable')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    executable, isolation = mesen_portable.prepare(output, installed)
    sources = {'rom': rom, 'mesen': executable}
    for name in ('capture_retail_mode_entry.py', 'mesen_retail_mode_entry.lua', 'mesen_portable.py'):
        target = output / name
        shutil.copyfile(Path(__file__).with_name(name), target)
        sources[name] = target
    env = {key: value for key, value in os.environ.items() if not key.startswith('NBA95_')}
    env.update(NBA95_CAPTURE_DIR=output.as_posix(), NBA95_RETAIL_MODE=str(args.mode))
    command = [str(executable), '--testrunner', '--timeout=150', str(rom),
               str(sources['mesen_retail_mode_entry.lua'])]
    manifest = {'schema': 1, 'kind': 'normal controller-only original first mode entry',
                'requested_mode': args.mode, 'state_injection': False, 'rom_patch': False,
                'limits': ['first mode entry only', 'no completed match or save/reload claim',
                           'factory isolated save state', 'fixed normal menu input schedule'],
                'arguments': command, 'environment': {key: value for key, value in env.items()
                                                      if key.startswith('NBA95_')},
                'sources': {key: {'path': str(path), 'sha256': mesen_portable.sha(path)}
                            for key, path in sources.items()}, 'isolation': isolation}

    def save():
        (output / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')

    save()
    with (output / 'stdout.log').open('wb') as stdout, (output / 'stderr.log').open('wb') as stderr:
        try:
            result = subprocess.run(command, cwd=executable.parent, env=env, stdout=stdout,
                                    stderr=stderr, timeout=180, creationflags=subprocess.CREATE_NO_WINDOW)
            manifest['exit_code'] = result.returncode
        except subprocess.TimeoutExpired:
            manifest['timed_out'] = True
            save()
            raise
    save()
    if result.returncode or not (output / 'capture_complete.json').is_file():
        raise RuntimeError('Incomplete original mode entry; retained at ' + str(output))
    complete = json.loads((output / 'capture_complete.json').read_text())
    if complete['requested_mode'] != args.mode or complete['observed_mode'] != args.mode:
        raise ValueError('Normal menu input did not select the requested mode')
    if complete['route_events'] < 1:
        raise ValueError('No original mode entry was observed')
    manifest['isolation'] = mesen_portable.verify(output, isolation)
    for name, identity in manifest['sources'].items():
        if mesen_portable.sha(identity['path']) != identity['sha256']:
            raise ValueError('Capture source changed: ' + name)
    manifest['completion'] = complete
    manifest['artifacts'] = {path.name: {'bytes': path.stat().st_size, 'sha256': mesen_portable.sha(path)}
                             for path in output.iterdir() if path.is_file() and path.name != 'manifest.json'}
    save()
    print(json.dumps(complete))


if __name__ == '__main__':
    main()
