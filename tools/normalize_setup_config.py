"""Retain natural native Setup configuration observations without invoking C."""
import argparse
import hashlib
import json
from pathlib import Path,PureWindowsPath
import re

from differential_compare import object_without_duplicates
from verify_ppu_brightness import validate_settings,SAMPLE_POINTS,same_settings

ROM_SHA256='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
MANIFEST_WITNESSES=Path(__file__).resolve().parents[1]/'tests/fixtures/setup-config-manifest-witnesses.json'
MANIFEST_WITNESSES_SHA256='dc6bbc1cc202807aebd9f780395bd46f4ee0ecac54425d2deac517f557d8b34b'

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


def canonical_bytes(value):
    return json.dumps(value,sort_keys=True,separators=(',',':')).encode('utf-8')


def manifest_witnesses():
    """A separately retained, reviewed copy binds provenance as well as rows.

    Changing fixture metadata and its local hashes cannot silently relabel a
    controlled/failed capture. A new accepted native corpus requires review of
    this registry too. These are native source identities, never C goldens.
    """
    raw=MANIFEST_WITNESSES.read_bytes()
    if hashlib.sha256(raw).hexdigest()!=MANIFEST_WITNESSES_SHA256:
        raise ValueError('native manifest witness registry changed')
    data=json.loads(raw,object_pairs_hook=object_without_duplicates)
    if data.get('schema')!='nba95-setup-native-manifest-witnesses-v1':
        raise ValueError('unsupported native manifest witness registry')
    return {row['name']:row for row in data['journeys']}


