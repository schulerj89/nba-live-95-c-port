"""Exact native internal-stage checks; no fitted frame window or tolerance.

The Lua capture is native execution with disclosed menu Mode intervention.
The two probes call production C helpers; this does not prove C scheduler,
normal menu initialization, complete F43A continuation, or whole-game parity.
"""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path
from differential_compare import object_without_duplicates

SNAPSHOT_FIELDS = ('actor_id','actor_ptr','attachment','boost','controller',
    'cpu_ps','cpu_sp','cpu_x','current_ptr','cycle','dead','direction','dispatch_dt',
    'dp_aa','dp_ac','dp_ae','dp_b0','draw_direction','event','flags','frame','live',
    'owner','pc','profile_42','ready','receiver','rng','steering_direction',
    'target_x','target_y','timer','transfer','vx','vy','whistle','x','x_fraction',
    'y','y_fraction','z')
STAGE_PCS = {'entry':0x86f43a,'pre_motion':0x86f4e2,
             'velocity_entry':0x85a82c,'post_motion':0x86f4e6,
             'restored':0x86f4f2,'arrived':0x86f520,'prepared':0x86f58f,
             'reload':0x86f654,'exit':None}


def json_read(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'),
                      object_pairs_hook=object_without_duplicates)


def validate_capture_rows(rows):
    if not isinstance(rows,list) or not rows:
        raise ValueError('empty or invalid native capture')
    previous_cycle=previous_frame=-1
    required={'entry','pre_motion','velocity_entry','post_motion','restored','exit'}
    for index,row in enumerate(rows,1):
        stages=set(row)-{'call','schema'}
        if row.get('schema')!='nba95-inbound-internal-v1' or \
                type(row.get('call')) is not int or row['call']!=index or \
                not required<=stages or not stages<=STAGE_PCS.keys() or \
                ('arrived' not in stages and 'reload' not in stages) or \
                ('prepared' in stages and 'arrived' not in stages):
            raise ValueError(f'missing/reordered/invalid native stages: {index}')
        ordered=['entry','pre_motion','velocity_entry','post_motion','restored',
                 'arrived','prepared','reload','exit']
        for stage in (key for key in ordered if key in row):
            snapshot=row[stage]
            if type(snapshot) is not dict or set(snapshot)!=set(SNAPSHOT_FIELDS):
                raise ValueError(f'incomplete native snapshot: {index}/{stage}')
            for field,value in snapshot.items():
                maximum={'cycle':2**53-1,'pc':0xffffff,'frame':0x7fffffff,
                         'profile_42':255,'cpu_ps':255}.get(field,0xffff)
                if type(value) is not int or not 0<=value<=maximum:
                    raise ValueError(f'invalid native integer: {index}/{stage}/{field}')
            if stage=='exit':
                if snapshot['pc'] not in (0x86f439,0x86f653):
                    raise ValueError('invalid native exit PC')
            elif snapshot['pc']!=STAGE_PCS[stage]:
                raise ValueError(f'wrong native stage PC: {stage}')
            if snapshot['cycle']<previous_cycle or snapshot['frame']<previous_frame:
                raise ValueError('native internal snapshots out of order')
            previous_cycle,previous_frame=snapshot['cycle'],snapshot['frame']


