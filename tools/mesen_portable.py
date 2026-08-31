"""Private Mesen configuration for evidence captures; never change user state."""
import hashlib
import json
from pathlib import Path
import shutil


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def prepare(output, installed):
    output = Path(output).resolve()
    runtime = output / 'portable-mesen'
    saves = output / 'isolated-saves'
    runtime.mkdir()
    saves.mkdir()
    executable = runtime / 'Mesen.exe'
    shutil.copyfile(installed, executable)
    settings = {
        'Debug': {'ScriptWindow': {'AllowIoOsAccess': True, 'ScriptTimeout': 60,
                                   'SaveScriptBeforeRun': False}},
        'Preferences': {'SingleInstance': False, 'PauseWhenInBackground': False,
                        'AutoLoadPatches': False, 'OverrideSaveDataFolder': True,
                        'SaveDataFolder': str(saves)},
        'Snes': {'Port1': {'Type': 'SnesController'}, 'Port2': {'Type': 'None'},
                 'DisableFrameSkipping': True, 'EnableRandomPowerOnState': False,
                 'RamPowerOnState': 'AllZeros', 'ForceFixedResolution': False,
                 'Overscan': {'Top': 7, 'Bottom': 8, 'Left': 0, 'Right': 0}},
        'Video': {'VideoFilter': 'None', 'AspectRatio': 'NoStretching', 'Brightness': 0,
                  'Contrast': 0, 'Hue': 0, 'Saturation': 0, 'ScanlineIntensity': 0,
                  'UseBilinearInterpolation': False, 'ScreenRotation': 'None'},
    }
    initial = output / 'initial-mesen-settings.json'
    initial.write_text(json.dumps(settings, indent=2) + '\n', encoding='utf-8')
    shutil.copyfile(initial, runtime / 'settings.json')
    return executable, dict(method='private portable executable/settings',
        home=str(runtime), save_folder=str(saves), initial_saves=[], settings=settings,
        initial_settings_sha256=sha(initial), post_settings_verified=False)


def verify(output, isolation):
    output = Path(output).resolve()
    runtime = output / 'portable-mesen'
    if sha(output / 'initial-mesen-settings.json') != isolation['initial_settings_sha256']:
        raise ValueError('initial Mesen settings changed during capture')
    loaded = json.loads((runtime / 'settings.json').read_text(encoding='utf-8-sig'))

    def subset(actual, expected, prefix=''):
        for key, value in expected.items():
            if key not in actual:
                raise ValueError('missing persisted Mesen setting: ' + prefix + key)
            if isinstance(value, dict):
                if not isinstance(actual[key], dict):
                    raise ValueError('invalid persisted Mesen settings group')
                subset(actual[key], value, prefix + key + '.')
            elif type(actual[key]) is not type(value) or actual[key] != value:
                raise ValueError('persisted Mesen setting differs: ' + prefix + key)

    subset(loaded, isolation['settings'])
    observed = (output / 'observed-script-data-folder.txt').read_text().strip()
    if not Path(observed).resolve().is_relative_to(runtime.resolve()):
        raise ValueError('Lua observed a different Mesen home')
    isolation.update(observed_script_data_folder=observed, post_settings_verified=True,
                     post_settings_sha256=sha(runtime / 'settings.json'),
                     final_saves={path.name: sha(path) for path in
                                  (output / 'isolated-saves').glob('*') if path.is_file()})
    return isolation
