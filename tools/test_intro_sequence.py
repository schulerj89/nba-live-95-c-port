"""Exact address/phase-aligned intro rendering against independent native RGB.

This replaces historical C hashes/optional asynchronous PNG tolerances.
Whole cold-boot timing, skip behavior, audio, and EA-to-title remain FAIL/open;
this gate does not shift captures to claim a continuous native journey.
"""
import argparse
import json
from pathlib import Path
import struct
import subprocess
import tempfile
from PIL import Image
from intro_capture_resources import IntroResources, sha
from verify_intro_text import native_frames


def compare(capture,states,directory):
    # The C dispatcher still has its historical 345-frame lead-in. These
    # fixed mappings test the published renderer at known routine phases,
    # not equivalence of that lead-in with native CPU/resource scheduling.
    mappings=[(1,8,'license-full'),(134,142,'license-dim'),
              (136,154,'legal-rise'),(150,168,'legal-full'),
              (344,154,'legal-dim')]
    mappings += [(345+motion,540+motion,'EA-motion') for motion in range(303)]
    report=[]
    for c_frame,native_frame,phase in mappings:
        state=states[native_frame]
        if phase=='EA-motion' and (state['motion']!=c_frame-345 or
                state['brightness']!=15 or state['blank'] or state['main']!=17):
            raise ValueError('EA phase/PPU-state source changed')
        if state['sub']!=0:raise ValueError('unexpected native subscreen layers')
        if phase!='EA-motion' and (state['motion']!=-1 or state['blank'] or
                state['main']!=4 or state['brightness']!=(15 if phase.endswith('full') else 1)):
            raise ValueError('text phase/PPU-state source changed')
        raw=capture.read(f'frame_{native_frame:04d}.rgb',256*239*3)
        expected=raw[7*256*3:231*256*3]
        image=Image.open(directory/f'frame_{c_frame:04d}.bmp').convert('RGB')
        if image.size!=(256,224):raise ValueError('wrong C image geometry')
        actual=image.tobytes()
        pixels=sum(expected[i:i+3]!=actual[i:i+3] for i in range(0,len(actual),3))
        report.append(dict(phase=phase,c_frame=c_frame,native_frame=native_frame,
            differing_pixels=pixels,native_rgb_sha256=sha(expected),c_rgb_sha256=sha(actual)))
    return report


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('pack','exe','rom'):parser.add_argument('--'+name,required=True,type=Path)
    parser.add_argument('--native',type=Path,default=Path(__file__).resolve().parents[1]/'.analysis/intro-exact-20260830/capture-v4')
    parser.add_argument('--report',type=Path)
    args=parser.parse_args()
    capture=IntroResources(args.native);states=native_frames(capture)
    if sha(args.rom.read_bytes())!=capture.manifest['rom_sha256']:
        raise ValueError('C replay ROM differs from independent native reference')
    raw=args.pack.read_bytes()
    if raw[:8]!=b'NBA95PAK' or struct.unpack_from('<I',raw,8)[0]!=31:
        raise ValueError('unsupported pack format')
    count=struct.unpack_from('<I',raw,12)[0]
    ids=[struct.unpack_from('<I',raw,16+i*24)[0] for i in range(count)]
    if len(set(ids))!=len(ids) or not {75,76}.issubset(ids) or set(ids)&{1,2,3,4,5,6,70,71,72,73,74}:
        raise ValueError('intro pack retains forbidden legacy graphics or lacks indexed resources')
    with tempfile.TemporaryDirectory(prefix='nba95-indexed-intro-') as temp:
        directory=Path(temp)
        subprocess.run([str(args.exe.resolve()),'--headless','--rom',str(args.rom.resolve()),
            '--assets',str(args.pack.resolve()),'--frames','647','--dump-sequence-dir',str(directory)],
            capture_output=True,text=True,check=True,timeout=120)
        rows=compare(capture,states,directory)
    result={'scope':'303 EA motion frames and five native text-phase samples; NOT full intro timing/input/audio parity',
            'native_manifest_sha256':sha(capture.raw_manifest),'exe_sha256':sha(args.exe.read_bytes()),
            'pack_sha256':sha(raw),'comparisons':rows,'pass':all(row['differing_pixels']==0 for row in rows)}
    if args.report:args.report.write_text(json.dumps(result,indent=2)+'\n')
    differences=[row for row in rows if row['differing_pixels']]
    if differences:raise AssertionError(f'first exact rendering divergence: {differences[0]}')
    print('[TEST] PASS: '+result['scope'])


if __name__=='__main__':main()
