"""Reject erased or unclassified interruption/recovery/receiver-only evidence."""
import copy,json,argparse
from pathlib import Path
from pass_interruption_trace import PassInterruptionGuard

def base():
    return {'simulation_tick':100,'possession':{'pass_actor_raw':0,'pass_receiver_raw':4,'pass_active_raw':1},
            'actors':[{'animation':0x2f,'raw':{'control_mode':15 if i==0 else 10,
                'pass_released':0,'pass_band_62':6,'pass_family_c0':5,'contact_inhibit_5a':0}} for i in range(10)],
            'collision':{'player_count':1,'player_routine':0x86bfba,'player_a':0,'player_b':5}}

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    before=base();after=copy.deepcopy(before);after['simulation_tick']=102
    after['actors'][0]['raw']['control_mode']=8;after['actors'][0]['raw']['contact_inhibit_5a']=30;after['actors'][0]['animation']=0x35
    positive=0;rejected=[]
    for routine in(0x86bfba,0x86c91e):
        good=copy.deepcopy(after);good['collision']['player_routine']=routine
        g=PassInterruptionGuard();g.observe(before,good);assert g.entries==1
        returned=copy.deepcopy(good);returned['simulation_tick']=134;returned['actors'][0]['raw'].update(control_mode=11,contact_inhibit_5a=0)
        g.observe(good,returned);assert g.recoveries==1 and not g.interrupted;positive+=1
    mutations=[]
    for key in('pass_actor_raw','pass_receiver_raw','pass_active_raw'):
        mutations.append((key,lambda r,k=key:r['possession'].__setitem__(k,9)))
    mutations.append(('erased-active',lambda r:r['possession'].__setitem__('pass_active_raw',0)))
    mutations.append(('erased-identities',lambda r:r['possession'].update(pass_actor_raw=-1,pass_receiver_raw=-1)))
    for key,val in [('pass_band_62',12),('pass_family_c0',4),('pass_released',1)]:
        mutations.append((key,lambda r,k=key,v=val:r['actors'][0]['raw'].__setitem__(k,v)))
    for key,val in [('player_count',0),('player_routine',0x86c239),('player_a',3)]:
        mutations.append((key,lambda r,k=key,v=val:r['collision'].__setitem__(k,v)))
    mutations.append(('pose',lambda r:r['actors'][0].__setitem__('animation',0x2f)))
    for name,change in mutations:
        r=copy.deepcopy(after);change(r)
        try:PassInterruptionGuard().observe(before,r)
        except AssertionError:rejected.append(name)
        else:raise AssertionError('accepted '+name)
    for key,val in [('control_mode',15),('contact_inhibit_5a',2),('pass_released',1),('pass_band_62',12),('pass_family_c0',4)]:
        g=PassInterruptionGuard();g.observe(before,after)
        r=copy.deepcopy(after);r['simulation_tick']=134;r['actors'][0]['raw'].update(control_mode=11,contact_inhibit_5a=0);r['actors'][0]['raw'][key]=val
        try:g.observe(after,r)
        except AssertionError:rejected.append('recovery-'+key)
        else:raise AssertionError('accepted recovery '+key)
    for mode in(10,14):
        b=base();b['actors'][4]['raw']['control_mode']=mode
        r=copy.deepcopy(b);r['possession']['pass_receiver_raw']=-1;r['actors'][4]['raw']['control_mode']=8;r['collision']['player_a']=4
        g=PassInterruptionGuard();assert g.receiver_only_clear(b,r);assert g.receiver_only_clear(r,copy.deepcopy(r));positive+=2
        for key,val in [('pass_actor_raw',2),('pass_active_raw',0)]:
            bad=copy.deepcopy(r);bad['possession'][key]=val;assert not PassInterruptionGuard().receiver_only_clear(b,bad);rejected.append('receiver-'+str(mode)+'-'+key)
        for key,val in [('player_count',0),('player_routine',0x86c239),('player_a',3)]:
            bad=copy.deepcopy(r);bad['collision'][key]=val;assert not PassInterruptionGuard().receiver_only_clear(b,bad);rejected.append('receiver-'+str(mode)+'-'+key)
        bad=copy.deepcopy(b);bad['actors'][4]['raw']['control_mode']=15;assert not PassInterruptionGuard().receiver_only_clear(bad,r);rejected.append('receiver-'+str(mode)+'-not-mode10or14')
    report={'positive':positive,'rejected':len(rejected),'cases':rejected,'scope':'controlled regression guards, not native reachability','passed':True}
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