def read_compact(path):
    data=json_read(path)
    if data.get('schema')!='nba95-inbound-internal-compact-v1' or \
            data.get('snapshot_fields')!=list(SNAPSHOT_FIELDS):
        raise ValueError('unsupported or incomplete compact snapshot schema')
    source=data.get('source',{})
    for field in ('trace_sha256','rom_sha256','mesen_sha256','script_sha256',
                  'driver_sha256'):
        if not isinstance(source.get(field),str) or \
                not re.fullmatch('[0-9a-f]{64}',source[field]):
            raise ValueError(f'incomplete native source identity: {field}')
    if source.get('runtime_state_injection') is not False or \
            source.get('setup_mode_injection')!='Exhibition':
        raise ValueError('unsupported or absent native capture classification')
    cases=data.get('cases')
    if not isinstance(cases,list) or type(source.get('calls')) is not int or \
            len(cases)!=source['calls'] or len(cases)!=500:
        raise ValueError('retained internal corpus population changed')
    rows=[]
    for case in cases:
        row={'schema':'nba95-inbound-internal-v1','call':case['call']}
        for stage,values in case.items():
            if stage=='call':continue
            if stage not in STAGE_PCS or not isinstance(values,list) or \
                    len(values)!=len(SNAPSHOT_FIELDS):
                raise ValueError('truncated/unknown compact snapshot')
            row[stage]=dict(zip(SNAPSHOT_FIELDS,values))
        rows.append(row)
    validate_capture_rows(rows)
    return rows,source


def signed(word):
    return word - 0x10000 if word & 0x8000 else word


def delta(target, position):
    return signed((target - position) & 0xffff)


def run_probe(path, inputs, width):
    payload = ''.join(' '.join(f'{v:04x}' for v in row) + '\n' for row in inputs)
    completed = subprocess.run([str(path)], input=payload, text=True,
                               capture_output=True, check=True)
    lines = completed.stdout.splitlines()
    if len(lines) != len(inputs):
        raise ValueError(f'probe returned {len(lines)} rows for {len(inputs)} calls')
    values = []
    for line in lines:
        tokens = line.split()
        if len(tokens) != width or any(not re.fullmatch('[0-9a-fA-F]{1,4}', v)
                                       for v in tokens):
            raise ValueError(f'invalid complete probe row: {line!r}')
        values.append([int(token, 16) for token in tokens])
    return values


