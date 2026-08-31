"""Production CLI input replay, with native configuration and C-only driver gates.

Native fixtures remain immutable. Only their button schedules enter the CLI;
full expected values stay in this verifier. Setup-only entry deliberately omits
the C intro; these are stable menu checkpoints, not cold-boot/timing parity.
"""
import argparse
import csv
import json
import subprocess
from pathlib import Path

from normalize_setup_config import read_compact, sha
from verify_setup_config_runtime import compare

ROOT = Path(__file__).resolve().parents[1]
BUTTONS = dict(none=0, b=0x8000, y=0x4000, select=0x2000, start=0x1000,
               up=0x0800, down=0x0400, left=0x0200, right=0x0100,
               a=0x0080, x=0x0040, l=0x0020, r=0x0010)
TRACE_FIELDS = ('step held pressed released native state page row working_mode working_style '
                'working_level working_quarter committed_mode committed_style committed_level '
                'committed_quarter previous pending delay speed fast').split() + [
                    f'{prefix}{i}' for prefix, count in
                    (('rules', 13), ('options', 7), ('custom', 13), ('working', 13))
                    for i in range(count)]


def native_word(key):
    names = key.split('+')
    if len(set(names)) != len(names) or ('none' in names and len(names) != 1):
        raise ValueError('invalid native button schedule')
    return sum(BUTTONS[name] for name in names)


def host_word(native):
    return sum(1 << bit for bit in range(12) if native & (0x8000 >> bit))


def read_trace(path, frames):
    with Path(path).open(newline='') as stream:
        reader = csv.DictReader(stream)
        if reader.fieldnames != TRACE_FIELDS:
            raise ValueError('invalid input trace columns/order')
        rows = []
        for row in reader:
            if None in row or any(value is None for value in row.values()):
                raise ValueError('malformed input trace row')
            rows.append({key: int(value) for key, value in row.items()})
    if [row['step'] for row in rows] != list(range(1, frames + 1)):
        raise ValueError('input trace dropped/duplicated/reordered frames')
    return rows


def configuration(row, action):
    page = row['page']
    if page not in (-1, 0, 1, 2):
        raise ValueError('invalid CLI menu page')
    return dict(action=action, scene=row['state'], page=page,
                row=row['row'] if page >= 0 else -1,
                main=[row['committed_' + key] for key in ('mode', 'style', 'level', 'quarter')],
                rules=[row[f'rules{i}'] for i in range(13)],
                options=[row[f'options{i}'] for i in range(7)],
                custom=[row[f'custom{i}'] for i in range(13)],
                working=[row[f'working{i}'] for i in range({-1: 0, 0: 4, 1: 13, 2: 7}[page])])


def verify_inputs(rows, expected):
    if len(rows) != len(expected):
        raise ValueError('incorrect input frame population')
    previous = 0
    for row, native in zip(rows, expected):
        held = host_word(native)
        want = dict(held=held, pressed=held & ~previous, released=previous & ~held, native=native)
        for field, value in want.items():
            if row[field] != value:
                raise AssertionError(f"input frame{row['step']} {field}: {row[field]} != {value}")
        previous = held


