"""Full persistent-state replay for $86:9D6E/$9DA6 through $A476."""
import argparse
import json
from pathlib import Path
import subprocess
from verify_action_animation_vectors import memory, word, OFFSETS

ANIMATION=['upper_queue_cursor','lower_queue_cursor','upper_state','lower_state','base_state',
           'upper_phase','lower_phase','upper_accumulator','lower_accumulator','upper_lock','lower_lock']+[
           f'{half}_queue[{i}]' for half in ('upper','lower') for i in range(3)]
ACTOR={'velocity_x':0xe,'velocity_y':0x10,'velocity_z':0x12,'speed':0x4a,'mode':0x5e,
       'flags':0x7e,'timer':0x60,'status':0x28,'behavior_timer':0x64}
GLOBALS={'actor.activity':0x948,'actor.bounce_count':0x920,'actor.bounce_timer':0x91c,
    'x_fraction':0x3eed,'x':0x3eef,'y_fraction':0x3ef1,'y':0x3ef3,'z_fraction':0x3ef5,'z':0x3ef7,
    'velocity_x':0x3ef9,'velocity_y':0x3efb,'velocity_z':0x3efd,'owner':0x93e,'last_owner':0x9c8,
    'display_shooter':0x493b,'attempt_latch':0x94a,'dead_0966':0x966,'height_0968':0x968,
    'dead_096c':0x96c,'bounce_0920':0x920,'inner_veto':0x9f8,'live_state':0x936,'timeout_0930':0x930,
    'value':0x94c,'display_value':0x4939,'initial_value':0x96a,'roster_low':0x914,'roster_bank':0x916,
    'ball_record':0x910,'rng.state':0x7f6}

def convert(v,rom):
    if v['entry_pc'].lower() not in ('869d6e','869da6') or v['exit_pc'].lower()!='86a476':
        return None
    a,b=memory(v['entry']),memory(v['exit'])
    base,context=word(a,0x96),word(a,0x9e)
    roster=word(a,0xe0) | (word(a,0xe2)&255)<<16
    def romword(address):
        offset=((address>>16)&0x7f)*0x8000+(address&0x7fff)
        return rom[offset] | rom[offset+1]<<8
    actor_id=word(a,base)
    stats=word(a,0x3435+actor_id*2)
    controller=word(a,base+0x16)
    controller_stats=romword(0x879c71+controller*2) if controller<0x8000 else None
    def state(m):
        out={'s.actor.animation.'+name:word(m,base+o) for name,o in zip(ANIMATION,OFFSETS)}
        out.update({'s.actor.'+name:word(m,base+o) for name,o in ACTOR.items()})
        out.update({'s.'+name:word(m,o) for name,o in GLOBALS.items()})
        out.update({'s.facing':word(m,base+0x4e),'s.contact_inhibit':word(m,base+0x5a),
                    's.assist_43':word(m,context+0x43),'s.assist_45':word(m,context+0x45)})
        out.update({f's.player_stats[{i}]':word(m,stats+i*2) for i in range(5)})
        out.update({f's.controller_stats[{i}]':word(m,controller_stats+0x12+i*2) if controller_stats else 0 for i in range(5)})
        return out
    in_fields={'actor_x':4,'actor_y':8,'controller':0x16,'team_group':0x6e,'distance_8c':0x8c,
               'defense_8a':0x8a,'movement_4c':0x4c,'modifier_b2':0xb2}
    inputs={'in.'+name:word(a,base+o) for name,o in in_fields.items()}
    inputs.update({'in.'+name:word(a,o) for name,o in {
        'origin_x':0x900,'origin_y':0x902,'difficulty':0x17af,'shot_control_17c3':0x17c3,
        'shot_assistance_17bf':0x17bf,'hot_team_09c0':0x9c0,'free_throw_0978':0x978,
        'aim_0982':0x982,'power_0980':0x980,'clock_0928':0x928,'period_0926':0x926,
        'roster_low':0xe0,'roster_bank':0xe2}.items()})
    inputs.update({'in.basket_x':word(a,context+0xa),'in.basket_fraction':word(a,context+8),
        'in.assist_clock_47':word(a,context+0x47),
        'in.stamina_18':word(a,word(a,0x3435+word(a,0xc2)*2)+0x18),
        'in.boosted':int(word(a,base+0x72)!=0),'in.alternate_lower':int(word(a,base+0xa8)!=0),
        'in.special_entry':int(v['entry_pc'].lower()=='869da6')})
    inputs.update({'in.'+name:romword(roster+o)&255 for name,o in {
        'rating_two':0x36,'rating_three':0x37,'rating_free':0x38,'range_49':0x49}.items()})
    inputs.update(state(a))
    expected=state(b)
    asynchronous=None
    if 'launch_timeout' in v:
        # Mesen write callbacks expose the PC AFTER DEC $0930 (85:EE30).
        # Check every byte against that exact interrupt decrement, then
        # compare the C launch output with its own post-STA boundary.
        if bytes(rom[((0x85&0x7f)*0x8000)+0x6e30:((0x85&0x7f)*0x8000)+0x6e33])!=bytes((0xce,0x30,0x09)):
            raise ValueError('Unexpected interrupt countdown instruction')
        timer=v['launch_timeout'];events=v['timer_writes']
        if len(events)%2:raise ValueError('Partial countdown write')
        for high,low in zip(events[::2],events[1::2]):
            timer=(timer-1)&65535
            if high!=[0x931,timer>>8,0x85,0xee33] or low!=[0x930,timer&255,0x85,0xee33]:
                raise ValueError('Unexpected concurrent countdown writer')
        if timer!=word(b,0x930):raise ValueError('Countdown trace does not explain final output')
        expected['s.timeout_0930']=v['launch_timeout']
        asynchronous={'launch':v['launch_timeout'],'exit':timer,'writes':events,'writer':'85:EE30 DEC $0930'}
    for address in (0xb6,0xb8,0xba,0xbc,0xbe,0xc0,0x93a,0x978):
        if word(a,address)!=word(b,address): raise ValueError(f'preserved word changed {address:x}')
    return {'call':v['call'],'provenance':v.get('provenance','natural-ROM'),
            'input':inputs,'expected':expected,'asynchronous_countdown':asynchronous}

