"""Adversarial reader/protocol tests; native RGB expected values are immutable."""
import copy
from pathlib import Path
import subprocess
import unittest
from unittest.mock import patch

import verify_ppu_brightness as verifier

ROOT=Path(__file__).resolve().parents[1]


class BrightnessVerifierTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.rows,cls.rejected,cls.writes,cls.source=verifier.compact(
            ROOT/'tests/fixtures/ppu-brightness-witnesses.json')

    def test_native_grid_includes_all_channels_and_brightnesses(self):
        self.assertEqual(len(self.rows),1536)
        self.assertEqual(len(self.rejected),3)
        self.assertEqual(len(self.writes),43)
        self.assertEqual({(row['channel'],row['level'],row['brightness'])
                          for row in self.rows},
                         {(channel,level,brightness) for channel in range(3)
                          for level in range(32) for brightness in range(16)})

    def test_missing_extra_reordered_and_malformed_raster_records_fail(self):
        for mutation in ('missing','duplicate','reorder','frame','bool','sample',
                         'field','grid','reason','nonuniform'):
            rows=copy.deepcopy(self.rows)
            if mutation=='missing':rows.pop()
            elif mutation=='duplicate':rows[-1]=rows[0]
            elif mutation=='reorder':rows[0],rows[1]=rows[1],rows[0]
            elif mutation=='frame':rows[1]['frame']+=1
            elif mutation=='bool':rows[0]['level']=False
            elif mutation=='sample':rows[0]['samples'][0]=0x1000000
            elif mutation=='field':del rows[0]['main_layers']
            elif mutation=='grid':rows[0]['brightness']=1
            elif mutation=='reason':rows[0]['rejection_reason_bits']=64
            else:rows[0]['samples'][1]^=1
            with self.subTest(mutation=mutation),self.assertRaises(ValueError):
                verifier.validate(rows,self.rejected,self.writes)

    def test_retries_and_conflicts_cannot_disappear_or_be_reclassified(self):
        for mutation in ('missing_attempt','changed_input','reason','write',
                         'write_value','write_schema','write_bool'):
            rejected=copy.deepcopy(self.rejected);writes=copy.deepcopy(self.writes)
            if mutation=='missing_attempt':rejected.pop()
            elif mutation=='changed_input':rejected[0]['case']+=1
            elif mutation=='reason':rejected[0]['rejection_reason_bits']=0
            else:
                index=next(i for i,w in enumerate(writes) if w['conflict'])
                if mutation=='write':writes.pop(index)
                elif mutation=='write_value':writes[index]['injected_value']^=1
                elif mutation=='write_schema':del writes[index]['observed_pc']
                else:writes[index]['scanline']=False
            with self.subTest(mutation=mutation),self.assertRaises(ValueError):
                verifier.validate(self.rows,rejected,writes)

    def test_default_video_geometry_and_controller_configuration_required(self):
        for mutation in ('filter','brightness','random','overscan','controller','geometry'):
            source=copy.deepcopy(self.source)
            if mutation=='filter':source['video_settings']['VideoFilter']='Ntsc'
            elif mutation=='brightness':source['video_settings']['Brightness']=False
            elif mutation=='random':source['snes_settings']['EnableRandomPowerOnState']=True
            elif mutation=='overscan':source['snes_settings']['Overscan']['Top']=0
            elif mutation=='controller':source['snes_settings']['Port2']['Type']='SnesController'
            else:source['geometry']=[256,224]
            with self.subTest(mutation=mutation),self.assertRaises(ValueError):
                verifier.validate_settings(source['video_settings'],source['snes_settings'],
                                           source['sample_points'],source['geometry'])

    def test_exact_converter_output_protocol(self):
        for stdout in ('','0\n0\n','0 0\n','-1\n','True\n','16777216\n','0\n\n'):
            with self.subTest(stdout=stdout),patch.object(verifier.subprocess,'run',
                return_value=subprocess.CompletedProcess([],0,stdout,'')), \
                self.assertRaises(ValueError):
                verifier.replay(self.rows[:1],'not-executed')

    def test_failure_exit_is_not_converted_to_pass(self):
        with patch.object(verifier.subprocess,'run',side_effect=
            subprocess.CalledProcessError(1,['not-executed'])), \
            self.assertRaises(subprocess.CalledProcessError):
            verifier.replay(self.rows[:1],'not-executed')

    def test_all_three_output_channels_compared(self):
        row=self.rows[18*16+7]
        self.assertEqual(row['samples'][0],66<<16)
        expected=row['samples'][0]
        for shift in (0,8,16):
            with self.subTest(shift=shift),patch.object(verifier.subprocess,'run',
                return_value=subprocess.CompletedProcess([],0,str(expected^(1<<shift))+'\n','')):
                issues=verifier.replay([row],'not-executed')
                self.assertEqual(len(issues),1)
                self.assertEqual(issues[0][1],'C')

    def test_previous_eight_bit_formula_disagrees_with_native_witnesses(self):
        # A regression challenge, never a source for expected fixture values.
        mismatches=0
        for row in self.rows:
            old=((row['level']<<3)|(row['level']>>2))*row['brightness']//15
            native=(row['samples'][0]>>(16-row['channel']*8))&255
            mismatches+=old!=native
        self.assertEqual(mismatches,1122)


if __name__=='__main__':unittest.main()
