"""ROM table checks plus a separate C-only debugger metadata regression.

The F12 hashes are not ROM equivalence. Resource75/76 replaced eleven legacy
intro entries: the retained A1 executable/pack reproduced all old hashes,
and full-image comparisons proved only the index/count row changed. See
docs/intro-indexed-resources.md and the private f12-directory-audit report.
"""
import argparse
import hashlib
from pathlib import Path
import struct
import subprocess
import tempfile
from PIL import Image, ImageChops
from extract_assets import build_shot_gameplay_asset, build_fatigue_gameplay_asset, lorom_offset

OLD_F12={126:'92c8fe62770b73b205c9beb0d014b68f6614a011bfb142ac47e1c8322ab14458',
         128:'70db5852e6c46f451ab3a059b22aff0cfca2403fb4c585370819883b7d71bee2',
         160:'0c080a631497de4170b6ece2efbadcce7e167a15acaacbd186f00d2a325cefc1'}
# Retain the old full-image hashes above as migration provenance. These are
# the reviewed C baselines for the current pack with the six entries removed.
REDUCED_F12={126:'c97548e68407146f1ea29eb5aca82c3b0375eb67ec70c66ac55a77d7801e3109',
             128:'26dc531a5fbc6b2ba7965c980c444f2ef487f6b32ae5ad820c4923098f524e65',
             160:'4f506c80850e7fc84e923c6ccce255f6890463eb79d7a6445a110a7a13d36e0a'}

def main():
    p=argparse.ArgumentParser()
    for name in ('pack','rom','exe'):p.add_argument('--'+name,required=True)
    p.add_argument('--proof-dir')
    args=p.parse_args();raw=Path(args.pack).read_bytes()
    _,count=struct.unpack_from('<II',raw,8)
    entries=[struct.unpack_from('<6I',raw,16+i*24) for i in range(count)]
    entry=next(e for e in entries if e[0]==277)
    if entry[2:]!=(528,5,0,0x869eb2):raise AssertionError('shot table metadata')
    payload=raw[entry[1]:entry[1]+entry[2]]
    if payload!=build_shot_gameplay_asset(Path(args.rom).read_bytes()):raise AssertionError('shot tables differ from ROM')
    fatigue=next(e for e in entries if e[0]==278)
    if fatigue[2:]!=(88,4,8,0x8798da):raise AssertionError('fatigue table metadata')
    if raw[fatigue[1]:fatigue[1]+fatigue[2]]!=build_fatigue_gameplay_asset(Path(args.rom).read_bytes()):
        raise AssertionError('fatigue tables differ from ROM')
    rom=Path(args.rom).read_bytes()
    rosters=next(e for e in entries if e[0]==251)
    for i in range(29*12):
        start=rosters[1]+24+i*64
        address=struct.unpack_from('<I',raw,start)[0]
        rating=rom[lorom_offset(address)+0x35]
        if raw[start+28]!=rating or not 3<=rating<=10:
            raise AssertionError(f'roster recovery rating mismatch: {i}')
    old=bytearray(raw)
    # Remove the same six entries as the historical C-only debugger check.
    # This reduced pack is a test fixture, not a previous production version.
    # Displayed art must remain identical; only the count row may change.
    keep=[e for e in entries if e[0] not in (277,278,279,280,281,285)]
    struct.pack_into('<I',old,12,len(keep))
    for i,e in enumerate(keep):struct.pack_into('<6I',old,16+i*24,*e)
    # Keep all original payload offsets: the final unused directory slot is padding.
    with tempfile.TemporaryDirectory() as tmp:
        root=Path(tmp);prior=root/'without_shot_table.pak';prior.write_bytes(old)
        proof=Path(args.proof_dir) if args.proof_dir else root
        proof.mkdir(parents=True,exist_ok=True)
        for asset_id,expected in REDUCED_F12.items():
            images=[]
            for label,pack in (('before',prior),('after',Path(args.pack))):
                image=proof/f'f12_{asset_id}_{label}.bmp'
                subprocess.run([args.exe,'--headless','--asset-debug',str(asset_id),'--frames','1',
                    '--rom',args.rom,'--assets',str(pack),'--dump-frame',str(image)],capture_output=True,check=True)
                images.append(Image.open(image).convert('RGB'))
            if hashlib.sha256(images[0].tobytes()).hexdigest()!=expected:raise AssertionError('reviewed C-only reduced-pack F12 baseline not reproduced')
            bbox=ImageChops.difference(*images).getbbox()
            if not bbox or bbox[1]<19 or bbox[3]>27:raise AssertionError(f'non-count F12 pixels changed: {bbox}')
            print(f'[SHOT ASSETS] F12 {asset_id} only index/count changed: {bbox}; new hash={hashlib.sha256(images[1].tobytes()).hexdigest()}')
    print('[SHOT ASSETS] 528-byte shot / 88-byte fatigue tables and 348 stamina ratings equal ROM')

if __name__=='__main__':main()
