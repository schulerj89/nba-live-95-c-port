"""Exact native configuration projection through real C menu callers.

The C adapter enters Game Setup directly; native journeys execute from boot.
These are stable 24-word configuration checkpoints under identical button
schedules, not frame/intro equivalence or proof of runtime option effects.
"""
import argparse
import json
from pathlib import Path
import subprocess

from differential_compare import object_without_duplicates
from normalize_setup_config import read_compact,sha


def run_probe(probe,rom,pack,actions):
    payload=''.join(f"{a['key']} {a['hold']} {a['wait']}\n" for a in actions)
    run=subprocess.run([str(probe),str(rom),str(pack)],input=payload,text=True,
                       capture_output=True,check=True,timeout=60)
    return parse_probe_output(run.stdout,len(actions))


def parse_probe_output(stdout,action_count):
    if type(action_count) is not int or action_count<0:
        raise ValueError('invalid C action population')
    rows=[]
    for line in stdout.splitlines():
        if line.startswith('CONFIG_STATE '):
            row=json.loads(line.removeprefix('CONFIG_STATE '),object_pairs_hook=object_without_duplicates)
            if set(row)!={'action','scene','page','row','main','rules','options'}:
                raise ValueError('invalid C configuration projection schema')
            for field in ('action','scene','page','row'):
                if type(row[field]) is not int:raise ValueError('invalid C numeric field')
            for field,count in (('main',4),('rules',13),('options',7)):
                if type(row[field]) is not list or len(row[field])!=count or any(
                    type(value) is not int or not 0<=value<=65535 for value in row[field]):
                    raise ValueError('invalid C configuration word array')
            rows.append(row)
    if [row['action'] for row in rows]!=list(range(action_count+1)):
        raise ValueError('missing/extra/reordered C configuration output')
    return rows


def compare(journey,rows):
    native=[journey['states'][0]]+journey['states'][2::2]
    if len(native)!=len(rows):raise ValueError('configuration checkpoint population mismatch')
    issues=[]
    for index,(expected,actual) in enumerate(zip(native,rows)):
        for field in ('main','rules','options'):
            for word,(want,got) in enumerate(zip(expected[field],actual[field])):
                if want!=got:issues.append({'action':index,'field':field,'index':word,
                                            'native':want,'C':got})
    return issues


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('fixture','probe','rom','pack','report'):parser.add_argument('--'+name,required=True)
    parser.add_argument('--journey',action='append')
    args=parser.parse_args()
    journeys=read_compact(args.fixture)
    selected=set(args.journey or ('presets-v2','rules-v2','options-v2','held-v2'))
    if selected-{j['name'] for j in journeys}:raise ValueError('unknown native journey')
    sources={key:{'path':str(Path(getattr(args,key)).resolve()),
                  'sha256':sha(getattr(args,key))}
             for key in ('fixture','probe','rom','pack')}
    reports=[]
    for journey in journeys:
        if journey['name'] not in selected:continue
        if journey['native_manifest']['journey']=='load':
            raise ValueError('C disk persistence adapter is not implemented; cannot claim reload proof')
        if sha(args.rom)!=journey['native_manifest']['sources']['rom']['sha256']:
            raise ValueError('C input ROM differs from native capture')
        rows=run_probe(args.probe,args.rom,args.pack,journey['actions'])
        issues=compare(journey,rows)
        reports.append({'journey':journey['name'],'result':'FAIL' if issues else 'PASS',
                        'checkpoints':len(rows),'compared_words':24*len(rows),
                        'issues':issues,'actual':rows})
    passed=bool(reports) and all(r['result']=='PASS' for r in reports)
    for key,source in sources.items():
        if sha(getattr(args,key))!=source['sha256']:
            raise ValueError('C replay source changed during execution: '+key)
    report={'result':'PASS' if passed else 'FAIL',
        'scope':'stable 24-word native configuration projection through C menu callers; no native timing/graphics/audio proof',
        'exclusions':['C disk reload','working-buffer/Custom-storage projection','gameplay consumers'],
        'sources':sources,'journeys':reports}
    Path(args.report).write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'result':report['result'],'journeys':[
        {'name':r['journey'],'checkpoints':r['checkpoints'],'issues':len(r['issues']),
         'first':r['issues'][:1]} for r in reports]}))
    return 0 if passed else 1


if __name__=='__main__':raise SystemExit(main())
