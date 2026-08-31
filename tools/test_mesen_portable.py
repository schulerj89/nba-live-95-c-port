"""Ensure an evidence capture cannot claim a different/shared Mesen home."""
import copy
import json
from pathlib import Path
import tempfile
import unittest

from mesen_portable import prepare, verify


class PortableCaptureTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        binary = self.root / 'installed.exe'
        binary.write_bytes(b'fake executable: never run by unit test')
        self.exe, self.identity = prepare(self.root, binary)
        self.observed = self.root / 'observed-script-data-folder.txt'
        self.observed.write_text(str(self.exe.parent / 'LuaScriptData' / 'capture'))

    def test_private_copy_and_actual_home_attestation(self):
        self.assertEqual(self.exe.read_bytes(), (self.root / 'installed.exe').read_bytes())
        self.assertFalse(self.identity['post_settings_verified'])
        result = verify(self.root, self.identity)
        self.assertTrue(result['post_settings_verified'])
        self.assertEqual(result['final_saves'], {})

    def test_other_home_missing_attestation_and_initial_setting_drift_fail(self):
        self.observed.write_text(str(self.root / 'unrelated-home' / 'LuaScriptData'))
        with self.assertRaises(ValueError): verify(self.root, self.identity)
        self.observed.unlink()
        with self.assertRaises(OSError): verify(self.root, self.identity)
        (self.root / 'initial-mesen-settings.json').write_text('{}')
        with self.assertRaises(ValueError): verify(self.root, self.identity)

    def test_every_critical_persisted_setting_is_checked(self):
        base = self.identity['settings']
        leaves = []
        def collect(node, path=()):
            for key, value in node.items():
                if isinstance(value, dict): collect(value, path + (key,))
                else: leaves.append((path + (key,), value))
        collect(base)
        for path, value in leaves:
            with self.subTest(path=path):
                changed = copy.deepcopy(base)
                parent = changed
                for key in path[:-1]: parent = parent[key]
                parent[path[-1]] = (not value) if type(value) is bool else 'changed'
                (self.exe.parent / 'settings.json').write_text(json.dumps(changed))
                with self.assertRaises(ValueError): verify(self.root, self.identity)


if __name__ == '__main__':
    unittest.main()
