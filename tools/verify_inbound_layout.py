"""Replay captured C37D target outputs; never use capture timing in C."""
import argparse
import copy
import json
from pathlib import Path
import subprocess
import mesen_portable

SOURCE = {
    'rom': '2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870',
    'mesen': 'd2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b',
    'capture': 'fdff75e3f5516f9ee99a01196a3564b8f6c8bb8c5eb055c89ee504c119d7c564',
    'runner': '2451b827a536d477e784d7ae99f6717377fc4b67c188e61f96bbe61c5406ad3b',
    'isolation_helper': '1bc6db2d68d836c7c6af180137a3d5e8e4ea454d7cb8a97e9e95cc6312ddc3bb',
}
CASES = [None, (1,404,-45,336,404), (1,-404,45,-336,-404),
         (1,404,-300,336,404), (1,-404,300,-336,-404),
         (1,404,-45,-336,404), (4,404,-45,336,404),
         (4,404,-45,-336,404), (-1,404,-45,336,404)]
ADDRESSES = [0x956,0x9b0,0x9b2,0x952,0x4775,0x3eef,0x7f6,0x994]


def require(ok, reason):
    if not ok:
        raise ValueError(reason)


def read(path):
    def pairs(items):
        result = {}
        for key, value in items:
            require(key not in result, 'duplicate JSON key')
            result[key] = value
        return result
    return json.loads(Path(path).read_text(), object_pairs_hook=pairs,
                      parse_constant=lambda _: (_ for _ in ()).throw(ValueError('nonfinite JSON')))


def word(raw, address):
    return int.from_bytes(raw[address:address+2], 'little')


