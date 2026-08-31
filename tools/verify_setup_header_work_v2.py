"""Separate source-work and native validation for the bounded header pre-wait continuation.

Native entry registers are diagnostic-only leaf inputs. No native clock or
snapshot is input to the work producer. This is not an end-to-end phase gate.
"""
from setup_native_trace_contract import validate_chronology, validate_native_scope
import argparse
from collections import Counter
from setup_codec_trace_contract import validate_source_events
import verify_setup_fb30_work as fb30
import verify_setup_codec_work_v2 as fb46
import verify_setup_producer_work_v2 as producer
import hashlib
import json
from pathlib import Path
import re
import subprocess

from verify_setup_scheduler import (CAPTURE_KIND, CAPTURE_SCHEDULE, ROM_SHA, MESEN_SHA,
    require, digest, pairs, read_json, exact_keys, integer, sha256, path_value,
    typed_equal, expected_settings, validate_rows, read_capture, check_previous)

BUILD_SOURCES = {'tools/generate_setup_header_work.py', 'include/nba_setup_header_work.h', 'src/nba_setup_header_program.inc', 'tools/setup_header_work_probe.c', 'include/nba_setup_codec_work.h', 'tools/build_setup_header_work_probe.ps1', 'src/nba_setup_header_work.c'}
CAPTURE_REVISIONS = {'script': '6b2a5966ed90530c1c202accc9db3dfec4fe63c5a3c080d9e27dcc3694d2a36e', 'base_script': '1653867f508681901809885a72956e822417abfab0eaa246eda9b854673f5285', 'runner': '698c80a4a319e5f53c0bbce8c7486b81fc89a40feded9fdc2e7add4f9cfb1650', 'codec_script': 'd7bc0dfb9678ccc3ce0aabde1e55ce8ee24b9da61b0cdd6ffdf558bae3597f78', 'producer_script': 'e1aae6bed286b144f7c938e04412919fde4be899e80871c583d5b3c7910bb067'}

