"""Separate source-work and native validation for the bounded sound initialization prefix continuation.

Native snapshots/registers seed an explicitly isolated differential bus only.
The source continuation has no snapshot, captured-clock, or SPC-response API.
This is not normal initialization or an end-to-end phase gate.
"""
import argparse
from setup_sound_fetch_contract import validate_65816_fetches
import hashlib
from setup_native_trace_contract import validate_chronology, validate_native_scope
import verify_setup_sound_prefix_v2 as previous_verifier
import json
from pathlib import Path
import re
import subprocess

from verify_setup_scheduler import (CAPTURE_KIND, CAPTURE_SCHEDULE, ROM_SHA, MESEN_SHA,
    require, digest, pairs, read_json, exact_keys, integer, sha256, path_value,
    typed_equal, expected_settings, validate_rows, check_previous)

BUILD_SOURCES = {'include/nba_setup_codec_work.h', 'src/nba_setup_sound_init.c', 'include/nba_setup_sound_init.h', 'tools/setup_sound_init_probe.c', 'tools/generate_setup_sound_init.py', 'src/nba_setup_sound_init_program.inc', 'tools/build_setup_sound_init_probe.ps1'}
CAPTURE_REVISIONS = {'script': 'b1c05e6ad93e346f019c4f9de9b41585dbb2cb4b3144f53851a885bf31774d25', 'runner': '7f337e3e68cfed56e39faf0f895adfadd22d6b3fc3677c135a696ba55c1e79d4', 'base_script': '1653867f508681901809885a72956e822417abfab0eaa246eda9b854673f5285', 'interrupt_script': '7ef283dae8aae6a93279dd6618c6df02595525e41f116abd522c6634924f9b0c', 'sound_prefix_script': '51a1481b24a915c7fdcc91fdb93b85c0a2fae89a616bb0ff78ca93eb853d7382'}

