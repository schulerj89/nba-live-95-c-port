"""Asset extent, ROM-map provenance, and optional live VRAM pixel oracle."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from PIL import Image
from extract_assets import decode_bg_layer,load_verified_rom

def assets(path):
    raw=Path(path).read_bytes()
    assert raw[:8]==b'NBA95PAK' and struct.unpack_from('<I',raw,8)[0]==29
    result={}
    for i in range(struct.unpack_from('<I',raw,12)[0]):
        ident,offset,size,w,h,flags=struct.unpack_from('<6I',raw,16+i*24)
        result[ident]=(raw[offset:offset+size],w,h,flags)
    return result

def main():
    p=argparse.ArgumentParser();p.add_argument('--pack',required=True);p.add_argument('--rom',required=True)
    p.add_argument('--native-dir');p.add_argument('--report');args=p.parse_args()
    pack=assets(args.pack);rom=load_verified_rom(args.rom)
    assert struct.unpack_from('<HH',rom,0x100000)==(148,52)
    raw,w,h,bank=pack[279]
    assert (w,h,bank)==(148,52,0xa08000) and raw==rom[0x100000:0x100006+148*52*2]
    data,w,h,teams=pack[273]
    assert (w,h,teams)==(1184,416,29)
    assert data[:8]==b'NBCOURT2' and struct.unpack_from('<4I',data,8)==(2,29,w,h)
    size=w*h*4
    assert len(data)==24+29*size
    # Camera extrema must fit without changing the requested origin.
    assert 328+582+256<=w and -53+243+224<=h
    right=[]
    for team in range(29):
        frame=data[24+team*size:24+(team+1)*size]
        right.append(hashlib.sha256(b''.join(frame[(y*w+776)*4:(y*w+1032)*4] for y in range(224))).hexdigest())
    assert len(set(right))>=27,'right-side team court identities collapsed'
    witnesses=Path(__file__).resolve().parents[1]/'tests/fixtures/court-viewport-witnesses.json'
    captured=json.loads(witnesses.read_text())
    assert len(captured)==12,'missing native viewport witnesses'
    for witness in captured:
        x,y=witness['published_camera'];px=x+582;py=y+243
        visible=b''.join(raw[6+(((px+col)//8)*52+(py+row)//8)*2:
                            8+(((px+col)//8)*52+(py+row)//8)*2]
                         for row in range(224) for col in range(256))
        assert hashlib.sha256(visible).hexdigest()==witness['visible_map_words_sha256'],'ROM/PPU court layout changed'
    print('Court assets PASS: ROM header148x52; all map words; 29 complete1184x416 surfaces; camera bounds')
    report=[]
    if args.native_dir:
        root=Path(args.native_dir)
        for path in sorted(root.glob('native_*_state.txt')):
            state=dict(line.strip().split('=',1) for line in path.read_text().splitlines())
            def n(k):return int(state[k])
            stem=str(path).removesuffix('_state.txt')
            vram=Path(stem+'_vram.bin').read_bytes();cgram=Path(stem+'_cgram.bin').read_bytes()
            native=decode_bg_layer(vram,cgram,n('ppu.layers[1].tilemapAddress')*2,n('ppu.layers[1].chrAddress')*2,
                4,state['ppu.layers[1].doubleWidth']=='true',state['ppu.layers[1].doubleHeight']=='true',
                n('ppu.layers[1].hscroll'),n('ppu.layers[1].vscroll'))
            # WRAM can already contain the next logical camera while PPU
            # still displays the previous pass. Resolve the published PPU
            # scroll against the resident circular-map origin, not a frame
            # number or guessed offset chosen to make pictures match.
            dest=n('destination');hs=n('ppu.layers[1].hscroll');vs=n('ppu.layers[1].vscroll')
            ring_x=(dest&31)+((dest&0x400)>>5);ring_y=(dest&0x3e0)>>5
            px=n('coarse_x')*8+(((hs>>3)-ring_x+32)%64-32)*8+(hs&7)
            py=n('coarse_y')*8+(((vs>>3)-ring_y+16)%32-16)*8+(vs&7)
            x=px-582;y=py-242
            home=n('home_team')
            frame=data[24+home*size:24+(home+1)*size]
            expected=b''.join(frame[((y+243+row)*w+x+582)*4:((y+243+row)*w+x+582+256)*4] for row in range(224))
            mismatch=sum(native[i:i+4]!=expected[i:i+4] for i in range(0,len(native),4))
            # Independent ROM-map layout with CURRENT native CHR/CGRAM:
            # distinguishes mapping bugs from animated crowd tile graphics.
            rebuilt=bytearray(vram)
            for ty in range(29):
                for tx in range(33):
                    tile_x=(px>>3)+tx;tile_y=(py>>3)+ty
                    off=6+(tile_x*52+tile_y)*2
                    dst=0x1000+(0x800 if tx>=32 else 0)+(ty*32+(tx&31))*2
                    rebuilt[dst:dst+2]=raw[off:off+2]
            mapped=decode_bg_layer(rebuilt,cgram,0x1000,0x4000,4,True,False,px&7,py&7)
            geometry_mismatch=sum(native[i:i+4]!=mapped[i:i+4] for i in range(0,len(native),4))
            visible=bytearray()
            for row in range(224):
                for col in range(256):
                    vx=(col+hs)%512;vy=(row+vs+1)%256
                    off=0x1000+(0x800 if vx>=256 else 0)+((vy//8)*32+((vx//8)&31))*2
                    visible.extend(vram[off:off+2])
            dynamic_tiles=set()
            for i in range(256*224):
                if native[i*4:i*4+4]!=expected[i*4:i*4+4]:
                    tx=(px+i%256)>>3;ty=(py+1+i//256)>>3
                    dynamic_tiles.add(struct.unpack_from('<H',raw,6+(tx*52+ty)*2)[0]&0x3ff)
            Image.frombytes('RGBA',(256,224),native,'raw','BGRA').convert('RGB').save(stem+'_bg2.png')
            Image.frombytes('RGBA',(256,224),expected,'raw','BGRA').convert('RGB').save(stem+'_pack.png')
            report.append({'frame':path.stem,'home':home,'published_camera':[x,y],'static_art_pixel_differences':mismatch,
                           'geometry_pixel_mismatches':geometry_mismatch,'different_art_tiles':sorted(dynamic_tiles),
                           'visible_map_words_sha256':hashlib.sha256(visible).hexdigest(),
                           'native_bg2_sha256':hashlib.sha256(native).hexdigest()})
        print(json.dumps(report,indent=2))
        if args.report:Path(args.report).write_text(json.dumps(report,indent=2)+'\n')
        assert report and not any(v['geometry_pixel_mismatches'] for v in report),'native map/scroll mismatch'

if __name__=='__main__':main()