def validate_capture_manifest(manifest, directory, rom):
    # Known legacy captures omit four before-wait dumps. Their core identities
    # are still mandatory. The complete on-disk file inventory must be attested
    # as well, preventing an omitted declaration from silently avoiding hashing.
    tail = True
    fields = {'schema', 'kind', 'state_injection', 'rom_patch', 'accepted', 'sources',
              'arguments', 'isolation', 'schedule', 'exit_code', 'artifacts'}
    exact_keys(manifest, fields | {'codec_summary', 'producer_summary', 'header_work_summary'}, 'capture manifest')
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
        source_paths['codec_script'] = directory / 'codec_base.lua'
        source_paths['producer_script'] = directory / 'producer_base.lua'
        require(type(manifest['header_work_summary']) is str and re.fullmatch(r'ok; scopes=4; boundaries=8; instructions=[1-9][0-9]*; bus=[1-9][0-9]*\n',manifest['header_work_summary']), 'invalid header summary')
        require(type(manifest['producer_summary']) is str and re.fullmatch(r'ok; scopes=4; boundaries=48; instructions=[1-9][0-9]*; bus=[1-9][0-9]*\n',manifest['producer_summary']), 'invalid producer summary')
        require(type(manifest['codec_summary']) is str and
                re.fullmatch(r'ok; scopes=4; calls=20; instructions=[1-9][0-9]*; writes=[1-9][0-9]*\n',
                             manifest['codec_summary']) is not None, 'invalid codec summary')
    exact_keys(manifest['sources'], source_paths, 'capture sources')
    for name, expected in source_paths.items():
        entry = manifest['sources'][name]
        exact_keys(entry, {'path', 'sha256'}, 'capture source ' + name)
        sha256(entry['sha256'], 'capture source ' + name)
        require(path_value(entry['path'], name) == expected.resolve(), 'capture source path differs: ' + name)
    require(manifest['sources']['rom']['sha256'] == ROM_SHA and
            manifest['sources']['mesen']['sha256'] == MESEN_SHA, 'canonical ROM/Mesen identity mismatch')
    require(all(manifest['sources'][name]['sha256'] == expected
                for name, expected in CAPTURE_REVISIONS.items()), 'capture source revision differs')
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
    if tail:
        core |= {'scheduler_base.lua', 'codec_complete.txt', 'codec_instructions.jsonl',
                 'codec_boundaries.jsonl', 'codec_writes.jsonl'}
        core |= {f'codec_{n:02d}_{stage}.wram' for n in range(1, 21) for stage in ('entry', 'exit')}
        core |= {'codec_base.lua','producer_instructions.jsonl','producer_boundaries.jsonl','producer_bus.jsonl','producer_complete.txt'}
        core |= {f'producer_{n:02d}_{stage}.wram' for n in range(1,5) for stage in ('entry','exit')}
        core |= {'producer_base.lua','header_work_instructions.jsonl','header_work_boundaries.jsonl','header_work_bus.jsonl','header_work_complete.txt'}
    exact_keys(manifest['artifacts'], core, 'capture artifacts')
    require(inventory == core, 'capture artifact inventory differs from required declarations')
    for name, entry in manifest['artifacts'].items():
        exact_keys(entry, {'bytes', 'sha256'}, 'capture artifact ' + name)
        integer(entry['bytes'], 0 if name == 'mesen.log' else 1, 2**63 - 1, 'artifact bytes ' + name)
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
    source_root = Path(manifest['sources']['src/nba_setup_header_work.c']['path']).resolve().parents[1]
    require(all(Path(entry['path']).resolve() == (source_root / name).resolve()
                for name, entry in manifest['sources'].items()), 'build source paths differ')
    require(Path(manifest['executable']['path']).resolve() == exe.resolve() and
            digest(exe) == manifest['executable']['sha256'], 'executable changed')
    for entry in manifest['sources'].values():
        require(digest(entry['path']) == entry['sha256'], 'source changed since probe build')
    return manifest


def json_lines(path):
    return [json.loads(line, object_pairs_hook=pairs) for line in path.read_text().splitlines()]


def integer_rows(rows, schema, ordinal):
    for index, row in enumerate(rows):
        exact_keys(row, schema, 'trace row')
        for key, limit in schema.items():
            if key == 'tag':
                require(row[key] in {'codec.entry', 'codec.exit'}, 'unknown codec boundary')
            else:
                integer(row[key], 0, limit, key)
        if ordinal:
            require(row[ordinal] == index, 'trace ordinal discontinuity')


def native_records(directory):
    common=dict(pc=0xffffff,scope=4,cpu_cycles=2**63-1,master_clock=2**63-1,
                a=65535,x=65535,y=65535,ps=255,db=255,dp=65535,sp=65535)
    instructions=json_lines(directory/'header_work_instructions.jsonl')
    integer_rows(instructions,dict(common,instruction=2**31-1),'instruction')
    require(all(r['scope']==1 and r['dp']==0 for r in instructions),'header instruction scope differs')
    boundaries=json_lines(directory/'header_work_boundaries.jsonl')
    require(len(boundaries)==8,'four header entry/pre-wait pairs required')
    for index,row in enumerate(boundaries):
        exact_keys(row,{*common,'tag','event'},'header boundary')
        for key,limit in common.items():integer(row[key],0,limit,key)
        integer(row['event'],index,index,'header boundary event')
        require(row['tag']==('header.entry' if index%2==0 else 'header.before_wait') and
                row['pc']==(0x80eec6 if index%2==0 else 0x80ef1a) and row['scope']==index//2+1 and
                row['dp']==0 and row['ps']&0x38==0,'header source bracket differs')
    bus=json_lines(directory/'header_work_bus.jsonl')
    for index,row in enumerate(bus):
        schema=dict(address=0xffffff,value=255,pc=0xffffff,event=2**31-1,scope=1,dma_active=2,
                    cpu_cycles=2**63-1,master_clock=2**63-1)
        exact_keys(row,{'kind',*schema},'header bus')
        for key,limit in schema.items():integer(row[key],0,limit,key)
        require(row['kind'] in ('read','write') and row['scope']==1 and row['event']==index and
                row['dma_active'] in (0,2),'header bus source differs')
    expected=f'ok; scopes=4; boundaries=8; instructions={len(instructions)}; bus={len(bus)}\n'
    require((directory/'header_work_complete.txt').read_text()==expected and
            read_json(directory/'manifest.json')['header_work_summary']==expected,'header completion differs')
    return boundaries,instructions,bus


