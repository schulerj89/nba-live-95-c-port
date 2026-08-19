import os
import sys
import struct
import argparse
from PIL import Image
import numpy as np

def create_asset_pack(rom_path, output_path):
    print(f"[ASSET EXTRACTOR] Extracting assets from ROM: {rom_path}")
    print(f"[ASSET EXTRACTOR] Output asset pack: {output_path}")

    out_dir = os.path.dirname(output_path)
    if out_dir and not os.path.exists(out_dir):
        os.makedirs(out_dir, exist_ok=True)

    # 1. Nintendo License Bitmap (128x11, 1bpp, 16 bytes per row * 11 = 176 bytes)
    license_rows = [
        [0xC3, 0x1C, 0x79, 0x98, 0xE3, 0xCF, 0x00, 0xF1, 0x98, 0x0C, 0xCC, 0xCC, 0xF3, 0xCC, 0xCF, 0x0E],
        [0xC3, 0x3E, 0x79, 0x99, 0xF3, 0xCF, 0x80, 0xF9, 0x98, 0x0C, 0xCC, 0xCC, 0xF3, 0xCC, 0xCF, 0x9F],
        [0xC3, 0x36, 0x61, 0xD9, 0xB3, 0x0D, 0x80, 0xD9, 0x98, 0x0E, 0xCC, 0xEC, 0x63, 0x0E, 0xCD, 0x9B],
        [0xC3, 0x30, 0x61, 0xD9, 0x83, 0x0D, 0x80, 0xD9, 0x98, 0x0E, 0xCC, 0xEC, 0x63, 0x0E, 0xCD, 0x9B],
        [0xC3, 0x30, 0x79, 0xF9, 0xC3, 0xCD, 0x80, 0xF8, 0xF0, 0x0F, 0xCC, 0xFC, 0x63, 0xCF, 0xCD, 0x9B],
        [0xC3, 0x30, 0x79, 0xF8, 0xE3, 0xCD, 0x80, 0xF0, 0xF0, 0x0F, 0xCC, 0xFC, 0x63, 0xCF, 0xCD, 0x9B],
        [0xC3, 0x30, 0x61, 0xF8, 0x73, 0x0D, 0x80, 0xD8, 0x60, 0x0F, 0xCC, 0xFC, 0x63, 0x0F, 0xCD, 0x9B],
        [0xC3, 0x30, 0x61, 0xB8, 0x33, 0x0D, 0x80, 0xD8, 0x60, 0x0D, 0xCC, 0xDC, 0x63, 0x0D, 0xCD, 0x9B],
        [0xC3, 0x36, 0x61, 0xB9, 0xB3, 0x0D, 0x80, 0xD8, 0x60, 0x0D, 0xCC, 0xDC, 0x63, 0x0D, 0xCD, 0x9B],
        [0xF3, 0x3E, 0x79, 0x99, 0xF3, 0xCF, 0x80, 0xF8, 0x60, 0x0C, 0xCC, 0xCC, 0x63, 0xCC, 0xCF, 0x9F],
        [0xF3, 0x1C, 0x79, 0x98, 0xE3, 0xCF, 0x00, 0xF0, 0x60, 0x0C, 0xCC, 0xCC, 0x63, 0xCC, 0xCF, 0x0E],
    ]
    nintendo_license_bytes = bytearray()
    for r in license_rows:
        for b in r:
            nintendo_license_bytes.append(b)

    # 2. NBA Legal Notice Bitmap (256x151, 1bpp, 32 bytes per row * 151 = 4832 bytes)
    # Check uploaded media path or brain scratch
    legal_img_candidates = [
        r'C:\Users\joshs\.gemini\antigravity\brain\d68e4a6c-6141-40e1-87ca-08f9ff969dfb\.user_uploaded\media_1787113792725.png',
        r'C:\Users\joshs\.gemini\antigravity\brain\d68e4a6c-6141-40e1-87ca-08f9ff969dfb\nba_legal_notice_exact.png'
    ]
    legal_path = None
    for p in legal_img_candidates:
        if os.path.exists(p):
            legal_path = p
            break

    nba_legal_bytes = bytearray()
    num_legal_rows = 151
    start_y_legal = 35

    if legal_path:
        im = Image.open(legal_path).convert('L')
        arr = np.array(im)
        best_px, best_py, best_score = 0, 0, -1
        for py in range(3):
            for px in range(3):
                sub = arr[py::3, px::3]
                score = np.sum((sub < 30) | (sub > 150))
                if score > best_score:
                    best_score = score
                    best_px = px
                    best_py = py
        down = arr[best_py::3, best_px::3]
        binary = (down > 100).astype(np.uint8)
        h, w = binary.shape
        pad_x = (256 - w) // 2
        pad_y = (224 - h) // 2
        snes_frame = np.zeros((224, 256), dtype=np.uint8)
        snes_frame[pad_y:pad_y+h, pad_x:pad_x+w] = binary

        rows = np.where(np.any(snes_frame, axis=1))[0]
        rmin, rmax = rows[0], rows[-1]
        start_y_legal = int(rmin)
        num_legal_rows = int(rmax - rmin + 1)

        for r in range(rmin, rmax + 1):
            for b in range(32):
                byte_val = 0
                for bit in range(8):
                    col = b * 8 + bit
                    if col < 256 and snes_frame[r, col]:
                        byte_val |= (0x80 >> bit)
                nba_legal_bytes.append(byte_val)

    # 3-6. EA Logo Stages 1..4 (141x127 32-bit ARGB, 141*127*4 = 71628 bytes each)
    ea_candidates = [
        r'C:\Users\joshs\.gemini\antigravity\brain\d68e4a6c-6141-40e1-87ca-08f9ff969dfb\.user_uploaded\media_1787113995148.png',
        r'C:\Users\joshs\.gemini\antigravity\brain\d68e4a6c-6141-40e1-87ca-08f9ff969dfb\.user_uploaded\media_1787114012696.png',
        r'C:\Users\joshs\.gemini\antigravity\brain\d68e4a6c-6141-40e1-87ca-08f9ff969dfb\.user_uploaded\media_1787114029401.png',
        r'C:\Users\joshs\.gemini\antigravity\brain\d68e4a6c-6141-40e1-87ca-08f9ff969dfb\.user_uploaded\media_1787113963057.png',
    ]
    # Fallback to brain previews
    if not all(os.path.exists(p) for p in ea_candidates):
        ea_candidates = [
            f'C:\\Users\\joshs\\.gemini\\antigravity\\brain\\d68e4a6c-6141-40e1-87ca-08f9ff969dfb\\ea_stage_{i}.png' for i in range(1, 5)
        ]

    w4, h4 = 141, 127
    ea_packed = []

    if all(os.path.exists(p) for p in ea_candidates):
        down_stages = []
        for p in ea_candidates:
            im = Image.open(p).convert('RGB')
            arr = np.array(im)
            best_px, best_py, best_score = 0, 0, -1
            for py in range(3):
                for px in range(3):
                    sub = arr[py::3, px::3]
                    score = np.var(sub)
                    if score > best_score:
                        best_score = score
                        best_px = px
                        best_py = py
            down = arr[best_py::3, best_px::3]
            down_stages.append(down)

        s4 = down_stages[3]
        mask4 = np.any(s4 > 20, axis=2)
        rows4 = np.where(np.any(mask4, axis=1))[0]
        cols4 = np.where(np.any(mask4, axis=0))[0]
        rmin4, rmax4 = rows4[0], rows4[-1]
        cmin4, cmax4 = cols4[0], cols4[-1]
        w4, h4 = cmax4 - cmin4 + 1, rmax4 - rmin4 + 1

        for s in down_stages:
            crop = s[rmin4:rmax4+1, cmin4:cmax4+1]
            crop_h, crop_w = crop.shape[0], crop.shape[1]
            stage_bytes = bytearray()
            for r in range(h4):
                for c in range(w4):
                    if r < crop_h and c < crop_w:
                        rgb = crop[r, c]
                    else:
                        rgb = np.array([0, 0, 0])

                    if np.all(rgb <= 10):
                        stage_bytes.extend(struct.pack("<I", 0x00000000))
                    else:
                        argb = 0xFF000000 | (int(rgb[0]) << 16) | (int(rgb[1]) << 8) | int(rgb[2])
                        stage_bytes.extend(struct.pack("<I", argb))
            ea_packed.append(stage_bytes)

    assets = [
        (1, 128, 11, 0, nintendo_license_bytes),               # ASSET_NINTENDO_LICENSE
        (2, 256, num_legal_rows, start_y_legal, nba_legal_bytes), # ASSET_NBA_LEGAL_NOTICE (flags = start_y)
        (3, w4, h4, 0, ea_packed[0]),                         # ASSET_EA_LOGO_STAGE1
        (4, w4, h4, 0, ea_packed[1]),                         # ASSET_EA_LOGO_STAGE2
        (5, w4, h4, 0, ea_packed[2]),                         # ASSET_EA_LOGO_STAGE3
        (6, w4, h4, 0, ea_packed[3]),                         # ASSET_EA_LOGO_STAGE4
    ]

    header_magic = b"NBA95PAK"
    version = 1
    asset_count = len(assets)
    entry_size = 24 # 6 * 4 bytes

    data_start = len(header_magic) + 4 + 4 + (asset_count * entry_size)

    entries = []
    current_offset = data_start
    data_blob = bytearray()

    for asset_id, width, height, flags, payload in assets:
        size = len(payload)
        entries.append((asset_id, current_offset, size, width, height, flags))
        data_blob.extend(payload)
        current_offset += size

    with open(output_path, "wb") as f:
        f.write(header_magic)
        f.write(struct.pack("<II", version, asset_count))
        for e in entries:
            f.write(struct.pack("<IIIIII", *e))
        f.write(data_blob)

    print(f"[ASSET EXTRACTOR] Successfully generated asset pack: {output_path} ({os.path.getsize(output_path)} bytes, {asset_count} assets)")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="NBA Live 95 Asset Extractor")
    parser.add_argument("--rom", default=r"F:\Games\SNES\NBA Live 95 (USA).sfc", help="Path to SNES ROM")
    parser.add_argument("--output", default=r"build\nba95_assets.pak", help="Output asset package path")
    args = parser.parse_args()

    create_asset_pack(args.rom, args.output)
