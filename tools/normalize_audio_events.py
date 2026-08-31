"""Extract an explicit projection from native audio boundaries, never from C.

The unmodified raw CPU/DSP/WRAM traces remain ignored evidence and are hashed
here. The compact fixture owns event words, shared RNG and ordered call args;
it does not own SPC allocation, synthesis, playback timing or NMI scheduling.
"""
import argparse,hashlib,json,re
from collections import defaultdict
from pathlib import Path,PureWindowsPath
from differential_compare import object_without_duplicates
from verify_ppu_brightness import validate_settings,SAMPLE_POINTS,same_settings

ROM_SHA='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
# Native-only capture/projection identity. A different accepted corpus needs
# explicit evidence review; changing C output never authorizes this value.
NATIVE_FIXTURE_SHA256='20762a4a35ed00cf7f04084dacead537cf98884f57262b44244fa1aafc4393b3'
OP_FIELDS=['kind','caller_pc','target_pc','command','index','value','event13e7','crowd13e9','rng07f6']
INPUT_FIELDS=['event13e7','crowd13e9','bounce13e5','crowd17bb','rng07f6']
OUTPUT_FIELDS=['event13e7','crowd13e9','rng07f6']
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def canonical(v):return json.dumps(v,sort_keys=True,separators=(',',':')).encode()
def read_json(path):return json.loads(Path(path).read_text(encoding='utf-8-sig'),object_pairs_hook=object_without_duplicates)
def require(ok,message):
    if not ok:raise ValueError(message)
def uint(v,maximum=65535):require(type(v) is int and 0<=v<=maximum,'invalid native unsigned integer')
def raw_lines(path):return [json.loads(x,object_pairs_hook=object_without_duplicates) for x in Path(path).read_text().splitlines()]
def validate_manifest(m):
    require(type(m) is dict and type(m.get('exit_code')) is int and m['exit_code']==0,'native process did not exit0')
    require(m.get('mode') in ('natural','controlled') and m.get('rom_patch') is False and
            m.get('cpu_register_injection') is False and m.get('initial_save_files')==[],
            'invalid native capture provenance')
    require(m.get('navigation')=='normal controller-only title/Main/Options/Team/Player Setup, center selection1','native navigation classification changed')
    expected='none' if m['mode']=='natural' else 'declared audio input word writes only at82FD65 after100courtframes; all before/after writes retained'
    require(m.get('interventions')==expected,'native intervention classification changed')
    sources=m.get('sources')
    require(type(sources) is dict and set(sources)=={'rom','mesen','script','runner','settings'},'native source set changed')
    for source in sources.values():
        require(type(source) is dict and set(source)=={'path','sha256'} and type(source['path']) is str and
                PureWindowsPath(source['path']).is_absolute() and type(source['sha256']) is str and
                re.fullmatch('[0-9a-f]{64}',source['sha256']),'invalid native source identity')
    require(sources['rom']['sha256']==ROM_SHA,'incorrect ROM identity')
    iso=m.get('isolation',{});directory=PureWindowsPath(sources['script']['path']).parent
    home=directory/'portable-mesen';saves=directory/'isolated-saves'
    require(type(iso) is dict and iso.get('post_settings_verified') is True and
        type(iso.get('home')) is str and PureWindowsPath(iso['home'])==home and
        type(iso.get('save_folder')) is str and PureWindowsPath(iso['save_folder'])==saves and
        type(iso.get('observed_script_data_folder')) is str and
        PureWindowsPath(iso['observed_script_data_folder']).is_relative_to(home/'LuaScriptData'),
        'unverified native private home')
    require(PureWindowsPath(sources['mesen']['path'])==home/'Mesen.exe' and
        PureWindowsPath(sources['settings']['path'])==home/'settings.json','native runtime outside private home')
    settings=iso.get('settings',{})
    require(type(settings) is dict and same_settings(settings.get('Debug'),
        {'ScriptWindow':{'AllowIoOsAccess':True,'ScriptTimeout':60,'SaveScriptBeforeRun':False}}) and
        same_settings(settings.get('Preferences'),{'SingleInstance':False,'PauseWhenInBackground':False,
            'AutoLoadPatches':False,'OverrideSaveDataFolder':True,'SaveDataFolder':iso['save_folder']}),
        'unverified native preferences')
    validate_settings(settings.get('Video'),settings.get('Snes'),SAMPLE_POINTS,[256,239])
    require(m.get('arguments')==[sources['mesen']['path'],'--testrunner','--timeout=300',
            sources['rom']['path'],sources['script']['path']],'native executable arguments changed')
    completion=m.get('completion',{})
    require(type(completion) is dict and set(completion)=={'frames','court','dispatches','controlled_cases','mode'},'native completion schema changed')
    require(completion['mode']==m['mode'],'native mode/completion disagree')
    for field in ('frames','court','dispatches','controlled_cases'):uint(completion[field],0x7fffffff)
    require(completion['frames']>completion['court']>0 and completion['dispatches']>0 and
            completion['controlled_cases']==(0 if m['mode']=='natural' else 49),'incomplete native capture')
    artifacts=m.get('artifacts')
    require(type(artifacts) is dict and {'events.jsonl','writes.jsonl','frames.jsonl','audio-writes.jsonl',
            'capture_complete.json','observed-script-data-folder.txt','actions.json','actions.jsonl'}<=artifacts.keys(),
            'native artifact set incomplete')
    for name,source in artifacts.items():
        require(type(name) is str and PureWindowsPath(name).name==name and type(source) is dict and
                set(source)=={'bytes','sha256'},'invalid native artifact identity')
        uint(source['bytes'],0x7fffffff)
        require(type(source['sha256']) is str and re.fullmatch('[0-9a-f]{64}',source['sha256']),'invalid native artifact hash')