def validate_capture_manifest(manifest, directory, rom):
    # Preserve every earlier prefix observation, and require five new init
    # pairs plus the natural reset upload. Declarations and inventory count.
    tail = True
    fields = {'schema', 'kind', 'state_injection', 'rom_patch', 'accepted', 'sources',
              'arguments', 'isolation', 'schedule', 'exit_code', 'artifacts'}
    exact_keys(manifest, fields | {'interrupt_summary','sound_prefix_summary','sound_init_summary'}, 'capture manifest')
    integer(manifest['schema'], 1, 1, 'capture schema')
    integer(manifest['exit_code'], 0, 0, 'capture exit_code')
    require(manifest['accepted'] is True and manifest['state_injection'] is False and
            manifest['rom_patch'] is False, 'successful natural capture required')
    require(manifest['kind'] == CAPTURE_KIND and manifest['schedule'] == CAPTURE_SCHEDULE,
            'capture kind/input schedule differs')
    source_paths = {'rom': Path(rom).resolve(), 'mesen': directory / 'portable-mesen/Mesen.exe',
                    'script': directory / 'capture.lua', 'runner': directory / 'capture_runner.py',
                    'settings': directory / 'initial-settings.json'}
    if tail:
        source_paths['base_script'] = directory / 'scheduler_base.lua'
        source_paths['interrupt_script'] = directory / 'interrupt_base.lua'
        source_paths['sound_prefix_script'] = directory / 'sound_prefix_base.lua'
        require(type(manifest['sound_prefix_summary']) is str and re.fullmatch(r'ok; calls=46; instructions=[1-9][0-9]*; bus=[1-9][0-9]*; boundaries=92\n', manifest['sound_prefix_summary']), 'invalid prefix summary')
        require(type(manifest['sound_init_summary']) is str and re.fullmatch(r'ok; calls=5; instructions=[1-9][0-9]*; bus=[1-9][0-9]*; boundaries=10; callers=5; interrupts=0; upload=2\n',manifest['sound_init_summary']),'invalid initialization summary')
        require(type(manifest['interrupt_summary']) is str and
                re.fullmatch(r'ok; scopes=4; nmi=46; instructions=[1-9][0-9]*; bus=[1-9][0-9]*\n',
                             manifest['interrupt_summary']) is not None, 'invalid interrupt summary')
    exact_keys(manifest['sources'], source_paths, 'capture sources')
    for name, expected in source_paths.items():
        entry = manifest['sources'][name]
        exact_keys(entry, {'path', 'sha256'}, 'capture source ' + name)
        sha256(entry['sha256'], 'capture source ' + name)
        require(path_value(entry['path'], name) == expected.resolve(), 'capture source path differs: ' + name)
    require(manifest['sources']['rom']['sha256'] == ROM_SHA and
            manifest['sources']['mesen']['sha256'] == MESEN_SHA, 'canonical ROM/Mesen identity mismatch')
    require(all(manifest['sources'][k]['sha256']==v for k,v in CAPTURE_REVISIONS.items()),'capture revision differs')
    arguments = manifest['arguments']
    require(type(arguments) is list and len(arguments) == 5 and all(type(a) is str for a in arguments),
            'invalid capture arguments')
    require(arguments[1:3] == ['--testrunner', '--timeout=300'] and
            all(path_value(arguments[index], 'capture argument') == source_paths[name].resolve()
                for index, name in ((0, 'mesen'), (3, 'rom'), (4, 'script'))), 'capture arguments differ')
    core = {'capture.lua', 'capture_runner.py', 'initial-settings.json', 'mesen.log',
            'observed_environment.txt', 'scheduler.jsonl', 'state_fields.txt', 'capture_complete.txt'}
    core |= {f'header_{n:02d}_{stage}.wram' for n in range(1, 5) for stage in ('entry', 'after_wait')}
    before = {f'header_{n:02d}_before_wait.wram' for n in range(1, 5)}
    inventory = {p.name for p in directory.iterdir() if p.is_file() and p.name != 'manifest.json'}
    core |= before
    core |= {'interrupt_base.lua','sound_prefix_instructions.jsonl','sound_prefix_bus.jsonl','sound_prefix_boundaries.jsonl','sound_prefix_complete.txt'}
    core |= {f'sound_prefix_{n:02d}_{stage}.wram' for n in range(1,47) for stage in ('entry','exit')}
    if tail:
        core |= {'scheduler_base.lua', 'interrupt_complete.txt', 'interrupt_instructions.jsonl',
                 'interrupt_boundaries.jsonl', 'interrupt_bus.jsonl'}
        core |= {f'interrupt_{n:02d}_{stage}.wram' for n in range(1, 47) for stage in ('entry', 'exit')}
    core |= {'sound_prefix_base.lua','sound_init_instructions.jsonl','sound_init_bus.jsonl','sound_init_boundaries.jsonl','sound_init_callers.jsonl','sound_init_interrupts.jsonl','sound_init_upload.jsonl','sound_init_complete.txt','sound_init_uploaded.spc'}
    core |= {f'sound_init_{n:02d}_{stage}.wram' for n in range(1,6) for stage in ('entry','exit')}
    exact_keys(manifest['artifacts'], core, 'capture artifacts')
    require(inventory == core, 'capture artifact inventory differs from required declarations')
    for name, entry in manifest['artifacts'].items():
        exact_keys(entry, {'bytes', 'sha256'}, 'capture artifact ' + name)
        integer(entry['bytes'], 0 if name in ('mesen.log','sound_init_interrupts.jsonl') else 1, 2**63 - 1, 'artifact bytes ' + name)
        if name=='sound_init_uploaded.spc':integer(entry['bytes'],65536,65536,'SPC RAM bytes')
        if name.endswith('.wram'):
            integer(entry['bytes'], 0x20000, 0x20000, 'WRAM snapshot bytes')
        sha256(entry['sha256'], 'capture artifact ' + name)
    isolation = manifest['isolation']
    exact_keys(isolation, {'home', 'save_folder', 'initial_saves', 'settings', 'observed', 'post_settings_sha256'},
               'capture isolation')
    require(path_value(isolation['home'], 'home') == directory / 'portable-mesen' and
            path_value(isolation['save_folder'], 'save folder') == directory / 'isolated-saves', 'private home/save path differs')
    require(type(isolation['initial_saves']) is list and isolation['initial_saves'] == [], 'fresh empty saves required')
    typed_equal(isolation['settings'], expected_settings(directory), 'declared settings')
    exact_keys(isolation['observed'], {'output', 'home'}, 'observed environment')
    require(path_value(isolation['observed']['output'], 'observed output') == directory and
            path_value(isolation['observed']['home'], 'observed home').is_relative_to(directory / 'portable-mesen'),
            'declared observed environment differs')
    sha256(isolation['post_settings_sha256'], 'persisted settings')

