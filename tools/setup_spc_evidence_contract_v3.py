"""Strict evidence envelopes; no emulated behavior or timing prediction."""
import json
from pathlib import Path

def check(ok, message):
    if not ok:
        raise ValueError(message)

def unique_object(pairs):
    result = {}
    for key, value in pairs:
        check(key not in result, 'duplicate JSON field: ' + key)
        result[key] = value
    return result

def loads(text):
    return json.loads(text, object_pairs_hook=unique_object,
                      parse_constant=lambda value: invalid_constant(value))

def invalid_constant(value):
    raise ValueError('non-JSON numeric constant: ' + value)

def exact(actual, expected):
    check(type(actual) is type(expected), 'evidence value type')
    if isinstance(expected, dict):
        check(set(actual) == set(expected), 'evidence nested keys')
        for key in expected:
            exact(actual[key], expected[key])
    elif isinstance(expected, list):
        check(len(actual) == len(expected), 'evidence list length')
        for a, b in zip(actual, expected):
            exact(a, b)
    else:
        check(actual == expected, 'evidence value differs')

def capture_envelope(m, p, rom, kind, timeout=60):
    p = p.resolve()
    check(set(m) == {'schema','kind','state_injection','rom_patch','accepted',
                    'settings','sources','initial_saves','arguments','exit_code',
                    'post_settings_sha256','artifacts'}, 'capture envelope keys')
    check(m['kind'] == kind, 'capture kind')
    paths = {'rom':rom.resolve(), 'mesen':p/'portable-mesen/Mesen.exe',
             'script':p/'capture.lua', 'runner':p/'runner.py',
             'settings':p/'initial-settings.json'}
    check(set(m['sources']) == set(paths), 'source envelope closure')
    for key, path in paths.items():
        value = m['sources'][key]
        check(set(value) == {'path','sha256'} and type(value['path']) is str,
              'source envelope keys')
        check(Path(value['path']).resolve() == path, 'capture source path: ' + key)
    exact(m['arguments'], [str(paths['mesen']), '--testrunner',
                          '--timeout='+str(timeout), str(paths['rom']), str(paths['script'])])
    for value in m['artifacts'].values():
        check(set(value) == {'bytes','sha256'}, 'artifact envelope keys')
    # This runner creates these settings exactly; persisted application settings
    # may contain extra defaults and are checked separately by owned-key subset.
    settings = {'Debug':{'ScriptWindow':{'AllowIoOsAccess':True,'ScriptTimeout':timeout}},
                'Preferences':{'SingleInstance':False,'PauseWhenInBackground':False,
                  'AutoLoadPatches':False,'OverrideSaveDataFolder':True,'SaveDataFolder':str(p/'saves')},
                'Snes':{'Port1':{'Type':'SnesController'},'Port2':{'Type':'None'},
                  'EnableRandomPowerOnState':False,'RamPowerOnState':'AllZeros','DisableFrameSkipping':True}}
    exact(m['settings'], settings)

def build_envelope(m):
    check(set(m) == {'schema','compiler_exit','sources','executable'}, 'build envelope keys')
    for value in [m['executable'], *m['sources'].values()]:
        check(set(value) == {'path','sha256'} and type(value['path']) is str,
              'build identity envelope keys')

def state_file(path):
    pairs = [line.split('=',1) for line in path.read_text().splitlines()]
    check(all(len(pair) == 2 for pair in pairs), 'state line shape')
    return unique_object(pairs)

def normal_spc_state(s):
    # Caller preconditions, not a replacement for the hardware clock owner.
    exact({k:s[k] for k in ('spc.internalSpeed','spc.externalSpeed','spc.writeEnabled')},
          {'spc.internalSpeed':'0','spc.externalSpeed':'0','spc.writeEnabled':'true'})

def resident_terminal(instructions, rows):
    """Require exactly one final unresolved read, after only its two fetches.

    $048B MOV A,FD and $0622 MOV F3,A both fetch opcode/DP byte first.
    The pending timer/DSP data cycle is external and must not be accepted.
    Completed-instruction durations are independently compared to native entries.
    """
    check(rows and rows[0]['kind'] == 'instruction' and rows[-1]['kind'] == 'stop',
          'required initial instruction and final stop')
    check(sum(row['kind'] == 'stop' for row in rows) == 1, 'exactly one stop')
    cycles = [row for row in rows if row['kind'] == 'cycle']
    for i, ins in enumerate(instructions):
        final = i == len(instructions)-1
        end = len(cycles) if final else instructions[i+1]['cycles']
        group = cycles[ins['cycles']:end]
        check(group, 'empty instruction cycles')
        for j, row in enumerate(group):
            check(row['end'] is (not final and j == len(group)-1),
                  'instruction_end does not match actual completion boundary')
        if final:
            stop = rows[-1]
            check(ins['pc'] in (0x48b,0x622) and len(group) == 2,
                  'pending hardware instruction must accept exactly two fetches')
            check(stop['cycles'] == ins['cycles']+2 and stop['phase'] == 2,
                  'pending stop cycle/phase')
            check(all(stop[k] == ins[k] for k in ('pc','a','x','y','sp','ps')),
                  'pending fetches must preserve registers')
