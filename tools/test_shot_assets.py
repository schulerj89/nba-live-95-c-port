"""ROM table extraction and proof that the additive pack entry changes only F12's count."""
import argparse
import hashlib
from pathlib import Path
import struct
import subprocess
import tempfile
from PIL import Image, ImageChops
from extract_assets import build_shot_gameplay_asset, build_fatigue_gameplay_asset, lorom_offset

OLD_F12={126:'73056055dbc5ebcb67e0005ad49c6025c06cc9dd4018141399f3c591eb382920',
         128:'a9034cc85cf92e237fdb9bc9d88219d63c754120a3904a9676ceb0273083ca42',
         160:'db347dfe608602a1677140cd13f80d5e3ebf149c69b7a9a6416fdb50e33c886d'}

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
    # The historical count predates shot/fatigue, court map279, jump280 and
    # graphics-scratch281. Remove all five for the old-count oracle; displayed asset art must
    # remain identical and only the count row may change.
    keep=[e for e in entries if e[0] not in (277,278,279,280,281)]
    struct.pack_into('<I',old,12,len(keep))
    for i,e in enumerate(keep):struct.pack_into('<6I',old,16+i*24,*e)
    # Keep all original payload offsets: the final unused directory slot is padding.
    with tempfile.TemporaryDirectory() as tmp:
        root=Path(tmp);prior=root/'without_shot_table.pak';prior.write_bytes(old)
        proof=Path(args.proof_dir) if args.proof_dir else root
        proof.mkdir(parents=True,exist_ok=True)
        for asset_id,expected in OLD_F12.items():
            images=[]
            for label,pack in (('before',prior),('after',Path(args.pack))):
                image=proof/f'f12_{asset_id}_{label}.bmp'
                subprocess.run([args.exe,'--headless','--asset-debug',str(asset_id),'--frames','1',
                    '--rom',args.rom,'--assets',str(pack),'--dump-frame',str(image)],capture_output=True,check=True)
                images.append(Image.open(image).convert('RGB'))
            if hashlib.sha256(images[0].tobytes()).hexdigest()!=expected:raise AssertionError('old F12 baseline not reproduced')
            bbox=ImageChops.difference(*images).getbbox()
            if not bbox or bbox[1]<19 or bbox[3]>27:raise AssertionError(f'non-count F12 pixels changed: {bbox}')
            print(f'[SHOT ASSETS] F12 {asset_id} only index/count changed: {bbox}; new hash={hashlib.sha256(images[1].tobytes()).hexdigest()}')
    print('[SHOT ASSETS] 528-byte shot / 88-byte fatigue tables and 348 stamina ratings equal ROM')

if __name__=='__main__':main()
