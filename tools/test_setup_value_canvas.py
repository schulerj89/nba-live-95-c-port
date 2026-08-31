"""Native witness selection/integrity tests; independent replay is separate."""
from pathlib import Path
import unittest
from normalize_setup_config import read_compact,sha
from verify_setup_main_canvas import witnesses

ROOT=Path(__file__).resolve().parents[1]

class ValueCanvasWitnessTests(unittest.TestCase):
    def test_main_fixture_identity_and_every_value_observed(self):
        path=ROOT/'tests/fixtures/setup-config-main-visual-native-witnesses.json'
        self.assertEqual(sha(path),'1a21d6c113993e4cfea995a1907e876127b391077940aea1f169a008846607f9')
        cases=witnesses(read_compact(path)[0])
        self.assertEqual([row['action'] for row in cases],list(range(40)))
        for field,count in enumerate((4,3,3,4)):
            self.assertEqual({row['input'][field] for row in cases},set(range(count)))
        self.assertEqual(cases[0]['input'],[0,0,0,3])

    def test_rules_factory_multi_off_and_reentries_are_retained(self):
        path=ROOT/'tests/fixtures/setup-config-faces-native-witnesses.json'
        self.assertEqual(sha(path),'fd7ef6387e0957f2e7db7d3b795c414eeaa8e96851cfd76542173ce5e38fe5c2')
        cases=witnesses(read_compact(path)[0],'rules')
        self.assertEqual([row['action'] for row in cases],[5,11,17,23,29,35,41])
        self.assertTrue(all(row['input']==[0,0,1,0,0,0,0,0,1,1,1,0,0] for row in cases))
        self.assertTrue(all(len(row['native_sha256'])==64 for row in cases))

if __name__=='__main__':unittest.main()
