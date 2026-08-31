"""Adversarial protocol tests, not native/C parity by themselves."""
import copy
import json
from pathlib import Path
import unittest

import verify_setup_config_adjustments as v
from run_mesen_isolated import environment

ROOT=Path(__file__).resolve().parents[1]


class AdjustmentProtocolTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.journey=v.read_compact(ROOT/'tests/fixtures/setup-config-native-witnesses.json')[3]
        cls.rows=v.native_adjustments(cls.journey)

    def output(self,rows):return '\n'.join('CONFIG_ADJUST '+json.dumps(row) for row in rows)

    def test_native_protocol_double_is_accepted(self):
        # Expected-only protocol double: never reported as a C parity run.
        rows=v.parse_adjustments(self.output(self.rows),self.journey['actions'])
        self.assertEqual(v.compare(self.rows,rows),[])
        self.assertEqual(len(rows),540)

    def test_every_event_time_and_value_are_exact(self):
        for index in range(len(self.rows)):
            for field in ('offset','value'):
                rows=list(self.rows);rows[index]=dict(rows[index]);rows[index][field]^=1
                issues=v.compare(self.rows,rows)
                self.assertEqual(issues,[dict(event=index,field=field,
                    native=self.rows[index][field],C=rows[index][field])])

    def test_each_scalar_and_each_array_word_at_all_pc_types_are_compared(self):
        representatives={row['pc']:index for index,row in enumerate(self.rows)}
        self.assertEqual(set(representatives),set(v.ENTRY)|v.EXIT)
        for index in representatives.values():
            for field in v.FIELDS:
                words=range(len(self.rows[index][field])) if type(self.rows[index][field]) is list else [None]
                for word in words:
                    rows=list(self.rows);rows[index]=copy.deepcopy(rows[index])
                    if word is None:rows[index][field]^=1
                    else:rows[index][field][word]^=1
                    issues=v.compare(self.rows,rows)
                    self.assertEqual(len(issues),1)
                    self.assertEqual((issues[0]['event'],issues[0]['field']),(index,field))

    def test_missing_extra_reordered_and_boolean_records_fail(self):
        for kind in ('missing','extra','order','boolean','array_short','array_long','field','pc','command','time'):
            rows=copy.deepcopy(self.rows)
            if kind=='missing':rows.pop()
            elif kind=='extra':rows.append(copy.deepcopy(rows[-1]))
            elif kind=='order':rows[0],rows[-1]=rows[-1],rows[0]
            elif kind=='boolean':rows[0]['repeat_input']=False
            elif kind=='array_short':rows[0]['working'].pop()
            elif kind=='array_long':rows[0]['main'].append(0)
            elif kind=='field':del rows[0]['previous_input']
            elif kind=='pc':rows[0]['pc']=0
            elif kind=='command':rows[0]['command']=0x300
            else:rows[0]['offset']=self.journey['actions'][rows[0]['action']-1]['wait']
            with self.subTest(kind=kind):
                try:parsed=v.parse_adjustments(self.output(rows),self.journey['actions'])
                except ValueError:continue
                self.assertTrue(v.compare(self.rows,parsed))

    def test_duplicate_keys_and_invalid_json_fail(self):
        for line in ('CONFIG_ADJUST {"action":1,"action":2}','CONFIG_ADJUST nope'):
            with self.assertRaises(ValueError):v.parse_adjustments(line,self.journey['actions'])

    def test_missing_native_entry_exit_or_wrong_frame_cannot_be_projected(self):
        for kind in ('entry','exit','frame','action'):
            journey=copy.deepcopy(self.journey)
            indices=[i for i,e in enumerate(journey['events']) if e['pc'] in set(v.ENTRY)|v.EXIT]
            if kind=='entry':journey['events'].pop(indices[0])
            elif kind=='exit':journey['events'].pop(indices[1])
            elif kind=='frame':journey['events'][indices[1]]['frame']+=1
            else:journey['events'][indices[1]]['action']+=1
            with self.subTest(kind=kind),self.assertRaises(ValueError):v.native_adjustments(journey)

    def test_explicit_capture_environment_does_not_share_capture_variables(self):
        inherited={'PATH':'system','NBA95_CAPTURE_DIR':'wrong','NBA95_CAPTURE_MENU':'other'}
        one=environment(inherited,['NBA95_CAPTURE_DIR=one','NBA95_CONFIG_JOURNEY=faces'])
        two=environment(inherited,['NBA95_CAPTURE_DIR=two','NBA95_CONFIG_JOURNEY=input'])
        self.assertEqual(inherited['NBA95_CAPTURE_DIR'],'wrong')
        self.assertEqual(one,{'PATH':'system','NBA95_CAPTURE_DIR':'one','NBA95_CONFIG_JOURNEY':'faces'})
        self.assertEqual(two['NBA95_CAPTURE_DIR'],'two')
        for pairs in (['PATH=x'],['NBA95_CAPTURE_DIR=x','NBA95_CAPTURE_DIR=y'],['NBA95_CAPTURE_DIR']):
            with self.assertRaises(ValueError):environment(inherited,pairs)


if __name__=='__main__':unittest.main()
