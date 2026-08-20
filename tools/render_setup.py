"""Software-composite the Game Setup screen from VRAM/CGRAM, SNES BG Mode 1.

Used to validate that data rebuilt from the ROM reproduces the real frame.
PPU configuration is the one captured live on the Game Setup screen:
  BG1 chr $6000 map $1800 64x32, scroll 512/1023   (header banner)
  BG2 chr $2000 map $1000 64x32, scroll 0/vscroll  (scrolling backdrop)
  BG3 chr $8000 map $0000 32x64, scroll 0/0        (menu text, 2bpp)
"""
import sys, os
from PIL import Image

W, H = 256, 224

def load(path):
    return open(path, "rb").read()

def make_pal(cgram):
    pal = []
    for i in range(256):
        w = cgram[i*2] | (cgram[i*2+1] << 8)
        pal.append((((w) & 31)*255//31, ((w>>5)&31)*255//31, ((w>>10)&31)*255//31))
    return pal

def tile_px(vram, base, idx, bpp):
    off = base + idx * 8 * bpp
    out = [[0]*8 for _ in range(8)]
    for y in range(8):
        rows = []
        for p in range(0, bpp, 2):
            lo = vram[(off + y*2 + p*8) & 0xFFFF]
            hi = vram[(off + y*2 + 1 + p*8) & 0xFFFF]
            rows.append((lo, hi))
        for x in range(8):
            b = 7 - x
            v = 0
            for pi,(lo,hi) in enumerate(rows):
                v |= ((lo >> b) & 1) << (pi*2)
                v |= ((hi >> b) & 1) << (pi*2+1)
            out[y][x] = v
    return out

def sample(vram, pal, mapbase, chrbase, bpp, wide, tall, hs, vs, x, y):
    """Return (r,g,b) or None when the pixel is transparent."""
    mw = 512 if wide else 256
    mh = 512 if tall else 256
    px = (x + hs) % mw
    py = (y + vs) % mh
    tx, ty = px // 8, py // 8
    quad = 0
    if wide and tx >= 32: quad += 1
    if tall and ty >= 32: quad += 2 if wide else 1
    ent = mapbase + quad*0x800 + ((ty % 32)*32 + (tx % 32))*2
    e = vram[ent] | (vram[ent+1] << 8)
    tid = e & 0x3FF
    pnum = (e >> 10) & 7
    hf = (e >> 14) & 1
    vf = (e >> 15) & 1
    t = tile_px(vram, chrbase, tid, bpp)
    sy = 7-(py % 8) if vf else (py % 8)
    sx = 7-(px % 8) if hf else (px % 8)
    v = t[sy][sx]
    if v == 0:
        return None
    ncol = 1 << bpp
    return pal[(pnum*ncol + v) & 0xFF]

def render(vram, cgram, bg2_vs, layers=("bg3","bg1","bg2")):
    pal = make_pal(cgram)
    img = Image.new("RGB", (W, H), (0,0,0))
    p = img.load()
    cfg = {
        "bg1": dict(mapbase=0x1800, chrbase=0x6000, bpp=4, wide=True,  tall=False, hs=512, vs=1023),
        "bg2": dict(mapbase=0x1000, chrbase=0x2000, bpp=4, wide=True,  tall=False, hs=0,   vs=bg2_vs),
        "bg3": dict(mapbase=0x0000, chrbase=0x8000, bpp=2, wide=False, tall=True,  hs=0,   vs=0),
    }
    for y in range(H):
        for x in range(W):
            col = (0,0,0)
            for name in layers:           # painter order: last wins
                c = sample(vram, pal, **cfg[name], x=x, y=y)
                if c is not None:
                    col = c
            p[x,y] = col
    return img

if __name__ == "__main__":
    d = sys.argv[1] if len(sys.argv) > 1 else ".analysis/setup_capture/replay"
    vs = int(sys.argv[2]) if len(sys.argv) > 2 else 51
    vram = load(os.path.join(d, "setup_vram.bin"))
    cgram = load(os.path.join(d, "setup_cgram.bin"))
    img = render(vram, cgram, vs)
    out = os.path.join(".analysis/setup_capture", "render_from_rom.png")
    img.resize((W*2, H*2), Image.NEAREST).save(out)
    print("wrote", out)
