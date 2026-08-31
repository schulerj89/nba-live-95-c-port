"""Strict evidence/build/probe contracts for an isolated sound-prefix component."""
import argparse
import copy
import subprocess
import tempfile
from pathlib import Path
import unittest
from unittest.mock import patch
import verify_setup_sound_prefix as v


class Contracts(unittest.TestCase):
    def bad_manifest(self,change):
        bad=copy.deepcopy(self.manifest);change(bad)
        with self.assertRaises(ValueError):v.validate_capture_manifest(bad,self.native,self.rom)

    def test_every_source_required(self):
        for name in self.manifest['sources']:
            with self.subTest(name=name):self.bad_manifest(lambda m:m['sources'].pop(name))

    def test_every_artifact_required(self):
        for name in self.manifest['artifacts']:
            with self.subTest(name=name):self.bad_manifest(lambda m:m['artifacts'].pop(name))

    def test_capture_revisions_pinned(self):
        for name in v.CAPTURE_REVISIONS:
            self.bad_manifest(lambda m:m['sources'][name].update(sha256='0'*64))

    def test_numeric_domains(self):
        for value in (True,1.0,-1):self.bad_manifest(lambda m:m.update(schema=value))
        for value in (False,0.0):self.bad_manifest(lambda m:m.update(exit_code=value))

    def test_settings_identity_required(self):
        self.bad_manifest(lambda m:m['isolation'].pop('post_settings_sha256'))
        self.bad_manifest(lambda m:m['isolation']['settings']['Snes'].update(EnableRandomPowerOnState=True))
        old=v.digest
        with patch.object(v,'digest',side_effect=lambda p:'0'*64 if Path(p).name=='settings.json' else old(p)):
            with self.assertRaises(ValueError):v.read_native(self.native,self.rom)

    def test_every_build_source_required(self):
        for name in [None,*self.build['sources']]:
            bad=copy.deepcopy(self.build)
            if name is None:bad['sources']={}
            else:bad['sources'].pop(name)
            with patch.object(v,'read_json',return_value=bad):
                with self.assertRaises(ValueError):v.check_build(self.exe)

    def test_build_boolean_exit_rejected(self):
        bad=copy.deepcopy(self.build);bad['compiler_exit']=False
        with patch.object(v,'read_json',return_value=bad):
            with self.assertRaises(ValueError):v.check_build(self.exe)

    def test_probe_source_contracts(self):
        r=subprocess.run([str(self.exe),'--self-test',str(self.rom)],text=True,capture_output=True)
        self.assertEqual(r.returncode,0,r.stdout+r.stderr)
        self.assertIn('immutable unresolved SPC read',r.stdout)

    def test_invalid_numeric_tokens_reject_without_outputs(self):
        with tempfile.TemporaryDirectory(prefix='sound-prefix-contract-')as temp:
            trace=Path(temp)/'out.jsonl';wram=Path(temp)/'out.wram'
            for state in ('-0,0,0,8158,128,19','4294967296,0,0,8158,128,19','0g,0,0,8158,128,19',
                          '0,0,0,8158,128,19x','0,0,0,8158,128','0,0,0,8158,128,19,0'):
                r=subprocess.run([str(self.exe),str(self.rom),str(self.native/'sound_prefix_01_entry.wram'),state,str(trace),str(wram),'isolated-component-differential'],capture_output=True)
                self.assertEqual(r.returncode,2,state);self.assertFalse(trace.exists());self.assertFalse(wram.exists())


def main():
    p=argparse.ArgumentParser()
    for name in ('native','rom','exe'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();Contracts.native=a.native.resolve();Contracts.rom=a.rom.resolve();Contracts.exe=a.exe.resolve()
    Contracts.manifest=v.read_json(Contracts.native/'manifest.json');Contracts.build=v.check_build(Contracts.exe)
    v.read_native(Contracts.native,Contracts.rom)
    result=unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(Contracts))
    return 0 if result.wasSuccessful()else 1


if __name__=='__main__':raise SystemExit(main())
