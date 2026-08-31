"""Strict bounded queue/epoch comparison against natural Mesen observations.

This does not execute complete constructors or predict producer/bus timing.
No native fixture or production timing is written by this tool.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROM_SHA = '2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
MESEN_SHA = 'd2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b'
BUILD_SOURCES = {'include/nba_setup_scheduler.h', 'src/nba_setup_scheduler.c',
                 'tools/setup_scheduler_probe.c', 'tools/build_setup_scheduler_probe.ps1'}
CAPTURE_KIND = 'natural controller-only Setup resource scheduler observation'
CAPTURE_SCHEDULE = ('Canonical Simulation/3min normalization; Rules A470, row2 Right640, Start830; '
                    'reentry A1100, row2 Right1270, Start1460. Evidence labels rebase after normalization; emulation uninterrupted.')


def require(condition, message):
    if not condition:
        raise ValueError(message)


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def pairs(entries):
    result = {}
    for key, value in entries:
        require(key not in result, 'duplicate JSON key: ' + key)
        result[key] = value
    return result


def read_json(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'), object_pairs_hook=pairs)


def exact_keys(value, keys, label):
    require(type(value) is dict and set(value) == set(keys), 'invalid/missing ' + label + ' fields')


def integer(value, minimum, maximum, label):
    require(type(value) is int and minimum <= value <= maximum, 'invalid numeric domain: ' + label)


def sha256(value, label):
    require(type(value) is str and re.fullmatch('[0-9a-f]{64}', value) is not None,
            'invalid SHA256: ' + label)


def path_value(value, label):
    require(type(value) is str and value != '' and '\0' not in value, 'invalid path: ' + label)
    return Path(value).resolve()


def typed_equal(actual, expected, label, subset=False):
    require(type(actual) is type(expected), 'type mismatch: ' + label)
    if type(expected) is dict:
        require(set(expected) <= set(actual) if subset else set(actual) == set(expected),
                'missing/unexpected settings: ' + label)
        for key, value in expected.items():
            typed_equal(actual[key], value, label + '.' + key, subset=subset)
    else:
        require(actual == expected, 'value mismatch: ' + label)


def expected_settings(directory):
    return {
        'Debug': {'ScriptWindow': {'AllowIoOsAccess': True, 'ScriptTimeout': 60, 'SaveScriptBeforeRun': False}},
        'Preferences': {'SingleInstance': False, 'PauseWhenInBackground': False, 'AutoLoadPatches': False,
                        'OverrideSaveDataFolder': True, 'SaveDataFolder': str(directory / 'isolated-saves')},
        'Snes': {'Port1': {'Type': 'SnesController'}, 'Port2': {'Type': 'None'}, 'DisableFrameSkipping': True,
                 'EnableRandomPowerOnState': False, 'RamPowerOnState': 'AllZeros', 'ForceFixedResolution': False,
                 'Overscan': {'Top': 7, 'Bottom': 8, 'Left': 0, 'Right': 0}},
        'Video': {'VideoFilter': 'None', 'AspectRatio': 'NoStretching', 'Brightness': 0,
                  'Contrast': 0, 'Hue': 0, 'Saturation': 0}}


def validate_capture_manifest(manifest, directory, rom):
    # Known legacy captures omit four before-wait dumps. Their core identities
    # are still mandatory. The complete on-disk file inventory must be attested
    # as well, preventing an omitted declaration from silently avoiding hashing.
    tail = (directory / 'scheduler_base.lua').is_file()
    fields = {'schema', 'kind', 'state_injection', 'rom_patch', 'accepted', 'sources',
              'arguments', 'isolation', 'schedule', 'exit_code', 'artifacts'}
    exact_keys(manifest, fields | ({'interrupt_summary'} if tail else set()), 'capture manifest')
    integer(manifest['schema'], 1, 1, 'capture schema')
    integer(manifest['exit_code'], 0, 0, 'capture exit_code')
    require(manifest['accepted'] is True and manifest['state_injection'] is False and
            manifest['rom_patch'] is False, 'successful natural capture required')
    require(manifest['kind'] == CAPTURE_KIND and manifest['schedule'] == CAPTURE_SCHEDULE,
            'capture kind/input schedule differs')
    source_paths = {'rom': Path(rom).resolve(), 'mesen': directory / 'portable-mesen/Mesen.exe',
                    'script': directory / 'capture.lua', 'runner': directory / 'capture_runner.py',
                    'settings': directory / 'initial-settings.json'}
    if tail:
        source_paths['base_script'] = directory / 'scheduler_base.lua'
        require(type(manifest['interrupt_summary']) is str and
                re.fullmatch(r'ok; scopes=4; nmi=46; instructions=[1-9][0-9]*; bus=[1-9][0-9]*\n',
                             manifest['interrupt_summary']) is not None, 'invalid interrupt summary')
    exact_keys(manifest['sources'], source_paths, 'capture sources')
    for name, expected in source_paths.items():
        entry = manifest['sources'][name]
        exact_keys(entry, {'path', 'sha256'}, 'capture source ' + name)
        sha256(entry['sha256'], 'capture source ' + name)
        require(path_value(entry['path'], name) == expected.resolve(), 'capture source path differs: ' + name)
    require(manifest['sources']['rom']['sha256'] == ROM_SHA and
            manifest['sources']['mesen']['sha256'] == MESEN_SHA, 'canonical ROM/Mesen identity mismatch')
    arguments = manifest['arguments']
    require(type(arguments) is list and len(arguments) == 5 and all(type(a) is str for a in arguments),
            'invalid capture arguments')
    require(arguments[1:3] == ['--testrunner', '--timeout=300'] and
            all(path_value(arguments[index], 'capture argument') == source_paths[name].resolve()
                for index, name in ((0, 'mesen'), (3, 'rom'), (4, 'script'))), 'capture arguments differ')
    core = {'capture.lua', 'capture_runner.py', 'initial-settings.json', 'mesen.log',
            'observed_environment.txt', 'scheduler.jsonl', 'state_fields.txt', 'capture_complete.txt'}
    core |= {f'header_{n:02d}_{stage}.wram' for n in range(1, 5) for stage in ('entry', 'after_wait')}
    before = {f'header_{n:02d}_before_wait.wram' for n in range(1, 5)}
    inventory = {p.name for p in directory.iterdir() if p.is_file() and p.name != 'manifest.json'}
    if inventory & before:
        core |= before
    if tail:
        core |= {'scheduler_base.lua', 'interrupt_complete.txt', 'interrupt_instructions.jsonl',
                 'interrupt_boundaries.jsonl', 'interrupt_bus.jsonl'}
        core |= {f'interrupt_{n:02d}_{stage}.wram' for n in range(1, 47) for stage in ('entry', 'exit')}
    exact_keys(manifest['artifacts'], core, 'capture artifacts')
    require(inventory == core, 'capture artifact inventory differs from required declarations')
    for name, entry in manifest['artifacts'].items():
        exact_keys(entry, {'bytes', 'sha256'}, 'capture artifact ' + name)
        integer(entry['bytes'], 0 if name == 'mesen.log' else 1, 2**63 - 1, 'artifact bytes ' + name)
        if name.endswith('.wram'):
            integer(entry['bytes'], 0x20000, 0x20000, 'WRAM snapshot bytes')
        sha256(entry['sha256'], 'capture artifact ' + name)
    isolation = manifest['isolation']
    exact_keys(isolation, {'home', 'save_folder', 'initial_saves', 'settings', 'observed', 'post_settings_sha256'},
               'capture isolation')
    require(path_value(isolation['home'], 'home') == directory / 'portable-mesen' and
            path_value(isolation['save_folder'], 'save folder') == directory / 'isolated-saves', 'private home/save path differs')
    require(type(isolation['initial_saves']) is list and isolation['initial_saves'] == [], 'fresh empty saves required')
    typed_equal(isolation['settings'], expected_settings(directory), 'declared settings')
    exact_keys(isolation['observed'], {'output', 'home'}, 'observed environment')
    require(path_value(isolation['observed']['output'], 'observed output') == directory and
            path_value(isolation['observed']['home'], 'observed home').is_relative_to(directory / 'portable-mesen'),
            'declared observed environment differs')
    sha256(isolation['post_settings_sha256'], 'persisted settings')


def validate_rows(rows):
    require(type(rows) is list and rows, 'empty native scheduler observations')
    base = set(('bg2_phase168f bg2_scroll0613 brightness0562 busy05cb callback_05c2 callback_05c5 '
                'cpu_a cpu_cycles cpu_d cpu_ps cpu_sp cpu_x cpu_y dma_mode0561 epoch0564 epoch_block059c '
                'event global_frame header_active header_count label master_clock nmi_depth palette_size0568 '
                'pc ppu_frame ppu_vram_address queue_budget queue_head queue_tail scanline tag wait_owner').split())
    version2 = set('cpu_db hclock source0c palette_source056c palette_dest056a budget_mode07f0'.split())
    version3 = set('ppu_cgram_address ppu_vram_increment ppu_vram_remapping ppu_vram_increment_high'.split())
    common = base | (version2 if set(rows[0]) & version2 else set()) | (version3 if set(rows[0]) & version3 else set())
    dma = set('mask channel dma_mode bbus source size'.split())
    owners = {
        'backdrop.entry': 0x80EC68, 'copy.immediate_exit': 0x808BCF, 'copy.queued_exit': 0x808C2A,
        'copy.submit': 0x808BA1, 'decompress.entry': 0x80C62B, 'decompress.exit': 0x80C682,
        'epoch.after_guard': 0x8084AB, 'epoch.before_increment': 0x8084A8,
        'fill.submit': 0x808AD2, 'fill.immediate_exit': 0x808B34, 'frame.end': 0,
        'header.after_wait': 0x80EF1E, 'header.before_wait': 0x80EF1A,
        'header.entry': 0x80EEC6, 'header.exit': 0x80EF8D, 'main.constructor': 0x81BA8E,
        'nmi.before_publish': 0x8081E3, 'nmi.entry': 0x80815A, 'nmi.exit': 0x80859B,
        'nmi.reentrant_exit': 0x808171, 'nmi.queue_budget_exhausted': 0x8082E3,
        'nmi.queue_completed': 0x8083CE, 'palette.immediate_exit': 0x808A41,
        'palette.queued_exit': 0x808A56, 'palette.submit': 0x808A02,
        'queue.wait_entry': 0x8086DA, 'queue.wait_exit': 0x8086E7, 'rules.constructor': 0x81CF62,
        'setup.nmi_callback': 0x81F9FC, 'wait.entry': 0x8086B0, 'wait.loaded': 0x8086B7,
        'wait.resume': 0x8086BC}
    byte_fields = {'cpu_ps', 'cpu_db', 'bbus', 'dma_mode', 'ppu_cgram_address'}
    long_fields = {'pc', 'callback_05c2', 'callback_05c5', 'source', 'source0c', 'palette_source056c'}
    clocks = {'master_clock', 'cpu_cycles'}
    counters = {'event', 'global_frame', 'ppu_frame', 'label'}
    limits = {'header_active': 1, 'header_count': 4, 'wait_owner': 4, 'nmi_depth': 64,
              'scanline': 261, 'hclock': 1364, 'ppu_vram_increment_high': 1,
              'ppu_vram_remapping': 3, 'channel': 7, 'mask': 255}
    for index, row in enumerate(rows):
        require(type(row) is dict and type(row.get('tag')) is str, 'invalid native row/tag')
        tag = row['tag']
        require(tag in owners or tag == 'dma.submit', 'unknown native observation tag')
        extra = dma if tag == 'dma.submit' else {'loaded_epoch'} if tag == 'wait.loaded' else set()
        if tag == 'nmi.before_publish' and version2 <= common:
            extra |= {'queue_hex'}
        exact_keys(row, common | extra, 'native row ' + str(index))
        for name, value in row.items():
            if name == 'tag':
                continue
            if name == 'queue_hex':
                require(type(value) is str and re.fullmatch('[0-9a-f]{1024}', value) is not None,
                        'invalid queue byte string')
                continue
            maximum = limits.get(name, (2**64 - 1 if name in clocks else 2**32 - 1 if name in counters
                                        else 0xFFFFFF if name in long_fields else 255 if name in byte_fields else 65535))
            integer(value, 0, maximum, 'native ' + name)
        require(row['event'] == index, 'event order/gaps')
        require(tag == 'dma.submit' or row['pc'] == owners[tag], 'native hook/PC mismatch')
        require(row['queue_head'] < 512 and row['queue_tail'] < 512 and
                row['queue_head'] % 8 == 0 and row['queue_tail'] % 8 == 0, 'invalid queue cursor')
        if 'ppu_vram_increment' in row:
            require(row['ppu_vram_increment'] in (1, 32, 128), 'invalid PPU VRAM increment')
    require(all(a['master_clock'] <= b['master_clock'] and a['cpu_cycles'] <= b['cpu_cycles']
                for a, b in zip(rows, rows[1:])), 'native clock order')


def read_capture(directory, rom):
    directory = Path(directory).resolve()
    manifest = read_json(directory / 'manifest.json')
    validate_capture_manifest(manifest, directory, rom)
    for entry in manifest['sources'].values():
        require(digest(entry['path']) == entry['sha256'], 'source changed: ' + entry['path'])
    for name, entry in manifest['artifacts'].items():
        require(Path(name).name == name, 'invalid artifact path')
        path = directory / name
        require(path.stat().st_size == entry['bytes'] and digest(path) == entry['sha256'],
                'capture artifact changed: ' + name)
    observed = pairs(line.split('=', 1) for line in
                     (directory / 'observed_environment.txt').read_text().splitlines())
    typed_equal(observed, manifest['isolation']['observed'], 'actual observed environment')
    settings = expected_settings(directory)
    typed_equal(read_json(directory / 'initial-settings.json'), settings, 'actual initial settings')
    post = directory / 'portable-mesen/settings.json'
    require(digest(post) == manifest['isolation']['post_settings_sha256'], 'persisted settings identity differs')
    typed_equal(read_json(post), settings, 'actual persisted settings', subset=True)
    require((directory / 'capture_complete.txt').read_text() ==
            'ok; headers=4; normal controller-only Rules repeat journey\n', 'missing sentinel')
    rows = [json.loads(line, object_pairs_hook=pairs) for line in
            (directory / 'scheduler.jsonl').read_text().splitlines()]
    validate_rows(rows)
    return rows


def check_build(exe):
    manifest = read_json(exe.parent / 'build-manifest.json')
    exact_keys(manifest, {'schema', 'compiler_exit', 'sources', 'executable'}, 'build manifest')
    integer(manifest['schema'], 1, 1, 'build schema')
    integer(manifest['compiler_exit'], 0, 0, 'compiler_exit')
    exact_keys(manifest['sources'], BUILD_SOURCES, 'build sources')
    for entry in [manifest['executable'], *manifest['sources'].values()]:
        exact_keys(entry, {'path', 'sha256'}, 'build identity')
        path_value(entry['path'], 'build identity')
        sha256(entry['sha256'], 'build identity')
    source_root = Path(manifest['sources']['src/nba_setup_scheduler.c']['path']).resolve().parents[1]
    require(all(Path(entry['path']).resolve() == (source_root / name).resolve()
                for name, entry in manifest['sources'].items()), 'build source paths differ')
    require(Path(manifest['executable']['path']).resolve() == exe.resolve() and
            digest(exe) == manifest['executable']['sha256'], 'executable changed')
    for entry in manifest['sources'].values():
        require(digest(entry['path']) == entry['sha256'], 'source changed since probe build')
    return manifest


def queue_cases(rows):
    cases = []
    active = None
    operations = []
    for row in rows:
        tag = row['tag']
        if tag == 'nmi.before_publish':
            require(active is None, 'missing native queue exit')
            require(row['nmi_depth'] == 1, 'unexpected nested queue publication')
            require(re.fullmatch('[0-9a-f]{1024}', row.get('queue_hex', '')) is not None,
                    'complete queue bytes required; use the extended capture')
            require((row['cpu_ps'] & 0x10) != 0, 'before-publication X width changed')
            active = row
            operations = []
        elif active is not None and tag == 'dma.submit':
            require(row['nmi_depth'] == 1 and row['mask'] == 1 and row['channel'] == 0,
                    'unexpected native publication channel')
            require(all(key in row for key in ('ppu_cgram_address', 'ppu_vram_increment',
                                               'ppu_vram_increment_high', 'ppu_vram_remapping')),
                    'observed CGRAM/VMAIN required; use native scheduler v3 capture')
            destination = row['ppu_cgram_address'] if row['bbus'] == 0x22 else row['ppu_vram_address']
            operations.append([row['dma_mode'], row['bbus'], row['source'], row['size'], destination,
                               row['ppu_vram_increment'], row['ppu_vram_increment_high'], row['ppu_vram_remapping']])
        elif active is not None and tag in ('nmi.queue_completed', 'nmi.queue_budget_exhausted'):
            require(row['nmi_depth'] == 1 and row['cpu_d'] == 0 and (row['cpu_ps'] & 0x10) == 0,
                    'native queue exit width/context changed')
            cases.append((active, row, operations))
            active = None
        elif active is not None and tag == 'nmi.exit':
            raise ValueError('NMI returned without observed queue boundary')
    require(active is None and len(cases) > 0, 'incomplete/empty queue observations')
    return cases


def probe_lines(exe, commands):
    run = subprocess.run([str(exe)], input='\n'.join(commands) + '\n', text=True,
                         capture_output=True, check=True, timeout=30)
    require(run.stderr == '', 'unexpected probe diagnostic: ' + run.stderr)
    return run.stdout.splitlines()


def check_queues(rows, exe):
    cases = queue_cases(rows)
    commands = ['queue ' + ' '.join(str(start[k]) for k in
        ('queue_head', 'queue_tail', 'queue_budget', 'palette_size0568',
         'palette_dest056a', 'palette_source056c')) + ' ' + start['queue_hex']
        for start, _, _ in cases]
    lines = iter(probe_lines(exe, commands))
    count = 0
    modes = Counter()
    for start, end, expected_ops in cases:
        operations = []
        current_vmain = [start['ppu_vram_increment'], start['ppu_vram_increment_high'], start['ppu_vram_remapping']]
        for line in lines:
            fields = line.split()
            require(fields and fields[0] in ('op', 'end'), 'malformed C queue output')
            if fields[0] == 'end':
                require(len(fields) == 5, 'malformed C queue end')
                actual_end = list(map(int, fields[1:]))
                break
            require(len(fields) == 7, 'malformed C publication')
            mode, port, vmain, source, size, destination = map(int, fields[1:])
            require(vmain in (0, 0x81, 0xFF), 'invalid C VMAIN action')
            if vmain != 0xFF:
                current_vmain = [32 if vmain == 0x81 else 1, int(bool(vmain & 0x80)), 0]
            operations.append([mode, port, source, size, destination] + current_vmain)
            if vmain != 0xFF:
                current_vmain = [1, 1, 0]  # native special branches restore $2115=$80
            modes[f'{mode:02x}/{port:02x}'] += 1
        else:
            raise ValueError('truncated C queue output')
        # Hooks execute BEFORE native STX $35, so X, not the stale $35,
        # is the native final read cursor at both exit boundaries.
        expected_end = [int(end['tag'] == 'nmi.queue_budget_exhausted'), end['cpu_x'],
                        end['queue_budget'], end['palette_size0568']]
        require(actual_end == expected_end,
                f"queue state differs at label{start['label']}: {actual_end} != {expected_end}")
        require(operations == expected_ops,
                f"publication differs at label{start['label']}: {operations} != {expected_ops}")
        count += len(operations)
    require(next(lines, None) is None, 'trailing C queue output')
    return dict(dispatches=len(cases), publications=count, modes=dict(modes),
                budget_stops=sum(end['tag'] == 'nmi.queue_budget_exhausted' for _, end, _ in cases))


def header_intervals(rows):
    headers = []
    for row in rows:
        if row['tag'] == 'header.entry':
            require(row['header_count'] == len(headers) + 1, 'header order')
            headers.append({'entry': row})
        elif row['tag'] in ('header.before_wait', 'header.after_wait'):
            require(headers and row['header_count'] == len(headers), 'header exit without entry')
            key = 'before_wait' if row['tag'] == 'header.before_wait' else 'after_wait'
            require(key not in headers[-1], 'duplicate header boundary')
            headers[-1][key] = row
    require(len(headers) == 4 and all(set(h) == {'entry', 'before_wait', 'after_wait'} for h in headers),
            'four complete native headers required')
    return headers


def check_epochs(rows, exe):
    headers = header_intervals(rows)
    commands, expected = [], []
    for header in headers:
        first, last = header['before_wait']['event'], header['after_wait']['event']
        events = rows[first:last + 1]
        loads = [r for r in events if r['tag'] == 'wait.loaded']
        resumes = [r for r in events if r['tag'] == 'wait.resume']
        require(len(loads) == len(resumes) == 1, 'header wait coverage differs')
        load, resume = loads[0], resumes[0]
        require(load['cpu_ps'] & 0x30 == 0 and load['cpu_a'] == load['loaded_epoch'] == load['epoch0564'],
                'wait load width/register contract differs')
        require(resume['epoch0564'] != load['loaded_epoch'] and resume['nmi_depth'] == 0,
                'native resume must follow epoch change and RTI')
        commands.append(f"load {load['loaded_epoch']}")
        for row in rows[load['event']:resume['event'] + 1]:
            tag = row['tag']
            if tag == 'epoch.before_increment':
                require((row['cpu_ps'] & 0x20) == 0 and row['epoch_block059c'] == 0,
                        'increment entry width/guard differs')
                commands += [f"state {row['epoch0564']} {row['epoch_block059c']} 1", 'increment']
                expected += ['ready 0', f"epoch {(row['epoch0564'] + 1) & 65535}"]
            elif tag in ('wait.loaded', 'epoch.after_guard', 'nmi.exit', 'wait.resume'):
                active = int(row['nmi_depth'] > 0)
                commands.append(f"state {row['epoch0564']} {row['epoch_block059c']} {active}")
                expected.append('ready ' + ('1' if tag == 'wait.resume' else '0'))
    actual = probe_lines(exe, commands)
    require(actual == expected, 'C wait/epoch protocol differs at native boundaries')
    return [dict(invocation=h['entry']['header_count'],
                 **{key: {field: h[key][field] for field in
                           ('label', 'scanline', 'master_clock', 'cpu_cycles', 'epoch0564')}
                    for key in ('entry', 'before_wait', 'after_wait')}) for h in headers]


def check_byte_epochs(rows, exe):
    commands, expected, cases = [], [], []
    for load in rows:
        if load['tag'] != 'wait.loaded' or (load['cpu_ps'] & 0x20) == 0:
            continue
        require((load['cpu_a'] & 255) == (load['loaded_epoch'] & 255) ==
                (load['epoch0564'] & 255), 'byte wait load/register contract differs')
        resume = next((row for row in rows[load['event'] + 1:]
                       if row['tag'] in ('wait.loaded', 'wait.resume')), None)
        require(resume is not None and resume['tag'] == 'wait.resume',
                'missing byte wait resume')
        require(resume['nmi_depth'] == 0 and (resume['cpu_ps'] & 0x20) != 0 and
                (resume['epoch0564'] & 255) != (load['epoch0564'] & 255),
                'byte wait resumed before byte change/RTI')
        commands.append(f"load8 {load['epoch0564']}")
        for row in rows[load['event']:resume['event'] + 1]:
            if row['tag'] in ('wait.loaded', 'epoch.after_guard', 'nmi.exit', 'wait.resume'):
                commands.append(f"state {row['epoch0564']} {row['epoch_block059c']} {int(row['nmi_depth'] > 0)}")
                expected.append('ready ' + ('1' if row['tag'] == 'wait.resume' else '0'))
        cases.append(dict(label=load['label'], epoch=load['epoch0564'],
                          loaded_byte=load['loaded_epoch'] & 255,
                          resumed_epoch=resume['epoch0564']))
    require(cases and probe_lines(exe, commands) == expected,
            'C byte wait protocol differs at native boundaries')
    return cases


def check_previous(rows, previous):
    index = {(r['master_clock'], r['tag']): r for r in rows}
    require(len(index) == len(rows), 'duplicate native event identity')
    for old in previous:
        new = index.get((old['master_clock'], old['tag']))
        require(new is not None, 'old native event disappeared')
        require(all(new.get(key) == value for key, value in old.items() if key != 'event'),
                'existing native observation changed after adding telemetry')
    return len(previous)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', required=True, type=Path)
    parser.add_argument('--previous-native', type=Path, action='append', default=[])
    parser.add_argument('--rom', required=True, type=Path)
    parser.add_argument('--exe', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    args = parser.parse_args()
    require(not args.report.exists(), 'report already exists; preserve previous evidence')
    build = check_build(args.exe.resolve())
    rows = read_capture(args.native, args.rom)
    report = dict(schema=1, scope='bounded queue publication and four header epoch waits; no producer timing/production parity',
                  native_manifest_sha256=digest(args.native / 'manifest.json'), build=build,
                  queue=check_queues(rows, args.exe.resolve()), headers=check_epochs(rows, args.exe.resolve()),
                  byte_waits=check_byte_epochs(rows, args.exe.resolve()))
    report['unchanged_previous_captures'] = [dict(manifest_sha256=digest(previous / 'manifest.json'),
        events=check_previous(rows, read_capture(previous, args.rom))) for previous in args.previous_native]
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS bounded scheduler primitives:', json.dumps(report['queue']),
          'four header waits and', len(report['byte_waits']), 'byte-width waits')


if __name__ == '__main__':
    main()
