"""Adversarial checks on C replay protocol; expected values remain native-owned."""
import copy
import json
from pathlib import Path
import subprocess
import unittest
from unittest.mock import patch

import verify_setup_config_runtime as verifier


ROOT=Path(__file__).resolve().parents[1]
FIXTURE=ROOT/'tests/fixtures/setup-config-native-witnesses.json'


class ConfigurationReplayProtocolTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.journey=verifier.read_compact(FIXTURE)[0]
        cls.native=[cls.journey['states'][0]]+cls.journey['states'][2::2]
        # Protocol double copied from actual ROM observations. It is not a C
        # parity proof: only rejection/coverage behavior is tested here.
        cls.rows=verifier.native_projections(cls.journey)

    def output(self,rows):
        return '\n'.join('CONFIG_STATE '+json.dumps(row) for row in rows)

    def test_native_protocol_double_is_accepted(self):
        rows=verifier.parse_probe_output(self.output(self.rows),len(self.rows)-1)
        self.assertEqual(verifier.compare(self.journey,rows),[])

    def test_every_owned_word_at_every_boundary_is_compared(self):
        checked=0
        for action in range(len(self.rows)):
            for field in ('main','rules','options','custom','working'):
                for word in range(len(self.rows[action][field])):
                    rows=copy.deepcopy(self.rows)
                    rows[action][field][word]^=1
                    issues=verifier.compare(self.journey,rows)
                    self.assertEqual(len(issues),1)
                    self.assertEqual((issues[0]['action'],issues[0]['field'],issues[0]['index']),
                                     (action,field,word))
                    checked+=1
        self.assertEqual(checked,sum(37+len(row['working']) for row in self.rows))

    def test_wrong_scene_page_and_cursor_fail(self):
        for action in range(len(self.rows)):
            for field in ('scene','page','row'):
                rows=copy.deepcopy(self.rows)
                rows[action][field]^=1
                issues=verifier.compare(self.journey,rows)
                self.assertTrue(any(issue['action']==action and issue['field']==field for issue in issues))

    def test_unproven_match_handoff_fails(self):
        journey=copy.deepcopy(self.journey)
        journey['events']=[e for e in journey['events'] if e['pc']!=0x81bf59]
        with self.assertRaises(ValueError):verifier.native_projections(journey)

    def test_action_labels_do_not_choose_expected_page(self):
        journey=copy.deepcopy(self.journey)
        for action in journey['actions']:action['label']='incorrect_label'
        self.assertEqual(verifier.native_projections(journey),self.rows)

    def test_missing_extra_reordered_or_duplicate_rows_fail(self):
        for kind in ('missing','extra','reordered','duplicate'):
            rows=copy.deepcopy(self.rows)
            if kind=='missing':rows.pop()
            elif kind=='extra':rows.append(dict(rows[-1],action=len(rows)))
            elif kind=='reordered':rows[0],rows[1]=rows[1],rows[0]
            else:rows[1]['action']=0
            with self.subTest(kind=kind),self.assertRaises(ValueError):
                verifier.parse_probe_output(self.output(rows),len(self.rows)-1)

    def test_malformed_schema_types_ranges_or_word_populations_fail(self):
        for kind in ('field_missing','field_extra','array_short','array_extra',
                     'array_boolean','array_negative','array_wide','array_string',
                     'action_boolean','row_string'):
            rows=copy.deepcopy(self.rows)
            row=rows[0]
            if kind=='field_missing':del row['rules']
            elif kind=='field_extra':row['unused']=0
            elif kind=='array_short':row['rules'].pop()
            elif kind=='array_extra':row['rules'].append(0)
            elif kind=='array_boolean':row['options'][0]=True
            elif kind=='array_negative':row['options'][0]=-1
            elif kind=='array_wide':row['options'][0]=65536
            elif kind=='array_string':row['options'][0]='30'
            elif kind=='action_boolean':row['action']=False
            else:row['row']='0'
            with self.subTest(kind=kind),self.assertRaises(ValueError):
                verifier.parse_probe_output(self.output(rows),len(self.rows)-1)

    def test_duplicate_json_keys_and_invalid_json_fail(self):
        for output in ('CONFIG_STATE {"action":0,"action":1}',
                       'CONFIG_STATE not-json'):
            with self.subTest(output=output),self.assertRaises(ValueError):
                verifier.parse_probe_output(output,0)

    def test_expected_values_never_reach_c_process(self):
        actions=[dict(key='left',hold=3,wait=60),dict(key='right',hold=180,wait=210)]
        completed=subprocess.CompletedProcess([],0,stdout=self.output(self.rows[:3]))
        with patch.object(verifier.subprocess,'run',return_value=completed) as call:
            verifier.run_probe('probe','rom','pack',actions)
        args,kwargs=call.call_args
        self.assertEqual(args,(['probe','rom','pack'],))
        self.assertEqual(kwargs['input'],'left 3 60\nright 180 210\n')
        self.assertTrue(kwargs['check'])
        self.assertEqual(kwargs['timeout'],60)

    def test_process_failure_cannot_produce_pass_rows(self):
        for error in (subprocess.CalledProcessError(1,['probe'],output=self.output(self.rows)),
                      subprocess.TimeoutExpired(['probe'],60)):
            with self.subTest(error=type(error).__name__), \
                    patch.object(verifier.subprocess,'run',side_effect=error), \
                    self.assertRaises(type(error)):
                verifier.run_probe('probe','rom','pack',self.journey['actions'])


if __name__=='__main__':unittest.main()
