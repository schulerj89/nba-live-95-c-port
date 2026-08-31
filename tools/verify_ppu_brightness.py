"""Compare production conversion against independently sampled Mesen PPU RGB.

This is a controlled hardware experiment, not a natural ROM routine or complete
frame parity test. Neither fixture creation nor validity selection uses C RGB.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import subprocess

from differential_compare import object_without_duplicates
from snes_ppu_oracle import cgram_color

FIELDS=('case','frame','channel','level','color','brightness','samples',
        'forced_blank','ppu_brightness','main_layers','sub_layers','cgram_word',
        'conflicting_writes','rejection_reason_bits')
VIDEO={'VideoFilter':'None','AspectRatio':'NoStretching','Brightness':0,
       'Contrast':0,'Hue':0,'Saturation':0,'ScanlineIntensity':0,
       'UseBilinearInterpolation':False,'ScreenRotation':'None'}
SNES={'Port1':{'Type':'SnesController'},'Port2':{'Type':'None'},
      'DisableFrameSkipping':True,'EnableRandomPowerOnState':False,
      'RamPowerOnState':'AllZeros','ForceFixedResolution':False,
      'Overscan':{'Top':7,'Bottom':8,'Left':0,'Right':0}}
SAMPLE_POINTS=[[0,7],[255,7],[128,119],[0,230],[255,230]]


def same_settings(actual,expected):
    """Type-sensitive subsets allow persisted Mesen settings to add defaults."""
    if type(expected) is dict:
        return type(actual) is dict and all(key in actual and
            same_settings(actual[key],value) for key,value in expected.items())
    return type(actual) is type(expected) and actual==expected


def validate_settings(video,snes,points,geometry):
    if not same_settings(video,VIDEO) or not same_settings(snes,SNES) or \
            points!=SAMPLE_POINTS or geometry!=[256,239]:
        raise ValueError('unverified raster/filter/controller configuration')


def read_json(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'),
                      object_pairs_hook=object_without_duplicates)


def read_lines(path):
    return [json.loads(line,object_pairs_hook=object_without_duplicates)
            for line in Path(path).read_text().splitlines()]


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def uint(value,maximum):
    if type(value) is not int or not 0<=value<=maximum:
        raise ValueError('invalid unsigned integer in native brightness evidence')
    return value


def validate(rows,rejected,writes):
    if len(rows)!=1536:
        raise ValueError('all 3 channels x32 levels x16 brightness cases required')
    by_frame={}
    for accepted,collection in ((True,rows),(False,rejected)):
        for row in collection:
            if type(row) is not dict or set(row)!=set(FIELDS):
                raise ValueError('missing/extra native raster fields')
            for key in FIELDS:
                if key not in ('samples','forced_blank'):
                    uint(row[key],0x7fffffff if key=='frame' else 65535)
            if type(row['forced_blank']) is not bool or \
                    type(row['samples']) is not list or len(row['samples'])!=5:
                raise ValueError('invalid native raster sample schema')
            for color in row['samples']:uint(color,0xffffff)
            index=row['case']-1
            if not 0<=index<1536 or row['brightness']!=index%16 or \
                    row['level']!=(index//16)%32 or row['channel']!=index//512 or \
                    row['color']!=row['level']<<(row['channel']*5):
                raise ValueError('native input grid changed')
            reason=(int(row['forced_blank'])|
                (2 if row['ppu_brightness']!=row['brightness'] else 0)|
                (4 if row['main_layers'] else 0)|(8 if row['sub_layers'] else 0)|
                (16 if row['cgram_word']!=row['color'] else 0)|
                (32 if len(set(row['samples']))!=1 else 0)|
                (64 if row['conflicting_writes'] else 0))
            if reason!=row['rejection_reason_bits'] or accepted!=(reason==0):
                raise ValueError('invalid raster acceptance/rejection reason')
            if row['frame'] in by_frame:
                raise ValueError('duplicate native raster frame')
            by_frame[row['frame']]=row
    if [row['case'] for row in rows]!=list(range(1,1537)):
        raise ValueError('missing/reordered/duplicated accepted cases')
    frames=sorted(by_frame)
    if frames!=list(range(120,120+len(rows)+len(rejected))):
        raise ValueError('missing native attempts')
    next_case=1
    for frame in frames:
        row=by_frame[frame]
        if row['case']!=next_case:
            raise ValueError('retry changed input or reordered cases')
        if not row['rejection_reason_bits']:next_case+=1
    conflicts=Counter()
    for write in writes:
        required={'case','frame','kind','address','value','injected_value',
                  'conflict','scanline','observed_pc'}
        if write.get('kind')=='cgram_command':required.add('observed_cgram_word')
        if set(write)!=required:raise ValueError('invalid native hardware-write schema')
        uint(write['case'],1536);uint(write['scanline'],261)
        uint(write['observed_pc'],65535);uint(write['injected_value'],65535)
        frame=uint(write['frame'],0x7fffffff)
        if frame not in by_frame or write['case']!=by_frame[frame]['case']:
            raise ValueError('native hardware write outside captured attempt')
        row=by_frame[frame]
        value=uint(write['value'],255)
        address=uint(write['address'],0xffffff)
        kind=write['kind']
        if kind=='register':
            reg=address&0xffff
            if reg not in (0x2100,0x212c,0x212d,0x2130,0x2131,0x2133):
                raise ValueError('unexpected hardware register')
            expected=row['brightness'] if reg==0x2100 else 0
            observed=value
        elif kind=='cgram_command':
            if (address&0xffff) not in (0x2121,0x2122):
                raise ValueError('unexpected CGRAM command')
            expected=row['color'];observed=uint(write['observed_cgram_word'],65535)
        elif kind=='cgram':
            if address not in (0,1):raise ValueError('unexpected CGRAM byte')
            expected=(row['color']>>(address*8))&255;observed=value
        else:raise ValueError('unknown hardware write kind')
        conflict=observed!=expected
        if type(write['conflict']) is not bool or write['conflict']!=conflict or \
                write['injected_value']!=expected:
            raise ValueError('hardware conflict classification changed')
        conflicts[frame]+=int(conflict)
    if any(row['conflicting_writes']!=conflicts[frame] for frame,row in by_frame.items()):
        raise ValueError('missing/extra conflict observations')


def capture(directory):
    directory=Path(directory);manifest=read_json(directory/'manifest.json')
    isolation=manifest.get('isolation',{})
    if isolation.get('method')!='private portable executable/settings' or \
            isolation.get('post_settings_verified') is not True:
        raise ValueError('unverified Mesen home/configuration')
    validate_settings(isolation['settings']['Video'],isolation['settings']['Snes'],
                      manifest['sample_points'],[256,239])
    if any(manifest.get(key) is not False for key in
           ('cpu_state_injection','rom_patch','wram_injection')):
        raise ValueError('unexpected CPU/ROM/WRAM injection')
    loaded_settings=read_json(manifest['sources']['settings']['path'])
    if sha(manifest['sources']['settings']['path'])!=manifest['sources']['settings']['sha256']:
        raise ValueError('loaded portable settings changed')
    validate_settings(loaded_settings['Video'],loaded_settings['Snes'],
                      manifest['sample_points'],[256,239])
    observed=(directory/'observed-script-data-folder.txt').read_text().strip()
    if observed!=isolation['observed_script_data_folder'] or not \
            Path(observed).resolve().is_relative_to(Path(isolation['home']).resolve()/'LuaScriptData'):
        raise ValueError('native Lua did not confirm private portable home')
    for key,name in (('trace','brightness.jsonl'),('rejected','rejected-attempts.jsonl'),
                     ('native_writes','native-hardware-writes.jsonl')):
        if manifest['sources'][key]['sha256']!=sha(directory/name):
            raise ValueError('native source hash mismatch')
    rows=read_lines(directory/'brightness.jsonl')
    rejected=read_lines(directory/'rejected-attempts.jsonl')
    writes=read_lines(directory/'native-hardware-writes.jsonl')
    if manifest['rejected_attempts']!=len(rejected):raise ValueError('rejected count changed')
    validate(rows,rejected,writes)
    return rows,rejected,writes,manifest


def compact(path):
    fixture=read_json(path)
    if fixture.get('schema')!='nba95-controlled-ppu-brightness-v1' or \
            fixture.get('row_fields')!=list(FIELDS):
        raise ValueError('unsupported brightness fixture schema')
    rows=[]
    for values in fixture['cases']:
        if not isinstance(values,list) or len(values)!=len(FIELDS):
            raise ValueError('incomplete compact raster row')
        rows.append(dict(zip(FIELDS,values)))
    source=fixture['source']
    for key in ('rom','mesen','script','settings','trace','rejected','native_writes'):
        if not re.fullmatch('[0-9a-f]{64}',source['sha256'][key]):
            raise ValueError('incomplete source identity')
    if source['classification']!='controlled PPU/CGRAM experiment; no CPU/ROM/WRAM injection':
        raise ValueError('incorrect capture provenance')
    validate_settings(source['video_settings'],source['snes_settings'],
                      source['sample_points'],source['geometry'])
    validate(rows,fixture['rejected_attempts'],fixture['native_hardware_writes'])
    return rows,fixture['rejected_attempts'],fixture['native_hardware_writes'],source


def replay(rows,probe):
    payload=''.join(f"{row['color']} {row['brightness']}\n" for row in rows)
    run=subprocess.run([str(probe)],input=payload,text=True,capture_output=True,
                       check=True,timeout=60)
    lines=run.stdout.splitlines()
    if len(lines)!=len(rows) or any(not re.fullmatch('[0-9]+',line) for line in lines):
        raise ValueError('missing/extra/malformed compiled converter output')
    errors=[]
    for row,line in zip(rows,lines):
        actual=uint(int(line),0xffffff);expected=row['samples'][0]
        if actual!=expected:errors.append([row['case'],'C',expected,actual])
        cgram=bytes((row['color']&255,row['color']>>8))+bytes(510)
        rgb=cgram_color(cgram,0,row['brightness'])
        python=(rgb[0]<<16)|(rgb[1]<<8)|rgb[2]
        if python!=expected:errors.append([row['case'],'Python snapshot renderer',expected,python])
    return errors


def main():
    p=argparse.ArgumentParser(description=__doc__)
    source=p.add_mutually_exclusive_group(required=True)
    source.add_argument('--capture-dir');source.add_argument('--vectors')
    p.add_argument('--probe',required=True);p.add_argument('--write-fixture')
    p.add_argument('--report')
    a=p.parse_args()
    rows,rejected,writes,metadata=capture(a.capture_dir) if a.capture_dir else compact(a.vectors)
    if a.write_fixture:
        if not a.capture_dir or Path(a.write_fixture).exists():
            raise ValueError('new fixtures require raw capture and a new output path')
        source={'classification':'controlled PPU/CGRAM experiment; no CPU/ROM/WRAM injection',
            'captured_utc':metadata['captured_utc'],'capture_script':'tools/mesen_ppu_brightness.lua',
            'sha256':{key:value['sha256'] for key,value in metadata['sources'].items()},
            'video_settings':metadata['isolation']['settings']['Video'],
            'snes_settings':metadata['isolation']['settings']['Snes'],
            'injections':metadata['injections'],'retry_policy':metadata['retry_policy'],
            'sample_points':metadata['sample_points'],'geometry':[256,239]}
        fixture={'schema':'nba95-controlled-ppu-brightness-v1','source':source,
            'row_fields':list(FIELDS),'rejected_attempts':rejected,
            'native_hardware_writes':writes}
        prefix=json.dumps(fixture,indent=2)[:-2]
        encoded=[json.dumps([row[field] for field in FIELDS],separators=(',',':')) for row in rows]
        Path(a.write_fixture).write_text(prefix+',\n"cases":[\n'+',\n'.join(encoded)+'\n]}\n')
    errors=replay(rows,a.probe)
    report={'result':'FAIL' if errors else 'PASS','cases':len(rows),
        'converter_comparisons':len(rows),'native_samples_observed':len(rows)*5,
        'rejected_attempts':len(rejected),
        'native_hardware_writes':len(writes),'issues':errors,
        'scope':'emulator RGB555 INIDISP and default RGB expansion; no color math or natural scene timing proof',
        'probe_sha256':sha(a.probe),'input_sha256':sha(a.vectors) if a.vectors else
            metadata['sources']['trace']['sha256']}
    if a.report:Path(a.report).write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))
    return 1 if errors else 0


if __name__=='__main__':raise SystemExit(main())
