"""Retain natural native Setup configuration observations without invoking C."""
import argparse
import hashlib
import json
from pathlib import Path

from differential_compare import object_without_duplicates
from verify_ppu_brightness import validate_settings,SAMPLE_POINTS

FIELDS=tuple(sorted(('kind','pc','frame','setup_frame','action','main','working',
    'rules','options','sram48_56','sram_marker','row','value','maximum','controller',
    'repeat_input','held_input','previous_input','pending_input','repeat_delay',
    'repeat_speed','repeat_flag','dirty','original_style','a','x','y','p',
    'brightness','forced_blank')))
ARRAYS={'main':4,'working':13,'rules':13,'options':7,'sram48_56':15}
PCS={0x81c19a,0x81c1a9,0x81c232,0x81c24a,0x81c24b,0x81bfaa,0x81c00b,
     0x81c398,0x81c3d3,0x81c3d5,0x81c41d,0x81bed5,0x81bee6,0x81bf59,
     0x81bf6a,0x81d446,0x81d4c0,0x81d47a,0x81d494,0x81d4a9,0x81d4fa,
     0x81d52f,0x81d53b,0x828d92,0x828e5f,0x828dc6,0x828dda,0x828d0a,
     0x828eb3,0x828ecb,0x828ed4,0x828ee4}


def read_json(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'),
                      object_pairs_hook=object_without_duplicates)


def read_lines(path):
    return [json.loads(line,object_pairs_hook=object_without_duplicates)
            for line in Path(path).read_text().splitlines()]


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def integer(value,lo=0,hi=65535):
    if type(value) is not int or not lo<=value<=hi:
        raise ValueError('invalid native configuration integer')


def validate_row(row):
    if type(row) is not dict or set(row)!=set(FIELDS):
        raise ValueError('missing/extra native configuration fields')
    for key,value in row.items():
        if key in ARRAYS:
            if type(value) is not list or len(value)!=ARRAYS[key]:
                raise ValueError('incomplete native configuration array')
            for word in value:integer(word,hi=255 if key=='sram48_56' else 65535)
        elif key=='forced_blank':
            if type(value) is not bool:raise ValueError('invalid PPU flag type')
        elif key=='kind':
            if value not in ('initial_setup','before_action','after_action','boundary'):
                raise ValueError('unknown native record kind')
        else:
            integer(value,lo=-1 if key=='setup_frame' else 0,
                    hi=0xffffff if key=='pc' else 0x7fffffff if key in
                    ('frame','setup_frame') else 255 if key in ('sram_marker','p')
                    else 15 if key=='brightness' else 65535)


def validate(actions,states,events):
    if type(actions) is not list or not actions:raise ValueError('empty native input journey')
    for action in actions:
        if type(action) is not dict or set(action)!={'key','label','wait','hold'}:
            raise ValueError('invalid native action schema')
        if action['key'] not in ('none','up','down','left','right','a','b','start') or \
                type(action['label']) is not str or not action['label']:
            raise ValueError('invalid native controller action')
        integer(action['hold'],hi=1000);integer(action['wait'],lo=1,hi=1000)
        if action['hold']>=action['wait']:raise ValueError('action has no release interval')
    if len(states)!=1+2*len(actions):raise ValueError('missing native action boundaries')
    for row in states+events:validate_row(row)
    initial=states[0]
    if initial['kind']!='initial_setup' or initial['action']!=0 or \
            initial['setup_frame']!=400 or initial['pc']!=0:
        raise ValueError('invalid initial native Setup boundary')
    previous=initial
    for i,action in enumerate(actions,1):
        before,after=states[2*i-1:2*i+1]
        if before['kind']!='before_action' or after['kind']!='after_action' or \
                before['action']!=i or after['action']!=i or before['pc'] or after['pc']:
            raise ValueError('wrong/reordered native action pair')
        if any(before[k]!=previous[k] for k in FIELDS if k not in ('kind','action')):
            raise ValueError('missing state between consecutive native actions')
        if after['frame']-before['frame']!=action['wait'] or \
                after['setup_frame']-before['setup_frame']!=action['wait']:
            raise ValueError('native input schedule/frame mismatch')
        previous=after
    prior=-1
    for event in events:
        if event['kind']!='boundary' or event['pc'] not in PCS or \
                event['frame']<prior or event['action']>len(actions):
            raise ValueError('invalid/reordered native execution boundary')
        if event['action']:
            before,after=states[2*event['action']-1:2*event['action']+1]
            if not before['frame']<=event['frame']<=after['frame']:
                raise ValueError('native execution outside its input action')
        prior=event['frame']
    if not events or events[0]['pc']!=0x81c19a:
        raise ValueError('missing native factory/load entry')