def verify(rows, motion_probe, arrival_probe):
    validate_capture_rows(rows)
    motion_in, motion_out, arrival_in, arrival_out = [], [], [], []
    faults, changed_velocity, arrived_count, rejected_count = [], 0, 0, 0
    controllers, signs = set(), set()
    required = {'schema', 'call', 'entry', 'pre_motion', 'velocity_entry',
                'post_motion', 'restored', 'exit'}
    for index, row in enumerate(rows, 1):
        if not required <= row.keys() or row['schema'] != 'nba95-inbound-internal-v1':
            raise ValueError(f'incomplete internal stages at call {index}')
        if row['call'] != index:
            raise ValueError('missing/reordered/duplicated call')
        e, pre, ve, post, restored = (row[k] for k in
                                    ('entry', 'pre_motion', 'velocity_entry',
                                     'post_motion', 'restored'))
        for name in required - {'schema', 'call'}:
            if row[name]['actor_ptr'] != e['actor_ptr'] or \
                    row[name]['current_ptr'] != e['actor_ptr']:
                raise ValueError(f'actor-context change at call {index}/{name}')
        controllers.add(e['controller'])
        signs.update('negative' if e[k] & 0x8000 else 'nonnegative'
                     for k in ('vx', 'vy'))
        for k in ('x', 'y', 'x_fraction', 'y_fraction', 'target_x', 'target_y'):
            if any(stage[k] != e[k] for stage in (pre, ve, post, restored)):
                faults.append([index, 'unexpected pre-arrival mutation', k])
        if restored['dp_aa'] != e['target_x'] or restored['dp_ae'] != e['target_y']:
            faults.append([index, 'raw target not restored'])
        for velocity, target, scratch in (('vx','target_x','dp_aa'),
                                          ('vy','target_y','dp_ae')):
            v = signed(e[velocity])
            # Native CMP/ROR arithmetic shift then subtract FFFF for negatives.
            compensation = (v // 16) + int(v < 0)
            expected = (e[target] - compensation) & 0xffff
            if pre[scratch] != expected:
                faults.append([index, 'compensated target', scratch,
                               expected, pre[scratch]])
        motion_in.append([e[k] for k in ('x','y','target_x','target_y','vx','vy',
                         'boost','profile_42','dispatch_dt')] +
                         [int(e['z'] != 0 or e['live'] == 0x81), e['owner']])
        motion_out.append([post[k] for k in ('vx','vy','boost')] + [ve['dp_aa']])
        changed_velocity += int((e['vx'],e['vy']) != (post['vx'],post['vy']))
        inside = all(-9 <= delta(restored[t], restored[p]) <= 8
                     for t,p in (('target_x','x'),('target_y','y')))
        if ('arrived' in row) != inside:
            faults.append([index, 'raw arrival branch', inside])
        arrived_count += int(inside)
        rejected_count += int(not inside)
        if inside and 'prepared' in row:
            a, p = row['arrived'], row['prepared']
            arrival_in.append([a[k] for k in ('dead','attachment','flags','vx','vy',
                'ready','whistle','event','transfer','receiver','direction',
                'draw_direction')])
            arrival_out.append([p[k] for k in ('dead','attachment','flags','vx','vy',
                'ready','whistle','event','transfer','draw_direction')])
    actual_motion = run_probe(motion_probe, motion_in, 4)
    actual_arrival = run_probe(arrival_probe, arrival_in, 10)
    for name, actual, expected in (('motion',actual_motion,motion_out),
                                  ('arrival',actual_arrival,arrival_out)):
        faults.extend([name, i, want, got] for i,(want,got) in
                      enumerate(zip(expected,actual),1) if want != got)
    return {'result':'PASS' if not faults else 'FAIL', 'calls':len(rows),
            'motion_calls':len(motion_in),'arrival_prepared_calls':len(arrival_in),
            'arrived':arrived_count,'rejected':rejected_count,
            'same_dispatch_velocity_changes':changed_velocity,
            'controller_words':sorted(controllers),'velocity_signs':sorted(signs),
            'issues':faults,
            'scope':'native internal target/branch and production helper projection; '
                    'no whole-runtime or natural-menu parity claim'}


def main():
    p=argparse.ArgumentParser()
    source=p.add_mutually_exclusive_group(required=True)
    source.add_argument('--capture',type=Path)
    source.add_argument('--vectors',type=Path)
    p.add_argument('--motion-probe',required=True,type=Path)
    p.add_argument('--arrival-probe',required=True,type=Path)
    p.add_argument('--output',type=Path)
    a=p.parse_args()
    if a.capture:
        metadata=json_read(a.capture.parent/'inbound-internal.meta.json')
        manifest=json_read(a.capture.parent/'run.json')
        rows=[json.loads(line,object_pairs_hook=object_without_duplicates)
              for line in a.capture.read_text().splitlines()]
        capture_sha256=hashlib.sha256(a.capture.read_bytes()).hexdigest()
        if metadata.get('schema')!='nba95-inbound-internal-v1' or \
                metadata.get('requested_calls')!=len(rows) or \
                manifest.get('calls')!=len(rows) or \
                manifest.get('trace_sha256')!=capture_sha256 or \
                metadata.get('runtime_state_injection') is not False or \
                manifest.get('runtime_state_injection') is not False:
            raise ValueError('missing, truncated, altered, or wrongly classified capture')
    else:
        rows,manifest=read_compact(a.vectors)
        capture_sha256=manifest['trace_sha256']
    report=verify(rows,a.motion_probe,a.arrival_probe)
    report['capture_sha256']=capture_sha256
    report['rom_sha256']=manifest['rom_sha256']
    report['setup_intervention']=manifest['setup_mode_injection']
    report['input_kind']='raw native capture' if a.capture else 'retained native full-stage fixture'
    if a.vectors:
        report['fixture_sha256']=hashlib.sha256(a.vectors.read_bytes()).hexdigest()
    report['probe_sha256']={str(path):hashlib.sha256(path.read_bytes()).hexdigest()
                            for path in (a.motion_probe,a.arrival_probe)}
    if a.output: a.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))
    if report['issues']: raise SystemExit(1)


if __name__=='__main__': main()
