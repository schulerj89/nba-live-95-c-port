"""Exact controlled native clock formatter parity; not gameplay timing proof."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROM_SHA = '2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
MESEN_SHA = 'd2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b'
SCRIPT_SHA = 'c7c4d487a5ff890762f54b7197733a5b7e00fe627254d57887f2ddb7bfd5a42e'
ADDRESSES = [0x492b, 0x8de, 0x8e8, 0x8f6, 0x928, 0x92a, 0x9b4, 0x13e7]
CASES = [(0x87baf5, v, 0) for v in (0, 1, 59, 60, 599, 600, 3599, 3600, 43199, 43200, 65535)] + [
    (0x87bb59, v, b) for v in (0, 1, 5, 6, 59, 60, 61, 3599, 3600, 61440, 65534, 65535) for b in (0, 1)]
KEYS = {'case', 'controlled', 'routine', 'entry_frame', 'exit_frame',
        'entry', 'exit', 'entry_text', 'exit_text'}


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def unique(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError('duplicate JSON key')
        result[key] = value
    return result


def parse(text):
    return json.loads(text, object_pairs_hook=unique)


def words(value, limit, count=8):
    if type(value) is not list or len(value) != count or any(type(x) is not int or not 0 <= x <= limit for x in value):
        raise ValueError('wrong native array shape/type/domain')


def validate_rows(rows):
    if type(rows) is not list or len(rows) != len(CASES):
        raise ValueError('expected all35 native formatter cases')
    previous = -1
    for number, (row, case) in enumerate(zip(rows, CASES), 1):
        if type(row) is not dict or set(row) != KEYS or row['controlled'] is not True:
            raise ValueError('wrong native row schema/provenance')
        for key in ('case', 'routine', 'entry_frame', 'exit_frame'):
            if type(row[key]) is not int or row[key] < 0:
                raise ValueError('wrong native scalar type/domain')
        if row['case'] != number or row['routine'] != case[0] or not previous < row['entry_frame'] <= row['exit_frame']:
            raise ValueError('missing/duplicate/reordered native boundary')
        previous = row['exit_frame']
        for key in ('entry', 'exit'):
            words(row[key], 65535)
        for key in ('entry_text', 'exit_text'):
            words(row[key], 255)
        current = 43200 if case[0] == 0x87baf5 else 1
        if row['entry'] != [0, 0, 1, current, current, case[1], case[2], 32]:
            raise ValueError('native pre-call words differ from declared controlled case')
    return rows


def native_rows(directory, rom):
    directory = Path(directory)
    if sha(Path(rom).read_bytes()) != ROM_SHA:
        raise ValueError('wrong original ROM')
    manifest = parse((directory / 'manifest.json').read_text())
    if manifest.get('schema') != 1 or type(manifest.get('schema')) is not int or \
            manifest.get('kind') != 'controlled native HUD clock formatter entry/exit' or \
            manifest.get('state_injection') is not True or manifest.get('rom_patch') is not False or \
            manifest.get('accepted_capture') is not True or type(manifest.get('exit_code')) is not int or manifest['exit_code'] != 0 or \
            manifest.get('addresses') != ADDRESSES or manifest.get('rom_sha256') != ROM_SHA or \
            manifest.get('mesen_sha256') != MESEN_SHA or manifest.get('script_sha256') != SCRIPT_SHA:
        raise ValueError('wrong native capture provenance')
    artifacts = manifest.get('artifacts')
    required = {'capture.lua', 'capture_runner.py', 'mesen_portable.py', 'clock-cases.jsonl',
                'initial-mesen-settings.json', 'observed-script-data-folder.txt', 'capture_complete.txt', 'mesen.log'}
    if type(artifacts) is not dict or not required <= set(artifacts):
        raise ValueError('missing raw native/source attestation')
    for name, entry in artifacts.items():
        if Path(name).name != name or type(entry) is not dict or set(entry) != {'size', 'sha256'} or \
                type(entry['size']) is not int or entry['size'] < 0:
            raise ValueError('invalid native artifact attestation')
        data = (directory / name).read_bytes()
        if len(data) != entry['size'] or sha(data) != entry['sha256']:
            raise ValueError('changed native artifact: ' + name)
    if sha((directory / 'capture.lua').read_bytes()) != SCRIPT_SHA or \
            sha((directory / 'portable-mesen/Mesen.exe').read_bytes()) != MESEN_SHA:
        raise ValueError('executed native script/emulator identity differs')
    if (directory / 'capture_complete.txt').read_bytes() != b'35 controlled native formatter entry/exit pairs; restored WRAM\n':
        raise ValueError('incomplete native capture')
    isolation = manifest.get('isolation', {})
    observed = (directory / 'observed-script-data-folder.txt').read_text().strip()
    expected = directory.resolve() / 'portable-mesen/LuaScriptData/capture'
    if Path(observed).resolve() != expected or isolation.get('observed_script_data_folder') != observed or \
            isolation.get('initial_saves') != [] or isolation.get('post_settings_verified') is not True:
        raise ValueError('native portable home or initial saves differ')
    settings = parse((directory / 'initial-mesen-settings.json').read_text())
    if sha((directory / 'initial-mesen-settings.json').read_bytes()) != isolation.get('initial_settings_sha256') or \
            settings.get('Snes', {}).get('EnableRandomPowerOnState') is not False or \
            settings['Snes'].get('RamPowerOnState') != 'AllZeros' or \
            settings['Snes'].get('DisableFrameSkipping') is not True:
        raise ValueError('native initialization differs')
    return validate_rows([parse(line) for line in (directory / 'clock-cases.jsonl').read_text().splitlines()]), manifest


def read_output(stdout):
    lines = [line for line in stdout.splitlines() if line.startswith('HUD_CLOCK')]
    if len(lines) != 35:
        raise ValueError('missing/extra C formatter rows')
    rows = []
    for number, line in enumerate(lines, 1):
        if re.fullmatch(r'HUD_CLOCK(?: [0-9]+){18}', line) is None:
            raise ValueError('malformed C formatter protocol')
        values = list(map(int, line.split()[1:]))
        if values[0] != number or values[1] != CASES[number - 1][0]:
            raise ValueError('duplicate/reordered C formatter boundary')
        words(values[2:10], 65535)
        words(values[10:], 255)
        rows.append(values)
    return rows


def compare_rows(native, actual):
    if len(native) != 35 or len(actual) != 35:
        raise ValueError('incomplete native/C comparison')
    failures = []
    for row, output in zip(native, actual):
        expected = [row['case'], row['routine'], *row['exit'], *row['exit_text']]
        if output != expected:
            failures.append(dict(case=row['case'], expected=expected, actual=output))
    return failures


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('native', 'rom', 'probe', 'pack'):
        parser.add_argument('--' + name, required=True, type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    native, manifest = native_rows(args.native, args.rom)
    inputs = ''.join(' '.join(map(str, [r['routine'], *r['entry'], *r['entry_text']])) + '\n' for r in native)
    run = subprocess.run([str(args.probe.resolve()), str(args.pack.resolve())], input=inputs,
                         capture_output=True, text=True, timeout=30, check=True)
    actual = read_output(run.stdout)
    failures = compare_rows(native, actual)
    report = dict(scope='35 controlled native formatter entry/exit cases; normal gameplay scheduling excluded',
                  native_manifest_sha256=sha((args.native / 'manifest.json').read_bytes()),
                  probe_sha256=sha(args.probe.read_bytes()), pack_sha256=sha(args.pack.read_bytes()),
                  comparisons=35, words_per_exit=8, text_bytes_per_exit=8, failures=failures)
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + '\n')
    if failures:
        raise AssertionError('native formatter mismatch: ' + json.dumps(failures[0]))
    print('PASS:35 controlled native formatter cases; all8 WRAM words and8 text bytes exact')


if __name__ == '__main__':
    main()
