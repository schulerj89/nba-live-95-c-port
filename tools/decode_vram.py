"""Decode a Mesen VRAM/CGRAM capture of the Game Setup screen into per-layer images.

The SNES PPU state for this screen (captured live from the ROM) is BG Mode 1:
  BG1  4bpp  64x32 tilemap
  BG2  4bpp  64x32 tilemap
  BG3  2bpp  32x64 tilemap
"""
import sys, os
from PIL import Image

OUT = os.path.join(os.path.dirname(__file__), "..", ".analysis", "setup_capture")

vram = open(os.path.join(OUT, "vram.bin"), "rb").read()
cgram = open(os.path.join(OUT, "cgram.bin"), "rb").read()


def palette():
    pal = []
    for i in range(256):
        w = cgram[i * 2] | (cgram[i * 2 + 1] << 8)
        r = (w & 31) * 255 // 31
        g = ((w >> 5) & 31) * 255 // 31
        b = ((w >> 10) & 31) * 255 // 31
        pal.append((r, g, b))
    return pal


PAL = palette()


def tile_pixels(base, idx, bpp):
    """Return an 8x8 array of palette indices for tile `idx`."""
    size = 8 * bpp
    off = base + idx * size
    px = [[0] * 8 for _ in range(8)]
    for y in range(8):
        planes = []
        for p in range(0, bpp, 2):
            lo = vram[(off + y * 2 + p * 8) & 0xFFFF]
            hi = vram[(off + y * 2 + 1 + p * 8) & 0xFFFF]
            planes.append((lo, hi))
        for x in range(8):
            bit = 7 - x
            v = 0
            for pi, (lo, hi) in enumerate(planes):
                v |= ((lo >> bit) & 1) << (pi * 2)
                v |= ((hi >> bit) & 1) << (pi * 2 + 1)
            px[y][x] = v
    return px


def render_bg(tilemap_byte, chr_byte, bpp, wide, tall, name, pal_base_shift=0):
    cols = 64 if wide else 32
    rows = 64 if tall else 32
    img = Image.new("RGB", (cols * 8, rows * 8), (0, 0, 0))
    px = img.load()
    for sy in range(rows):
        for sx in range(cols):
            # 32x32 screen quadrants are stored sequentially
            quad = (1 if sx >= 32 else 0) + (2 if sy >= 32 else 0)
            if not wide:
                quad = (2 if sy >= 32 else 0)
            qx, qy = sx % 32, sy % 32
            ent = tilemap_byte + quad * 0x800 + (qy * 32 + qx) * 2
            if ent + 1 >= len(vram):
                continue
            e = vram[ent] | (vram[ent + 1] << 8)
            tid = e & 0x3FF
            pnum = (e >> 10) & 7
            hflip = (e >> 14) & 1
            vflip = (e >> 15) & 1
            tp = tile_pixels(chr_byte, tid, bpp)
            ncol = 1 << bpp
            for y in range(8):
                for x in range(8):
                    v = tp[7 - y if vflip else y][7 - x if hflip else x]
                    c = PAL[(pnum * ncol + v) & 0xFF] if v else (0, 0, 0)
                    px[sx * 8 + x, sy * 8 + y] = c
    img.save(os.path.join(OUT, name))
    print("wrote", name, img.size)


if __name__ == "__main__":
    # try both word- and byte-address interpretations of the reported values
    mode = sys.argv[1] if len(sys.argv) > 1 else "word"
    m = 2 if mode == "word" else 1
    render_bg(3072 * m, 12288 * m, 4, True, False, f"bg1_{mode}.png")
    render_bg(2048 * m, 4096 * m, 4, True, False, f"bg2_{mode}.png")
    render_bg(0 * m, 16384 * m, 2, False, True, f"bg3_{mode}.png")
