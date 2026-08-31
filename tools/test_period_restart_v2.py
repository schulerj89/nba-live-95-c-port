"""Source combinations, malformed protocol, and independent boundary mutations."""
import argparse, copy, importlib.util, json, struct, subprocess
from pathlib import Path
from unittest.mock import patch
from period_restart_source_reference_v2 import formation_pair, witness

def main():
    p=argparse.ArgumentParser()
    for k in ('verifier','native','rom','exe','output'):p.add_argument('--'+k,type=Path,required=True)
    a=p.parse_args()
    for k in vars(a):setattr(a,k,getattr(a,k).resolve())
    a.output.mkdir(parents=True,exist_ok=False)
    spec=importlib.util.spec_from_file_location('period_test_verifier',a.verifier);v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
    v.check_build(a.exe);v.check_source(a.rom);names=v.mapping();index={name:i for i,(name,_)in enumerate(names)};checks=[]
    def value(row,name):return row['words'][index[name]]
    raw=a.rom.read_bytes();source_witness=witness(raw)
    (a.output/'source-reference.json').write_text(json.dumps(source_witness,indent=2)+'\n')
    reset='z velocity_x velocity_y velocity_z speed contact_inhibit action_timer behavior_timer boost_timer recovery_inhibit behavior_flags upper_phase lower_phase upper_accumulator lower_accumulator upper_lock lower_lock'.split()
    for period in range(5):
        for tip in (0,5):
            for anchor in (-336,336):
                name=f'combination-{period}-{tip}-{anchor}'
                words=[(0x9123+37*i)&65535 for i in range(len(names))]
                words[index['ready_09ba']]=1 if anchor<0 else 0xffff
                data=v.binary_input(period,tip,(anchor,-anchor),words)
                rows=v.run_probe(a.exe,a.output,name,data);final=rows[-1]
                assert len(rows)==(18 if 0<period<4 else 16)
                for actor in range(10):
                    pair=actor%5;side=actor//5;prefix=f'actor{actor}.'
                    x,y,d=formation_pair(raw,period,tip,anchor,pair)[side]
                    before=next(r for r in rows if r['kind']==1 and r['actor']==pair)
                    for field,want in [('x',x),('y',y),('target_x',x),('target_y',y),('direction',d),('requested_direction',d),('movement_direction',d),('field_a6',pair),('formation_timer',300),('id',actor),('team_group',side*5),('team_context',0x46eb+side*128)]:
                        assert value(before,prefix+field)==want&65535,(name,actor,field)
                    for field in reset:assert value(before,prefix+field)==0,(name,actor,field)
                    for field in ('x_fraction','y_fraction','z_fraction'):
                        assert value(final,prefix+field)==words[index[prefix+field]],(name,actor,'fraction preserved')
                    assert value(final,prefix+'focal_distance')==(120 if pair else 0)
                    expected_mode=4 if pair==0 else 2
                    if 0<period<4 and actor==(tip^(0 if period==3 else 5))+2:expected_mode=11
                    assert value(final,prefix+'mode')==expected_mode
                for field in ('ready_09ba','dead_ball_x_09b0','dead_ball_y_09b2'):
                    assert value(final,field)==words[index[field]],(name,field,'must remain carried')
                assert value(final,'transfer_09b8')==value(final,'activity_0948')==value(final,'pass_word_094a')==0
                for field in ('pass_actor_0942','pass_word_0944','receiver_0946','last_side_093c','last_actor_097e'):assert value(final,field)==65535
                for field in ('ball.x_fraction','ball.y_fraction','ball.z_fraction'):assert value(final,field)==0
                assert value(final,'ball.id')==10 and value(final,'ball.team_group')==65535
                assert [value(final,f'object_list.{i}')for i in range(12)]==[0x34eb+(i//2+(5 if i%2 else 0))*256 for i in range(10)]+[0x3eeb,0]
                if 0<period<4:
                    side=tip^(0 if period==3 else 5);actor=side+2;selected_anchor=anchor if side==0 else -anchor
                    x,y,d=(-394,64,2)if selected_anchor>=0 else(394,-64,6)
                    for field,want in [('side_0952',side),('actor_0954',actor),('owner_093e',actor),('owner_pointer_0940',0x34eb+256*actor),('camera_093a',side),('layout_0956',0),('target_x_0958',x),('target_y_095a',y),('direction_095c',d),('play_request_0994',1),('play_0996',1),('timer_092e',300),('live_0936',0x82),('ball.z',24),('attachment_0968',24),('attachment_09f6',24)]:assert value(final,field)==want&65535,(name,field)
                    assert value(final,'ball.x')==value(final,f'actor{actor}.x') and value(final,'ball.y')==value(final,f'actor{actor}.y')
                    assert all(value(final,'ball.'+f)==0 for f in ('velocity_x','velocity_y','velocity_z'))
                else:
                    assert value(final,'live_0936')==0x81 and value(final,'owner_093e')==65535 and value(final,'owner_pointer_0940')==0
                    assert value(final,'ball.z')==80 and value(final,'ball.velocity_z')==600 and value(final,'ball.x')==value(final,'ball.y')==0
                    for field in ('side_0952','actor_0954','layout_0956','target_x_0958','target_y_095a','direction_095c','attachment_0968','attachment_09f6','ready_09ba','timer_092e','play_request_0994','play_0996'):
                        assert value(final,field)==words[index[field]],(name,'opening preserves',field)
                checks.append({'name':name,'passed':True,'scope':'source-only parent work; excluded children not executed'})
    # Actual malformed typed file inputs; all source-only data remains separate
    # from native fixtures and none is accepted as a normal initialization seed.
    base=v.binary_input(1,5,(-336,336),[0]*len(names))
    malformed=[('short',base[:-1],3),('extra',base+b'x',9),('bad_version',base[:2]+b'\x02\0'+base[4:],3),('bad_period',base[:4]+b'\x05\0'+base[6:],4),('bad_tip',base[:6]+b'\x01\0'+base[8:],4),('missing_child',base[:12]+b'\x0c\0'+base[14:],7)]
    for name,data,code in malformed:
        path=a.output/(name+'.input');path.write_bytes(data);r=subprocess.run([str(a.exe),str(path)],capture_output=True)
        checks.append({'name':name,'passed':type(r.returncode)is int and r.returncode==code})
    def invoke(name):return v.main(argparse.Namespace(native=[a.native],rom=a.rom,exe=a.exe,output=a.output/name))
    invoke('native-baseline')
    def negative(name,context,touched):
        try:
            with context:invoke(name)
        except (ValueError,TypeError,KeyError,AssertionError,IndexError)as e:rejected=True;reason=str(e)
        else:rejected=False;reason='accepted'
        assert touched,'unreachable corruption: '+name
        checks.append({'name':name,'passed':rejected,'reason':reason})
    runner=v.subprocess.run
    for name,kind,new in [('bool_exit','returncode',False),('float_exit','returncode',0.0),('stderr','stderr','unexpected')]:
        touched=[]
        def run(*args,**kwargs):r=runner(*args,**kwargs);setattr(r,kind,new);touched.append(True);return r
        negative(name,patch.object(v.subprocess,'run',run),touched)
    probe=v.run_probe
    changes=[('C_wrong_kind',lambda r:r[0].update(kind=2)),('C_wrong_child',lambda r:r[0].update(child=0)),('C_wrong_actor',lambda r:r[0].update(actor=9)),('C_missing_terminal',lambda r:r.pop()),('C_extra_terminal',lambda r:r.append(copy.deepcopy(r[-1]))),('C_A6_zero_quirk',lambda r:r[2]['words'].__setitem__(index['actor1.field_a6'],0)),('C_ready_cleared',lambda r:r[-1]['words'].__setitem__(index['ready_09ba'],0)),('C_dead_coordinate_changed',lambda r:r[-1]['words'].__setitem__(index['dead_ball_x_09b0'],0x7777))]
    for name,change in changes:
        touched=[]
        def run(*args,**kwargs):rows=probe(*args,**kwargs);before=copy.deepcopy(rows);change(rows);assert before!=rows;touched.append(True);return rows
        negative(name,patch.object(v,'run_probe',run),touched)
    # Alter only the parsed view, after the real file identity is read. Original
    # frozen captures and their hashes are never rewritten for negative tests.
    decode=v.loads
    manifest_changes=[
        ('native_missing_trace_hash',lambda m:m['artifacts'].pop('boundaries.jsonl')),
        ('native_empty_sources',lambda m:m.update(sources={})),
        ('native_wrong_arguments',lambda m:m['arguments'].__setitem__(2,'--timeout=1')),
        ('native_bool_seed',lambda m:m.update(period_seed=False)),
        ('native_unknown_scope',lambda m:m.update(kind='normal period parity')),
        ('native_extra_isolation_key',lambda m:m['isolation'].update(unchecked=True)),
        ('native_settings_hash',lambda m:m['isolation'].update(post_settings_sha256='0'*64)),
        ('native_settings_value',lambda m:m['isolation']['settings']['Snes'].update(EnableRandomPowerOnState=True)),
    ]
    for name,change in manifest_changes:
        touched=[]
        def altered(text):
            obj=decode(text)
            if isinstance(obj,dict)and 'period_seed'in obj and 'artifacts'in obj:
                old=json.dumps(obj,sort_keys=True);change(obj);assert old!=json.dumps(obj,sort_keys=True);touched.append(True)
            return obj
        negative(name,patch.object(v,'loads',altered),touched)
    for name,change in [
        ('native_wrong_hook',lambda r:r.update(pc=0)),
        ('native_bool_register',lambda r:r.update(a=False)),
        ('native_decimal_mode',lambda r:r.update(ps=r['ps']|8)),
        ('native_missing_register',lambda r:r.pop('ps')),
        ('native_fields_disagree',lambda r:r['fields'].__setitem__('09ba',0)),
        ('native_wrong_raw',lambda r:r.update(raw='../outside.bin')),
        ('native_backward_frame',lambda r:r.update(frame=0)),
    ]:
        touched=[]
        def altered(text):
            obj=decode(text)
            if isinstance(obj,dict)and obj.get('tag')=='appearance.first.before'and not touched:
                old=json.dumps(obj,sort_keys=True);change(obj);assert old!=json.dumps(obj,sort_keys=True);touched.append(True)
            return obj
        negative(name,patch.object(v,'loads',altered),touched)
    for name,text in [('duplicate_json','{"pc":1,"pc":2}'),('nonfinite_json','{"value":NaN}')]:
        try:decode(text)
        except ValueError:rejected=True
        else:rejected=False
        checks.append({'name':name,'passed':rejected})
    reader=v.word
    for name,address in [('native_wrong_B6',0xb6),('native_wrong_cursor',0x9a)]:
        touched=[]
        def wrong_word(raw,addr):
            value=reader(raw,addr)
            if addr==address:value^=1;touched.append(True)
            return value
        negative(name,patch.object(v,'word',wrong_word),touched)
    result={'passed':all(c['passed']for c in checks),'checks':checks,'test_sha256':v.sha(__file__),'verifier_sha256':v.sha(a.verifier),'reference_sha256':v.sha(Path(__file__).with_name('period_restart_source_reference_v2.py'))}
    (a.output/'report.json').write_text(json.dumps(result,indent=2)+'\n');print('PASS'if result['passed']else'FAIL',len(checks));return 0 if result['passed']else 1
if __name__=='__main__':raise SystemExit(main())
