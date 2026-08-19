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

    # 3-6. EA Logo Stages 1..4
    ea_candidates = [
        r'C:\Users\joshs\.gemini\antigravity\brain\d68e4a6c-6141-40e1-87ca-08f9ff969dfb\.user_uploaded\media_1787113995148.png',
        r'C:\Users\joshs\.gemini\antigravity\brain\d68e4a6c-6141-40e1-87ca-08f9ff969dfb\.user_uploaded\media_1787114012696.png',
        r'C:\Users\joshs\.gemini\antigravity\brain\d68e4a6c-6141-40e1-87ca-08f9ff969dfb\.user_uploaded\media_1787114029401.png',
        r'C:\Users\joshs\.gemini\antigravity\brain\d68e4a6c-6141-40e1-87ca-08f9ff969dfb\.user_uploaded\media_1787113963057.png',
    ]
    if not all(os.path.exists(p) for p in ea_candidates):
        ea_candidates = [
            f'C:\\Users\\joshs\\.gemini\\antigravity\\brain\\d68e4a6c-6141-40e1-87ca-08f9ff969dfb\\ea_stage_{i}.png' for i in range(1, 5)
        ]

    w4, h4 = 145, 127
    ea_flags = (53 << 16) | 48  # start_x = 53, start_y = 48
    ea_packed = []

    if all(os.path.exists(p) for p in ea_candidates):
        # Determine best downsampling phase from Stage 4
        im4 = Image.open(ea_candidates[3]).convert('RGB')
        arr4 = np.array(im4)
        best_px4, best_py4, best_score4 = 0, 0, -1
        for py in range(3):
            for px in range(3):
                sub = arr4[py::3, px::3]
                score = float(np.var(sub))
                if score > best_score4:
                    best_score4 = score
                    best_px4, best_py4 = px, py

        downs = []
        for p in ea_candidates:
            im = Image.open(p).convert('RGB')
            arr = np.array(im)
            downs.append(arr[best_py4::3, best_px4::3])

        # Find E-piece top anchor in each stage to align relative offsets
        def find_e_anchor(down):
            r_ch = down[:,:,0].astype(float)
            g_ch = down[:,:,1].astype(float)
            b_ch = down[:,:,2].astype(float)
            orange = (r_ch > 80) & (g_ch < 80) & (b_ch < 60)
            rows = np.where(np.any(orange, axis=1))[0]
            cols = np.where(np.any(orange, axis=0))[0]
            if len(rows) > 0:
                top_row = rows[0]
                left_col = np.where(orange[top_row])[0][0]
                return (left_col, top_row)
            return None

        anchors = [find_e_anchor(d) for d in downs]
        master = anchors[3]
        offsets = [(master[0] - a[0], master[1] - a[1]) if a and master else (0, 0) for a in anchors]

        # Stage 4 reference position on 256x224
        mask4 = np.any(downs[3] > 20, axis=2)
        rows4 = np.where(np.any(mask4, axis=1))[0]
        cols4 = np.where(np.any(mask4, axis=0))[0]
        s4_w = cols4[-1] - cols4[0] + 1
        s4_h = rows4[-1] - rows4[0] + 1
        s4_ox, s4_oy = cols4[0], rows4[0]

        snes_sx = (256 - s4_w) // 2
        snes_sy = (224 - s4_h) // 2

        snes_frames = []
        for i, (down, (dx, dy)) in enumerate(zip(downs, offsets)):
            frame = np.zeros((224, 256, 3), dtype=np.uint8)
            h, w = down.shape[:2]
            for r in range(h):
                for c in range(w):
                    s4_x = c + dx
                    s4_y = r + dy
                    sx = s4_x - s4_ox + snes_sx
                    sy = s4_y - s4_oy + snes_sy
                    if 0 <= sx < 256 and 0 <= sy < 224:
                        rgb = down[r, c]
                        if np.any(rgb > 10):
                            frame[sy, sx] = rgb
            snes_frames.append(frame)

        # Union bounding box across all 4 frames
        all_rows, all_cols = [], []
        for frame in snes_frames:
            m = np.any(frame > 10, axis=2)
            rs = np.where(np.any(m, axis=1))[0]
            cs = np.where(np.any(m, axis=0))[0]
            if len(rs) > 0:
                all_rows.extend([rs[0], rs[-1]])
                all_cols.extend([cs[0], cs[-1]])

        urmin, urmax = min(all_rows), max(all_rows)
        ucmin, ucmax = min(all_cols), max(all_cols)
        w4 = ucmax - ucmin + 1
        h4 = urmax - urmin + 1
        ea_flags = (int(ucmin) << 16) | int(urmin)

        for frame in snes_frames:
            crop = frame[urmin:urmax+1, ucmin:ucmax+1]
            stage_bytes = bytearray()
            for r in range(h4):
                for c in range(w4):
                    rgb = crop[r, c]
                    if np.all(rgb <= 10):
                        stage_bytes.extend(struct.pack("<I", 0x00000000))
                    else:
                        argb = 0xFF000000 | (int(rgb[0]) << 16) | (int(rgb[1]) << 8) | int(rgb[2])
                        stage_bytes.extend(struct.pack("<I", argb))
            ea_packed.append(stage_bytes)

    # 7. Audio: EA Intro Voice / Sound Effect
    def decode_brr_to_pcm(data):
        pcm = []
        p1, p2 = 0, 0
        pos = 0
        while pos + 9 <= len(data):
            h = data[pos]
            shift = h >> 4
            f = (h >> 2) & 3
            end = (h & 1) != 0
            if shift > 12:
                break
            pos += 1
            for b in range(8):
                byte_val = data[pos + b]
                for nibble in [(byte_val >> 4) & 0xF, byte_val & 0xF]:
                    sample = nibble if nibble < 8 else nibble - 16
                    sample = (sample << shift) >> 1
                    if f == 0: out = sample
                    elif f == 1: out = sample + p1 + ((-p1) >> 4)
                    elif f == 2: out = sample + (p1 << 1) + ((-((p1 << 1) + p1)) >> 5) - p2 + (p2 >> 4)
                    elif f == 3: out = sample + (p1 << 1) + ((-(p1 + (p1 << 2) + (p1 << 3))) >> 6) - p2 + (((p2 << 1) + p2) >> 4)
                    else: out = sample
                    out = max(-32768, min(32767, int(out)))
                    pcm.append(out)
                    p2, p1 = p1, out
            pos += 8
            if end:
                break
        return pcm

    def make_wav_bytes(pcm_samples, num_channels=1, sample_rate=16000, bits_per_sample=16):
        data_size = len(pcm_samples) * 2
        header = struct.pack(
            '<4sI4s4sIHHIIHH4sI',
            b'RIFF',
            36 + data_size,
            b'WAVE',
            b'fmt ',
            16,
            1, # PCM
            num_channels,
            sample_rate,
            sample_rate * num_channels * 2,
            num_channels * 2,
            bits_per_sample,
            b'data',
            data_size
        )
        raw_data = struct.pack(f'<{len(pcm_samples)}h', *pcm_samples)
        return header + raw_data

    audio_bytes = bytearray()
    custom_audio_candidates = [
        os.path.join(os.path.dirname(output_path), "ea_sports_intro.wav"),
        os.path.join(os.path.dirname(rom_path), "ea_sports_intro.wav"),
        "ea_sports_intro.wav"
    ]
    for cap in custom_audio_candidates:
        if os.path.exists(cap):
            with open(cap, "rb") as af:
                audio_bytes = af.read()
            print(f"[ASSET EXTRACTOR] Loaded external audio track: {cap} ({len(audio_bytes)} bytes)")
            break

    if len(audio_bytes) == 0 and os.path.exists(rom_path):
        with open(rom_path, "rb") as rf:
            rom_data = rf.read()
        # Decode authentic digitized voice audio sample from ROM at offset 0x043025 (35760 samples)
        if len(rom_data) > 0x043025 + 20115:
            pcm = decode_brr_to_pcm(rom_data[0x043025:0x043025 + 20115])
            if len(pcm) > 0:
                audio_bytes = make_wav_bytes(pcm, sample_rate=14000)
                print(f"[ASSET EXTRACTOR] Extracted authentic ROM voice sample (0x043025, 14000 Hz): {len(audio_bytes)} WAV bytes")

    assets = [
        (1, 128, 11, 0, nintendo_license_bytes),               # ASSET_NINTENDO_LICENSE
        (2, 256, num_legal_rows, start_y_legal, nba_legal_bytes), # ASSET_NBA_LEGAL_NOTICE (flags = start_y)
        (3, w4, h4, ea_flags, ea_packed[0]),                  # ASSET_EA_LOGO_STAGE1
        (4, w4, h4, ea_flags, ea_packed[1]),                  # ASSET_EA_LOGO_STAGE2
        (5, w4, h4, ea_flags, ea_packed[2]),                  # ASSET_EA_LOGO_STAGE3
        (6, w4, h4, ea_flags, ea_packed[3]),                  # ASSET_EA_LOGO_STAGE4
    ]

    if len(audio_bytes) > 0:
        assets.append((7, 0, 0, 0, audio_bytes))              # ASSET_AUDIO_EA_INTRO

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
    parser.add_argument("--rom", required=True, help="Path to SNES ROM (.sfc/.smc)")
    parser.add_argument("--output", default=r"build\nba95_assets.pak", help="Output asset package path")
    args = parser.parse_args()

    create_asset_pack(args.rom, args.output)
