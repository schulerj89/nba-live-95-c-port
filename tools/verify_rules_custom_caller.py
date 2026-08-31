"""Native-backed bounded Rules Custom effect through actual C menu dispatch.

C starts from explicitly supplied native configuration and enters Setup directly.
This excludes C factory defaults, presets, persistence, rendering and game effects.
"""
import argparse
import json
from pathlib import Path
import subprocess

from differential_compare import object_without_duplicates
from normalize_setup_config import read_compact,sha


def verify(fixture,probe,rom,pack):
    journeys={j['name']:j for j in read_compact(fixture)}
    cases=[('right45_B_left45_commit','presets-v2','rules',13,16),
           ('left0_then_right0','rules-v2','rules',7,8),
           ('options_do_not_mark_custom','options-v2','options',7,9)]
    reports=[]
    sources={str(Path(path).resolve()):sha(path) for path in (fixture,probe,rom,pack)}
    for name,journey_name,page,first,last in cases:
        journey=journeys[journey_name]
        if sha(rom)!=journey['native_manifest']['sources']['rom']['sha256']:
            raise ValueError('C ROM differs from captured native ROM')
        before=journey['states'][2*first-1]
        actions=journey['actions'][first-1:last]
        expected=[before]+[journey['states'][2*i] for i in range(first,last+1)]
        seed=before['main']+before['rules']+before['options']
        payload=' '.join(map(str,seed))+'\n'+''.join(
            f"{action['key']} {action['hold']} {action['wait']}\n" for action in actions)
        process=subprocess.run([str(probe),str(rom),str(pack),page],input=payload,
            text=True,capture_output=True,check=True,timeout=60)
        actual=[json.loads(line.removeprefix('CONFIG_STATE '),object_pairs_hook=object_without_duplicates)
                for line in process.stdout.splitlines() if line.startswith('CONFIG_STATE ')]
        if [row.get('action') for row in actual]!=list(range(len(expected))):
            raise ValueError('missing/extra/reordered C caller snapshot')
        issues=[]
        for i,(want,got) in enumerate(zip(expected,actual)):
            if set(got)!={'action','scene','page','row','main','rules','options','working'} or \
                    any(type(got[field]) is not int for field in ('action','scene','page','row')):
                raise ValueError('invalid C caller snapshot schema')
            # include/nba_game.h: NBA_STATE_GAME_SETUP == 5. All compared
            # boundaries must still belong to the production Setup scene.
            if got['scene']!=5:
                issues.append({'case':name,'snapshot':i,'field':'scene',
                               'C':got['scene'],'expected':5})
            menu_page=0 if i and actions[i-1]['key']=='start' else 1 if page=='rules' else 2
            if got['page']!=menu_page or got['row']!=want['row']:
                issues.append({'case':name,'snapshot':i,'field':'page/row','C':[got['page'],got['row']],
                               'expected':[menu_page,want['row']]})
            count=4 if menu_page==0 else 13 if page=='rules' else 7
            for field,words in (('main',4),('rules',13),('options',7),('working',count)):
                if type(got[field]) is not list or len(got[field])!=words or \
                        any(type(value) is not int or not 0<=value<=65535 for value in got[field]):
                    raise ValueError('invalid C caller word array')
                for word in range(words):
                    if want[field][word]!=got[field][word]:
                        issues.append({'case':name,'snapshot':i,'field':field,'index':word,
                                       'native':want[field][word],'C':got[field][word]})
        reports.append({'name':name,'result':'FAIL' if issues else 'PASS','native_journey':journey_name,
            'native_actions':[first,last],'C_initial_configuration':seed,'actions':actions,
            'actual':actual,'issues':issues,'stdout':process.stdout})
    if any(sha(path)!=expected for path,expected in sources.items()):
        raise ValueError('C caller input/source changed while running')
    return {'result':'PASS' if all(case['result']=='PASS' for case in reports) else 'FAIL',
        'scope':'controlled native configuration prestate; real C Setup/menu input dispatch; exact working/committed words',
        'exclusions':['factory defaults','Arcade/Simulation/Custom presets','disk persistence',
                      'pixel/timing equivalence','in-game runtime consumers'],
        'sources':sources,'cases':reports}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for field in ('fixture','probe','rom','pack','report'):p.add_argument('--'+field,required=True)
    a=p.parse_args();report=verify(a.fixture,a.probe,a.rom,a.pack)
    Path(a.report).write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'result':report['result'],'cases':[
        {'name':row['name'],'result':row['result'],'issues':row['issues']} for row in report['cases']]}))
    return 0 if report['result']=='PASS' else 1


if __name__=='__main__':raise SystemExit(main())
