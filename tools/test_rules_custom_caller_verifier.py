"""Adversarial protocol tests, not C gameplay or native parity results."""
import copy
import json
from pathlib import Path
import subprocess
import unittest
from unittest.mock import patch

import verify_rules_custom_caller as verifier


class CustomCallerProtocolTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.fixture=Path(__file__).resolve().parents[1]/'tests/fixtures/setup-config-native-witnesses.json'
        cls.journeys=verifier.read_compact(cls.fixture)
        journeys={j['name']:j for j in cls.journeys}
        cls.rom_sha=cls.journeys[0]['native_manifest']['sources']['rom']['sha256']
        cls.rows=[];cls.payloads=[]
        for name,page,first,last in (('presets-v2','rules',13,16),
                                    ('rules-v2','rules',7,8),
                                    ('options-v2','options',7,9)):
            journey=journeys[name];before=journey['states'][2*first-1]
            actions=journey['actions'][first-1:last]
            expected=[before]+[journey['states'][2*i] for i in range(first,last+1)]
            rows=[]
            for i,row in enumerate(expected):
                menu=0 if i and actions[i-1]['key']=='start' else 1 if page=='rules' else 2
                count=4 if menu==0 else 13 if page=='rules' else 7
                rows.append(dict(action=i,scene=5,page=menu,row=row['row'],
                    **{field:copy.deepcopy(row[field]) for field in ('main','rules','options')},
                    working=copy.deepcopy(row['working'][:count])))
            cls.rows.append(rows)
            seed=before['main']+before['rules']+before['options']
            cls.payloads.append(' '.join(map(str,seed))+'\n'+''.join(
                f"{a['key']} {a['hold']} {a['wait']}\n" for a in actions))

    def run_double(self,rows):
        # These are ROM-observation protocol doubles, never a C parity claim.
        completed=[subprocess.CompletedProcess([],0,stdout='\n'.join(
            'CONFIG_STATE '+json.dumps(row) for row in case)) for case in rows]
        with patch.object(verifier,'read_compact',return_value=self.journeys), \
                patch.object(verifier,'sha',return_value=self.rom_sha), \
                patch.object(verifier.subprocess,'run',side_effect=completed) as call:
            report=verifier.verify(self.fixture,'probe','rom','pack')
        return report,call.call_args_list

    def test_independent_native_protocol_double_and_only_prestate_inputs(self):
        report,calls=self.run_double(self.rows)
        self.assertEqual(report['result'],'PASS')
        for call,payload in zip(calls,self.payloads):
            self.assertEqual(call.kwargs['input'],payload)
            self.assertTrue(call.kwargs['check'])
            self.assertEqual(call.kwargs['timeout'],60)

    def test_every_committed_and_working_word_at_every_snapshot_is_checked(self):
        checked=0
        for case in range(3):
            for snapshot,row in enumerate(self.rows[case]):
                for field in ('main','rules','options','working'):
                    for word in range(len(row[field])):
                        rows=copy.deepcopy(self.rows);rows[case][snapshot][field][word]^=1
                        report,_=self.run_double(rows)
                        self.assertEqual(report['result'],'FAIL')
                        self.assertEqual(len(report['cases'][case]['issues']),1)
                        checked+=1
        self.assertEqual(checked,411)

    def test_scene_page_row_are_checked(self):
        for field in ('scene','page','row'):
            rows=copy.deepcopy(self.rows);rows[0][0][field]^=1
            with self.subTest(field=field):
                self.assertEqual(self.run_double(rows)[0]['result'],'FAIL')

    def test_incomplete_reordered_duplicate_or_extra_snapshots_fail(self):
        for kind in ('missing','reordered','duplicate','extra'):
            rows=copy.deepcopy(self.rows)
            if kind=='missing':rows[0].pop()
            elif kind=='reordered':rows[0][0],rows[0][1]=rows[0][1],rows[0][0]
            elif kind=='duplicate':rows[0][1]=rows[0][0]
            else:rows[0].append(dict(rows[0][-1],action=len(rows[0])))
            with self.subTest(kind=kind),self.assertRaises(ValueError):self.run_double(rows)

    def test_malformed_types_fields_ranges_and_arrays_fail(self):
        for kind in ('field_missing','field_extra','array_short','array_extra',
                     'array_bool','array_negative','array_wide','array_string','action_bool','scene_string'):
            rows=copy.deepcopy(self.rows);row=rows[0][0]
            if kind=='field_missing':del row['working']
            elif kind=='field_extra':row['unexpected']=1
            elif kind=='array_short':row['working'].pop()
            elif kind=='array_extra':row['working'].append(0)
            elif kind=='array_bool':row['working'][0]=True
            elif kind=='array_negative':row['working'][0]=-1
            elif kind=='array_wide':row['working'][0]=65536
            elif kind=='array_string':row['working'][0]='45'
            elif kind=='action_bool':row['action']=False
            else:row['scene']='5'
            with self.subTest(kind=kind),self.assertRaises(ValueError):self.run_double(rows)

    def test_failed_or_timed_out_process_cannot_pass(self):
        for error in (subprocess.CalledProcessError(1,['probe']),subprocess.TimeoutExpired(['probe'],60)):
            with patch.object(verifier,'read_compact',return_value=self.journeys), \
                    patch.object(verifier,'sha',return_value=self.rom_sha), \
                    patch.object(verifier.subprocess,'run',side_effect=error), \
                    self.assertRaises(type(error)):
                verifier.verify(self.fixture,'probe','rom','pack')


if __name__=='__main__':unittest.main()
