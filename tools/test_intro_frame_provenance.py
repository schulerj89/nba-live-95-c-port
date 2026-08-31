"""Independent integrity tests for intro evidence readers and phase mappings.

These tests mutate attested metadata or use explicitly synthetic black rasters
to test rejection behavior. They are not ROM-equivalence or rendering tests.
The separate intro sequence/font gates compare fresh C outputs to native RGB.
"""
import argparse
import copy
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

NATIVE = None
native_frames = IntroResources = compare = None


class Evidence:
    def __init__(self, frames, marks):
        self.frames = frames
        self.marks = marks

    def read(self, name, size=None):
        rows = self.frames if name == 'frames.jsonl' else self.marks
        if name.endswith('.jsonl'):
            return ('\n'.join(json.dumps(row) for row in rows)+'\n').encode()
        if name.endswith('.rgb'):
            # Synthetic black expected data tests the comparator only.
            return bytes(256*239*3)
        raise AssertionError('unexpected evidence read '+name)


class IntroFrameIntegrity(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        capture = IntroResources(NATIVE)
        cls.frames = native_frames(capture)
        cls.marks = [json.loads(line) for line in capture.read('marks.jsonl').splitlines()]

    def reject(self, frames=None, marks=None):
        with self.assertRaises(ValueError):
            native_frames(Evidence(self.frames if frames is None else frames,
                                   self.marks if marks is None else marks))

    def test_accepted_native_population(self):
        self.assertEqual(len(native_frames(Evidence(self.frames, self.marks))), 1500)
        self.assertEqual(len(self.marks), 11)

    def test_frame_population_and_order(self):
        for label, rows in (
            ('missing first', self.frames[1:]), ('missing last', self.frames[:-1]),
            ('duplicate', self.frames+[self.frames[-1]]),
            ('replace with duplicate', self.frames[:540]+[self.frames[539]]+self.frames[541:]),
            ('reordered', self.frames[:540]+self.frames[540:542][::-1]+self.frames[542:])):
            with self.subTest(case=label): self.reject(frames=rows)

    def test_frame_fields_and_types(self):
        for key, value in self.frames[540].items():
            missing=copy.deepcopy(self.frames);del missing[540][key]
            with self.subTest(field=key,mutation='missing'):self.reject(frames=missing)
            values=(0,1) if isinstance(value,bool) else (True,float(value),'0',None)
            for replacement in values:
                changed=copy.deepcopy(self.frames);changed[540][key]=replacement
                with self.subTest(field=key,mutation=repr(replacement)):self.reject(frames=changed)
        extra=copy.deepcopy(self.frames);extra[540]['unexpected']=0
        self.reject(frames=extra)

    def test_frame_index_and_ppu_domains(self):
        changes=[('license',541),('global',591),('motion',1),
                 ('brightness',-1),('brightness',16),('main',-1),('main',32),
                 ('sub',-1),('sub',32)]
        for key,value in changes:
            rows=copy.deepcopy(self.frames);rows[540][key]=value
            with self.subTest(field=key,value=value):self.reject(frames=rows)

    def test_mark_population_order_fields_and_types(self):
        for rows in (self.marks[:-1],self.marks[1:],self.marks+[self.marks[-1]],
                     self.marks[:5]+[self.marks[6],self.marks[5]]+self.marks[7:]):
            self.reject(marks=rows)
        for index in (0,5,10):
            for key,value in self.marks[index].items():
                rows=copy.deepcopy(self.marks);del rows[index][key]
                with self.subTest(mark=index,field=key,mutation='missing'):self.reject(marks=rows)
                values=(None,1) if isinstance(value,str) else (True,float(value),'0',None)
                for replacement in values:
                    rows=copy.deepcopy(self.marks);rows[index][key]=replacement
                    with self.subTest(mark=index,field=key,value=repr(replacement)):self.reject(marks=rows)
        for key,value in (('pc',0x82F2EB),('label','not-ea'),('global',593),
                          ('license',541),('motion',0)):
            rows=copy.deepcopy(self.marks);rows[5][key]=value
            with self.subTest(origin_field=key):self.reject(marks=rows)
        rows=copy.deepcopy(self.marks);rows[5]['global']+=1;rows[5]['license']+=1
        self.reject(marks=rows)
        rows=copy.deepcopy(self.marks);rows[5]['unexpected']=0
        self.reject(marks=rows)

    def test_duplicate_json_fields(self):
        base=Evidence(self.frames,self.marks)
        for name,key in (('frames.jsonl','"license": 540'),('marks.jsonl','"license": 540')):
            class Duplicate:
                def read(self, requested, size=None):
                    raw=base.read(requested,size)
                    if requested==name:
                        raw=raw.replace(key.encode(),(key+', '+key).encode(),1)
                    return raw
            with self.subTest(file=name):
                with self.assertRaises(ValueError):native_frames(Duplicate())

    def test_selected_phase_metadata_and_pixel_comparison(self):
        from PIL import Image
        evidence=Evidence(self.frames,self.marks)
        black=Image.new('RGB',(256,224))
        with patch('test_intro_sequence.Image.open',return_value=black):
            baseline=compare(evidence,self.frames,Path('synthetic-no-files'))
            self.assertEqual(len(baseline),308)
            self.assertTrue(all(row['differing_pixels']==0 for row in baseline))
            for index in (8,142,154,168,540,842):
                for key,value in (('brightness',0),('blank',True),('main',0),('sub',1)):
                    rows=copy.deepcopy(self.frames);rows[index][key]=value
                    with self.subTest(frame=index,field=key):
                        with self.assertRaises(ValueError):compare(evidence,rows,Path('synthetic-no-files'))
        white=Image.new('RGB',(256,224),'white')
        with patch('test_intro_sequence.Image.open',return_value=white):
            rows=compare(evidence,self.frames,Path('synthetic-no-files'))
            self.assertTrue(all(row['differing_pixels']==256*224 for row in rows))
        with patch('test_intro_sequence.Image.open',return_value=Image.new('RGB',(255,224))):
            with self.assertRaises(ValueError):compare(evidence,self.frames,Path('synthetic-no-files'))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native',required=True,type=Path)
    parser.add_argument('--tools-dir',type=Path,default=Path(__file__).resolve().parent)
    args=parser.parse_args()
    NATIVE=args.native.resolve()
    sys.path.insert(0,str(args.tools_dir.resolve()))
    from intro_capture_resources import IntroResources
    from verify_intro_text import native_frames
    from test_intro_sequence import compare
    unittest.main(argv=['test_intro_frame_provenance.py'])