def verify(capture, probe):
    capture = Path(capture).resolve()
    m = read(capture / 'manifest.json')
    case = m['case']
    require(type(case) is int and 0 <= case < len(CASES), 'case domain')
    require(m['schema'] == 1 and type(m['schema']) is int and m['kind'] == 'native inbound layout boundary', 'schema')
    require(m['controlled'] is (case != 0) and m['cpu_writes'] is False and m['rom_patch'] is False, 'control declaration')
    require(m['controlled_words'] == (ADDRESSES if case else []), 'controlled words')
    require(type(m['exit_code']) is int and m['exit_code'] == 0, 'process exit')
    require(m['completion'] == f'case={case}\n' == (capture / 'capture_complete.txt').read_text(), 'completion')
    require(set(m['sources']) == set(SOURCE), 'source set')
    for key, expected in SOURCE.items():
        item = m['sources'][key]
        require(set(item) == {'path','sha256'} and item['sha256'] == expected, 'source metadata '+key)
        require(mesen_portable.sha(item['path']) == expected, 'source bytes '+key)
    require(m['arguments'] == [str(capture / 'portable-mesen' / 'Mesen.exe'), '--testrunner', '--timeout=240', m['sources']['rom']['path'], str(capture / 'capture.lua')], 'command')
    require(m['environment'] == {'NBA95_CAPTURE_DIR': capture.as_posix(), 'NBA95_LAYOUT_CASE': str(case)}, 'environment')
    required = {'before.bin','entry.bin','exit.bin','before.json','entry.json','exit.json','pcs.json',
                'capture_complete.txt','observed-script-data-folder.txt','initial-mesen-settings.json',
                'capture.lua','capture_inbound_layout.py','mesen_portable.py','stdout.log','stderr.log','progress.json'}
    require(set(m['artifacts']) == required, 'artifact set')
    for name, item in m['artifacts'].items():
        require(set(item) == {'bytes','sha256'} and type(item['bytes']) is int, 'artifact shape')
        path = capture / name
        require(path.stat().st_size == item['bytes'] and mesen_portable.sha(path) == item['sha256'], 'artifact identity '+name)
    iso = m['isolation']
    settings = {
        'Debug': {'ScriptWindow': {'AllowIoOsAccess': True, 'ScriptTimeout': 60, 'SaveScriptBeforeRun': False}},
        'Preferences': {'SingleInstance': False, 'PauseWhenInBackground': False, 'AutoLoadPatches': False,
                        'OverrideSaveDataFolder': True, 'SaveDataFolder': str(capture/'isolated-saves')},
        'Snes': {'Port1': {'Type': 'SnesController'}, 'Port2': {'Type': 'None'}, 'DisableFrameSkipping': True,
                 'EnableRandomPowerOnState': False, 'RamPowerOnState': 'AllZeros', 'ForceFixedResolution': False,
                 'Overscan': {'Top': 7, 'Bottom': 8, 'Left': 0, 'Right': 0}},
        'Video': {'VideoFilter': 'None', 'AspectRatio': 'NoStretching', 'Brightness': 0, 'Contrast': 0,
                  'Hue': 0, 'Saturation': 0, 'ScanlineIntensity': 0, 'UseBilinearInterpolation': False, 'ScreenRotation': 'None'},
    }
    canonical = lambda value: json.dumps(value, sort_keys=True)
    require(canonical(iso['settings']) == canonical(settings) == canonical(read(capture/'initial-mesen-settings.json')), 'settings recipe')
    require(iso['home'] == str(capture / 'portable-mesen') and iso['save_folder'] == str(capture / 'isolated-saves') and iso['initial_saves'] == [] and iso['post_settings_verified'] is True, 'isolation')
    require(mesen_portable.verify(capture, copy.deepcopy(iso)) == iso, 'isolation recheck')
    raw = {tag:(capture / (tag+'.bin')).read_bytes() for tag in ('before','entry','exit')}
    require(all(len(v) == 0x4b00 for v in raw.values()), 'raw size')
    expected_entry = bytearray(raw['before'])
    if case:
        layout,x,y,anchor,ball = CASES[case]
        for address, value in zip(ADDRESSES, (layout,x,y,5,anchor,ball,0x9146,0)):
            expected_entry[address:address+2] = (value & 65535).to_bytes(2,'little')
    require(raw['entry'] == expected_entry, 'only declared WRAM injection')
    states = {tag:read(capture / (tag+'.json')) for tag in raw}
    for tag, s in states.items():
        require(set(s) == {'frame','court','cpu'} and type(s['frame']) is int and type(s['court']) is int, 'boundary shape')
        require(0 <= s['court'] < 13610 and s['frame'] == s['court']+4390 and s['frame'] < 18000, 'route clocks')
        require(set(s['cpu']) == {'a','x','y','ps','sp','d','dbr','k','pc'}, 'CPU shape')
        require(all(type(v) is int and 0 <= v <= (255 if k in ('ps','dbr','k') else 65535) for k,v in s['cpu'].items()), 'CPU domains')
        require(s['cpu']['k'] == 0x85 and s['cpu']['d'] == 0 and s['cpu']['ps'] & 0x30 == 0, 'CPU mode')
        require(s['cpu']['pc'] == (0xc5c0 if tag == 'exit' else 0xc37d), 'boundary PC')
    require(states['before'] == states['entry'], 'no CPU/clock injection')
    # These anchors attest this immutable capture route, never production.
    require(states['entry']['frame'] == 5374 and states['entry']['court'] == 984, 'first natural call anchor')
    require(states['exit']['frame'] >= states['entry']['frame'], 'boundary order')
    pcs = read(capture / 'pcs.json')
    require(isinstance(pcs,list) and 2 <= len(pcs) <= 1000 and pcs[0] == 0x85c37d and pcs[-1] == 0x85c5c0, 'PC scope')
    require(all(type(pc) is int and 0x85c37d <= pc <= 0x85c65b for pc in pcs), 'PC domain')
    if case:
        destination = 0x85c450 if CASES[case][0] == 4 else 0x85c50b
        require(destination in pcs, 'original dispatcher branch')
    entry, end = raw['entry'], raw['exit']
    side = word(entry,0x952)
    require(side in (0,5), 'side')
    args = [word(entry,a) for a in (0x956,0x9b0,0x9b2,0x46f5 if side == 0 else 0x4775,0x3eef,0x7f6)]
    expected = [word(end,a) for a in (0x958,0x95a,0x95c,0x996)]
    expected += [int(word(end,0x994) != word(entry,0x994)),word(end,0x7f6)]
    run = subprocess.run([str(Path(probe).resolve())], input=' '.join(f'{v:04x}' for v in args)+'\n', text=True, capture_output=True, timeout=30)
    require(run.returncode == 0 and len(run.stdout.splitlines()) == 1, 'C response count')
    fields = run.stdout.split()
    require(len(fields) == 6 and all(len(v) == 4 and all(c in '0123456789abcdef' for c in v) for v in fields), 'C response words')
    actual = [int(v,16) for v in fields]
    return dict(case=case, passed=actual == expected, inputs=args, expected=expected, actual=actual,
                controlled=case != 0, entry_frame=states['entry']['frame'], native_instructions=len(pcs),
                manifest_sha256=mesen_portable.sha(capture/'manifest.json'), probe_sha256=mesen_portable.sha(probe))


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--capture', type=Path, action='append', required=True)
    p.add_argument('--probe', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    require(not a.output.exists(), 'output must be new')
    reports = [verify(c,a.probe) for c in a.capture]
    result = dict(passed=all(r['passed'] for r in reports), cases=reports,
                  scope='six target/play/RNG words at genuine original C37D calls; controlled inputs labeled; no whole-game native trajectory proof')
    a.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result))
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