def read_native(directory, rom):
    directory = Path(directory).resolve()
    manifest = read_json(directory / 'manifest.json')
    validate_capture_manifest(manifest, directory, rom)
    for entry in manifest['sources'].values():
        require(digest(entry['path']) == entry['sha256'], 'source changed: ' + entry['path'])
    for name, entry in manifest['artifacts'].items():
        require(Path(name).name == name, 'invalid artifact path')
        path = directory / name
        require(path.stat().st_size == entry['bytes'] and digest(path) == entry['sha256'],
                'capture artifact changed: ' + name)
    observed = pairs(line.split('=', 1) for line in
                     (directory / 'observed_environment.txt').read_text().splitlines())
    typed_equal(observed, manifest['isolation']['observed'], 'actual observed environment')
    settings = expected_settings(directory)
    typed_equal(read_json(directory / 'initial-settings.json'), settings, 'actual initial settings')
    post = directory / 'portable-mesen/settings.json'
    require(digest(post) == manifest['isolation']['post_settings_sha256'], 'persisted settings identity differs')
    typed_equal(read_json(post), settings, 'actual persisted settings', subset=True)
    require((directory / 'capture_complete.txt').read_text() ==
            'ok; headers=4; normal controller-only Rules repeat journey\n', 'missing sentinel')
    rows = [json.loads(line, object_pairs_hook=pairs) for line in
            (directory / 'scheduler.jsonl').read_text().splitlines()]
    validate_rows(rows)
    return rows

def check_build(exe):
    manifest = read_json(exe.parent / 'build-manifest.json')
    exact_keys(manifest, {'schema', 'compiler_exit', 'sources', 'executable'}, 'build manifest')
    integer(manifest['schema'], 1, 1, 'build schema')
    integer(manifest['compiler_exit'], 0, 0, 'compiler_exit')
    exact_keys(manifest['sources'], BUILD_SOURCES, 'build sources')
    for entry in [manifest['executable'], *manifest['sources'].values()]:
        exact_keys(entry, {'path', 'sha256'}, 'build identity')
        path_value(entry['path'], 'build identity')
        sha256(entry['sha256'], 'build identity')
    source_root = Path(manifest['sources']['src/nba_setup_sound_init.c']['path']).resolve().parents[1]
    require(all(Path(entry['path']).resolve() == (source_root / name).resolve()
                for name, entry in manifest['sources'].items()), 'build source paths differ')
    require(Path(manifest['executable']['path']).resolve() == exe.resolve() and
            digest(exe) == manifest['executable']['sha256'], 'executable changed')
    for entry in manifest['sources'].values():
        require(digest(entry['path']) == entry['sha256'], 'source changed since probe build')
    return manifest

def json_lines(path):
    return [json.loads(line, object_pairs_hook=pairs) for line in path.read_text().splitlines()]


REGISTERS = ('a','x','y','sp','db','ps')
COMMON=dict(pc=0xffffff,call=5,cpu_cycles=2**63-1,master_clock=2**63-1,a=65535,x=65535,y=65535,sp=65535,db=255,ps=255,dp=65535)