def capture(path):
    directory=Path(path).resolve();manifest=read_json(directory/'manifest.json')
    if not (directory/'capture_complete.txt').is_file():raise ValueError('incomplete capture')
    if manifest['classification']!='natural controller-only configuration journey' or \
            any(manifest.get(key) is not False for key in
                ('cpu_state_injection','rom_patch','wram_injection','sram_injection')):
        raise ValueError('incorrect native journey provenance')
    isolation=manifest['isolation']
    if isolation['method']!='private portable executable/settings' or \
            isolation.get('post_settings_verified') is not True:
        raise ValueError('unverified portable Mesen home')
    observed=(directory/'observed-script-data-folder.txt').read_text().strip()
    if observed!=isolation['observed_script_data_folder'] or not \
            Path(observed).resolve().is_relative_to(directory/'portable-mesen'/'LuaScriptData'):
        raise ValueError('native Lua home differs from private home')
    for source in manifest['sources'].values():
        if sha(source['path'])!=source['sha256']:raise ValueError('native source changed')
    settings=read_json(manifest['sources']['settings']['path'])
    validate_settings(settings['Video'],settings['Snes'],SAMPLE_POINTS,[256,239])
    actions=read_json(directory/'actions.json')
    states=read_lines(directory/'action-states.jsonl');events=read_lines(directory/'events.jsonl')
    validate(actions,states,events)
    return {'name':directory.name,'native_manifest':manifest,'actions':actions,
            'states':states,'events':events}


def summarize(journey):
    result={'name':journey['name'],'actions':len(journey['actions']),
            'boundary_events':len(journey['events']),
            'initial_main':journey['states'][0]['main'],
            'initial_rules':journey['states'][0]['rules'],
            'initial_options':journey['states'][0]['options']}
    coverage={}
    for i,action in enumerate(journey['actions'],1):
        if action['key'] not in ('left','right'):continue
        before,after=journey['states'][2*i-1:2*i+1]
        row=before['row']
        coverage.setdefault((row,action['key']),set()).add(
            (before['working'][row],after['working'][row]))
    result['observed_working_edges']=[{'row':key[0],'direction':key[1],
        'edges':[list(edge) for edge in sorted(edges)]} for key,edges in sorted(coverage.items())]
    return result


def compact(journeys):
    encoded=[]
    for journey in journeys:
        result={key:value for key,value in journey.items() if key not in ('states','events')}
        for key in ('states','events'):
            result[key]=[[row[field] for field in FIELDS] for row in journey[key]]
        encoded.append(result)
    return {'schema':'nba95-native-setup-configuration-v1',
            'scope':'natural menu configuration and observed routine boundaries; no runtime options-effect or whole-frame proof',
            'row_fields':list(FIELDS),'journeys':encoded}


def read_compact(path):
    """Verify lossless encoded rows against the recorded original JSONL hashes."""
    fixture=read_json(path)
    if fixture.get('schema')!='nba95-native-setup-configuration-v1' or \
            fixture.get('row_fields')!=list(FIELDS):
        raise ValueError('unsupported native configuration fixture')
    result=[];names=set()
    for encoded in fixture['journeys']:
        if set(encoded)!={'name','native_manifest','actions','states','events'} or \
                type(encoded['name']) is not str or encoded['name'] in names:
            raise ValueError('invalid/duplicate native journey')
        names.add(encoded['name'])
        journey={key:value for key,value in encoded.items() if key not in ('states','events')}
        for key,file in (('states','action-states.jsonl'),('events','events.jsonl')):
            rows=[]
            for values in encoded[key]:
                if type(values) is not list or len(values)!=len(FIELDS):
                    raise ValueError('incomplete encoded native snapshot')
                rows.append(dict(zip(FIELDS,values)))
            # Lua writes sorted JSON keys without whitespace. Reconstitution
            # preserves every original byte, so changing a captured word also
            # invalidates the recorded raw-trace identity.
            content=''.join(json.dumps(row,sort_keys=True,separators=(',',':'))+'\n'
                            for row in rows).encode('utf-8')
            if hashlib.sha256(content).hexdigest()!=journey['native_manifest']['sources'][file]['sha256']:
                raise ValueError('encoded native snapshots differ from original raw trace')
            journey[key]=rows
        actions=json.dumps(journey['actions'],sort_keys=True,separators=(',',':')).encode('utf-8')
        if hashlib.sha256(actions).hexdigest()!=journey['native_manifest']['sources']['actions.json']['sha256']:
            raise ValueError('encoded native input schedule changed')
        validate(journey['actions'],journey['states'],journey['events'])
        result.append(journey)
    if not result:raise ValueError('empty native configuration corpus')
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',action='append',required=True)
    parser.add_argument('--output',required=True);parser.add_argument('--report')
    args=parser.parse_args()
    target=Path(args.output)
    if target.exists():parser.error('native fixture output must be new')
    journeys=[capture(path) for path in args.capture]
    if len({journey['name'] for journey in journeys})!=len(journeys):
        raise ValueError('duplicate native journey')
    fixture=compact(journeys)
    target.write_text(json.dumps(fixture,separators=(',',':'))+'\n',encoding='utf-8')
    report={'fixture_sha256':sha(target),'journeys':[summarize(j) for j in journeys]}
    if args.report:Path(args.report).write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'journeys':len(journeys),'actions':sum(len(j['actions']) for j in journeys),
                      'events':sum(len(j['events']) for j in journeys),'fixture_sha256':sha(target)}))


if __name__=='__main__':main()
