"""Harness unit tests use synthetic rows, never claim emulator equivalence."""
import copy,json,subprocess,sys,tempfile,unittest
from pathlib import Path
from differential_compare import compare,load,schema

class DifferentialTests(unittest.TestCase):
    def setUp(self):
        self.fields=schema()
        self.rows=[dict(sequence=i,checkpoint=p,outer_frame=0 if i==0 else 2,
                        inputs=[0]*5,state={k:0 for k in self.fields},writers={})
                   for i,p in enumerate(('baseline','actors.begin','actors.end'))]
        self.key=next(iter(self.fields))

    def test_equal_projection(self):
        self.assertEqual(compare(self.rows,self.rows,1)['status'],'PROJECTION_MATCH')

    def test_explicit_routine_boundaries(self):
        rows=copy.deepcopy(self.rows[:2])
        rows[0]['checkpoint']='ball.init.entry';rows[1]['checkpoint']='ball.init.exit'
        plan=['ball.init.entry','ball.init.exit']
        self.assertEqual(compare(rows,rows,0,checkpoint_plan=plan)['status'],'PROJECTION_MATCH')
        port=copy.deepcopy(rows);port[1]['state'][self.key]=600
        result=compare(rows,port,0,checkpoint_plan=plan)
        self.assertEqual((result['status'],result['checkpoint']),('DIVERGENCE','ball.init.exit'))
        with self.assertRaises(ValueError):compare(rows,port,0,checkpoint_plan=plan[::-1])

    def test_first_difference_and_writer(self):
        rom=copy.deepcopy(self.rows);port=copy.deepcopy(self.rows)
        rom[1]['state'][self.key]=65535;rom[1]['writers'][self.key]=0x80cee7
        port[2]['state'][self.key]=9
        r=compare(rom,port,1)
        self.assertEqual((r['status'],r['sequence']),('DIVERGENCE',1))
        self.assertEqual(r['differences'][0]['last_write_observed_pc'],0x80cee7)

    def test_sentinels_are_real_state(self):
        for value in (65535,32768):
            port=copy.deepcopy(self.rows);port[0]['state'][self.key]=value
            self.assertEqual(compare(self.rows,port,1)['status'],'INITIAL_STATE_MISMATCH')

    def test_fractional_low_bit_is_not_discarded(self):
        port=copy.deepcopy(self.rows);port[2]['state']['3eed']=1
        self.assertEqual(compare(self.rows,port,1)['sequence'],2)

    def test_incomplete_or_extra_fields(self):
        for change in ('missing','extra','null','bool'):
            port=copy.deepcopy(self.rows)
            if change=='missing':del port[0]['state'][self.key]
            elif change=='extra':port[0]['state']['ffff']=0
            else:port[0]['state'][self.key]=None if change=='null' else True
            with self.assertRaises(ValueError):compare(self.rows,port,1)

    def test_reordered_or_truncated_capture(self):
        for rows in (self.rows[:-1],[self.rows[0],self.rows[2],self.rows[1]],self.rows+[self.rows[-1]]):
            with self.assertRaises(ValueError):compare(self.rows,rows,1)

    def test_non_neutral_inputs_rejected(self):
        for value in (1,False,0.0):
            port=copy.deepcopy(self.rows);port[1]['inputs'][0]=value
            with self.assertRaises(ValueError):compare(self.rows,port,1)

    def test_non_object_checkpoint(self):
        for value in (None,[],0):
            port=copy.deepcopy(self.rows);port[1]=value
            with self.assertRaises(ValueError):compare(self.rows,port,1)

    def test_nmi_frame_split_is_disclosed(self):
        port=copy.deepcopy(self.rows);port[2]['outer_frame']=3
        r=compare(self.rows,port,1)
        self.assertEqual(r['status'],'PROJECTION_MATCH')
        self.assertEqual(r['timing_differences'],[dict(sequence=2,rom_frame=2,port_frame=3)])

    def test_duplicate_json_key(self):
        with tempfile.TemporaryDirectory() as temp:
            p=Path(temp)/'bad.jsonl';p.write_text('{"state":{},"state":{}}\n')
            with self.assertRaises(ValueError):load(p)

    def test_cli_reports_failure_without_hiding_it(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);rom=root/'rom.jsonl';port=root/'port.jsonl';report=root/'report.json'
            rom.write_text(''.join(json.dumps(r)+'\n' for r in self.rows))
            rows=copy.deepcopy(self.rows);rows[0]['state'][self.key]=1
            command=[sys.executable,str(Path(__file__).with_name('differential_compare.py')),
                     '--rom-trace',str(rom),'--port-trace',str(port),'--sweeps','1','--report',str(report)]
            for expected,code,data in (('INITIAL_STATE_MISMATCH',1,rows),
                                        ('INVALID_CAPTURE',2,rows[:-1]),
                                        ('PROJECTION_MATCH',0,self.rows)):
                port.write_text(''.join(json.dumps(r)+'\n' for r in data))
                result=subprocess.run(command,capture_output=True,text=True)
                self.assertEqual(result.returncode,code,result.stderr)
                self.assertEqual(json.loads(report.read_text())['status'],expected)

if __name__=='__main__':unittest.main()
