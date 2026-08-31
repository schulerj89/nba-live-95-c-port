"""Synthetic adversarial checks of verifier protocols, never ROM goldens."""
import copy
import io
import json
import subprocess
import sys
import unittest
from contextlib import redirect_stdout
from pathlib import Path
from unittest.mock import patch

import verify_inbound_arrival_vectors as arrival
import verify_inbound_motion_vectors as motion
from verify_inbound_internal import (SNAPSHOT_FIELDS, read_compact, run_probe,
                                    validate_capture_rows, verify)

ROOT=Path(__file__).resolve().parents[1]


class InboundProtocolTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.compact=ROOT/'tests/fixtures/inbound-internal-witnesses.json'
        cls.native_rows,_=read_compact(cls.compact)

    def test_motion_requires_the_complete_four_word_protocol(self):
        self.assertEqual(motion.parse_output('0001 0002 0003 0008\n',1),
                         [[1,2,3,8]])
        for text in ('','0001 0002 0003\n','0001 0002 0003 0008 junk\n',
                     '0001 0002 0003 0008\n\n',
                     '0001 0002 0003 0008\n0001 0002 0003 0008\n'):
            with self.subTest(text=text),self.assertRaises(ValueError):
                motion.parse_output(text,1)

    def test_motion_rejects_malformed_or_out_of_word_range_tokens(self):
        for token in ('-001','10000','GGGG','0x00','0','True'):
            with self.subTest(token=token),self.assertRaises(ValueError):
                motion.parse_output('0001 0002 0003 '+token+'\n',1)

    def test_arrival_missing_rows_never_prints_pass(self):
        fixture=json.loads((ROOT/'tests/fixtures/inbound-arrival-witnesses.json').read_text())
        argv=['verify','--vectors','unused.json','--probe','not-executed']
        output=io.StringIO()
        with patch.object(Path,'read_text',return_value=json.dumps(fixture)), \
             patch.object(sys,'argv',argv), \
             patch.object(arrival.subprocess,'run',return_value=
                          subprocess.CompletedProcess(argv,0,'','')), \
             redirect_stdout(output):
            with self.assertRaises(ValueError): arrival.main()
        self.assertNotIn('PASS',output.getvalue())

    def test_arrival_population_metadata_cannot_hide_missing_calls(self):
        fixture=json.loads((ROOT/'tests/fixtures/inbound-arrival-witnesses.json').read_text())
        fixture['calls']=[]
        with patch.object(Path,'read_text',return_value=json.dumps(fixture)), \
             patch.object(sys,'argv',['verify','--vectors','unused','--probe','unused']), \
             patch.object(arrival.subprocess,'run') as run:
            with self.assertRaises(AssertionError): arrival.main()
            run.assert_not_called()

    def test_internal_probe_requires_exact_rows_and_all_fields(self):
        for stdout in ('','1 2 3\n','1 2 3 4 5\n','1 2 3 10000\n',
                       '1 2 3 4\n1 2 3 4\n'):
            with self.subTest(stdout=stdout), \
                 patch('verify_inbound_internal.subprocess.run',return_value=
                       subprocess.CompletedProcess([],0,stdout,'')), \
                 self.assertRaises(ValueError):
                run_probe('not-executed',[[0]],4)

    def test_probe_failure_is_not_converted_into_a_pass(self):
        with patch('verify_inbound_internal.subprocess.run',side_effect=
                   subprocess.CalledProcessError(1,['not-executed'])), \
             self.assertRaises(subprocess.CalledProcessError):
            run_probe('not-executed',[[0]],4)

    def test_retained_fixture_lossless_schema_and_all_stages(self):
        self.assertEqual(len(self.native_rows),500)
        self.assertEqual(sum('prepared' in row for row in self.native_rows),389)
        self.assertEqual(sum(len(row)-2 for row in self.native_rows),3889)
        for row in self.native_rows:
            for stage,snapshot in row.items():
                if stage not in ('call','schema'):
                    self.assertEqual(set(snapshot),set(SNAPSHOT_FIELDS))

    def test_internal_missing_field_wrong_pc_boolean_and_time_rejected(self):
        for kind in ('field','stage','pc','boolean','time','call','extra_stage'):
            changed=copy.deepcopy(self.native_rows[:2])
            if kind=='field':del changed[0]['post_motion']['vx']
            elif kind=='stage':del changed[0]['velocity_entry']
            elif kind=='pc':changed[0]['post_motion']['pc']=0
            elif kind=='boolean':changed[0]['post_motion']['vx']=True
            elif kind=='time':changed[1]['entry']['frame']=0
            elif kind=='call':changed[1]['call']=1
            else:changed[0]['unrecognized']=changed[0]['entry']
            with self.subTest(kind=kind),self.assertRaises(ValueError):
                validate_capture_rows(changed)

    def test_every_motion_and_arrival_output_compared_including_direction(self):
        row=copy.deepcopy(next(row for row in self.native_rows if 'prepared' in row))
        row['call']=1
        expected_motion=[[row['post_motion'][key] for key in ('vx','vy','boost')]
                         +[row['velocity_entry']['dp_aa']]]
        expected_arrival=[[row['prepared'][key] for key in
            ('dead','attachment','flags','vx','vy','ready','whistle','event',
             'transfer','draw_direction')]]
        with patch('verify_inbound_internal.run_probe',side_effect=
                   [expected_motion,expected_arrival]):
            self.assertEqual(verify([row],'not-executed','not-executed')['issues'],[])
        for target,width in (('motion',4),('arrival',10)):
            for field in range(width):
                motion_output=copy.deepcopy(expected_motion)
                arrival_output=copy.deepcopy(expected_arrival)
                output=motion_output if target=='motion' else arrival_output
                output[0][field]^=1
                with self.subTest(target=target,field=field), \
                     patch('verify_inbound_internal.run_probe',side_effect=
                           [motion_output,arrival_output]):
                    self.assertEqual(verify([row],'not-executed','not-executed')
                                     ['result'],'FAIL')


if __name__=='__main__': unittest.main()