def capture(directory):
    directory=Path(directory).resolve();m=read_json(directory/'manifest.json')
    validate_manifest(m)
    for source in m['sources'].values():require(sha(source['path'])==source['sha256'],'native source changed')
    for name,source in m['artifacts'].items():
        require((directory/name).stat().st_size==source['bytes'] and sha(directory/name)==source['sha256'],'native artifact changed')
    require(m['isolation']['post_settings_verified'] is True,'settings not verified')
    require(Path(m['isolation']['observed_script_data_folder']).is_relative_to(directory/'portable-mesen'/'LuaScriptData'),'wrong portable home')
    settings=read_json(m['sources']['settings']['path'])
    require(settings==m['isolation']['settings'],'settings differ from manifest')
    validate_settings(settings['Video'],settings['Snes'],SAMPLE_POINTS,[256,239])
    raw=raw_lines(directory/'events.jsonl');groups=defaultdict(list)
    for row in raw:
        if row['dispatch']:groups[row['dispatch']].append(row)
    require(sorted(groups)==list(range(1,m['completion']['dispatches']+1)),'missing native dispatch')
    writes=raw_lines(directory/'writes.jsonl')
    controlled=defaultdict(list)
    for write in writes:
        if write['kind']=='controlled_word':controlled[write['dispatch']].append(write)
    require(not controlled if m['mode']=='natural' else len(controlled)==49,'wrong controlled population')
    cases=[]
    for number,rows in groups.items():
        before=[x for x in rows if x['kind']=='dispatch.entry_before_control']
        entry=[x for x in rows if x['kind']=='dispatch.entry'];exit=[x for x in rows if x['kind']=='dispatch.exit']
        require(len(before)==len(entry)==len(exit)==1 and rows[-1]==exit[0],'incomplete native dispatch')
        entry,exit=entry[0],exit[0]
        require(entry['cpu']['d']==exit['cpu']['d']==0 and entry['cpu']['ps']&0x30==0,'unexpected native M/X/D context')
        operations=[];returns=[];command_pcs=[]
        for row in rows:
            kind=row['kind'];c=row['cpu'];r=row['raw']
            if kind=='command.return':
                require(command_pcs and row['pc']==command_pcs[len(returns)]+4,'command return/caller mismatch')
                returns.append(c['a']);continue
            if kind not in ('command.entry','voice_volume.entry','crowd_queue.entry'):continue
            stack=row['stack'];caller=(stack[2]<<16)|(((stack[0]|(stack[1]<<8))-3)&65535)
            if kind=='command.entry':
                operation=[0,caller,row['pc'],c['a'],0,0];command_pcs.append(caller)
            elif kind=='voice_volume.entry':operation=[1,caller,row['pc'],0,c['x'],c['y']]
            else:operation=[2,caller,row['pc'],c['a'],c['x'],c['y']]
            operations.append(operation+[r['event13e7'],r['crowd13e9'],r['rng07f6']])
        require(len(returns)==len(command_pcs),'missing native sound driver return')
        provenance='natural'
        if controlled and number>=min(controlled):
            provenance='controlled' if number in controlled else 'post-controlled-continuation'
        r=entry['raw'];e=exit['raw']
        cases.append({'dispatch':number,'frame':entry['frame'],'court':entry['court'],
            'provenance':provenance,'input':[r['event13e7'],r['crowd13e9'],r['bounce13e5'],entry['options'][3],r['rng07f6']],
            'external_command_returns_a':returns,'output':[e['event13e7'],e['crowd13e9'],e['rng07f6']],
            'operations':operations,'interventions':controlled.get(number,[])})
    return {'name':directory.name,'native_manifest':m,
            'native_manifest_raw_utf8':(directory/'manifest.json').read_bytes().decode('utf-8'),
            'manifest_sha256':sha(directory/'manifest.json'),
            'cases_sha256':hashlib.sha256(canonical(cases)).hexdigest(),'cases':cases}

