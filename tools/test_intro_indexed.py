"""Adversarial provenance and asset-header checks for indexed intro resources.

These are harness-integrity tests, separate from independent ROM pixel parity.
They do not update or fabricate expected game outputs.
"""
import argparse
import copy
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest
from unittest.mock import patch
from intro_capture_resources import IntroResources
from build_intro_indexed import build, build_text

NATIVE = ROM = PROBE = None


class IntroIntegrity(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.original_read = staticmethod(Path.read_bytes)
        cls.manifest_path = NATIVE/'manifest.json'
        cls.manifest = json.loads(cls.original_read(cls.manifest_path))
        cls.ea, _ = build(ROM,NATIVE)
        cls.text, _ = build_text(ROM,NATIVE)

    def read_with_manifest(self, raw):
        original = self.original_read
        def read(path):
            return raw if path.resolve()==self.manifest_path.resolve() else original(path)
        return patch.object(Path,'read_bytes',read)

    def test_valid_attestation(self):
        IntroResources(NATIVE)

    def test_reject_changed_attestation(self):
        cases = [
            (('accepted_capture',),1), (('exit_code',),False), (('exit_code',),1),
            (('scope',),'controlled state injection'), (('rom_sha256',),'0'*64),
            (('mesen_sha256',),'0'*64), (('script_sha256',),'0'*64),
            (('isolation','initial_saves'),['prior.srm']),
            (('isolation','post_settings_verified'),False),
            (('isolation','method'),'user profile'),
            (('isolation','initial_settings_sha256'),'0'*64),
            (('isolation','post_settings_sha256'),'0'*64),
            (('isolation','settings','Snes','DisableFrameSkipping'),False),
            (('isolation','settings','Snes','EnableRandomPowerOnState'),True),
            (('isolation','settings','Preferences','AutoLoadPatches'),True),
            (('isolation','settings','Video','Brightness'),1),
            (('artifacts','capture.lua','size'),True),
            (('artifacts','complete.txt','sha256'),'0'*64)]
        for path,value in cases:
            with self.subTest(field=path):
                manifest=copy.deepcopy(self.manifest);target=manifest
                for key in path[:-1]:target=target[key]
                target[path[-1]]=value
                with self.read_with_manifest(json.dumps(manifest).encode()):
                    with self.assertRaises(ValueError):IntroResources(NATIVE)

    def test_reject_duplicate_keys(self):
        raw=json.dumps(self.manifest)
        raw=raw.replace('"exit_code": 0','"exit_code": 1, "exit_code": 0')
        with self.read_with_manifest(raw.encode()):
            with self.assertRaises(ValueError):IntroResources(NATIVE)

    def test_reject_altered_script_even_with_rehashed_manifest(self):
        from intro_capture_resources import sha
        script=self.original_read(NATIVE/'capture.lua')+b'\n-- changed\n'
        manifest=copy.deepcopy(self.manifest)
        manifest['artifacts']['capture.lua']={'size':len(script),'sha256':sha(script)}
        original=self.original_read
        def read(path):
            if path.resolve()==self.manifest_path:return json.dumps(manifest).encode()
            if path.resolve()==NATIVE/'capture.lua':return script
            return original(path)
        with patch.object(Path,'read_bytes',read):
            with self.assertRaises(ValueError):IntroResources(NATIVE)

    def invoke(self, asset_id, payload, width=0, height=0, flags=0):
        raw=struct.pack('<8sII6I',b'NBA95PAK',31,1,asset_id,40,len(payload),width,height,flags)+payload
        with tempfile.TemporaryDirectory(prefix='nba95-intro-gate-') as directory:
            path=Path(directory)/'test.pak';path.write_bytes(raw)
            return subprocess.run([str(PROBE),str(path)],capture_output=True).returncode

    def test_accept_current_resources(self):
        self.assertEqual(self.invoke(75,self.ea),0)
        self.assertEqual(self.invoke(76,self.text),0)

    def test_reject_every_header_byte_change(self):
        for asset_id,data,length in ((75,self.ea,32),(76,self.text,40)):
            for offset in range(length):
                with self.subTest(asset=asset_id,offset=offset):
                    altered=bytearray(data);altered[offset]^=128
                    self.assertEqual(self.invoke(asset_id,altered),1)

    def test_reject_bad_tilegroup_dimensions_and_text_termination(self):
        for asset_id,data,offsets in ((75,self.ea,range(71392,71398)),
                (75,self.ea,range(71578,71584)),(76,self.text,(4336,4567))):
            for offset in offsets:
                with self.subTest(asset=asset_id,offset=offset):
                    altered=bytearray(data);altered[offset]^=1
                    self.assertEqual(self.invoke(asset_id,altered),1)

    def test_reject_bad_metadata_and_lengths(self):
        for asset_id,data in ((75,self.ea),(76,self.text)):
            for payload,w,h,f in ((data[:-1],0,0,0),(data+b'\0',0,0,0),
                    (data,1,0,0),(data,0,1,0),(data,0,0,1)):
                with self.subTest(asset=asset_id,size=len(payload),metadata=(w,h,f)):
                    self.assertEqual(self.invoke(asset_id,payload,w,h,f),1)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native',required=True,type=Path)
    parser.add_argument('--rom',required=True,type=Path)
    parser.add_argument('--probe',required=True,type=Path)
    args=parser.parse_args()
    NATIVE,ROM,PROBE=args.native.resolve(),args.rom.resolve(),args.probe.resolve()
    unittest.main(argv=['test_intro_indexed.py'])