def run_probe(exe,rom,output,entry=None):
    prefix=output/('native-entry' if entry else 'default')
    args=[str(exe),str(rom),str(prefix.with_suffix('.wram')),str(prefix.with_suffix('.jsonl'))]
    if entry:args += [','.join(str(entry[k])for k in ('a','x','y','sp','db','ps'))+',400']
    result=subprocess.run(args,text=True,capture_output=True)
    require(result.returncode==0 and result.stderr=='','fresh header probe failed')
    prefix.with_suffix('.json').write_text(result.stdout)
    report=json.loads(result.stdout,object_pairs_hook=pairs)
    exact_keys(report,{'schema','status','bus_valid','return_pc','cycles','master','slow','instructions',
        'dma_bytes','dma_jobs','cursor','sp','wmadd','counts'},'header report')
    for key in set(report)-{'bus_valid','counts'}:integer(report[key],0,2**63-1,key)
    require(report['schema']==1 and report['status']==1 and report['bus_valid'] is True and
            report['return_pc']==0x80ef1a,'header work did not complete before wait')
    require((report['cycles'],report['master'],report['instructions'],report['dma_bytes'],report['dma_jobs'])==
            (440,2764,126,8206,3),'independent header source checksum differs')
    require(report['master']==report['cycles']*6+report['slow']*2 and
            report['sp']==(entry['sp'] if entry else 0x1fef),'header intrinsic/stack contract differs')
    require(type(report['counts']) is dict and report['counts'],'source counts required')
    for pc,count in report['counts'].items():
        require(re.fullmatch(r'80[0-9A-F]{4}',pc) is not None,'invalid header source PC')
        integer(count,1,2**31-1,'source count')
    require(sum(report['counts'].values())==report['instructions'],'source counts do not sum')
    ci,cw=validate_source_events(json_lines(prefix.with_suffix('.jsonl')),report)
    scratch=prefix.with_suffix('.wram').read_bytes()
    require(len(scratch)==0x20000 and scratch[0x15d9:0x15e3]==bytes(10),'source five-word clear failed')
    return report,scratch,ci,cw,json_lines(Path(str(prefix.with_suffix('.jsonl'))+'.dma.jsonl'))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('native','previous-native','rom','exe','output'):parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args()
    native,previous,rom,exe,output=(p.resolve()for p in (args.native,args.previous_native,args.rom,args.exe,args.output))
    require(digest(rom)==ROM_SHA,'canonical ROM required')
    build=check_build(exe);rows=read_native(native,rom);old=producer.read_native(previous,rom)
    invariance=check_previous(rows,old)
    for name in ('codec_instructions.jsonl','codec_writes.jsonl','codec_boundaries.jsonl',
                 'producer_instructions.jsonl','producer_bus.jsonl','producer_boundaries.jsonl'):
        require((native/name).read_bytes()==(previous/name).read_bytes(),'preceding observations changed: '+name)
    boundaries,instructions,bus=native_records(native)
    validate_chronology(boundaries, 'header boundaries', strict=True)
    validate_native_scope(instructions, bus, boundaries[0], boundaries[1], 'header native')
    output.mkdir(parents=True,exist_ok=False)
    report,scratch,ci,cw,dma=run_probe(exe,rom,output,boundaries[0])
    default,_,_,_,default_dma=run_probe(exe,rom,output)
    require(all(report[k]==default[k]for k in ('cycles','master','counts','dma_bytes','dma_jobs')) and
            dma==default_dma,'native typed entry changed source work/effects')
    excluded=set();nmis=[r for r in instructions if r['pc']==0x80815a]
    for nmi in nmis:
        position=instructions.index(nmi);require(0<position<len(instructions)-1,'incomplete header NMI bracket')
        resume=instructions[position+1]
        for k,value in enumerate((0x80,(resume['pc']>>8)&255,resume['pc']&255,resume['ps'])):
            matches=[r for r in bus if not r['dma_active'] and r['kind']=='write' and r['cpu_cycles']==nmi['cpu_cycles']-9+k]
            require(len(matches)==1,'header NMI hardware write cardinality differs')
            r=matches[0]
            require(r['address']==nmi['sp']+4-k and r['value']==value and r['pc']==instructions[position-1]['pc'],
                    'header NMI hardware stack source protocol differs')
            excluded.add(r['event'])
    ni=[r for r in instructions if r['pc']!=0x80815a]
    nw=[r for r in bus if r['kind']=='write' and not r['dma_active'] and r['event']not in excluded]
    require(len(ni)==len(ci) and len(nw)==len(cw),'header source event lengths differ')
    require(Counter(f'{r["pc"]:06X}'for r in ni)==report['counts'],'header source counts differ')
    for index,(c,n)in enumerate(zip(ci,ni)):
        require(all(c[k]==n[k]for k in ('pc','a','x','y','ps','db','sp')),f'header instruction/register differs {index}')
    start,end=boundaries[:2]
    ins=[r for r in rows if r['tag']=='nmi.entry' and start['master_clock']<r['master_clock']<end['master_clock']]
    outs=[r for r in rows if r['tag']=='nmi.exit' and start['master_clock']<r['master_clock']<end['master_clock']]
    require(len(ins)==len(outs)==len(nmis),'header NMI count differs')
    dma_service={r['cpu_cycles']for r in bus if r['dma_active']}
    intrinsic_checked=0
    for index,(c,cn,n,nn)in enumerate(zip(ci,ci[1:]+[dict(cycle=report['cycles']+1,master=report['master'])],ni,ni[1:]+[end])):
        interrupts=[(a,b)for a,b in zip(ins,outs)if n['master_clock']<a['master_clock']<nn['master_clock']]
        cpu=nn['cpu_cycles']-n['cpu_cycles']-sum(b['cpu_cycles']-a['cpu_cycles']+19 for a,b in interrupts)
        require(cpu==cn['cycle']-c['cycle'],f'header instruction CPU differs {index}')
        if n['cpu_cycles']+2 not in dma_service:
            master=nn['master_clock']-n['master_clock']-sum(b['master_clock']-a['master_clock']+142 for a,b in interrupts)
            residue=master-(cn['master']-c['master'])
            require(residue>=0 and residue%40==0,f'header non-DMA intrinsic interval differs {index}')
            intrinsic_checked+=1
    instruction_index=0
    for index,(c,n)in enumerate(zip(cw,nw)):
        require(all(c[k]==n[k]for k in ('pc','address','value')),f'ordered header CPU write differs {index}')
        while instruction_index+1<len(ci) and ci[instruction_index+1]['cycle']<=c['cycle']:instruction_index+=1
        require(c['cycle']-ci[instruction_index]['cycle']+1==n['cpu_cycles']-ni[instruction_index]['cpu_cycles'],
                f'header write bus position differs {index}')
    native_dma=[r for r in bus if r['dma_active']==2]
    triggers=[(c,n)for c,n in zip(cw,nw)if c['address']&65535==0x420b]
    require(len(triggers)==3 and len(native_dma)==2*len(dma)==16412 and
            all(c['value']==2 for c,n in triggers),'header DMA request/cardinality differs')
    jobs=Counter()
    for index,(c,r,w)in enumerate(zip(dma,native_dma[::2],native_dma[1::2])):
        schema=dict(job=2,index=4095,cycle=440,source=0xffffff,address=0xffff,value=255)
        exact_keys(c,schema,'header DMA effect')
        for key,limit in schema.items():integer(c[key],0,limit,key)
        require(c['job']==0 if index==0 else c['job']in(dma[index-1]['job'],dma[index-1]['job']+1),'header DMA job order differs')
        require(c['index']==jobs[c['job']],'header DMA byte order differs');jobs[c['job']]+=1
        trigger,ntrigger=triggers[c['job']]
        require(c['cycle']==trigger['cycle'] and r['cpu_cycles']==ntrigger['cpu_cycles']+2,'header DMA request/service boundary differs')
        require(r['kind']=='read' and w['kind']=='write' and w['event']==r['event']+1 and
                r['address']==c['source'] and w['address']==c['address'] and r['value']==w['value']==c['value'] and
                r['cpu_cycles']==w['cpu_cycles'] and w['master_clock']-r['master_clock']==4,
                f'header DMA source/payload differs {index}')
    require(dict(jobs)=={0:4096,1:4096,2:14},'header DMA sizes differ')
    intervals=[]
    for start,end in zip(boundaries[::2],boundaries[1::2]):
        ins=[r for r in rows if r['tag']=='nmi.entry' and start['master_clock']<r['master_clock']<end['master_clock']]
        outs=[r for r in rows if r['tag']=='nmi.exit' and start['master_clock']<r['master_clock']<end['master_clock']]
        require(len(ins)==len(outs),'unpaired header NMI')
        pure=end['cpu_cycles']-start['cpu_cycles']-sum(b['cpu_cycles']-a['cpu_cycles']+19 for a,b in zip(ins,outs))
        require(pure==440,'header source CPU conservation differs')
        require((native/f'header_{start["scope"]:02d}_before_wait.wram').read_bytes()[0x15d9:0x15e3]==bytes(10),
                'native source clear span differs')
        intervals.append(dict(scope=start['scope'],pure_cpu=pure,nmis=len(ins)))
    identity={name:dict(path=str(path),sha256=digest(path))for name,path in {
        'rom':rom,'native_manifest':native/'manifest.json','previous_manifest':previous/'manifest.json','verifier':Path(__file__),
        'producer_verifier':Path(producer.__file__),'strict_scheduler_verifier':Path(__file__).with_name('verify_setup_scheduler.py'),
        'trace_contract':Path(__file__).with_name('setup_codec_trace_contract.py'),
        'native_trace_contract':Path(__file__).with_name('setup_native_trace_contract.py')}.items()}
    proof=dict(schema=1,scope='EEC6 through EF1A entry; excludes wait JSL/body',identity=identity,build=build,work=report,
        limitation='No DMA service/alignment, refresh/NMI/audio/SPC or wait-epoch prediction')
    validation=dict(schema=1,accepted=True,identity=identity,exact_instructions=len(ni),exact_cpu_writes=len(nw),exact_dma_bytes=len(dma),
        non_dma_intrinsic_intervals_checked=intrinsic_checked,observed_dma_service_intervals=len(ni)-intrinsic_checked,
        intervals=intervals,previous_invariance=invariance,
        limitation='Typed entry diagnostics; snapshots/clocks never source-work inputs')
    (output/'source-work-proof.json').write_text(json.dumps(proof,indent=2)+'\n')
    (output/'native-validation.json').write_text(json.dumps(validation,indent=2)+'\n')
    print(f'PASS: {len(ni)} header instruction states/CPU durations; {len(nw)} CPU write positions; {len(dma)} DMA bytes; four440CPU intervals')


if __name__=='__main__':main()

