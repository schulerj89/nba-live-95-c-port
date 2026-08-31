"""Replay the production event dispatcher against independent native calls.

Only native input words and explicit downstream command-return A values go to
the C process. Expected output/call arguments never enter that process.
"""
import argparse,json,subprocess
from pathlib import Path
from normalize_audio_events import read_fixture,sha,require,uint
from differential_compare import object_without_duplicates

def cases(data):
    return [(capture['name'],case) for capture in data['captures'] for case in capture['cases']]
def input_text(rows):
    return ''.join(' '.join(map(str,[i,*case['input'],len(case['external_command_returns_a']),
                    *case['external_command_returns_a']]))+'\n' for i,(_,case) in enumerate(rows))
def compare(rows,text):
    lines=text.splitlines();require(len(lines)==len(rows),'C output case count mismatch')
    issues=[];operations=0
    for i,((name,expected),line) in enumerate(zip(rows,lines)):
        got=json.loads(line,object_pairs_hook=object_without_duplicates)
        require(type(got) is dict and set(got)=={'id','output','returns_consumed','operations'},'invalid C output schema')
        require(type(got['id']) is int and got['id']==i,'C case order changed')
        require(type(got['output']) is list and len(got['output'])==3,'invalid C output state')
        for v in got['output']:uint(v)
        uint(got['returns_consumed'],14)
        require(type(got['operations']) is list and len(got['operations'])<=19,'invalid C operation population')
        for op in got['operations']:
            require(type(op) is list and len(op)==9,'incomplete C operation')
            for n,v in enumerate(op):uint(v,0xffffff if n in (1,2) else 65535)
        wanted={'id':i,'output':expected['output'],
                'returns_consumed':len(expected['external_command_returns_a']),
                'operations':expected['operations']}
        if got!=wanted:issues.append({'capture':name,'dispatch':expected['dispatch'],
            'provenance':expected['provenance'],'expected':wanted,'actual':got})
        operations+=len(expected['operations'])
    return {'result':'PASS' if not issues else 'FAIL','cases':len(rows),
        'ordered_operations':operations,'issues':issues,
        'scope':'Exact82FD65 event/RNG/ordered-callee projection with native downstream A returns as explicit external inputs. NMI scheduling, driver allocation/SPC synthesis and normal C caller wiring are excluded.'}
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',required=True)
    p.add_argument('--probe',required=True);p.add_argument('--report',required=True)
    a=p.parse_args();data=read_fixture(a.fixture);rows=cases(data)
    probe_before=sha(a.probe);fixture_before=sha(a.fixture)
    result=subprocess.run([a.probe],input=input_text(rows),text=True,capture_output=True)
    require(result.returncode==0,'C probe failed: '+str(result.returncode)+' '+result.stderr)
    report=compare(rows,result.stdout)
    require(sha(a.probe)==probe_before and sha(a.fixture)==fixture_before,'verification source changed while running')
    report['sources']={'probe':{'path':str(Path(a.probe).resolve()),'sha256':probe_before},
                       'fixture':{'path':str(Path(a.fixture).resolve()),'sha256':fixture_before}}
    out=Path(a.report);require(not out.exists(),'report must be new');out.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ('sources','issues')}))
    if report['issues']:print(json.dumps(report['issues'][:2]));return 1
    return 0
if __name__=='__main__':raise SystemExit(main())