def validate(data):
    require(type(data) is dict and set(data)=={'schema','scope','input_fields','output_fields','operation_fields','captures'},'invalid audio witness schema')
    require(data['schema']=='nba95-native-audio-events-v1' and data['input_fields']==INPUT_FIELDS and
            data['output_fields']==OUTPUT_FIELDS and data['operation_fields']==OP_FIELDS,'unknown audio witness fields')
    require(type(data['captures']) is list and data['captures'],'empty native audio corpus')
    names=set()
    for cap in data['captures']:
        require(type(cap) is dict and set(cap)=={'name','native_manifest','native_manifest_raw_utf8','manifest_sha256','cases_sha256','cases'},'invalid native capture schema')
        require(type(cap['name']) is str and cap['name'] and cap['name'] not in names,'duplicate native capture');names.add(cap['name'])
        m=cap['native_manifest'];validate_manifest(m)
        require(type(cap['native_manifest_raw_utf8']) is str and
            hashlib.sha256(cap['native_manifest_raw_utf8'].encode('utf-8')).hexdigest()==cap['manifest_sha256'] and
            json.loads(cap['native_manifest_raw_utf8'],object_pairs_hook=object_without_duplicates)==m,
            'native manifest differs from retained raw original')
        require(hashlib.sha256(canonical(cap['cases'])).hexdigest()==cap['cases_sha256'],'native audio case projection changed')
        require(len(cap['cases'])==m['completion']['dispatches'],'native audio population changed')
        controlled_cases=0;injected=False
        for number,case in enumerate(cap['cases'],1):
            require(type(case) is dict and set(case)=={'dispatch','frame','court','provenance','input','external_command_returns_a','output','operations','interventions'},'invalid native audio case')
            require(case['dispatch']==number and type(case['dispatch']) is int,'native audio dispatch missing/reordered')
            require(case['provenance'] in ('natural','controlled','post-controlled-continuation'),'unknown audio case provenance')
            require(type(case['interventions']) is list,'invalid intervention list')
            if case['interventions']:
                injected=True;controlled_cases+=1
                require(m['mode']=='controlled' and case['provenance']=='controlled','injection relabeled natural')
                for intervention in case['interventions']:
                    require(intervention.get('kind')=='controlled_word' and intervention.get('dispatch')==number and
                        intervention.get('address') in (0x13e7,0x13e9,0x13e5,0x17bb,0x07f6),
                        'undeclared controlled input')
            else:require(case['provenance']==('post-controlled-continuation' if injected else 'natural'),
                         'post-injection execution relabeled natural')
            uint(case['frame'],0x7fffffff);require(type(case['court']) is int and case['court']>=-1,'invalid court frame')
            for field,count in (('input',5),('output',3)):
                require(type(case[field]) is list and len(case[field])==count,'incomplete native audio state')
                for value in case[field]:uint(value)
            require(type(case['external_command_returns_a']) is list and len(case['external_command_returns_a'])<=14,'invalid external callee returns')
            for value in case['external_command_returns_a']:uint(value)
            require(type(case['operations']) is list and len(case['operations'])<=19,'invalid native call count')
            for op in case['operations']:
                require(type(op) is list and len(op)==9,'incomplete native operation')
                uint(op[0],2);uint(op[1],0xffffff);uint(op[2],0xffffff)
                require(op[2]==(0x809df3,0x80a82f,0x809f0f)[op[0]],'unknown native callee')
                for v in op[3:]:uint(v)
            require(sum(op[0]==0 for op in case['operations'])==len(case['external_command_returns_a']),'native returns/commands differ')
        require(controlled_cases==m['completion']['controlled_cases'],'controlled population changed')
    return data
def read_fixture(path):
    require(sha(path)==NATIVE_FIXTURE_SHA256,'immutable native audio fixture changed')
    return validate(read_json(path))
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',action='append',required=True);p.add_argument('--output',required=True)
    a=p.parse_args();out=Path(a.output);require(not out.exists(),'fixture output must be new')
    data={'schema':'nba95-native-audio-events-v1','scope':'Native82FD65 event/RNG/ordered-call projection; driver A returns are explicit external callee inputs. No NMI/SPC/mixer or whole-game parity claim.',
          'input_fields':INPUT_FIELDS,'output_fields':OUTPUT_FIELDS,'operation_fields':OP_FIELDS,'captures':[capture(d) for d in a.capture]}
    validate(data);out.write_text(json.dumps(data,separators=(',',':'))+'\n')
    print(json.dumps({'sha256':sha(out),'captures':[(c['name'],len(c['cases'])) for c in data['captures']]}))
if __name__=='__main__':main()
