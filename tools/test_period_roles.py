"""Source-derived cadence/overflow cases and before-only protocol corruptions."""
import argparse,copy,importlib.util,json,subprocess
from pathlib import Path
from unittest.mock import patch

def main():
    p=argparse.ArgumentParser()
    for name in ('verifier','native','rom','exe','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args()
    for name in vars(a):setattr(a,name,getattr(a,name).resolve())
    a.output.mkdir(parents=True,exist_ok=False)
    spec=importlib.util.spec_from_file_location('role_verifier',a.verifier);v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
    v.check_build(a.exe);v.source(a.rom);names=v.mapping();index={n:i for i,(n,_)in enumerate(names)};checks=[]
    def setv(words,name,value):words[index[name]]=value&65535
    def val(row,name):return row['words'][index[name]]
    def base():
        words=[0x5a17]*len(names)
        for name,value in [('context0.opponent_02',0x476b),('context1.opponent_02',0x46eb),('context0.first_actor_04',0x34eb),('context1.first_actor_04',0x39eb),('ball_x',0),('ball_y',0),('ball_pointer_0910',0x3eeb),('predicted_x_0918',400),('predicted_y_091a',-80),('camera_093a',0),('owner_093e',0),('cadence_09d2',12),('rebuild_09d6',0),('delta_00c6',2),('nearest_0092',0x1234),('ready_09ba',1),('dead_x_09b0',0xa123),('dead_y_09b2',0xb456)]:setv(words,name,value)
        for i in range(10):
            for name,value in [('x',10*i),('y',0),('assignment_74',65535),('direction_86',0x500+i),('pair_distance_8a',0x7100+i)]:setv(words,f'actor{i}.'+name,value)
        return words
    def run(name,words):return v.run_probe(a.exe,a.output,name,v.binary_input(words))
    # Original BCE4..BD0C: only subtraction N controls +30; camera is a
    # 16-bit sign test; 9D6 bypasses subtraction and remains carried at BD0D.
    # Tuples: initial,delta,rebuild,camera,final cadence,terminal kind,calls.
    cadence=[(12,2,0,0,8,2,2),(4,2,0,0,0,2,2),(3,2,0,0,29,4,1),
        (2,2,0,0,28,4,1),(1,2,0,0,29,4,0),(0,0,0,0,0,2,2),
        (65535,1,0,0,28,4,0),(32768,0,0,0,32798,4,0),
        (32768,1,0,0,32766,2,2),(32767,65535,0,0,32798,4,0),
        (10,65535,0,0,12,2,2),(1,2,0,65535,27,2,2),
        (1,2,0,32768,27,2,2),(1,2,0,255,29,4,0),
        (0,31,0,65535,65534,2,2),(0,2,1,65535,30,3,0),
        (65535,65535,32768,0,30,3,0),(12,2,65535,0,30,3,0)]
    for i,(initial,delta,rebuild,camera,final,kind,calls)in enumerate(cadence):
        words=base()
        for key,value in [('cadence_09d2',initial),('delta_00c6',delta),('rebuild_09d6',rebuild),('camera_093a',camera)]:setv(words,key,value)
        rows=run(f'cadence-{i}',words);r=rows[-1]
        assert r['kind']==kind and r['completed_calls']==calls
        assert r['pc']=={2:0x86e1f7,3:0x85bd0d,4:0x85be06}[kind]
        assert val(r,'cadence_09d2')==final and val(r,'rebuild_09d6')==rebuild
        for key in ('ready_09ba','dead_x_09b0','dead_y_09b2','delta_00c6','owner_093e','camera_093a'):assert val(r,key)==words[index[key]]
        assert val(r,'nearest_09da')==(0x34eb if len(rows)==2 else 0x39eb)
        assert val(r,'actor_0096')==(0x39eb if len(rows)==2 else 0x3eeb)
        assert val(r,'pair_009a')==words[index['pair_009a']]
        checks.append({'name':f'cadence-{i}','passed':True})
    # Literal instruction-derived F34F corner values, including the unusual
    # (0,1) zero distance and ASL truncation after absolute(-32768).
    geometry=[(0,0,8,0),(1,0,2,1),(-1,0,6,1),(0,1,1,0),(0,2,0,2),
        (1,1,1,1),(100,40,2,110),(32767,0,2,32767),
        (-32768,0,7,0),(0,-32768,4,32768),(32767,-32768,2,32767),(-32768,32767,7,40959)]
    for i,(dx,dy,direction,distance)in enumerate(geometry):
        words=base();setv(words,'actor5.x',0);setv(words,'actor5.y',0);setv(words,'actor5.assignment_74',0);setv(words,'actor0.x',dx);setv(words,'actor0.y',dy)
        r=run(f'geometry-{i}',words)[-1]
        assert val(r,'actor5.pair_distance_8a')==val(r,'actor0.pair_distance_8a')==distance,(i,'distance')
        assert val(r,'actor5.direction_86')==(0x505 if direction==8 else direction),(i,'direction')
        assert val(r,'actor0.direction_86')==(0x500 if direction==8 else direction^4),(i,'opposite')
        assert val(r,'direction_00b2')==direction
        checks.append({'name':f'geometry-{i}','passed':True})
    for i,(x,y,distance,pointer)in enumerate([(-32768,0,8192,0x34eb),(32767,-32768,40959,0x1234),(-32768,-32768,40960,0x1234),(100,40,110,0x34eb)]):
        words=base()
        for actor in range(10):setv(words,f'actor{actor}.x',x);setv(words,f'actor{actor}.y',y)
        r=run(f'focal-overflow-{i}',words)[-1]
        assert all(val(r,f'actor{actor}.focal_distance_8e')==distance for actor in range(10))
        assert val(r,'nearest_09da')==val(r,'nearest_0092')==pointer
        checks.append({'name':f'focal-overflow-{i}','passed':True})
    # Owner gates the focal source, not which actor's XY is dereferenced.
    for owner in (0,9,32767,32768,65535):
        words=base();setv(words,'owner_093e',owner);r=run('focal-owner-'+str(owner),words)[-1]
        assert val(r,'focal_x_00b6')==(0 if owner<32768 else 400)
        assert val(r,'focal_y_00ba')==(0 if owner<32768 else (-80&65535))
        assert val(r,'object_008e')==(0x3eeb if owner<32768 else words[index['object_008e']])
        checks.append({'name':'focal-owner-'+str(owner),'passed':True})
    good=base();data=v.binary_input(good)
    inputs=[('short',data[:-1],3),('extra',data+b'x',3),('bad_magic',b'xx'+data[2:],3),('bad_version',data[:2]+b'\x02\0'+data[4:],3)]
    for name,key,value in [('odd_assignment','actor0.assignment_74',1),('unrepresented_assignment','actor0.assignment_74',20),('wrong_ball_pointer','ball_pointer_0910',0x34eb),('wrong_context','context0.opponent_02',0x46eb)]:
        words=base();setv(words,key,value);inputs.append((name,v.binary_input(words),4))
    for name,data,code in inputs:
        path=a.output/(name+'.input');path.write_bytes(data);r=subprocess.run([str(a.exe),str(path)],capture_output=True)
        checks.append({'name':name,'passed':type(r.returncode)is int and r.returncode==code})
    def invoke(name):return v.main(argparse.Namespace(native=[a.native],rom=a.rom,exe=a.exe,output=a.output/name))
    invoke('native-baseline')
    def negative(name,context,hits):
        try:
            with context:invoke(name)
        except (ValueError,TypeError,KeyError,AssertionError,IndexError)as e:rejected=True;reason=str(e)
        else:rejected=False;reason='accepted'
        assert hits,'unreachable corruption '+name
        checks.append({'name':name,'passed':rejected,'reason':reason})
    runner=v.subprocess.run
    for name,key,value in [('bool_exit','returncode',False),('float_exit','returncode',0.0),('stderr','stderr','unexpected')]:
        hits=[]
        def altered(*args,**kwargs):r=runner(*args,**kwargs);setattr(r,key,value);hits.append(True);return r
        negative(name,patch.object(v.subprocess,'run',altered),hits)
    probe=v.run_probe
    changes=[('missing_first',lambda r:r.pop(0)),('missing_last',lambda r:r.pop()),('extra_last',lambda r:r.append(copy.deepcopy(r[-1]))),
        ('wrong_first_PC',lambda r:r[0].update(pc=0)),('wrong_calls',lambda r:r[-1].update(completed_calls=1)),
        ('wrong_kind',lambda r:r[-1].update(kind=3)),('wrong_geometry',lambda r:r[-1]['words'].__setitem__(index['actor5.pair_distance_8a'],0)),
        ('wrong_nearest',lambda r:r[-1]['words'].__setitem__(index['nearest_09da'],0)),('wrong_rebuild',lambda r:r[-1]['words'].__setitem__(index['rebuild_09d6'],1)),
        ('wrong_ready',lambda r:r[-1]['words'].__setitem__(index['ready_09ba'],0))]
    for name,change in changes:
        hits=[]
        def altered(*args,**kwargs):r=probe(*args,**kwargs);old=copy.deepcopy(r);change(r);assert r!=old;hits.append(True);return r
        negative(name,patch.object(v,'run_probe',altered),hits)
    result={'passed':all(c['passed']for c in checks),'checks':checks,'test_sha256':v.sha(__file__),'verifier_sha256':v.sha(a.verifier)}
    (a.output/'report.json').write_text(json.dumps(result,indent=2)+'\n');print('PASS'if result['passed']else'FAIL',len(checks));return not result['passed']
if __name__=='__main__':raise SystemExit(main())