def run_case(args, directory, name, frames, flags, expected=None):
    trace = directory / f'{name}.csv'
    command = [str(args.exe), '--headless', '--rom', str(args.rom), '--assets', str(args.pack),
               '--setup-only', '--frames', str(frames), '--input-trace', str(trace), *flags]
    run = subprocess.run(command, text=True, capture_output=True, timeout=90)
    (directory / f'{name}.log').write_text(run.stdout + run.stderr)
    if run.returncode != 0 or run.stdout.count('[HEADLESS] Headless execution completed successfully.') != 1:
        raise AssertionError(f'{name}: CLI failed exit/completion guard: {run.returncode}')
    rows = read_trace(trace, frames)
    if expected is not None:
        verify_inputs(rows, expected)
    else:
        verify_inputs(rows, [r['native'] for r in rows])
        for before, after in zip(rows, rows[1:]):
            if before['held'] and after['held']:
                raise AssertionError('automatic taps lack a release frame')
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ('exe', 'rom', 'pack', 'output'):
        parser.add_argument('--' + key, type=Path, required=True)
    args = parser.parse_args()
    for key in ('exe', 'rom', 'pack', 'output'):
        setattr(args, key, getattr(args, key).resolve())
    args.output.mkdir(parents=True, exist_ok=False)
    sources = {key: dict(path=str(getattr(args, key)), sha256=sha(getattr(args, key)))
               for key in ('exe', 'rom', 'pack')}
    sources['main_source'] = dict(path=str(ROOT / 'src/main.c'), sha256=sha(ROOT / 'src/main.c'))
    for name in ('test_headless_input.py', 'verify_setup_config_runtime.py',
                 'normalize_setup_config.py', 'differential_compare.py'):
        path = ROOT / 'tools' / name
        sources[name] = dict(path=str(path), sha256=sha(path))
    reports = []
    selections = [('setup-config-native-witnesses.json', {'presets-v2', 'rules-v2', 'options-v2', 'held-v2'}),
                  ('setup-config-main-visual-native-witnesses.json', {'main-v4'}),
                  ('setup-config-input-native-witnesses.json', {'input-v1'}),
                  ('setup-config-faces-native-witnesses.json', {'faces-v1'})]
    for fixture_name, names in selections:
        fixture = ROOT / 'tests/fixtures' / fixture_name
        sources[fixture_name] = dict(path=str(fixture), sha256=sha(fixture))
        for journey in read_compact(fixture):
            if journey['name'] not in names:
                continue
            if sha(args.rom) != journey['native_manifest']['sources']['rom']['sha256']:
                raise ValueError('CLI input ROM differs from native evidence')
            words = [(400, 0)]
            checkpoints = [400]
            for action in journey['actions']:
                if action['hold']:
                    words.append((action['hold'], native_word(action['key'])))
                if action['wait'] > action['hold']:
                    words.append((action['wait'] - action['hold'], 0))
                checkpoints.append(checkpoints[-1] + action['wait'])
            script = args.output / (journey['name'] + '.input')
            script.write_text(''.join(f'{frames} {word:04x}\n' for frames, word in words))
            expected = [word for frames, word in words for _ in range(frames)]
            rows = run_case(args, args.output, journey['name'], len(expected),
                            ['--input-script', str(script)], expected)
            actual = [configuration(rows[step - 1], action) for action, step in enumerate(checkpoints)]
            issues = compare(journey, actual)
            reports.append(dict(journey=journey['name'], checkpoints=len(actual), frames=len(rows), issues=issues))

    # C-only automatic-driver contracts: defaults, real config/commit, clamping,
    # repeated returns, and Main adjustment preceding a submenu journey.
    automatic = [
        ('factory', ['--setup-menu', 'rules'], 500, 1, [0, 0, 0, 3], 0),
        ('custom', ['--setup-simulation-three-minute', '--setup-menu', 'rules',
                    '--setup-menu-row', '2', '--setup-menu-right', '1', '--setup-menu-confirm'],
                    600, 0, [0, 2, 0, 0], 0),
        ('clamp', ['--setup-menu', 'rules', '--setup-menu-row', '13'], 500, 1, [0, 0, 0, 3], 12),
        ('repeat', ['--setup-menu', 'rules', '--setup-menu-confirm', '--setup-menu-visits', '2'],
                   1000, 0, [0, 0, 0, 3], 0),
        ('main_then_rules', ['--setup-main-row', '1', '--setup-main-right', '1',
                             '--setup-menu', 'rules'], 500, 1, [0, 1, 0, 3], 0),
    ]
    for name, flags, frames, page, main, cursor in automatic:
        rows = run_case(args, args.output, name, frames, flags)
        final = configuration(rows[-1], 0)
        if final['page'] != page or final['main'] != main or final['row'] != cursor:
            raise AssertionError(f'{name}: wrong real-menu outcome: {final}')
        if name == 'repeat':
            pairs = [(before['page'], after['page']) for before, after in zip(rows, rows[1:])]
            if pairs.count((0, 1)) != 2 or pairs.count((1, 0)) != 2 or \
                    sum(r['native'] == BUTTONS['a'] for r in rows) != 2 or \
                    sum(r['native'] == BUTTONS['start'] for r in rows) != 2:
                raise AssertionError('automatic repeated journey did not complete two separate visits')
    invalid_scripts = ('', '0 0000\n', '-1 0000\n', '1 ffff\n', '1 10000\n',
                       '1 0041\n', '1 xyz\n', '2000001 0000\n', '1 0000 extra\n',
                       '1 0000\n-1 0400\n')
    for index, content in enumerate(invalid_scripts):
        script = args.output / f'invalid-{index}.input'
        script.write_text(content)
        run = subprocess.run([str(args.exe), '--headless', '--setup-only', '--rom', str(args.rom),
            '--assets', str(args.pack), '--frames', '1', '--input-script', str(script)],
            capture_output=True, text=True, timeout=30)
        if not run.returncode or 'Invalid input script' not in run.stderr:
            raise AssertionError('malformed input accepted or failed before intended parser')
    invalid_flags = [(['--input-script'], 'requires a file path'),
                     (['--input-trace'], 'requires a file path'),
                     (['--input-script', '--debug-state'], 'requires a file path'),
                     (['--input-trace', '--debug-state'], 'requires a file path'),
                     (['--setup-main-row', '0', '--setup-main-confirm', '--setup-menu', 'rules'],
                      'Conflicting automatic button scripts'),
                     (['--setup-menu', 'rules', '--title-press', '163'],
                      'Conflicting automatic button scripts')]
    for index, (flags, error) in enumerate(invalid_flags):
        forbidden_trace = args.output / f'invalid-flags-{index}.csv'
        run = subprocess.run([str(args.exe), '--headless', '--setup-only', '--rom', str(args.rom),
            '--assets', str(args.pack), '--frames', '500', '--input-trace', str(forbidden_trace), *flags],
            capture_output=True, text=True, timeout=30)
        if run.returncode != 1 or error not in run.stderr:
            raise AssertionError('invalid flags did not reach the intended guard')
        if forbidden_trace.exists():
            raise AssertionError('invalid flags executed frames/opened trace before rejection')
    for source in sources.values():
        if sha(source['path']) != source['sha256']:
            raise ValueError('source changed during CLI verification')
    passed = len(reports) == 7 and all(not r['issues'] for r in reports)
    report = dict(result='PASS' if passed else 'FAIL', sources=sources, native=reports,
                  c_only_automatic_cases=len(automatic), malformed_cases=len(invalid_scripts),
                  invalid_flag_cases=len(invalid_flags),
                  exclusions=['intro/transition timing', 'RGB/audio parity', 'gameplay consumers', 'disk persistence'])
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(result=report['result'], checkpoints=sum(r['checkpoints'] for r in reports),
                          input_frames=sum(r['frames'] for r in reports),
                          issues=[r for r in reports if r['issues']])))
    return 0 if passed else 1


if __name__ == '__main__':
    raise SystemExit(main())
