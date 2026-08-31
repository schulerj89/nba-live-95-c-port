"""Native scalar schemas, wire-type rejections, and unresolved read boundaries."""
import argparse,copy,hashlib,json,struct
from pathlib import Path
import setup_spc_state_contract_v4 as c

def read(p):return dict(line.split('=',1)for line in p.read_text().splitlines())
def main():
    p=argparse.ArgumentParser();p.add_argument('--init',type=Path,required=True);p.add_argument('--control',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    a.output.mkdir(parents=True,exist_ok=False);checks=[]
    def good(name,fn):fn();checks.append({'name':name,'passed':True})
    def bad(name,fn):
        try:fn()
        except (ValueError,TypeError,KeyError):passed=True
        else:passed=False
        checks.append({'name':name,'passed':passed})
    for kind,where in (('init',a.init),('control',a.control)):
        for path in where.glob('*.state'):good('native_'+path.name,lambda:c.validate(read(path),kind=='control'))
    init=read(a.init/'spc_init_pending_dsp.state')
    for name,field,value in [
        ('missing_DSP_voice','spc.dsp.voices[7].sampleBuffer11',None),
        ('missing_timer','spc.timer2.stage0',None),
        ('byte_overflow','spc.ps','256'),('bool_as_integer','spc.ps','false'),
        ('unsigned_negative','spc.pc','-1'),('negative_zero','spc.a','-0'),
        ('leading_zero','spc.a','032'),('unicode_digit','spc.a','３２'),
        ('integer_object','spc.a',32),('bool_object','spc.writeEnabled',True),
        ('timer_stage_range','spc.timer2.stage0','16'),
        ('timer_edge_range','spc.timer1.prevStage1','2'),
        ('signed_sample_overflow','spc.dsp.echoHistory0','32768'),
        ('DSP_word_overflow','spc.dsp.counter','65536'),
        ('voice_signed_overflow','spc.dsp.voices[0].envVolume','2147483648'),
        ('ratio_nonfinite','spc.clockRatio','1e9999'),('ratio_zero','spc.clockRatio','0'),
        ('invented_scalar','spc.fake','0'),('unserialized_enum','spc.dsp.voices[0].envMode','0'),
    ]:
        state=copy.deepcopy(init)
        if value is None:state.pop(field)
        else:state[field]=value
        bad(name,lambda:c.validate(state))
    instructions=list(struct.iter_unpack('<H5BQ',(a.init/'spc_init_instructions.bin').read_bytes()))
    reads=list(struct.iter_unpack('<HHBQ',(a.init/'spc_init_io_reads.bin').read_bytes()))
    ins,read_event=instructions[-1],reads[-1]
    good('pending_native_relation',lambda:c.pending_dsp(init,ins,read_event))
    # The callback's response is deliberately external to the incomplete C read.
    good('pending_response_not_consumed',lambda:c.pending_dsp(init,ins,read_event[:2]+((read_event[2]^255),read_event[3])))
    for field,value in [('spc.pc',str(0x3db)),('spc.cycle',str(ins[-1]+4)),('spc.cycle',str(ins[-1]+8)),('spc.a','33'),('spc.x','1'),('spc.y','1'),('spc.sp','254'),('spc.ps','8')]:
        state=copy.deepcopy(init);state[field]=value
        bad('pending_relation_'+field+'_'+value,lambda:c.pending_dsp(state,ins,read_event))
    control=read(a.control/'spc_control_1_before.state')
    good('control_native_relation',lambda:c.control_boundary(control,0x384,0x30))
    legal=copy.deepcopy(control);legal['spc.writeEnabled']='false'
    good('control_write_disabled_state_is_valid',lambda:c.validate(legal,True))
    bad('control_callback_requires_write_enable',lambda:c.control_boundary(legal,0x384,0x30))
    for field,value in [('spc.pc','900'),('source_pc','901'),('value','49')]:
        state=copy.deepcopy(control);state[field]=value
        bad('control_relation_'+field,lambda:c.control_boundary(state,0x384,0x30))
    # Storage-width validity is separate from normal DSP execution; preserve raw
    # timer output bytes rather than incorrectly imposing the timer-read nibble.
    legal=copy.deepcopy(control);legal['spc.timer0.output']='255';legal['spc.dsp.echoHistory0']='-32768';legal['spc.dsp.voices[7].sampleBuffer11']='32767'
    good('raw_output_and_signed_samples',lambda:c.validate(legal,True))
    result={'passed':all(row['passed']for row in checks),'checks':checks,'contract_sha256':hashlib.sha256(Path(c.__file__).read_bytes()).hexdigest(),'test_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
    (a.output/'report.json').write_text(json.dumps(result,indent=2)+'\n');print('PASS'if result['passed']else'FAIL',len(checks));return not result['passed']
if __name__=='__main__':raise SystemExit(main())
