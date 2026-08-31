"""Natural read-only D0AD/event writer capture in a separate portable Mesen."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
from mesen_portable import prepare, verify, sha
from capture_gameplay_hud import ROM_SHA, MESEN_SHA


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--rom', required=True, type=Path)
    parser.add_argument('--mesen', type=Path)
    args = parser.parse_args()
    rom = args.rom.resolve()
    if sha(rom) != ROM_SHA:
        raise ValueError('wrong original ROM')
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    installed = args.mesen or shutil.which('Mesen.exe')
    if not installed:
        raise ValueError('Mesen not found')
    mesen, isolation = prepare(out, Path(installed))
    if sha(mesen) != MESEN_SHA:
        raise ValueError('wrong Mesen executable')
    script = out / 'capture.lua'
    shutil.copyfile(Path(__file__).with_name('mesen_hud_event_owner.lua'), script)
    shutil.copyfile(__file__, out / 'capture_runner.py')
    shutil.copyfile(Path(__file__).with_name('mesen_portable.py'), out / 'mesen_portable.py')
    manifest = dict(schema=1, kind='natural read-only HUD shared event writer', state_injection=False,
                    rom_patch=False, selection=1, rom_sha256=ROM_SHA, mesen_sha256=MESEN_SHA,
                    script_sha256=sha(script), isolation=isolation, accepted_capture=False)
    env = {k: v for k, v in os.environ.items() if not k.startswith('NBA95_')}
    env['NBA95_CAPTURE_DIR'] = str(out)
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0
    try:
        with (out / 'mesen.log').open('w') as log:
            result = subprocess.run([str(mesen), '--testrunner', '--timeout=600', str(rom), str(script)],
                env=env, stdout=log, stderr=subprocess.STDOUT, startupinfo=startup, timeout=620)
        manifest['exit_code'] = result.returncode
        if result.returncode:
            raise RuntimeError('native writer capture failed')
        verify(out, isolation)
        if (out / 'capture_complete.txt').read_bytes() != b'natural D0AD shared-writer capture complete\n':
            raise ValueError('native writer capture incomplete')
        manifest['artifacts'] = {p.name: dict(size=p.stat().st_size, sha256=sha(p))
                                for p in out.iterdir() if p.is_file() and p.name != 'manifest.json'}
        manifest['accepted_capture'] = True
    finally:
        (out / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print('PASS: natural HUD shared-writer capture, no state injection', out)


if __name__ == '__main__':
    main()
