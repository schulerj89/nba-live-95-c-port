"""Attested native-memory resources; rendered image files are never opened.

The accepted script is the reviewed no-input, no-state-write cold-boot capture.
This verifies its identity and actual isolation artifacts, rather than trusting
an `accepted_capture` boolean supplied by a manifest.
"""
import hashlib
import json
from pathlib import Path

ROM_SHA256 = '2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
MESEN_SHA256 = 'd2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b'
SCRIPT_SHA256 = 'd8bb668ccffbbe6346b9d6720be0236820a40135c217a13c5e020569f9f1f1d7'
SCOPE = 'natural cold boot with no inputs; synchronous native RGB; investigation only'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def strict_json(raw):
    def pairs(items):
        result = {}
        for key, value in items:
            if key in result:
                raise ValueError('duplicate JSON key: ' + key)
            result[key] = value
        return result
    return json.loads(raw, object_pairs_hook=pairs)


def require_equal(actual, expected, label):
    if type(actual) is not type(expected):
        raise ValueError('wrong type: ' + label)
    if isinstance(expected, dict):
        for key, value in expected.items():
            if key not in actual:
                raise ValueError('missing setting: ' + label + '.' + key)
            require_equal(actual[key], value, label + '.' + key)
    elif actual != expected:
        raise ValueError('changed native provenance: ' + label)


class IntroResources:
    def __init__(self, directory):
        self.directory = Path(directory).resolve()
        self.raw_manifest = (self.directory / 'manifest.json').read_bytes()
        self.manifest = strict_json(self.raw_manifest)
        self.consumed = {}
        m = self.manifest
        require_equal(m, dict(scope=SCOPE, rom_sha256=ROM_SHA256,
            mesen_sha256=MESEN_SHA256, script_sha256=SCRIPT_SHA256,
            exit_code=0, accepted_capture=True), 'manifest')
        isolation = m['isolation']
        require_equal(isolation, dict(method='private portable executable/settings',
            initial_saves=[], post_settings_verified=True), 'isolation')
        runtime = self.directory / 'portable-mesen'
        if Path(isolation['home']).resolve() != runtime or \
                Path(isolation['save_folder']).resolve() != self.directory / 'isolated-saves':
            raise ValueError('capture did not use its own runtime/save directory')
        script = self.read('capture.lua')
        if sha(script) != SCRIPT_SHA256:
            raise ValueError('unreviewed capture script')
        initial = self.read('initial-mesen-settings.json')
        current = (runtime / 'settings.json').read_bytes()
        if sha(initial) != isolation['initial_settings_sha256'] or \
                sha(current) != isolation['post_settings_sha256'] or \
                sha((runtime / 'Mesen.exe').read_bytes()) != MESEN_SHA256:
            raise ValueError('changed Mesen executable/settings artifacts')
        expected = {'Debug': {'ScriptWindow': {'AllowIoOsAccess': True,
            'ScriptTimeout': 60, 'SaveScriptBeforeRun': False}},
            'Preferences': {'SingleInstance': False, 'PauseWhenInBackground': False,
                'AutoLoadPatches': False, 'OverrideSaveDataFolder': True,
                'SaveDataFolder': str(self.directory / 'isolated-saves')},
            'Snes': {'Port1': {'Type': 'SnesController'}, 'Port2': {'Type': 'None'},
                'DisableFrameSkipping': True, 'EnableRandomPowerOnState': False,
                'RamPowerOnState': 'AllZeros', 'ForceFixedResolution': False,
                'Overscan': {'Top': 7, 'Bottom': 8, 'Left': 0, 'Right': 0}},
            'Video': {'VideoFilter': 'None', 'AspectRatio': 'NoStretching',
                'Brightness': 0, 'Contrast': 0, 'Hue': 0, 'Saturation': 0,
                'ScanlineIntensity': 0, 'UseBilinearInterpolation': False,
                'ScreenRotation': 'None'}}
        require_equal(isolation['settings'], expected, 'declared settings')
        require_equal(strict_json(initial), expected, 'initial settings')
        require_equal(strict_json(current), expected, 'observed settings')
        observed = self.read('observed-script-data-folder.txt').decode().strip()
        if observed != isolation['observed_script_data_folder'] or not \
                Path(observed).resolve().is_relative_to(runtime / 'LuaScriptData'):
            raise ValueError('capture ran under a different Mesen home')
        if self.read('complete.txt') != b'1500\n':
            raise ValueError('incomplete cold-boot capture')

    def read(self, name, size=None):
        if Path(name).name != name or name in ('.', '..'):
            raise ValueError('resource must be a capture-local filename')
        data = (self.directory / name).read_bytes()
        record = self.manifest['artifacts'][name]
        if type(record.get('size')) is not int or len(data) != record['size'] or \
                (size is not None and len(data) != size) or sha(data) != record.get('sha256'):
            raise ValueError('native resource hash/size mismatch: ' + name)
        self.consumed[name] = record
        return data
