"""Protocol/evidence rejection tests; C parity is verify_audio_events.py."""
import copy,json,unittest
from pathlib import Path
import normalize_audio_events as native
import verify_audio_events as verifier
ROOT=Path(__file__).resolve().parents[1]
FIXTURE=ROOT/'tests/fixtures/audio-events-native-witnesses.json'

class VerifierIntegrity(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.fixture=native.read_fixture(FIXTURE);cls.rows=verifier.cases(cls.fixture)
    def output(self,rows=None):
        rows=rows or self.rows
        return [dict(id=i,output=c['output'],returns_consumed=len(c['external_command_returns_a']),operations=c['operations'])
                for i,(_,c) in enumerate(rows)]
    @staticmethod
    def text(rows):return '\n'.join(json.dumps(r) for r in rows)+'\n'
    def test_native_population_and_provenance_not_inferred_from_C(self):
        self.assertEqual([len(c['cases']) for c in self.fixture['captures']],[2206,406])
        self.assertEqual(sum(c['provenance']=='controlled' for _,c in self.rows),49)
        self.assertTrue(any(c['provenance']=='post-controlled-continuation' for _,c in self.rows))
        self.assertEqual(sum(len(c['operations']) for _,c in self.rows),173)
    def test_every_state_and_operation_word_is_compared(self):
        rows=[next(row for row in self.rows if len(row[1]['operations'])==19)]
        for index in range(3):
            output=copy.deepcopy(self.output(rows));output[0]['output'][index]^=1
            self.assertEqual(verifier.compare(rows,self.text(output))['result'],'FAIL')
        for op in range(19):
            for index in range(9):
                output=copy.deepcopy(self.output(rows));output[0]['operations'][op][index]^=1
                with self.subTest(op=op,index=index):
                    self.assertEqual(verifier.compare(rows,self.text(output))['result'],'FAIL')
    def test_parser_rejects_missing_extra_reordered_and_duplicate_outputs(self):
        base=self.output(self.rows[:3]);rows=self.rows[:3]
        for change in ('missing','extra','order','boolean','negative','field','short_operation','extra_operation_field'):
            output=copy.deepcopy(base)
            if change=='missing':output.pop()
            elif change=='extra':output.append(copy.deepcopy(output[-1]))
            elif change=='order':output.reverse()
            elif change=='boolean':output[0]['output'][0]=False
            elif change=='negative':output[0]['output'][0]=-1
            elif change=='field':output[0]['ignored']=0
            elif change=='short_operation':output[0]['operations']=[[0]*8]
            else:output[0]['operations']=[[0]*10]
            with self.subTest(change=change),self.assertRaises(ValueError):verifier.compare(rows,self.text(output))
        text=self.text(base).replace('"id": 0','"id": 0, "id": 0',1)
        with self.assertRaises(ValueError):verifier.compare(rows,text)
    def test_changed_fixture_projection_fails_independent_hash(self):
        data=copy.deepcopy(self.fixture);data['captures'][0]['cases'][0]['input'][0]^=1
        with self.assertRaisesRegex(ValueError,'projection changed'):native.validate(data)
    def test_mutated_provenance_exit_settings_and_source_identities_rejected(self):
        changes={
            'ROM patch':lambda m:m.update(rom_patch=True),
            'CPU injection':lambda m:m.update(cpu_register_injection=True),
            'failed exit':lambda m:m.update(exit_code=1),
            'boolean exit':lambda m:m.update(exit_code=False),
            'unknown classification':lambda m:m.update(mode='fake C run'),
            'relabel controls':lambda m:m.update(interventions='none'),
            'fake ROM':lambda m:m['sources']['rom'].update(sha256='0'*64),
            'malformed hash':lambda m:m['sources']['script'].update(sha256='bad'),
            'changed script':lambda m:m['sources']['script'].update(sha256='0'*64),
            'private home':lambda m:m['isolation'].update(home='C:/other'),
            'observed home':lambda m:m['isolation'].update(observed_script_data_folder='C:/other/LuaScriptData/capture'),
            'filter':lambda m:m['isolation']['settings']['Video'].update(VideoFilter='Ntsc'),
            'saved settings':lambda m:m['isolation'].update(post_settings_verified=False),
        }
        for name,change in changes.items():
            data=copy.deepcopy(self.fixture);change(data['captures'][1]['native_manifest'])
            with self.subTest(name=name),self.assertRaises(ValueError):native.validate(data)
    def test_retained_raw_manifest_is_hashed_and_decodes_to_same_metadata(self):
        data=copy.deepcopy(self.fixture);data['captures'][0]['native_manifest_raw_utf8']+=' '
        with self.assertRaises(ValueError):native.validate(data)
    def test_external_return_words_never_include_expected_call_or_output_data(self):
        rows=[next(row for row in self.rows if len(row[1]['operations'])==19)]
        numbers=[int(x) for x in verifier.input_text(rows).split()]
        self.assertEqual(numbers,[0,*rows[0][1]['input'],len(rows[0][1]['external_command_returns_a']),*rows[0][1]['external_command_returns_a']])
if __name__=='__main__':unittest.main()
