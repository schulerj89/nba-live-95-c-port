"""Exact native RGB comparison for font-resource rendering, not scene timing.

Original cold-boot images are independent expected results. C outputs come
from intro_text_probe or an independently built implementation of that API.
"""
import argparse
import json
from pathlib import Path
from intro_capture_resources import IntroResources, sha, strict_json


def native_frames(capture):
    rows=[strict_json(line) for line in capture.read('frames.jsonl').splitlines()]
    fields={'global','license','motion','brightness','blank','main','sub',
            'm7a','m7d','m7x','m7y','m7h','m7v'}
    if len(rows)!=1500:raise ValueError('missing native frame-state rows')
    marks=[strict_json(line) for line in capture.read('marks.jsonl').splitlines()]
    mark_fields={'global','license','motion','pc','label'}
    expected_marks=[(0x80FD9E,'license-builder'),(0x80FE7B,'license-hold'),
        (0x80FEE6,'legal-hold'),(0x80FF03,'legal-start-poll-hold'),
        (0x80FF28,'legal-fade-out'),(0x82F2EA,'ea-motion'),
        (0x82F2FE,'e-complete'),(0x82F37E,'a-complete'),
        (0x82F43A,'sports-complete'),(0x82F492,'ea-hold'),(0x80E1B1,'title-entry')]
    if len(marks)!=len(expected_marks):raise ValueError('missing native entry marks')
    for index,(mark,expected) in enumerate(zip(marks,expected_marks)):
        if set(mark)!=mark_fields or type(mark['label']) is not str or \
                any(type(mark[key]) is not int for key in mark_fields-{'label'}) or \
                (mark['pc'],mark['label'])!=expected or \
                (index and mark['global']<marks[index-1]['global']) or \
                mark['license']!=(-1 if index==0 else mark['global']-52) or \
                mark['motion']!=(-1 if index<=5 else mark['license']-540):
            raise ValueError('missing/reordered/mistyped native entry mark')
    origins=[row for row in marks if row['pc']==0x82F2EA]
    if len(origins)!=1 or origins[0]['license']!=540 or origins[0]['global']!=592:
        raise ValueError('native EA entry landmark changed')
    for index,row in enumerate(rows):
        if set(row)!=fields or type(row['blank']) is not bool or \
                any(type(row[key]) is not int for key in fields-{'blank'}) or \
                row['license']!=index or row['global']!=index+52 or \
                row['motion']!=(index-540 if index>=540 else -1) or \
                not 0<=row['brightness']<=15 or not 0<=row['main']<=31 or \
                not 0<=row['sub']<=31:
            raise ValueError('missing/duplicate/reordered/mistyped native frame state')
    return rows


def verify(native, actual):
    capture = IntroResources(native)
    states = native_frames(capture)
    actual = Path(actual)
    report = []
    for page in ('license', 'legal'):
        for brightness in range(16):
            frame = (8 if brightness == 15 else 143-brightness) if page == 'license' else 153+brightness
            black_reference = page=='license' and brightness==0
            state = states[frame]
            if state['blank'] is not black_reference or state['main']!=4 or state['sub']!=0 or \
                    state['brightness']!=(15 if black_reference else brightness):
                raise ValueError('native raster does not have the claimed PPU state')
            raw = capture.read(f'frame_{frame:04d}.rgb', 256*239*3)
            # The recorded Mesen viewport contains seven top/eight bottom
            # overscan lines. This fixed native 224-line viewport is stated in
            # the attested settings, not chosen to conceal differing pixels.
            expected = raw[7*256*3:231*256*3]
            output = (actual/f'{page}-{brightness:02d}.rgb').read_bytes()
            if len(output) != 256*224*3:
                raise ValueError('C raster has incorrect geometry')
            differences = sum(output[i:i+3] != expected[i:i+3] for i in range(0,len(output),3))
            report.append(dict(page=page, brightness=brightness, native_frame=frame,
                native_brightness=state['brightness'], native_forced_blank=state['blank'],
                classification='forced-blank black reference' if black_reference else 'matching native brightness',
                differing_pixels=differences, native_rgb_sha256=sha(expected), c_rgb_sha256=sha(output)))
    return {'scope': '31 matching native brightness rasters plus one forced-blank black reference; excludes scene timing/input/audio',
            'native_manifest_sha256':sha(capture.raw_manifest), 'comparisons':report,
            'pass':all(row['differing_pixels']==0 for row in report)}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native',required=True,type=Path)
    parser.add_argument('--actual',required=True,type=Path)
    parser.add_argument('--report',required=True,type=Path)
    args=parser.parse_args()
    result=verify(args.native,args.actual)
    args.report.write_text(json.dumps(result,indent=2)+'\n')
    print('PASS' if result['pass'] else 'FAIL',result['scope'])
    for row in result['comparisons']:
        if row['differing_pixels']:print(row)
    raise SystemExit(0 if result['pass'] else 1)


if __name__=='__main__':main()