def validate_manifest(manifest,name,*,bind=False):
    """Validate provenance in BOTH raw capture and permanent replay paths."""
    if type(manifest) is not dict or manifest.get('classification')!=\
            'natural controller-only configuration journey' or any(
            manifest.get(key) is not False for key in
            ('cpu_state_injection','rom_patch','wram_injection','sram_injection')):
        raise ValueError('incorrect native journey provenance')
    if manifest.get('journey') not in ('presets','rules','options','load','held','main','input','faces'):
        raise ValueError('unknown native journey classification')
    for key,expected in (('default_input_pulse_frames',3),('ordinary_action_period',60),
                         ('transition_action_period',260)):
        if type(manifest.get(key)) is not int or manifest[key]!=expected:
            raise ValueError('invalid native input schedule metadata')
    # The six original captures did not serialize ExitCode. Do not manufacture
    # a0: admit ONLY their independently pinned legacy manifests, whose hashed
    # runners publish the final manifest after the successful exit guard.
    witness=None
    if bind or 'process_exit_code' not in manifest:
        witness=manifest_witnesses().get(name)
        if witness is None or hashlib.sha256(canonical_bytes(manifest)).hexdigest()!=\
                witness['canonical_manifest_sha256'] or manifest!=witness['manifest']:
            raise ValueError('native manifest differs from retained original witness')
    if 'process_exit_code' in manifest:
        if type(manifest['process_exit_code']) is not int or manifest['process_exit_code']!=0:
            raise ValueError('native capture process exit was not0')
    elif witness.get('exit_evidence')!='legacy_runner_success_path_only_no_recorded_exit_code':
        raise ValueError('missing native process exit evidence')
    sources=manifest.get('sources')
    required={'rom','mesen','script','runner','settings','actions.json','action-states.jsonl','events.jsonl'}
    if type(sources) is not dict or not required<=sources.keys():
        raise ValueError('missing native source identities')
    for key,source in sources.items():
        if type(key) is not str or type(source) is not dict or set(source)!={'path','sha256'} or \
                type(source['path']) is not str or not PureWindowsPath(source['path']).is_absolute() or \
                type(source['sha256']) is not str or not re.fullmatch('[0-9a-f]{64}',source['sha256']):
            raise ValueError('invalid native source identity')
    if sources['rom']['sha256']!=ROM_SHA256:raise ValueError('wrong native ROM identity')
    isolation=manifest.get('isolation',{})
    if type(isolation) is not dict or isolation.get('method')!=\
            'private portable executable/settings' or isolation.get('post_settings_verified') is not True:
        raise ValueError('unverified portable Mesen home')
    directory=PureWindowsPath(sources['script']['path']).parent
    home=directory/'portable-mesen';saves=directory/'isolated-saves'
    for key,expected in (('home',home),('save_folder',saves)):
        if type(isolation.get(key)) is not str or PureWindowsPath(isolation[key])!=expected:
            raise ValueError('native private directory mismatch')
    observed=isolation.get('observed_script_data_folder')
    if type(observed) is not str or not PureWindowsPath(observed).is_relative_to(home/'LuaScriptData'):
        raise ValueError('native observed home outside private directory')
    if PureWindowsPath(sources['mesen']['path'])!=home/'Mesen.exe' or \
            PureWindowsPath(sources['settings']['path'])!=home/'settings.json':
        raise ValueError('native executable/settings outside private home')
    settings=isolation.get('settings',{})
    if type(settings) is not dict or not same_settings(settings.get('Debug'),
            {'ScriptWindow':{'AllowIoOsAccess':True,'ScriptTimeout':60,'SaveScriptBeforeRun':False}}) or \
            not same_settings(settings.get('Preferences'),{'SingleInstance':False,
                'PauseWhenInBackground':False,'AutoLoadPatches':False,'OverrideSaveDataFolder':True,
                'SaveDataFolder':isolation['save_folder']}):
        raise ValueError('unverified native capture preferences')
    validate_settings(settings.get('Video'),settings.get('Snes'),SAMPLE_POINTS,[256,239])
    arguments=manifest.get('arguments')
    if type(arguments) is not list or len(arguments)!=4 or arguments[:2]!=['--testrunner','--timeout=240'] or \
            any(type(v) is not str for v in arguments) or \
            PureWindowsPath(arguments[2].strip('"'))!=PureWindowsPath(sources['rom']['path']) or \
            PureWindowsPath(arguments[3].strip('"'))!=PureWindowsPath(sources['script']['path']):
        raise ValueError('native executable arguments disagree with sources')
    for key in ('initial_save_files','final_save_files'):
        entries=manifest.get(key)
        if type(entries) is not list or len(entries)>1:
            raise ValueError('invalid native save provenance')
        for source in entries:
            expected={'path','sha256'}|({'size'} if key=='final_save_files' else set())
            if type(source) is not dict or set(source)!=expected or \
                    type(source['path']) is not str or not PureWindowsPath(source['path']).is_absolute() or \
                    type(source['sha256']) is not str or not re.fullmatch('[0-9a-f]{64}',source['sha256']) or \
                    (key=='final_save_files' and (type(source['size']) is not int or source['size']!=8192)):
                raise ValueError('invalid native save identity')
    if manifest['initial_save_files'] and manifest['journey']!='load':
        raise ValueError('unexpected native reload prestate')


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
        keys=action['key'].split('+') if type(action['key']) is str else []
        if not keys or len(set(keys))!=len(keys) or any(key not in
                ('none','up','down','left','right','a','b','start','x','y','l','r','select') for key in keys) or \
                ('none' in keys and len(keys)!=1) or \
                type(action['label']) is not str or not action['label']:
            raise ValueError('invalid native controller action')
        integer(action['hold'],hi=1000);integer(action['wait'],lo=1,hi=1000)
        # Equal durations intentionally preserve a held word across action
        # boundaries. No release frame may be invented by the C adapter.
        if action['hold']>action['wait']:raise ValueError('hold exceeds action duration')
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
    validate_manifest(manifest,directory.name)
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
    if settings!=isolation['settings']:raise ValueError('native stored settings differ from manifest')
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
        validate_manifest(journey['native_manifest'],journey['name'],bind=True)
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
