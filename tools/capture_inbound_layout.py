"""Capture one natural or controlled original-ROM inbound layout boundary."""
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
    p.add_argument('--case', type=int, choices=range(9), required=True)
    p.add_argument('--rom', type=Path, required=True)
    p.add_argument('--mesen', type=Path, required=True)
    a = p.parse_args()
    assert mesen_portable.sha(a.rom) == '2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
    assert mesen_portable.sha(a.mesen) == 'd2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b'
    out = a.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    exe, isolation = mesen_portable.prepare(out, a.mesen)
    script = out / 'capture.lua'
    shutil.copyfile(Path(__file__).with_name('mesen_inbound_layout.lua'), script)
    shutil.copyfile(__file__, out / Path(__file__).name)
    shutil.copyfile(mesen_portable.__file__, out / 'mesen_portable.py')
    env = {k: v for k, v in os.environ.items() if not k.startswith('NBA95_')}
    additions = dict(NBA95_CAPTURE_DIR=out.as_posix(), NBA95_LAYOUT_CASE=str(a.case))
    env.update(additions)
    args = [str(exe), '--testrunner', '--timeout=240', str(a.rom.resolve()), str(script)]
    paths = dict(rom=a.rom.resolve(), mesen=exe, capture=script,
                 runner=out / Path(__file__).name, isolation_helper=out / 'mesen_portable.py')
    m = dict(schema=1, kind='native inbound layout boundary', case=a.case,
             controlled=a.case != 0, cpu_writes=False, rom_patch=False,
             controlled_words=[] if a.case == 0 else [0x956, 0x9b0, 0x9b2, 0x952, 0x4775, 0x3eef, 0x7f6, 0x994],
             arguments=args, environment=additions, isolation=isolation,
             sources={k: dict(path=str(v), sha256=mesen_portable.sha(v)) for k, v in paths.items()})
    def save():
        (out / 'manifest.json').write_text(json.dumps(m, indent=2) + '\n')
    save()
    with (out / 'stdout.log').open('wb') as stdout, (out / 'stderr.log').open('wb') as stderr:
        r = subprocess.run(args, cwd=exe.parent, env=env, stdout=stdout, stderr=stderr,
                           timeout=270, creationflags=subprocess.CREATE_NO_WINDOW)
    m['exit_code'] = r.returncode
    save()
    if r.returncode or not (out / 'capture_complete.txt').is_file():
        raise RuntimeError('capture failed; preserve ' + str(out))
    m['isolation'] = mesen_portable.verify(out, isolation)
    m['completion'] = (out / 'capture_complete.txt').read_text()
    m['artifacts'] = {v.name: dict(bytes=v.stat().st_size, sha256=mesen_portable.sha(v))
                      for v in out.iterdir() if v.is_file() and v.name != 'manifest.json'}
    save()
    print(m['completion'], flush=True)


if __name__ == '__main__':
    main()