def native_records(directory):
    result=[]
    for name,extra in [('instructions',{}),('bus',dict(address=0xffffff,value=255)),('boundaries',{}),('callers',{})]:
        rows=json_lines(directory/f'sound_init_{name}.jsonl')
        for i,r in enumerate(rows):
            fields=dict(COMMON,event=2**31-1,**extra)
            text_fields=[] if name=='instructions' else ['kind'] if name=='bus' else ['tag']
            exact_keys(r,{*fields,*text_fields},name)
            for k,limit in fields.items():integer(r[k],0,limit,k)
            require(r['event']==i and 1<=r['call']<=5 and r['dp']==0,'native ordinal/call/DP differs')
            if name=='bus':require(r['kind']in('read','write'),'bus kind differs')
            if name=='boundaries':require(r['tag']in('init.entry','init.spc_read'),'boundary kind differs')
            if name=='callers':require(r['tag']=='init.caller','caller kind differs')
        validate_chronology(rows,name,strict=name!='bus');result.append(rows)
    instructions,bus,bounds,callers=result
    require(len(bounds)==10 and len(callers)==5 and json_lines(directory/'sound_init_interrupts.jsonl')==[],
            'bounded five-call no-interrupt witness differs; interrupted prefixes need separate validation')
    expected=f'ok; calls=5; instructions={len(instructions)}; bus={len(bus)}; boundaries=10; callers=5; interrupts=0; upload=2\n'
    require((directory/'sound_init_complete.txt').read_text()==expected and read_json(directory/'manifest.json')['sound_init_summary']==expected,'initialization completion differs')
    return instructions,bus,bounds,callers


def upload_provenance(native,rom):
    data=rom.read_bytes();uploaded=(native/'sound_init_uploaded.spc').read_bytes()
    require(data[0x36:0x43]==bytes.fromhex('c220a983c6850c640e2206ab80'),'reset upload descriptor/caller source differs')
    source=int.from_bytes(data[0x39:0x3b],'little');cursor=source&32767;blocks=[]
    for unused in range(8):
        count=int.from_bytes(data[cursor:cursor+2],'little');destination=int.from_bytes(data[cursor+2:cursor+4],'little');cursor+=4
        if count==0:entry=destination;break
        require(destination+count<=65536 and cursor+count<=32768,'bounded initial upload block differs')
        payload=data[cursor:cursor+count]
        require(uploaded[destination:destination+count]==payload,'native ROM-to-ARAM payload differs')
        blocks.append(dict(rom_source=0x8000+cursor,aram=destination,bytes=count,sha256=hashlib.sha256(payload).hexdigest()));cursor+=count
    else:raise ValueError('upload terminator missing')
    require([(r['rom_source'],r['aram'],r['bytes'])for r in blocks]==[(0xc687,0x380,0x4f0)] and entry==0x380,'initial consumer layout differs')
    observations=json_lines(native/'sound_init_upload.jsonl');require(len(observations)==2,'one reset upload required')
    for index,row in enumerate(observations):
        fields=dict(COMMON,event=1,**({'source':0xffffff}if index==0 else {}))
        exact_keys(row,{*fields,'tag'},'upload boundary')
        for k,limit in fields.items():integer(row[k],0,limit,k)
        require(row['event']==index and row['call']==0 and row['dp']==0 and row['tag']==('upload.entry'if index==0 else'upload.exit') and row['pc']==(0x80ab06 if index==0 else 0x80ab7d),'upload source boundary differs')
    validate_chronology(observations,'upload boundaries',strict=True)
    require(observations[0]['source']==source and observations[1]['x']==0 and observations[1]['y']==cursor-(source&32767),'native upload source/end cursor differs')
    return dict(source=source,stream_bytes=cursor-(source&32767),entry=entry,blocks=blocks,
                scope='ROM descriptor/source and natural post-upload ARAM identity; no SPC execution or acknowledgement timing prediction')

