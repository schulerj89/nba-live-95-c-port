"""Replay complete native controller routine prestate; compare raw exit words.

This is bounded ROM verification, not a naturally synchronized C journey.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct
import subprocess

ROM_SHA = '2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
MESEN_SHA = 'd2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b'


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def unique(pairs):
    out = {}
    for key, value in pairs:
        if key in out:
            raise ValueError('duplicate JSON key: '+key)
        out[key] = value
    return out


def read_json(path):
    return json.loads(path.read_text(encoding='utf-8-sig'), object_pairs_hook=unique)


def word(raw, address):
    return struct.unpack_from('<H', raw, address)[0]


def project(raw, mode, prestate):
    if len(raw) != 0x20000:
        raise ValueError('incomplete native WRAM')
    expected = dict(records=[word(raw, 0x47eb+p*0x40+w*2) for p in range(5) for w in range(32)],
                    assignments=[word(raw, 0x34eb+a*0x100+0x16) for a in range(10)],
                    previous=[word(raw, 0x1677+p*2) for p in range(5)],
                    counts=[word(raw, 0x4726+s*0x80) for s in range(2)],
                    cursors=[word(raw, 0x4728+s*0x80) for s in range(2)])
    if mode == 'input':
        actor = word(prestate, 0x96)
        expected['input_effects'] = [word(raw, actor+0x72), word(raw, 0x964), word(raw, 0x492d)]
    if mode == 'acquire':
        expected['previous_controller'] = [word(raw,0xa00)]
    return expected


def verify(capture, probe, rom, modes):
    manifest = read_json(capture/'manifest.json')
    if manifest.get('schema') != 2 or manifest.get('state_injection') is not False or manifest.get('rom_patch') is not False:
        raise ValueError('capture is not an attested natural controller case')
    if type(manifest.get('exit_code')) is not int or manifest['exit_code'] != 0:
        raise ValueError('native process did not exit successfully')
    if manifest['sources']['rom']['sha256'] != ROM_SHA or sha(rom) != ROM_SHA or manifest['sources']['mesen']['sha256'] != MESEN_SHA:
        raise ValueError('unexpected native source identity')
    for key, source in manifest['sources'].items():
        if sha(source['path']) != source['sha256']:
            raise ValueError('changed source: '+key)
    for name, entry in manifest['artifacts'].items():
        path = capture/name
        if Path(name).name != name or path.stat().st_size != entry['bytes'] or sha(path) != entry['sha256']:
            raise ValueError('changed native artifact: '+name)
    if not manifest['isolation'].get('post_settings_verified') or manifest['isolation']['initial_saves'] != []:
        raise ValueError('missing fresh/private capture attestation')
    import mesen_portable
    mesen_portable.verify(capture, manifest['isolation'])
    complete = (capture/'capture_complete.txt').read_text()
    if complete != manifest['completion'] or 'init_calls=1\n' not in complete:
        raise ValueError('missing native initializer completion')
    rows = [json.loads(line, object_pairs_hook=unique) for line in (capture/'ownership.jsonl').read_text().splitlines()]
    starts = [r for r in rows if r['tag'] == 'player_setup.entry']
    if len(starts) != 1 or starts[0]['selections'] != [2,1,1,1,1]:
        raise ValueError('unexpected natural Player Setup prestate')
    pending, counts, failures, words = {}, Counter(), [], 0
    frame_crossings = []
    pcs = dict(initialize=(0x86e208,0x86e24b),allocate=(0x86e24c,0x86e389),
               input=(0x85ef3a,0x85efec),transfer=(0x86bc9b,0x86bd1e),acquire=(0x86d25a,0x86d34a))
    for index, row in enumerate(rows, 1):
        tag = row['tag']
        if '.' not in tag:
            continue
        mode, boundary = tag.split('.',1)
        if mode not in modes or boundary not in ('entry','exit'):
            continue
        if row['native_pc'] != pcs[mode][boundary == 'exit']:
            raise ValueError('wrong native execution boundary')
        path = capture/f'leaf_{index:05d}_{tag}.wram'
        if path.name not in manifest['artifacts']:
            raise ValueError('missing full raw routine boundary: '+path.name)
        if boundary == 'entry':
            if mode in pending:
                raise ValueError('nested unexpected native controller call')
            pending[mode] = (row, path)
            continue
        before, entry_path = pending.pop(mode)
        if before['court_frame'] != row['court_frame']:
            frame_crossings.append(dict(mode=mode,index=index,
                                        entry=before['court_frame'],exit=row['court_frame']))
        pre = entry_path.read_bytes()
        expected = project(path.read_bytes(),mode,pre)
        run = subprocess.run([str(probe),mode,str(entry_path),str(rom)],
                             text=True,capture_output=True,timeout=15)
        if run.returncode:
            raise ValueError(f'C probe failed {run.returncode}: {run.stderr}')
        lines = [v for v in run.stdout.splitlines() if v.startswith('{')]
        if len(lines) != 1:
            raise ValueError('expected one C JSON response')
        actual = json.loads(lines[0],object_pairs_hook=unique)
        if actual.keys() != expected.keys():
            raise ValueError('C projection changed fields')
        for key, values in expected.items():
            if not isinstance(actual[key],list) or len(actual[key]) != len(values):
                raise ValueError('incomplete C field: '+key)
            for slot,(got,want) in enumerate(zip(actual[key],values)):
                words += 1
                if type(got) is not int or got != want:
                    failures.append(dict(mode=mode,index=index,field=key,slot=slot,expected=want,actual=got))
        counts[mode] += 1
    if pending:
        raise ValueError('unterminated native leaf')
    for mode in modes:
        if mode in ('initialize','allocate') and counts[mode] != 1:
            raise ValueError('expected one native initialization/allocation')
    if manifest['selection'] != 1 and 'input' in modes and not counts['input']:
        raise ValueError('missing human input publication')
    return dict(kind='bounded controller routine native prestate replay',capture=str(capture),
                manifest_sha256=sha(capture/'manifest.json'),probe_sha256=sha(probe),
                rom_sha256=sha(rom),calls=dict(counts),compared_words=words,
                frame_crossings=frame_crossings,failures=failures,passed=not failures)


def main():
    p=argparse.ArgumentParser()
    p.add_argument('--capture',type=Path,required=True)
    p.add_argument('--probe',type=Path,required=True)
    p.add_argument('--rom',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--modes',nargs='+',choices=['initialize','allocate','input','transfer','acquire'],
                   default=['initialize','allocate','input','transfer','acquire'])
    a=p.parse_args()
    result=verify(a.capture.resolve(),a.probe.resolve(),a.rom.resolve(),a.modes)
    if a.output.exists():
        raise ValueError('keep prior verification output immutable')
    a.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k != 'failures'}))
    if result['failures']:
        print(json.dumps(result['failures'][:10]));raise SystemExit(1)


if __name__ == '__main__': main()
