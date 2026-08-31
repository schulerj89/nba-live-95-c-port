"""Attribute closure C-regression changes without updating an expected digest.

The single native image checks render state equivalence only. The old closure
journey skips input dispatch, seeds gameplay, and is not a natural ROM journey.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct

import numpy as np

from differential_compare import object_without_duplicates
from setup_transition_capture import read_ppu_states


def read_json(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'),
                      object_pairs_hook=object_without_duplicates)


def lines(path):
    return [json.loads(line,object_pairs_hook=object_without_duplicates)
            for line in Path(path).read_text().splitlines()]


def sha(path):
    digest=hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda:stream.read(1024*1024),b''):digest.update(block)
    return digest.hexdigest()


def fnv(raw):
    value=1469598103934665603
    for byte in raw:value=((value^byte)*1099511628211)&0xffffffffffffffff
    return f'{value:016x}'


def equal_files(first,second):
    """Compare retained bytes directly; SHA identities are separate metadata."""
    if first.stat().st_size!=second.stat().st_size:return False
    with first.open('rb') as left,second.open('rb') as right:
        while True:
            a=left.read(1024*1024);b=right.read(1024*1024)
            if a!=b:return False
            if not a:return True


def compare_custom_session_buffers(old,new,width,offset):
    if type(width) is not int or width<2 or type(offset) is not int or not 0<=offset<=width-2:
        raise ValueError('invalid named Style member offset/layout')
    if len(old)!=6000*width or len(new)!=len(old):raise ValueError('incomplete session records')
    for frame in range(6000):
        start=frame*width;end=start+width
        arow=old[start:end];brow=new[start:end]
        if arow[offset:offset+2]!=b'\x01\0' or brow[offset:offset+2]!=b'\x02\0' or \
                arow[:offset]!=brow[:offset] or arow[offset+2:]!=brow[offset+2:]:
            raise ValueError('session difference beyond prescribed native Style1-to2 at frame'+str(frame))


def verify_manifest(directory):
    manifest=read_json(directory/'manifest.json')
    if manifest['classification']!='C-versus-C closure regression attribution; not ROM equivalence':
        raise ValueError('incorrect diagnostic classification')
    if manifest['baseline']!='2723af610aab0ec63263a6449fa6a161a155f974':
        raise ValueError('unexpected historical regression baseline')
    for label in ('before','after'):
        run=manifest['runs'][label];root=directory/label
        for name,expected in run['source_sha256'].items():
            if sha(root/'source'/name)!=expected:raise ValueError('frozen source changed')
        if sha(root/'closure.exe')!=run['executable_sha256'] or \
                sha(run['pack']['path'])!=run['pack']['sha256']:
            raise ValueError('diagnostic executable/pack identity changed')
        actual={p.name for p in (root/'captures').iterdir() if p.is_file()}
        if actual!=set(run['outputs']):raise ValueError('missing/extra diagnostic artifacts')
        for name,expected in run['outputs'].items():
            if sha(root/'captures'/name)!=expected:raise ValueError('diagnostic artifact changed')
        if (root/'stdout.txt').read_text()!=run['stdout'] or (root/'stderr.txt').read_text()!=run['stderr']:
            raise ValueError('process output identity changed')
        pattern=r'GAMEPLAY100_CLOSURE (PASS|FAIL) digest=([0-9a-f]{16}) transitions=8 renders=65 motion=2910 resources=13122 possessions=72 code=(\d+)'
        matches=re.findall(pattern,run['stdout'])
        if len(matches)!=1 or (label=='before' and matches[0]!=('PASS','773c1df2a9820701','0')) or \
                (label=='after' and matches[0][2] not in ('0','81')):
            raise ValueError('legacy closure execution did not reach the expected diagnostic boundary')
        if run['exit_code']!=(0 if matches[0][2]=='0' else 1):raise ValueError('unexpected process exit')
    return manifest


def compare(directory,native_directory,allow_native_custom=False):
    directory=Path(directory).resolve();native_directory=Path(native_directory).resolve()
    manifest=verify_manifest(directory)
    before=directory/'before/captures';after=directory/'after/captures'
    expected_labels=['setup-main','rules-edited','options-edited','team-select','player-setup','matchup']+['lineup']*10+['gameplay']*50
    input_traces=[]
    trace_presence=[(root/'run0-ui-inputs.jsonl').is_file() for root in (before,after)]
    if any(trace_presence) and not all(trace_presence):raise ValueError('missing revision input trace')
    if all(trace_presence):
        for label,root,count,start_row in (('before',before,1,4),('after',after,5,0)):
            inputs=lines(root/'run0-ui-inputs.jsonl')
            expected=[32]*4+[256,64,8]+[32]*count+[256,64,8,8]
            if [r['pressed'] for r in inputs]!=expected:
                raise ValueError('closure input journey differs from explicit native/historical navigation')
            if inputs[7]['page_before']!=0 or inputs[7]['row_before']!=start_row or \
                    inputs[6+count]['row_after']!=5:
                raise ValueError('Rules return navigation did not reach Options from the expected Main row')
            input_traces.append({'revision':label,'pressed_events':inputs,
                'navigation':'historical incorrect retained-row4' if label=='before' else 'native Mainrow0 plus5Down'})
    repeat=[]
    for label,root in (('before',before),('after',after)):
        names={p.name.removeprefix('run0-') for p in root.glob('run0-*')}
        if names!={p.name.removeprefix('run1-') for p in root.glob('run1-*')}:
            raise ValueError('different repeat-run output populations')
        different=[name for name in sorted(names)
                   if not equal_files(root/('run0-'+name),root/('run1-'+name))]
        if different:raise ValueError('nondeterministic repeated closure: '+str(different))
        repeat.append({'revision':label,'identical_artifacts':len(names)})
    a=lines(before/'run0-samples.jsonl');b=lines(after/'run0-samples.jsonl')
    for rows in (a,b):
        if [r['sample'] for r in rows]!=list(range(66)) or [r['label'] for r in rows]!=expected_labels:
            raise ValueError('missing/reordered render samples')
        if [r['frame'] for r in rows[16:]]!=list(range(0,6000,120)):
            raise ValueError('gameplay render sample cadence changed')
    unchanged=[];changed=[]
    for x,y in zip(a,b):
        name=f"run0-sample{x['sample']:02d}-{x['label']}.pixels"
        old=(before/name).read_bytes();new=(after/name).read_bytes()
        if len(old)!=256*224*4 or len(new)!=len(old):raise ValueError('invalid render dimensions')
        if fnv(old)!=x['pixel_fnv'] or fnv(new)!=y['pixel_fnv']:raise ValueError('pixel/FNV mismatch')
        if old==new:unchanged.append(x['sample']);continue
        old_words=np.frombuffer(old,dtype='<u4').reshape(224,256)
        new_words=np.frombuffer(new,dtype='<u4').reshape(224,256)
        ys,xs=np.where(old_words!=new_words)
        changed.append({'sample':x['sample'],'label':x['label'],'changed_pixels':len(xs),
            'bounds':[int(xs.min()),int(ys.min()),int(xs.max()),int(ys.max())],
            'before':x,'after':y})
    if [r['sample'] for r in changed]!=[1]:raise ValueError('change not isolated to the Rules sample')
    state_results=[]
    header=lines(before/'run0-legacy-state.jsonl')[0]
    header_fields={'schema','tipoff_offset','tipoff_bytes','session_bytes'}
    if header.get('schema')=='closure-diagnostic-v2':header_fields.add('session_style_offset')
    if header!=lines(after/'run0-legacy-state.jsonl')[0] or set(header)!=header_fields or \
            header['schema'] not in ('closure-diagnostic-v1','closure-diagnostic-v2'):
        raise ValueError('different owned-state layouts')
    for name,size in (('owned-state.bin',6000*header['tipoff_bytes']),
                      ('session-state.bin',6000*header['session_bytes']),
                      ('legacy-state.jsonl',None),('telemetry.jsonl',None)):
        x=before/('run0-'+name);y=after/('run0-'+name)
        if size and (x.stat().st_size!=size or y.stat().st_size!=size):
            raise ValueError('incomplete gameplay state: '+name)
        if name=='session-state.bin' and allow_native_custom:
            if header['schema']!='closure-diagnostic-v2':raise ValueError('Custom attribution needs explicit member offset')
            offset=header['session_style_offset'];width=header['session_bytes']
            old=x.read_bytes();new=y.read_bytes()
            compare_custom_session_buffers(old,new,width,offset)
            state_results.append({'file':name,'bytes':size,'before_sha256':sha(x),'after_sha256':sha(y),
                'result':'NATIVE_CUSTOM_TRANSITION','field':'config.main_values[1]',
                'offset':offset,'old':1,'new':2,'records':6000,'all_other_bytes':'IDENTICAL'})
        else:
            if not equal_files(x,y):raise ValueError('gameplay state changed: '+name)
            state_results.append({'file':name,'bytes':x.stat().st_size,'sha256':sha(x),'result':'IDENTICAL'})
    legacy=lines(before/'run0-legacy-state.jsonl')[1:]
    if [r['frame'] for r in legacy]!=list(range(6000)):raise ValueError('incomplete legacy gameplay projection')
    with (before/'run0-telemetry.jsonl').open() as stream:
        if sum(1 for _ in stream)!=6000:raise ValueError('incomplete semantic gameplay telemetry')

    native=read_json(native_directory/'manifest.json')
    if native['kind']!='natural-input frontend journey' or native['result']['exit_code']!=0 or \
            native['configuration'].get('closure_variant')!='Native Left pulse619..621; row0 45-to44; capture618..621. No state writes.':
        raise ValueError('unexpected native Rules witness provenance')
    for source in native['sources'].values():
        if sha(source['path'])!=source['sha256']:raise ValueError('native witness source changed')
    for name,entry in native['artifacts']['files'].items():
        path=native_directory/name
        if path.stat().st_size!=entry['bytes'] or sha(path)!=entry['sha256']:
            raise ValueError('native witness artifact changed')
    observed=(native_directory/'observed-script-data-folder.txt').read_text().strip()
    if not Path(observed).resolve().is_relative_to(native_directory/'portable-mesen'/'LuaScriptData'):
        raise ValueError('native witness did not use its private Mesen home')
    native_states=[]
    for frame in range(618,622):
        ram=(native_directory/f'wram_closure{frame}.bin').read_bytes()
        if len(ram)!=0x20000:raise ValueError('incomplete native WRAM')
        native_states.append({'frame':frame,'row':struct.unpack_from('<H',ram,0x1693)[0],
            'working_rules':list(struct.unpack_from('<13H',ram,0x16fb)),
            'committed_main':list(struct.unpack_from('<4H',ram,0x17ab))})
    if native_states[0]['working_rules'][0]!=45 or native_states[2]['working_rules'][0]!=44 or \
            any(row['row']!=0 for row in native_states):raise ValueError('native input did not make Rules45-to44')
    if allow_native_custom and (native_states[0]['committed_main'][1]!=1 or
            native_states[2]['committed_main'][1]!=2 or a[0]['setup']['main_style']!=1 or
            b[0]['setup']['main_style']!=1 or a[1]['setup']['main_style']!=1 or
            b[1]['setup']['main_style']!=2):
        raise ValueError('Custom transition was not observed in both native and C Rules adjustments')
    # Frame620 was prescribed before capture by the observed original input
    # dispatch contract, not selected by minimizing image differences.
    raw=(native_directory/'open_step_620.rgb').read_bytes()
    if len(raw)!=256*239*3:raise ValueError('invalid synchronous native RGB geometry')
    native_rgb=raw[7*256*3:231*256*3]
    current=np.frombuffer((after/'run0-sample01-rules-edited.pixels').read_bytes(),dtype='<u4')
    rgb=np.stack(((current>>16)&255,(current>>8)&255,current&255),axis=1).astype(np.uint8).tobytes()
    if rgb!=native_rgb:raise ValueError('Rules44 render differs from the independently captured native frame620')
    ppu=read_ppu_states(native_directory/'open_transition_ppu_states.txt')[620]
    return {'result':'PASS','classification':'bounded C regression digest attribution; not whole-game ROM parity',
        'baseline':manifest['baseline'],'repeat_checks':repeat,'ui_input_traces':input_traces,
        'unchanged_render_samples':unchanged,
        'changed_render_samples':changed,'gameplay_updates':6000,'state_comparison':state_results,
        'native_rules44':{'frame':620,'active_pixels':256*224,'rgb_sha256':hashlib.sha256(native_rgb).hexdigest(),
            'raw_rgb_sha256':sha(native_directory/'open_step_620.rgb'),'ppu':ppu,'state_snapshots':native_states,
            'caveat':('state-aligned render and prescribed native Style1-to2 only; different input timelines'
                if allow_native_custom else
                'state-aligned render only; native committed Style becomesCustom, frozen C stilldoesnot; different input timelines')},
        'remaining_caveats':['Historical probe directly calls scene APIs; no complete nba_game dispatch journey',
            'Controlled RNG0x5A17 and clock43200; human/CPU ownership and native initialization not established',
            'Only66 rendered samples, not every frame; no audio proof',
            'Raw C bytes require this same compiler/struct layout and are not native state equivalence',
            'This auditor never edits the expected digest; acceptance requires independent integrator review'],
        'sources':{'capture_manifest_sha256':sha(directory/'manifest.json'),
                   'native_manifest_sha256':sha(native_directory/'manifest.json')}}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--capture',required=True);p.add_argument('--native',required=True);p.add_argument('--report',required=True)
    p.add_argument('--allow-native-custom-style',action='store_true',
        help='Require the independently observed Style1-to2 change in exactly6000 session records; everyotherbyte stays exact')
    a=p.parse_args();report=compare(a.capture,a.native,a.allow_native_custom_style)
    Path(a.report).write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'result':report['result'],'changed_samples':[
        {'sample':r['sample'],'pixels':r['changed_pixels']} for r in report['changed_render_samples']],
        'identical_gameplay_updates':6000,'native_rules44_pixels':57344}))


if __name__=='__main__':main()