def source_events(rows, report):
    """Every accepted C bus cycle; terminal SPC read is deliberately absent."""
    instructions=[];bus=[];clock=0;current=None;instruction_done=True
    for row in rows:
        if row.get('kind')=='instruction':
            exact_keys(row,{'kind','pc','cycle','master',*REGISTERS},'C instruction')
            for key in set(row)-{'kind'}:integer(row[key],0,2**63-1,key)
            require(instruction_done and row['cycle']==len(bus)+1 and row['master']==clock,'C mixed instruction chronology differs')
            require(row['pc']>>16==0x80 and row['a']<=65535 and row['x']<=65535 and row['y']<=65535 and row['sp']<=65535 and row['ps']<=255 and row['db']<=255,'C register domain differs')
            current=row;instructions.append(row);instruction_done=False
        else:
            exact_keys(row,{'kind','access','pc','cycle','master','address','value','end'},'C bus')
            require(row['kind']=='bus' and current and not instruction_done,'C bus outside instruction')
            for key in set(row)-{'kind'}:integer(row[key],0,2**63-1,key)
            require(row['cycle']==len(bus)+1 and row['pc']==current['pc'] and row['access']<=2 and row['address']<=0xffffff and row['value']<=255 and row['end']in(0,1),'C bus schema/chronology differs')
            address=row['address'];bank=address>>16;low=address&65535
            cost=6 if row['access']==2 else (6 if bank>=0xc0 else 8) if bank&0x40 else (6 if bank>=0x80 else 8) if low>=0x8000 else 8 if low<0x2000 or low>=0x6000 else 12 if 0x4000<=low<0x4200 else 6
            require(row['master']==clock+cost,'C intrinsic bus clocks differ')
            require(not(row['access']==0 and low==0x2140),'unresolved SPC response was consumed')
            clock=row['master'];instruction_done=bool(row['end']);bus.append(row)
    require(len(instructions)==report['instructions'] and len(bus)==report['cycles'] and clock==report['master'],'C totals differ')
    require(instruction_done==(report['stop']!=1),'C terminal instruction ownership differs')
    if report['stop']==1:
        require(current['pc']==0x80aacd and len(bus)-current['cycle']+1==3,'SPC boundary must retain only opcode/operand fetches')
    return instructions,bus


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('native','previous-native','rom','exe','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();native,previous,rom,exe,out=(getattr(a,k).resolve()for k in ('native','previous_native','rom','exe','output'))
    require(digest(rom)==ROM_SHA,'canonical ROM required');build=check_build(exe)
    rows=read_native(native,rom);old=previous_verifier.read_native(previous,rom);check_previous(rows,old)
    for name in ('scheduler.jsonl','interrupt_instructions.jsonl','interrupt_bus.jsonl','interrupt_boundaries.jsonl',
                 'sound_prefix_instructions.jsonl','sound_prefix_bus.jsonl','sound_prefix_boundaries.jsonl'):
        require((native/name).read_bytes()==(previous/name).read_bytes(),'previous observation differs: '+name)
    instructions,bus,bounds,callers=native_records(native);upload=upload_provenance(native,rom)
    out.mkdir(parents=True,exist_ok=False);details=[];data=rom.read_bytes()
    for call in range(1,6):
        start,end=bounds[2*call-2:2*call];caller=callers[call-1]
        require(start['call']==end['call']==caller['call']==call and start['tag']=='init.entry' and start['pc']==0x809b73 and start['ps']&0x38==0,'source entry contract differs')
        require(caller['pc']==([0x80814a,0x82ad48,0x82abf2,0x82abf2,0x82abf2][call-1]),'normal caller route differs')
        offset=((caller['pc']>>16)&127)*32768+(caller['pc']&32767)
        require(data[offset:offset+4]==bytes.fromhex('22739b80') and all(start[k]==caller[k]for k in REGISTERS if k!='sp') and start['sp']==caller['sp']-3 and start['cpu_cycles']==caller['cpu_cycles']+8,'source caller JSL state/CPU differs')
        require(start['master_clock']-caller['master_clock']==54,'source caller intrinsic JSL differs')
        native_i=[r for r in instructions if r['call']==call];native_b=[r for r in bus if r['call']==call]
        require(end['tag']=='init.spc_read' and end['pc']==0x80aacd,'bounded native stop differs')
        validate_native_scope(native_i,native_b,start,end,f'sound initialization {call}')
        trace=out/f'call_{call:02d}.jsonl';wram=out/f'call_{call:02d}.wram'
        result=subprocess.run([str(exe),str(rom),str(native/f'sound_init_{call:02d}_entry.wram'),','.join(str(start[k])for k in REGISTERS),str(trace),str(wram),'isolated-component-differential'],text=True,capture_output=True)
        require(type(result.returncode)is int and result.returncode==0 and type(result.stdout)is str and type(result.stderr)is str and result.stderr=='','fresh isolated initializer probe failed')
        report=json.loads(result.stdout,object_pairs_hook=pairs)
        exact_keys(report,{'schema','stop','boundary_pc','cycles','master','instructions','status',*REGISTERS},'initialization report')
        for k,v in report.items():integer(v,0,2**63-1,k)
        require(report['schema']==1 and report['stop']==1 and report['boundary_pc']==end['pc'] and report['status']==0,'initializer stop contract differs')
        (out/f'call_{call:02d}.json').write_text(json.dumps(report,indent=2)+'\n')
        ci,cb=source_events(json_lines(trace),report)
        require(len(ci)==len(native_i),'instruction count differs')
        for i,(c,n)in enumerate(zip(ci,native_i)):
            require(all(c[k]==n[k]for k in ('pc',*REGISTERS)),f'call {call} instruction/register differs {i}')
            require(c['cycle']-1==n['cpu_cycles']-start['cpu_cycles'],f'call {call} instruction CPU position differs {i}')
        require(all(report[k]==end[k]for k in REGISTERS),'final initializer registers differ')
        require(native_b[-1]['address']==0x802140 and native_b[-1]['kind']=='read' and native_b[-1]['pc']==0x80aacd,'native pending response identity differs')
        expected_bus=validate_65816_fetches(ci,cb,rom.read_bytes())
        require(len(expected_bus)==len(native_b)-1,'CPU data access count differs')
        for i,(c,n)in enumerate(zip(expected_bus,native_b[:-1])):
            require(c['pc']==n['pc'] and c['address']==n['address'] and c['value']==n['value'] and (c['access']==0)==(n['kind']=='read') and c['cycle']==n['cpu_cycles']-start['cpu_cycles'],f'call {call} ordered data bus position/value differs {i}')
        require(wram.read_bytes()==(native/f'sound_init_{call:02d}_exit.wram').read_bytes(),'complete initializer WRAM differs')
        terminal=dict(cycle=report['cycles']+2,master=report['master']+6)
        for c,cn,n,nn in zip(ci,ci[1:]+[terminal],native_i,native_i[1:]+[end]):
            require(cn['cycle']-c['cycle']==nn['cpu_cycles']-n['cpu_cycles'],'source CPU duration differs')
            residual=nn['master_clock']-n['master_clock']-(cn['master']-c['master'])
            require(residual>=0 and residual%40==0,'source intrinsic interval plus observed refresh differs')
        details.append(dict(call=call,caller=caller['pc'],instructions=len(ci),cpu_data_accesses=len(expected_bus),cycles=report['cycles'],master=report['master'],trace_sha256=digest(trace),wram_sha256=digest(wram)))
    identity={k:dict(path=str(path),sha256=digest(path))for k,path in {'rom':rom,'native_manifest':native/'manifest.json','previous_manifest':previous/'manifest.json','verifier':Path(__file__),'fetch_contract':Path(__file__).with_name('setup_sound_fetch_contract.py'),'native_contract':Path(__file__).with_name('setup_native_trace_contract.py'),'prefix_verifier':Path(previous_verifier.__file__),'scheduler_verifier':Path(__file__).with_name('verify_setup_scheduler.py')}.items()}
    summary=dict(schema=1,accepted=True,identity=identity,build=build,instances=details,upload_provenance=upload,
        scope='Isolated snapshot/register initializer differential; no native SPC response consumed; no normal-journey phase predictor',
        limitation='Stops before first AACD idle-port read; remaining channel-off loop, upload consumer execution, sequence start/dwell and production timing unresolved')
    (out/'report.json').write_text(json.dumps(summary,indent=2)+'\n')
    print('PASS:',len(details),'isolated initialization calls;',sum(d['instructions']for d in details),'instruction states;',sum(d['cpu_data_accesses']for d in details),'data bus positions; all full WRAM endpoints;1264 ROM-to-ARAM bytes')


if __name__=='__main__':main()

