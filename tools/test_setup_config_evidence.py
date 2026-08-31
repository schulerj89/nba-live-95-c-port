"""Native evidence schema/coverage checks, not C implementation parity tests."""
import copy
import json
from pathlib import Path
import unittest
from unittest.mock import patch

import normalize_setup_config as evidence

ROOT=Path(__file__).resolve().parents[1]
FIXTURE=ROOT/'tests/fixtures/setup-config-native-witnesses.json'
MAIN_FIXTURE=ROOT/'tests/fixtures/setup-config-main-native-witnesses.json'


class ConfigurationEvidenceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.journeys={j['name']:j for j in evidence.read_compact(FIXTURE)}

    def test_complete_immutable_corpus_population(self):
        expected={'presets-v2':29,'rules-v2':256,'options-v2':288,'held-v2':26,
                  'presets-reload-v2':1,'options-reload-v2':1}
        self.assertEqual({name:len(j['actions']) for name,j in self.journeys.items()},expected)
        self.assertEqual(sum(len(j['states']) for j in self.journeys.values()),1208)
        self.assertEqual(sum(len(j['events']) for j in self.journeys.values()),2556)

    def test_changed_native_word_fails_raw_hash_not_just_schema(self):
        for field in ('rules','main','options','working','sram48_56','repeat_delay'):
            fixture=evidence.read_json(FIXTURE)
            index=fixture['row_fields'].index(field)
            value=fixture['journeys'][0]['states'][0][index]
            if type(value) is list:value[0]^=1
            else:fixture['journeys'][0]['states'][0][index]^=1
            with self.subTest(field=field),patch.object(evidence,'read_json',return_value=fixture), \
                    self.assertRaisesRegex(ValueError,'raw trace'):
                evidence.read_compact('not-read')

    def test_missing_or_reordered_boundaries_and_input_duration_fail(self):
        for kind in ('row','order','boolean','field','pc','time','duration'):
            j=copy.deepcopy(self.journeys['presets-v2'])
            if kind=='row':j['states'].pop()
            elif kind=='order':j['states'][1],j['states'][2]=j['states'][2],j['states'][1]
            elif kind=='boolean':j['states'][0]['rules'][0]=False
            elif kind=='field':del j['events'][0]['repeat_delay']
            elif kind=='pc':j['events'][0]['pc']=0
            elif kind=='time':j['events'][1]['frame']=0
            else:j['actions'][0]['wait']+=1
            with self.subTest(kind=kind),self.assertRaises(ValueError):
                evidence.validate(j['actions'],j['states'],j['events'])

    def test_actual_native_sweeps_cover_all_bar_edges_in_both_directions(self):
        for name in ('rules-v2','options-v2'):
            coverage={(row['row'],row['direction']):{tuple(edge) for edge in row['edges']}
                      for row in evidence.summarize(self.journeys[name])['observed_working_edges']}
            for row in (0,1):
                self.assertEqual(coverage[row,'left'],{(value,max(0,value-1)) for value in range(46)})
                self.assertEqual(coverage[row,'right'],{(value,min(45,value+1)) for value in range(46)})
            maximum=12 if name=='rules-v2' else 6
            for row in range(2,maximum+1):
                states=3 if name=='options-v2' and row==2 else 2
                for key in ('left','right'):
                    self.assertEqual(len(coverage[row,key]),states)
                    self.assertEqual({a for a,b in coverage[row,key]},set(range(states)))
                    self.assertEqual({b for a,b in coverage[row,key]},set(range(states)))

    def test_reload_files_are_linked_to_preceding_natural_save_transactions(self):
        for prefix in ('presets','options'):
            origin=self.journeys[prefix+'-v2'];reload=self.journeys[prefix+'-reload-v2']
            self.assertEqual(origin['native_manifest']['final_save_files'][0]['sha256'],
                             reload['native_manifest']['initial_save_files'][0]['sha256'])
            final=origin['states'][-1];initial=reload['states'][0]
            for field in ('main','rules','options','sram48_56','sram_marker'):
                self.assertEqual(final[field],initial[field])
            self.assertEqual(initial['sram_marker'],0xda)
            self.assertIn(0x81c24b,{row['pc'] for row in reload['events']})
            self.assertNotIn(0x81c1a9,{row['pc'] for row in reload['events']})

    def test_clamp_press_marks_custom_without_premature_rules_commit(self):
        j=self.journeys['presets-v2'];before,after=j['states'][25:27]
        self.assertEqual(before['action'],13)
        self.assertEqual(before['working'][0],45)
        self.assertEqual(after['working'][0],45)
        self.assertEqual(before['main'][1],1)
        self.assertEqual(after['main'][1],2)
        self.assertEqual(before['rules'],after['rules'])
        # The next real decrement remains working until Start.
        after_edit=j['states'][30]
        self.assertEqual(after_edit['working'][0],44)
        self.assertEqual(after_edit['rules'][0],45)
        self.assertEqual(j['states'][32]['rules'][0],44)

    def test_every_main_value_cycles_both_directions_without_early_commit(self):
        j=evidence.read_compact(MAIN_FIXTURE)[0]
        self.assertEqual((len(j['actions']),len(j['states']),len(j['events'])),(44,89,63))
        coverage={(row['row'],row['direction']):{tuple(edge) for edge in row['edges']}
                  for row in evidence.summarize(j)['observed_working_edges']}
        for row,count in enumerate((4,3,3,4)):
            for key,delta in (('left',-1),('right',1)):
                self.assertEqual(coverage[row,key],{(value,(value+delta)%count) for value in range(count)})
        # All29 initial cycle/navigation actions preserve committed Main.
        for state in j['states'][:79]:self.assertEqual(state['main'],[0,0,0,3])
        self.assertEqual(j['states'][78]['working'][:4],[0,0,1,0])
        self.assertEqual(j['states'][80]['main'],[0,0,1,0])
        self.assertEqual([(j['states'][2*i-1]['row'],j['states'][2*i]['row'])
                          for i in (1,2,42,43)],[(0,5),(5,0),(0,5),(5,0)])

    def test_normal_options_start_actual_native_copy_boundaries_observed(self):
        j=evidence.read_compact(MAIN_FIXTURE)[0]
        rows=[row for row in j['events'] if row['pc'] in
              (0x828eb3,0x828ecb,0x828ed4,0x828ee4)]
        self.assertEqual([row['pc'] for row in rows],[0x828eb3,0x828ecb,0x828ed4,0x828ee4])
        self.assertTrue(all(row['action']==41 for row in rows))
        self.assertTrue(all(row['working'][:7]==row['options'] for row in rows))
        self.assertEqual(j['native_manifest']['process_exit_code'],0)


if __name__=='__main__':unittest.main()
