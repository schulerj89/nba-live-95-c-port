"""Replay complete native controller routine prestate; compare raw exit words.

This is bounded ROM verification, not a naturally synchronized C journey.
"""
import argparse
from collections import Counter
import copy
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import re

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
    return json.loads(path.read_text(encoding='utf-8-sig'), object_pairs_hook=unique,
                      parse_constant=lambda value: fail('nonfinite JSON number'))


def fail(message):
    raise ValueError(message)


def require(condition, message):
    if not condition:
        fail(message)


def exact(actual, expected):
    """JSON equality with integer/boolean/float types kept distinct."""
    if type(actual) is not type(expected):
        return False
    if isinstance(expected, dict):
        return actual.keys() == expected.keys() and all(exact(actual[k], v) for k, v in expected.items())
    if isinstance(expected, list):
        return len(actual) == len(expected) and all(exact(a, b) for a, b in zip(actual, expected))
    return actual == expected


def required_subset(actual, expected):
    require(isinstance(actual, dict), 'invalid settings group')
    for key, value in expected.items():
        require(key in actual, 'missing required setting: '+key)
        if isinstance(value, dict):
            required_subset(actual[key], value)
        else:
            require(exact(actual[key], value), 'wrong required setting: '+key)


def verify_isolation(capture, isolation):
    require(isolation.get('method') == 'private portable executable/settings', 'wrong isolation method')
    require(isolation.get('post_settings_verified') is True and
            exact(isolation.get('initial_saves'), []), 'missing fresh/private capture attestation')
    require(Path(isolation['home']).resolve() == capture/'portable-mesen' and
            Path(isolation['save_folder']).resolve() == capture/'isolated-saves', 'wrong private capture paths')
    initial = read_json(capture/'initial-mesen-settings.json')
    require(exact(initial, isolation.get('settings')), 'declared initial settings differ from actual file')
    required_subset(initial, {
        'Preferences': {'SingleInstance': False, 'PauseWhenInBackground': False,
            'AutoLoadPatches': False, 'OverrideSaveDataFolder': True,
            'SaveDataFolder': str(capture/'isolated-saves')},
        'Snes': {'Port1': {'Type': 'SnesController'}, 'Port2': {'Type': 'None'},
            'DisableFrameSkipping': True, 'EnableRandomPowerOnState': False,
            'RamPowerOnState': 'AllZeros', 'ForceFixedResolution': False,
            'Overscan': {'Top': 7, 'Bottom': 8, 'Left': 0, 'Right': 0}},
        'Video': {'VideoFilter': 'None', 'AspectRatio': 'NoStretching', 'Brightness': 0,
            'Contrast': 0, 'Hue': 0, 'Saturation': 0, 'ScanlineIntensity': 0,
            'UseBilinearInterpolation': False, 'ScreenRotation': 'None'},
    })
    require(sha(capture/'portable-mesen/settings.json') == isolation.get('post_settings_sha256'),
            'persisted settings identity differs')
    # The capture helper updates its argument with observations. Verify a copy
    # and compare it; never let that update repair an invalid manifest for us.
    import mesen_portable
    observed = mesen_portable.verify(capture, copy.deepcopy(isolation))
    for key in ('observed_script_data_folder', 'post_settings_verified', 'post_settings_sha256', 'final_saves'):
        require(exact(observed[key], isolation.get(key)), 'changed isolation attestation: '+key)


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
    capture, probe, rom = Path(capture).resolve(), Path(probe).resolve(), Path(rom).resolve()
    known_modes = ('initialize','allocate','input','transfer','acquire')
    require(isinstance(modes, (list, tuple)) and modes and len(set(modes)) == len(modes) and
            all(mode in known_modes for mode in modes), 'invalid or empty requested modes')
    manifest = read_json(capture/'manifest.json')
    if not exact(manifest.get('schema'), 2) or manifest.get('state_injection') is not False or manifest.get('rom_patch') is not False:
        raise ValueError('capture is not an attested natural controller case')
    require(type(manifest.get('selection')) is int and 0 <= manifest['selection'] <= 2,
            'invalid controller selection')
    # The first immutable runner predates configurable court_frames. Its
    # executed Lua line164 has the literal court==400 completion condition.
    # Accept that documented format only for this exact source pair, never
    # silently supply a default to newer or unknown capture manifests.
    legacy_fixed400 = 'court_frames' not in manifest and (
        manifest.get('sources', {}).get('capture', {}).get('sha256'),
        manifest.get('sources', {}).get('runner', {}).get('sha256')) == (
        'c284808364d27b49f73caba5eecd8621c8a083b6b0e9d24f326ffaf51faf64e9',
        '45d2ac97d7f85af3410db78bc4874f3334468c7f783dfddb78777e77ff57117c')
    court_frames = 400 if legacy_fixed400 else manifest.get('court_frames')
    require(type(court_frames) is int and 400 <= court_frames <= 2000, 'invalid court frame count')
    if type(manifest.get('exit_code')) is not int or manifest['exit_code'] != 0:
        raise ValueError('native process did not exit successfully')
    required_sources = {'rom': rom, 'mesen': capture/'portable-mesen/Mesen.exe',
        'capture': capture/'capture.lua', 'runner': capture/'capture_controller_contract.py',
        'isolation_helper': capture/'mesen_portable.py'}
    require(isinstance(manifest.get('sources'), dict) and required_sources.keys() == manifest['sources'].keys(),
            'changed required source identities')
    for key, path in required_sources.items():
        require(Path(manifest['sources'][key]['path']).resolve() == path, 'wrong source path: '+key)
    if manifest['sources']['rom']['sha256'] != ROM_SHA or sha(rom) != ROM_SHA or manifest['sources']['mesen']['sha256'] != MESEN_SHA:
        raise ValueError('unexpected native source identity')
    for key, source in manifest['sources'].items():
        if sha(source['path']) != source['sha256']:
            raise ValueError('changed source: '+key)
    required_artifacts = {'ownership.jsonl', 'capture_complete.txt', 'capture.lua',
        'capture_controller_contract.py', 'mesen_portable.py', 'initial-mesen-settings.json',
        'observed-script-data-folder.txt', 'first_court.wram'}
    require(isinstance(manifest.get('artifacts'), dict) and required_artifacts <= manifest['artifacts'].keys(),
            'missing required capture artifact')
    for name, entry in manifest['artifacts'].items():
        path = capture/name
        if Path(name).name != name or type(entry.get('bytes')) is not int or entry['bytes'] < 0 or \
                path.stat().st_size != entry['bytes'] or sha(path) != entry['sha256']:
            raise ValueError('changed native artifact: '+name)
    verify_isolation(capture, manifest['isolation'])
    environment = manifest['environment']
    # These immutable runner revisions have different attestation fields.
    # Reject invented routes and unknown revisions instead of interpreting
    # absent fields as permission to choose a different native journey.
    runner = manifest['sources']['runner']['sha256']
    pre_live_runners = {
        '45d2ac97d7f85af3410db78bc4874f3334468c7f783dfddb78777e77ff57117c',
        '4751b8199605fe9ad3fcf92c55cf58c85aed9616b20ec7bc689f1ad24cfbb18c',
    }
    expected_environment_keys = {'NBA95_CAPTURE_DIR', 'NBA95_CONTROL_SELECTION',
        'NBA95_CONTROL_TEAM_VARIANT', 'NBA95_CONTROL_PAUSE_AT'}
    if not legacy_fixed400:
        expected_environment_keys.add('NBA95_CONTROL_COURT_FRAMES')
    if runner in pre_live_runners:
        require('live_pass' not in manifest, 'runner does not attest live-pass mode')
    else:
        require(runner == 'f8a6a11971d090e90e24f0ed84e68178749c898edba109e8def1e41e5a5fb21d',
                'unknown controller capture runner contract')
        require(type(manifest.get('live_pass')) is bool, 'invalid live-pass mode')
        expected_environment_keys.add('NBA95_CONTROL_LIVE_PASS')
        require(environment.get('NBA95_CONTROL_LIVE_PASS') == ('1' if manifest['live_pass'] else '0'),
                'live-pass environment differs')
    require(isinstance(environment, dict) and environment.keys() == expected_environment_keys and
            environment.get('NBA95_CONTROL_TEAM_VARIANT') == '0' and
            environment.get('NBA95_CONTROL_PAUSE_AT') == '-1', 'changed controller-only capture route')
    require(exact(manifest.get('arguments'), [str(capture/'portable-mesen/Mesen.exe'),
        '--testrunner', '--timeout=180', str(rom), str(capture/'capture.lua')]),
        'executed native command differs')
    require(environment.get('NBA95_CONTROL_SELECTION') == str(manifest['selection']) and
            (('NBA95_CONTROL_COURT_FRAMES' not in environment) if legacy_fixed400 else
             environment.get('NBA95_CONTROL_COURT_FRAMES') == str(court_frames)) and
            Path(environment['NBA95_CAPTURE_DIR']).resolve() == capture, 'capture environment differs')
    complete = (capture/'capture_complete.txt').read_text()
    if complete != manifest['completion'] or 'init_calls=1\n' not in complete:
        raise ValueError('missing native initializer completion')
    completion = {}
    for line in complete.splitlines():
        require(re.fullmatch(r'[a-z_]+=[0-9]+', line) is not None, 'malformed completion sentinel')
        key, value = line.split('=')
        require(key not in completion, 'duplicate completion field')
        completion[key] = int(value)
    rows = [json.loads(line, object_pairs_hook=unique,
        parse_constant=lambda value: fail('nonfinite native row'))
        for line in (capture/'ownership.jsonl').read_text().splitlines()]
    require(completion.get('selection') == manifest['selection'] and completion.get('snapshots') == len(rows),
            'completion selection/row count differs')
    for row in rows:
        for key, low, high in (('native_pc',0,0xffffff),('global_frame',0,0x7fffffff),
                               ('court_frame',-1,court_frames),('cpu_d',0,0xffff)):
            require(type(row.get(key)) is int and low <= row[key] <= high, 'invalid native row '+key)
    starts = [r for r in rows if r['tag'] == 'player_setup.entry']
    if len(starts) != 1 or not exact(starts[0]['selections'], [2,1,1,1,1]):
        raise ValueError('unexpected natural Player Setup prestate')
    selections = [r for r in rows if r['tag'] == 'player_setup.after_input']
    require(len(selections) == 1 and exact(selections[0]['selections'], [manifest['selection'],1,1,1,1]),
            'declared selection differs from observed Player Setup result')
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
        # Input alone loads the original ROM for its pointer table. Permit
        # that one exact production loader line, not arbitrary diagnostics.
        response = run.stdout
        if mode == 'input':
            loader = '[ROM] Loaded successfully: "NBA Live \'95         " (Reset: 0x800D, Headered: No, Size: 1536 KiB)\n'
            require(response.startswith(loader), 'missing exact ROM loader response')
            response = response[len(loader):]
        # Parse the entire remaining response, rejecting extra text/JSON.
        actual = json.loads(response, object_pairs_hook=unique,
                            parse_constant=lambda value: fail('nonfinite C result'))
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
    require(words > 0, 'requested modes have no native comparisons')
    return dict(kind='bounded controller routine native prestate replay',capture=str(capture),
                manifest_sha256=sha(capture/'manifest.json'),probe_sha256=sha(probe),
                rom_sha256=sha(rom),calls={mode:counts[mode] for mode in modes},compared_words=words,
                capture_limit_source='frozen fixed400 source' if legacy_fixed400 else 'manifest and executed environment',
                unwitnessed_modes=[mode for mode in modes if not counts[mode]],verifier_sha256=sha(Path(__file__)),
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
