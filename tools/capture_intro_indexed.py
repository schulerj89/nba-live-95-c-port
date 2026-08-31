"""Capture 1,500 untouched native intro frames/resources in a private Mesen home.

The output directory must be new. The tracked, byte-pinned Lua never supplies
input, changes machine memory or loads a state. RGB is verification evidence;
the production builders consume only the attested raw resource snapshots.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess

from intro_capture_resources import (IntroResources, ROM_SHA256, MESEN_SHA256,
                                     SCRIPT_SHA256, SCOPE)
from mesen_portable import prepare, verify, sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', required=True, type=Path)
    parser.add_argument('--mesen', type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    rom = args.rom.resolve()
    installed = args.mesen or shutil.which('Mesen.exe')
    if not installed:
        raise ValueError('Mesen.exe not found; pass --mesen')
    installed = Path(installed).resolve()
    source = Path(__file__).with_name('mesen_intro_indexed_capture.lua')
    for path, expected, label in ((rom, ROM_SHA256, 'original ROM'),
                                 (installed, MESEN_SHA256, 'Mesen executable'),
                                 (source, SCRIPT_SHA256, 'reviewed Lua script')):
        if sha(path) != expected:
            raise ValueError('wrong identity: ' + label)
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    mesen, isolated = prepare(out, installed)
    script = out / 'capture.lua'
    shutil.copyfile(source, script)
    manifest = dict(scope=SCOPE, rom_sha256=sha(rom), mesen_sha256=sha(mesen),
                    script_sha256=sha(script), isolation=isolated,
                    runner_sha256=sha(__file__), accepted_capture=False)
    # Per-child environment: concurrent captures must not mutate shared
    # process-global NBA95_* variables and accidentally exchange destinations.
    env = {k: v for k, v in os.environ.items() if not k.startswith('NBA95_')}
    env.update(NBA95_CAPTURE_DIR=str(out), NBA95_INTRO_LIMIT='1500')
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0
    try:
        with (out / 'mesen.log').open('w') as log:
            result = subprocess.run([str(mesen), '--testrunner', '--timeout=180',
                str(rom), str(script)], env=env, stdout=log,
                stderr=subprocess.STDOUT, startupinfo=startup, timeout=200)
        manifest['exit_code'] = result.returncode
        if result.returncode:
            raise RuntimeError('Mesen capture failed; see the private mesen.log')
        verify(out, isolated)
        if (out / 'complete.txt').read_bytes() != b'1500\n':
            raise ValueError('incomplete native intro capture')
        manifest['artifacts'] = {p.name: dict(size=p.stat().st_size, sha256=sha(p))
                                for p in out.iterdir() if p.is_file()}
        manifest['accepted_capture'] = True
    finally:
        (out / 'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
    # Apply the same independent provenance checks used by asset extraction.
    IntroResources(out)
    print('PASS: 1500 synchronous native cold-boot frames, no inputs or state writes')


if __name__ == '__main__':
    main()