def main():
    p=argparse.ArgumentParser()
    for name in ('vectors','probe','pack'):p.add_argument('--'+name,required=True)
    p.add_argument('--rom');p.add_argument('--normalized',action='store_true')
    args=p.parse_args()
    raw=Path(args.vectors).read_text()
    if args.normalized: rows=json.loads(raw)
    else:
        rom=Path(args.rom).read_bytes()
        rows=[r for line in raw.splitlines() if line and (r:=convert(json.loads(line),rom))]
    if not rows: raise SystemExit('No complete launch calls')
    for row in rows:
        countdown=row.get('asynchronous_countdown')
        if not countdown: raise SystemExit('Missing owned-timeout evidence')
        timer=countdown['launch'];events=countdown['writes']
        if (countdown['writer']!='85:EE30 DEC $0930' or len(events)%2 or
                timer!=row['expected']['s.timeout_0930']):
            raise SystemExit('Invalid owned-timeout evidence')
        for high,low in zip(events[::2],events[1::2]):
            timer=(timer-1)&65535
            if high!=[0x931,timer>>8,0x85,0xee33] or low!=[0x930,timer&255,0x85,0xee33]:
                raise SystemExit('Invalid asynchronous countdown event')
        if timer!=countdown['exit']: raise SystemExit('Invalid asynchronous countdown exit')
    stream=''.join(str(len(r['input']))+' '+' '.join(f'{k} {v:x}' for k,v in r['input'].items())+'\n' for r in rows)
    run=subprocess.run([args.probe,args.pack],input=stream,text=True,capture_output=True)
    if run.returncode: raise SystemExit(run.stderr)
    lines=[line for line in run.stdout.splitlines() if line and not line.startswith('[')]
    if len(lines)!=len(rows):raise SystemExit('Missing probe rows')
    bad=[]
    for row,line in zip(rows,lines):
        words=line.split();actual={k:int(v,16) for k,v in zip(words[::2],words[1::2])}
        diffs={k:(v,actual.get(k)) for k,v in row['expected'].items() if v!=actual.get(k)}
        if diffs:bad.append((row['call'],row['provenance'],diffs))
    for mismatch in bad[:15]:print(mismatch)
    print(f'[COMPLETE SHOT] calls={len(rows)} mismatches={len(bad)}')
    if bad:raise SystemExit(1)

if __name__=='__main__':main()
