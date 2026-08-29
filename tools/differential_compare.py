"""Strict first-divergence gate for cpu-sweep-v1. No offsets or sentinels.

This validates an explicit state projection, never whole-game equivalence.
Baseline must match before any subsequent evolution can be called equivalent.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path

SCHEMA = Path(__file__).with_name('differential_fields.def')
CONTRACT = 'cpu-sweep-v1'

def schema():
    fields = {}
    for line in SCHEMA.read_text().splitlines():
        m = re.fullmatch(r'(WORD|ACTOR_WORD)\((0x[0-9A-Fa-f]+), "([a-z0-9_.]+)", (.+)\)', line)
        if not m:
            if line.startswith(('WORD(', 'ACTOR_WORD(')):
                raise ValueError('malformed schema line')
            continue
        kind, raw, name, _ = m.groups()
        for actor in range(10) if kind == 'ACTOR_WORD' else (None,):
            address = int(raw,16) + (0x34eb + actor*256 if actor is not None else 0)
            key = f'{address:04x}'
            if key in fields: raise ValueError('duplicate schema address')
            fields[key] = f'actors.{actor}.{name}' if actor is not None else name
    if not fields: raise ValueError('empty schema')
    return fields

def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def object_without_duplicates(pairs):
    out = {}
    for k,v in pairs:
        if k in out: raise ValueError(f'duplicate JSON key {k}')
        out[k] = v
    return out

def load(path):
    return [json.loads(line, object_pairs_hook=object_without_duplicates)
            for line in Path(path).read_text().splitlines() if line.strip()]

def validate(rows, fields, sweeps, label, checkpoint_plan=None):
    phases = checkpoint_plan if checkpoint_plan is not None else (
        ['baseline'] + ['actors.begin', 'actors.end'] * sweeps)
    if not phases or any(type(p) is not str or not p for p in phases):
        raise ValueError('invalid checkpoint plan')
    if len(rows) != len(phases):
        raise ValueError(f'{label}: expected {len(phases)} checkpoints, got {len(rows)}')
    previous_frame = -1
    for i,r in enumerate(rows):
        if not isinstance(r,dict):
            raise ValueError(f'{label}: checkpoint {i} is not an object')
        phase = phases[i]
        if type(r.get('sequence')) is not int or r.get('sequence') != i or r.get('checkpoint') != phase:
            raise ValueError(f'{label}: missing, reordered or duplicated checkpoint {i}')
        frame = r.get('outer_frame')
        if type(frame) is not int or frame < previous_frame or (i==0 and frame!=0):
            raise ValueError(f'{label}: invalid frame coordinate at {i}')
        previous_frame = frame
        inputs = r.get('inputs')
        if inputs != [0]*5 or any(type(v) is not int for v in inputs):
            raise ValueError(f'{label}: v1 supports only five neutral controllers')
        state = r.get('state')
        if not isinstance(state,dict) or set(state) != set(fields):
            raise ValueError(f'{label}: incomplete/unexpected state fields at {i}')
        if any(type(v) is not int or not 0<=v<=65535 for v in state.values()):
            raise ValueError(f'{label}: state is not raw unsigned16 at {i}')
        writers=r.get('writers')
        if not isinstance(writers,dict) or set(writers)-set(fields):
            raise ValueError(f'{label}: invalid writer metadata at {i}')
        if any(type(v) is not int or not 0<=v<=0xffffff for v in writers.values()):
            raise ValueError(f'{label}: invalid writer PC at {i}')

def compare(rom,port,sweeps,fields=None,checkpoint_plan=None):
    fields = schema() if fields is None else fields
    validate(rom,fields,sweeps,'ROM',checkpoint_plan)
    validate(port,fields,sweeps,'C',checkpoint_plan)
    # NMI can split a native sweep across video frames. State comparisons use
    # exact logical checkpoints; wall-frame differences remain visible, not
    # relabeled as state differences or concealed with a fitted offset.
    timing=[dict(sequence=i,rom_frame=a['outer_frame'],port_frame=b['outer_frame'])
            for i,(a,b) in enumerate(zip(rom,port)) if a['outer_frame']!=b['outer_frame']]
    for i,(a,b) in enumerate(zip(rom,port)):
        diffs=[]
        for key,name in fields.items():
            if a['state'][key] != b['state'][key]:
                diffs.append(dict(field=name,wram=f'7E:{key.upper()}',rom=a['state'][key],
                    port=b['state'][key],rom_hex=f"{a['state'][key]:04X}",
                    port_hex=f"{b['state'][key]:04X}",last_write_observed_pc=a['writers'].get(key)))
        if diffs:
            return dict(status='INITIAL_STATE_MISMATCH' if i==0 else 'DIVERGENCE',
                compared_fields=len(fields),
                sequence=i,checkpoint=a['checkpoint'],matching_checkpoints=i,
                rom_frame=a['outer_frame'],port_frame=b['outer_frame'],differences=diffs,
                timing_differences=timing,
                previous_matching_checkpoint=None if i==0 else rom[i-1]['checkpoint'],
                scope='partial raw-state projection; no whole-game equivalence claim')
    return dict(status='PROJECTION_MATCH',compared_fields=len(fields),matching_checkpoints=len(rom),sweeps=sweeps,
        timing_differences=timing,
        scope='partial raw-state projection only; unrepresented state/callers remain unproven')

def main():
    p=argparse.ArgumentParser();p.add_argument('--rom-trace',required=True);p.add_argument('--port-trace',required=True)
    p.add_argument('--sweeps',type=int,required=True);p.add_argument('--report',required=True)
    a=p.parse_args()
    try:
        if not 1<=a.sweeps<=1000:raise ValueError('sweeps outside1..1000')
        result=compare(load(a.rom_trace),load(a.port_trace),a.sweeps)
        code=0 if result['status']=='PROJECTION_MATCH' else 1
    except (ValueError,TypeError,KeyError,OSError) as e:
        result=dict(status='INVALID_CAPTURE',error=str(e));code=2
    result.update(contract=CONTRACT,schema_sha256=digest(SCHEMA))
    Path(a.report).write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2)[:3000]);return code
if __name__=='__main__':raise SystemExit(main())
