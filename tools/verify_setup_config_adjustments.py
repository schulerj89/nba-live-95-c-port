"""Exact real-menu adjustment entry/exit and input-repeat comparison.

Native rows come from read-only original-ROM execution hooks. The C adapter
receives only the controller schedule, then observes production dispatches.
Offsets are the exact zero-based frame from the corresponding input pulse;
no window, shift search, clipping, tolerance or expected-value injection.
"""
import argparse
import json
from pathlib import Path
import subprocess

from differential_compare import object_without_duplicates
from normalize_setup_config import read_compact,sha
from verify_setup_config_runtime import compare as compare_states,parse_probe_output

ENTRY={0x81d446:0x200,0x81d4c0:0x100,0x828d92:0x200,0x828e5f:0x100}
EXIT={0x81d494,0x828dda}
FIELDS=('action','offset','pc','command','row','value','maximum','controller',
        'previous_input','pending_input','repeat_input','repeat_delay',
        'repeat_speed','repeat_flag','working','main','rules','options')
ARRAYS={'main':4,'rules':13,'options':7}


def native_adjustments(journey):
    rows=[];pending=None
    for event in journey['events']:
        pc=event['pc']
        if pc not in ENTRY and pc not in EXIT:continue
        if event['action']<1:raise ValueError('adjustment outside input journey')
        if pc in ENTRY:
            if pending is not None:raise ValueError('native adjustment lacks exit')
            pending=event
            command=ENTRY[pc]
        else:
            if pending is None or event['action']!=pending['action'] or \
                    event['frame']!=pending['frame'] or \
                    (pc==0x81d494)!=(pending['pc']<0x820000):
                raise ValueError('native adjustment exit without same-frame entry')
            command=ENTRY[pending['pc']]
            pending=None
        action=journey['actions'][event['action']-1]
        if action['key']!={0x200:'left',0x100:'right'}[command]:
            raise ValueError('native dispatcher/input schedule disagreement')
        before=journey['states'][2*event['action']-1]
        row={field:event[field] for field in FIELDS if field not in ('offset','command','working')}
        row.update(offset=event['frame']-before['frame'],command=command,
                   working=event['working'][:13 if pc<0x820000 else 7])
        rows.append(row)
    if pending is not None:raise ValueError('last native adjustment lacks exit')
    if not rows:raise ValueError('empty native adjustment corpus')
    return rows


def parse_adjustments(stdout,actions):
    rows=[];prior=(0,0)
    for line in stdout.splitlines():
        if not line.startswith('CONFIG_ADJUST '):continue
        row=json.loads(line.removeprefix('CONFIG_ADJUST '),object_pairs_hook=object_without_duplicates)
        if type(row) is not dict or set(row)!=set(FIELDS):raise ValueError('invalid C adjustment schema')
        for field in FIELDS:
            if field in (*ARRAYS,'working'):continue
            value=row[field]
            if type(value) is not int or not 0<=value<=(0xffffff if field=='pc' else 65535):
                raise ValueError('invalid C adjustment integer')
        if row['pc'] not in set(ENTRY)|EXIT or row['command'] not in (0x100,0x200) or \
                not 1<=row['action']<=len(actions) or \
                row['offset']>=actions[row['action']-1]['wait']:
            raise ValueError('C adjustment outside owned input/PC population')
        if row['pc'] in ENTRY and ENTRY[row['pc']]!=row['command']:
            raise ValueError('C adjustment command/PC mismatch')
        counts=dict(ARRAYS,working=13 if row['pc']<0x820000 else 7)
        for field,count in counts.items():
            values=row[field]
            if type(values) is not list or len(values)!=count or any(
                    type(value) is not int or not 0<=value<=65535 for value in values):
                raise ValueError('invalid C adjustment word array')
        at=(row['action'],row['offset'])
        if at<prior:raise ValueError('C adjustment timeline reordered')
        prior=at;rows.append(row)
    return rows


def compare(expected,actual):
    if len(expected)!=len(actual):
        return [{'field':'population','native':len(expected),'C':len(actual)}]
    issues=[]
    for index,(want,got) in enumerate(zip(expected,actual)):
        for field in FIELDS:
            if want[field]!=got[field]:issues.append({'event':index,'field':field,
                                                     'native':want[field],'C':got[field]})
    return issues


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('fixture','probe','rom','pack','report'):parser.add_argument('--'+name,required=True)
    parser.add_argument('--journey',action='append')
    args=parser.parse_args()
    journeys=read_compact(args.fixture)
    selected=set(args.journey or ('presets-v2','rules-v2','options-v2','held-v2'))
    if selected-{j['name'] for j in journeys}:raise ValueError('unknown native journey')
    sources={key:{'path':str(Path(getattr(args,key)).resolve()),'sha256':sha(getattr(args,key))}
             for key in ('fixture','probe','rom','pack')}
    reports=[]
    for journey in journeys:
        if journey['name'] not in selected:continue
        if sha(args.rom)!=journey['native_manifest']['sources']['rom']['sha256']:
            raise ValueError('C ROM differs from captured native ROM')
        payload=''.join(f"{a['key']} {a['hold']} {a['wait']}\n" for a in journey['actions'])
        run=subprocess.run([args.probe,args.rom,args.pack,'adjustments'],input=payload,
                           text=True,capture_output=True,check=True,timeout=60)
        native=native_adjustments(journey)
        actual=parse_adjustments(run.stdout,journey['actions'])
        states=parse_probe_output(run.stdout,len(journey['actions']))
        issues=compare(native,actual)
        state_issues=compare_states(journey,states)
        reports.append({'journey':journey['name'],'events':len(native),
            'result':'FAIL' if issues or state_issues else 'PASS',
            'issues':issues,'state_issues':state_issues,'native':native,'actual':actual})
    for key,source in sources.items():
        if sha(getattr(args,key))!=source['sha256']:raise ValueError('replay source changed: '+key)
    passed=bool(reports) and all(r['result']=='PASS' for r in reports)
    report={'result':'PASS' if passed else 'FAIL','sources':sources,
        'scope':__doc__,'journeys':reports}
    Path(args.report).write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'result':report['result'],'journeys':[
        {k:r[k] for k in ('journey','events','result')}|{'issues':len(r['issues']),
        'first':r['issues'][:1],'state_issues':len(r['state_issues'])} for r in reports]}))
    return 0 if passed else 1


if __name__=='__main__':raise SystemExit(main())
