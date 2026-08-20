import os
import sys
import struct
import argparse
import hashlib
from PIL import Image
import numpy as np

import re
from snes65816_decompressor import Snes65816Decompressor

NBA_ROM_EXPECTED_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"


def load_verified_rom(path):
    with open(path, "rb") as rom_file:
        raw = rom_file.read()
    header_size = 512 if len(raw) % 1024 == 512 else 0
    normalized = raw[header_size:]
    actual = hashlib.sha256(normalized).hexdigest()
    if actual != NBA_ROM_EXPECTED_SHA256:
        raise RuntimeError(
            f"ROM SHA-256 mismatch: expected {NBA_ROM_EXPECTED_SHA256}, got {actual}"
        )
    return normalized

def decode_4bpp_tile(tile_bytes):
    pixels = np.zeros((8, 8), dtype=np.uint8)
    for y in range(8):
        b0 = tile_bytes[y * 2]
        b1 = tile_bytes[y * 2 + 1]
        b2 = tile_bytes[16 + y * 2]
        b3 = tile_bytes[16 + y * 2 + 1]
        for x in range(8):
            bit = 7 - x
            p = ((b0 >> bit) & 1) | (((b1 >> bit) & 1) << 1) | (((b2 >> bit) & 1) << 2) | (((b3 >> bit) & 1) << 3)
            pixels[y, x] = p
    return pixels

def bgr555_to_argb(w):
    r = (w & 0x1F) << 3
    g = ((w >> 5) & 0x1F) << 3
    b = ((w >> 10) & 0x1F) << 3
    return 0xFF000000 | ((r | (r >> 5)) << 16) | ((g | (g >> 5)) << 8) | (b | (b >> 5))


def create_asset_pack(rom_path, output_path):
    print(f"[ASSET EXTRACTOR] Extracting assets from ROM: {rom_path}")
    print(f"[ASSET EXTRACTOR] Output asset pack: {output_path}")
    rom_data = load_verified_rom(rom_path)

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
    intro_capture_dir = os.environ.get("NBA95_INTRO_CAPTURE_DIR") or os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", ".analysis",
        "intro_capture")
    legal_path = os.path.join(intro_capture_dir, "legal.png")
    if not os.path.exists(legal_path):
        raise RuntimeError(
            f"Missing intro capture: {legal_path}. Run "
            "tools/mesen_intro_capture.lua with the ROM first."
        )

    nba_legal_bytes = bytearray()
    num_legal_rows = 151
    start_y_legal = 35

    if legal_path:
        im = Image.open(legal_path).convert('L')
        if im.size != (256, 224):
            raise RuntimeError(
                f"Intro capture must be a native 256x224 Mesen frame: {legal_path}"
            )
        snes_frame = (np.array(im) > 100).astype(np.uint8)

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
        os.path.join(intro_capture_dir, f"ea_stage_{i}.png")
        for i in range(1, 5)
    ]
    missing_stages = [path for path in ea_candidates if not os.path.exists(path)]
    if missing_stages:
        raise RuntimeError(
            "Missing EA intro captures: " + ", ".join(missing_stages) +
            ". Run tools/mesen_intro_capture.lua with the ROM first."
        )

    w4, h4 = 0, 0
    ea_flags = 0
    ea_packed = []

    if all(os.path.exists(p) for p in ea_candidates):
        snes_frames = []
        for p in ea_candidates:
            im = Image.open(p).convert('RGB')
            if im.size != (256, 224):
                raise RuntimeError(
                    f"Intro capture must be a native 256x224 Mesen frame: {p}"
                )
            snes_frames.append(np.array(im))

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
                return pcm, pos
        return pcm, pos

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

    # 7-11. Audio: Authentic EA Voice Clips & Synchronized Intro Track
    audio_intro_bytes = bytearray()
    audio_e_bytes = bytearray()
    audio_a_bytes = bytearray()
    audio_sports_bytes = bytearray()
    audio_game_bytes = bytearray()

    if rom_data:
        # Extract authentic 4-part voice clips from ROM:
        # 1. "E" sample: ROM 0x12D9C5 (3492 bytes BRR, 6208 PCM samples, 0.39s)
        # 2. "A" sample: ROM 0x12801C (5580 bytes BRR, 9920 PCM samples, 0.62s)
        # 3. "Sports" sample: ROM 0x11E03D (5904 bytes BRR, 10496 PCM samples, 0.66s)
        # 4. "It's in the game" sample: ROM 0x11249B (9036 bytes BRR, 16064 PCM samples, 1.00s)
        e_pcm = []
        a_pcm = []
        sports_pcm = []
        game_pcm = []

        if len(rom_data) >= 0x12D9C5 + 3492:
            e_pcm, _ = decode_brr_to_pcm(rom_data[0x12D9C5:0x12D9C5 + 3492])
            if len(e_pcm) > 0:
                audio_e_bytes = make_wav_bytes(e_pcm, sample_rate=16000)
                print(f"[ASSET EXTRACTOR] Extracted ROM 'E' voice sample (0x12D9C5): {len(audio_e_bytes)} WAV bytes")

        if len(rom_data) >= 0x12801C + 5580:
            a_pcm, _ = decode_brr_to_pcm(rom_data[0x12801C:0x12801C + 5580])
            if len(a_pcm) > 0:
                audio_a_bytes = make_wav_bytes(a_pcm, sample_rate=16000)
                print(f"[ASSET EXTRACTOR] Extracted ROM 'A' voice sample (0x12801C): {len(audio_a_bytes)} WAV bytes")

        if len(rom_data) >= 0x11E03D + 5904:
            sports_pcm, _ = decode_brr_to_pcm(rom_data[0x11E03D:0x11E03D + 5904])
            if len(sports_pcm) > 0:
                audio_sports_bytes = make_wav_bytes(sports_pcm, sample_rate=16000)
                print(f"[ASSET EXTRACTOR] Extracted ROM 'Sports' voice sample (0x11E03D): {len(audio_sports_bytes)} WAV bytes")

        if len(rom_data) >= 0x11249B + 9036:
            game_pcm, _ = decode_brr_to_pcm(rom_data[0x11249B:0x11249B + 9036])
            if len(game_pcm) > 0:
                audio_game_bytes = make_wav_bytes(game_pcm, sample_rate=16000)
                print(f"[ASSET EXTRACTOR] Extracted ROM 'It's in the game' voice sample (0x11249B): {len(audio_game_bytes)} WAV bytes")

        # If no external composite intro was provided, build the complete 4-part synchronized slogan
        if len(e_pcm) > 0 and len(a_pcm) > 0 and len(sports_pcm) > 0 and len(game_pcm) > 0:
            rate = 16000
            total_len = int(rate * 5.05) # Authentic 5.05s SNES intro sequence
            composite = np.zeros(total_len, dtype=np.int16)

            # Stage 1: E at Frame 0 (t = 0.000s)
            e_start = int(rate * 0.00)
            e_len = min(len(e_pcm), total_len - e_start)
            composite[e_start:e_start+e_len] = np.array(e_pcm[:e_len], dtype=np.int16)

            # Stage 2: A at Frame 32 (t = 0.533s)
            a_start = int(rate * (32.0 / 60.0))
            a_len = min(len(a_pcm), total_len - a_start)
            composite[a_start:a_start+a_len] = np.array(a_pcm[:a_len], dtype=np.int16)

            # Stage 3: Sports at Frame 63 (t = 1.050s)
            s_start = int(rate * (63.0 / 60.0))
            s_len = min(len(sports_pcm), total_len - s_start)
            composite[s_start:s_start+s_len] = np.array(sports_pcm[:s_len], dtype=np.int16)

            # Stage 4: "It's in the game" at Frame 123 (t = 2.050s)
            g_start = int(rate * (123.0 / 60.0))
            g_len = min(len(game_pcm), total_len - g_start)
            composite[g_start:g_start+g_len] = np.array(game_pcm[:g_len], dtype=np.int16)

            audio_intro_bytes = make_wav_bytes(composite.tolist(), sample_rate=16000)
            print(f"[ASSET EXTRACTOR] Built complete 4-part intro slogan track: {len(audio_intro_bytes)} WAV bytes (5.05s)")

    # ------------------------------------------------------------------
    # $80:E01E title hardware state. Unlike the retired captured-video path,
    # these assets contain no rendered frames and no mixed audio. Mesen is used
    # as a hardware-state dumper: the port renders the ROM's planar tiles and
    # runs the ROM's SPC700 driver/BRR bank itself.
    title_capture_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                     "..", ".analysis", "title_capture")

    def read_required(name, expected_size=None):
        path = os.path.join(title_capture_dir, name)
        if not os.path.exists(path):
            raise RuntimeError(f"Missing title hardware capture: {path}. Run "
                               "tools/mesen_title_capture.lua with the ROM first.")
        data = open(path, "rb").read()
        if expected_size is not None and len(data) != expected_size:
            raise RuntimeError(f"Invalid {name}: expected {expected_size} bytes, got {len(data)}")
        return data

    def parse_text_events(name, hex_fields=()):
        events = []
        for raw in read_required(name).decode("ascii").splitlines():
            if not raw or raw.startswith("#"):
                continue
            fields = raw.split()
            events.append(tuple(int(v, 16 if i in hex_fields else 10)
                                for i, v in enumerate(fields)))
        return events

    title_vram_bytes = read_required("initial_vram.bin", 0x10000)
    title_cgram_bytes = read_required("initial_cgram.bin", 0x200)
    title_spc_ram_bytes = read_required("initial_spc_ram.bin", 0x10000)
    title_spc_dsp_bytes = read_required("initial_spc_dsp.bin", 0x80)

    state_text = read_required("initial_state.txt").decode("ascii")
    def state_int(key):
        match = re.search(r"^" + re.escape(key) + r"=(-?\d+)$", state_text, re.MULTILINE)
        if not match:
            raise RuntimeError(f"Title capture is missing state key {key}")
        return int(match.group(1))
    title_spc_state_bytes = b"NBTSPC1\0" + struct.pack(
        "<IH6B", 1, state_int("spc.pc"), state_int("spc.a"),
        state_int("spc.x"), state_int("spc.y"), state_int("spc.sp"),
        state_int("spc.ps"), 0)

    apu_events = parse_text_events("apu_ports.txt", (2,))
    cue_events = parse_text_events("cues.txt")
    frame_rows = parse_text_events("ppu_frames.txt")
    vram_events = parse_text_events("vram_writes.txt", (1, 2))
    cgram_events = parse_text_events("cgram_writes.txt", (1, 2))
    if not frame_rows:
        raise RuntimeError("Title PPU trace is empty")
    title_frame_count = max(row[0] for row in frame_rows) + 1

    def pack_timed_events(magic, events):
        packed = bytearray(struct.pack("<8sIII", magic, 1, title_frame_count, len(events)))
        for frame, field, value in events:
            if frame < 0 or frame >= title_frame_count or field < 0 or field > 255:
                raise RuntimeError(f"Invalid event in {magic!r}: {(frame, field, value)}")
            packed.extend(struct.pack("<HBB", frame, field, value))
        return bytes(packed)

    title_apu_trace_bytes = pack_timed_events(b"NBTAPU1\0", apu_events)
    title_cue_trace_bytes = pack_timed_events(
        b"NBTCUE1\0", [(frame, value, 0) for frame, value in cue_events])

    # The endFrame screenshot is the frame Mesen has just presented, while the
    # VRAM/CGRAM reads in that callback observe memory prepared for the next
    # presentation.  Delay those deltas by one frame so construction DMAs do
    # not appear as transient strips beneath the N/B/A/LIVE artwork.
    vram_by_frame = {}
    for frame, address, value in vram_events:
        if frame + 1 < title_frame_count:
            vram_by_frame.setdefault(frame + 1, []).append((address, value))
    cgram_by_frame = {}
    for frame, address, value in cgram_events:
        if frame + 1 < title_frame_count:
            cgram_by_frame.setdefault(frame + 1, []).append((address, value))
    rows_by_frame = {row[0]: row for row in frame_rows}

    title_ppu_trace = bytearray(struct.pack("<8sII", b"NBTPPU1\0", 1, title_frame_count))
    last_row = frame_rows[0]
    for frame in range(title_frame_count):
        row = rows_by_frame.get(frame, last_row)
        last_row = row
        if len(row) != 13:
            raise RuntimeError(f"Invalid PPU state row: {row}")
        vw = vram_by_frame.get(frame, [])
        cw = cgram_by_frame.get(frame, [])
        title_ppu_trace.extend(struct.pack("<BB10H2H", row[1], row[2],
                                           *row[3:13], len(vw), len(cw)))
        for address, value in vw:
            title_ppu_trace.extend(struct.pack("<HB", address, value))
        for address, value in cw:
            title_ppu_trace.extend(struct.pack("<HB", address, value))

    expected_primary_cues = [(1, 1), (2, 2), (3, 3), (4, 4), (5, 5), (6, 6)]
    actual_primary_cues = [(i + 1, event[1]) for i, event in enumerate(cue_events[:6])]
    if actual_primary_cues != expected_primary_cues:
        raise RuntimeError(f"Unexpected title cue order: {actual_primary_cues}")
    print(f"[ASSET EXTRACTOR] Packed ROM title hardware state: {title_frame_count} frames, "
          f"{len(apu_events)} APU writes, {len(cue_events)} cues, "
          f"{len(vram_events)} VRAM and {len(cgram_events)} CGRAM changes")



    # ------------------------------------------------------------------
    # Game Setup screen ($80:A2BF cluster) graphics.
    #
    # The screen is SNES BG Mode 1 built from three layers whose tile data
    # the ROM produces by running its own decompressor ($80:C62B) and then
    # DMAing the result into VRAM. The settled image supplies the complete tile
    # and map data; the entrance trace below documents the intervening DMAs.
    setup_vram_bytes = b""
    setup_cgram_bytes = b""
    capture_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                               "..", ".analysis", "setup_capture")
    vram_path = os.path.join(capture_dir, "vram.bin")
    cgram_path = os.path.join(capture_dir, "cgram.bin")
    if os.path.exists(vram_path) and os.path.exists(cgram_path):
        setup_vram_bytes = open(vram_path, "rb").read()
        setup_cgram_bytes = open(cgram_path, "rb").read()
        print(f"[ASSET EXTRACTOR] Game Setup VRAM {len(setup_vram_bytes)} bytes, "
              f"CGRAM {len(setup_cgram_bytes)} bytes")

        # Cross-check the capture against data decompressed from the ROM.
        try:
            emu = Snes65816Decompressor(rom_data)
            emu.decompress(0xAE, 0xC446, 0x7F, 0x2000)
            blob = bytes(emu.wram[0x12000:0x12000 + 0x3C0])
            if blob and blob in setup_vram_bytes[0x2000:0x6000]:
                print("[ASSET EXTRACTOR] Verified BG2 chr against ROM decompressor "
                      "($AE:C446 via $80:C62B)")
            else:
                print("[ASSET EXTRACTOR] Warning: BG2 chr did not match the ROM decompressor")
        except Exception as ex:
            print(f"[ASSET EXTRACTOR] Warning: setup-screen ROM cross-check failed: {ex}")
    else:
        print("[ASSET EXTRACTOR] Game Setup entrance capture missing; run:")
        print("    Mesen.exe <rom> tools/mesen_setup_transition_capture.lua")

    # ------------------------------------------------------------------
    # Title -> Game Setup audio handoff. The snapshot is taken on ROM frame
    # 1637 (the last visible title-fade frame). Every subsequent mirrored
    # $2140-$2143 write is stamped with `spc.cycle`, so the port can run the
    # original SPC700/BRR driver without a captured or mixed WAV.
    setup_transition_dir = os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", ".analysis",
        "setup_transition")

    def read_setup_transition(name, expected_size=None):
        path = os.path.join(setup_transition_dir, name)
        if not os.path.exists(path):
            raise RuntimeError(f"Missing Game Setup transition capture: {path}. Run "
                               "tools/mesen_setup_transition_capture.lua with the ROM first.")
        data = open(path, "rb").read()
        if expected_size is not None and len(data) != expected_size:
            raise RuntimeError(f"Invalid {name}: expected {expected_size} bytes, got {len(data)}")
        return data

    setup_spc_ram_bytes = read_setup_transition("spc_ram.bin", 0x10000)
    setup_spc_dsp_bytes = read_setup_transition("spc_dsp.bin", 0x80)
    setup_state_text = read_setup_transition("spc_state.txt").decode("ascii")

    setup_vram_events = {}
    for raw in read_setup_transition("entrance_vram_writes.txt").decode("ascii").splitlines():
        if not raw or raw.startswith("#"):
            continue
        transition_frame, address, value = raw.split()
        frame = int(transition_frame) - 106
        if not 1 <= frame <= 60:
            raise RuntimeError(f"Invalid Setup VRAM trace frame: {transition_frame}")
        setup_vram_events.setdefault(frame, []).append((int(address, 16), int(value, 16)))

    setup_cgram_events = {}
    for raw in read_setup_transition("entrance_cgram_writes.txt").decode("ascii").splitlines():
        if not raw or raw.startswith("#"):
            continue
        transition_frame, address, value = raw.split()
        frame = int(transition_frame) - 106
        if not 1 <= frame <= 60:
            raise RuntimeError(f"Invalid Setup CGRAM trace frame: {transition_frame}")
        setup_cgram_events.setdefault(frame, []).append((int(address, 16), int(value, 16)))

    setup_ppu_trace = bytearray(struct.pack("<8sII", b"NBSPPU1\0", 1, 61))
    for frame in range(61):
        vw = setup_vram_events.get(frame, [])
        cw = setup_cgram_events.get(frame, [])
        setup_ppu_trace.extend(struct.pack("<HH", len(vw), len(cw)))
        for address, value in vw:
            setup_ppu_trace.extend(struct.pack("<HB", address, value))
        for address, value in cw:
            setup_ppu_trace.extend(struct.pack("<HB", address, value))
    print(f"[ASSET EXTRACTOR] Packed Game Setup entrance trace: "
          f"{sum(map(len, setup_vram_events.values()))} VRAM and "
          f"{sum(map(len, setup_cgram_events.values()))} CGRAM changes")

    def setup_state_int(key):
        match = re.search(r"(?:^|\s)" + re.escape(key) + r"=(-?\d+)(?:\s|$)",
                          setup_state_text)
        if not match:
            raise RuntimeError(f"Game Setup capture is missing state key {key}")
        return int(match.group(1))

    setup_frames = setup_state_int("frames")
    setup_spc_state_bytes = b"NBTSSPC1" + struct.pack(
        "<IH6B", 1, setup_state_int("pc"), setup_state_int("a"),
        setup_state_int("x"), setup_state_int("y"), setup_state_int("sp"),
        setup_state_int("ps"), 0)

    setup_apu_events = []
    previous_cycle = -1
    for raw in read_setup_transition("apu_cycle_trace.txt").decode("ascii").splitlines():
        if not raw or raw.startswith("#"):
            continue
        cycle_text, port_text, value_text = raw.split()
        # Mesen exposes `spc.cycle` in 2.048 MHz half-cycle units. The C core
        # uses the SPC700's 1.024 MHz cycle domain, so normalize at pack time.
        event = (int(cycle_text) // 2, int(port_text), int(value_text, 16))
        if event[0] < previous_cycle or not 0 <= event[1] <= 3 or not 0 <= event[2] <= 255:
            raise RuntimeError(f"Invalid Game Setup APU event: {event}")
        previous_cycle = event[0]
        setup_apu_events.append(event)

    max_setup_cycles = setup_frames * 1024000 // 60
    if not setup_apu_events or setup_apu_events[-1][0] > max_setup_cycles:
        raise RuntimeError("Game Setup APU trace extends beyond its declared duration")

    setup_apu_trace = bytearray(struct.pack(
        "<8sIII", b"NBTSAPU1", 1, setup_frames, len(setup_apu_events)))
    for cycle, port, value in setup_apu_events:
        setup_apu_trace.extend(struct.pack("<IBB", cycle, port, value))
    print(f"[ASSET EXTRACTOR] Packed Game Setup SPC state: {setup_frames} frames, "
          f"{len(setup_apu_events)} cycle-timed APU writes")

    setup_dsp_events = []
    previous_cycle = -1
    for raw in read_setup_transition("dsp_cycle_trace.txt").decode("ascii").splitlines():
        if not raw or raw.startswith("#"):
            continue
        cycle_text, register_text, value_text = raw.split()
        event = (int(cycle_text) // 2, int(register_text, 16), int(value_text, 16))
        if event[0] < previous_cycle or not 0 <= event[1] < 0x80 or not 0 <= event[2] <= 255:
            raise RuntimeError(f"Invalid Game Setup DSP event: {event}")
        previous_cycle = event[0]
        setup_dsp_events.append(event)
    if not setup_dsp_events or setup_dsp_events[-1][0] > max_setup_cycles:
        raise RuntimeError("Game Setup DSP trace extends beyond its declared duration")
    setup_dsp_trace = bytearray(struct.pack(
        "<8sIII", b"NBTSDSP1", 1, setup_frames, len(setup_dsp_events)))
    for cycle, register, value in setup_dsp_events:
        setup_dsp_trace.extend(struct.pack("<IBB", cycle, register, value))
    print(f"[ASSET EXTRACTOR] Packed Game Setup S-DSP program: "
          f"{len(setup_dsp_events)} cycle-timed register writes")

    assets = [
        (1, 128, 11, 0, nintendo_license_bytes),               # ASSET_NINTENDO_LICENSE
        (2, 256, num_legal_rows, start_y_legal, nba_legal_bytes), # ASSET_NBA_LEGAL_NOTICE (flags = start_y)
        (3, w4, h4, ea_flags, ea_packed[0]),                  # ASSET_EA_LOGO_STAGE1
        (4, w4, h4, ea_flags, ea_packed[1]),                  # ASSET_EA_LOGO_STAGE2
        (5, w4, h4, ea_flags, ea_packed[2]),                  # ASSET_EA_LOGO_STAGE3
        (6, w4, h4, ea_flags, ea_packed[3]),                  # ASSET_EA_LOGO_STAGE4
    ]

    if len(audio_intro_bytes) > 0:
        assets.append((7, 0, 0, 0, audio_intro_bytes))         # ASSET_AUDIO_EA_INTRO
    if len(audio_e_bytes) > 0:
        assets.append((8, 0, 0, 0, audio_e_bytes))             # ASSET_AUDIO_EA_E
    if len(audio_a_bytes) > 0:
        assets.append((9, 0, 0, 0, audio_a_bytes))             # ASSET_AUDIO_EA_A
    if len(audio_sports_bytes) > 0:
        assets.append((10, 0, 0, 0, audio_sports_bytes))       # ASSET_AUDIO_EA_SPORTS
    if len(audio_game_bytes) > 0:
        assets.append((11, 0, 0, 0, audio_game_bytes))         # ASSET_AUDIO_EA_GAME
    assets.extend([
        (80, 0, 0, 0, title_vram_bytes),
        (81, 0, 0, 0, title_cgram_bytes),
        (82, 0, 0, 0, bytes(title_ppu_trace)),
        (83, 0, 0, 0, title_spc_ram_bytes),
        (84, 0, 0, 0, title_spc_dsp_bytes),
        (85, 0, 0, 0, title_spc_state_bytes),
        (86, 0, 0, 0, title_apu_trace_bytes),
        (87, 0, 0, 0, title_cue_trace_bytes),
        (88, 0, 0, 0, setup_spc_ram_bytes),
        (89, 0, 0, 0, setup_spc_dsp_bytes),
        (90, 0, 0, 0, setup_spc_state_bytes),
        (91, 0, 0, 0, bytes(setup_apu_trace)),
        (92, 0, 0, 0, bytes(setup_ppu_trace)),
        (93, 0, 0, 0, bytes(setup_dsp_trace)),
    ])

    # Extract all other audio samples from ROM into asset pack for debugger
    rom_sample_offsets = [
        0x043025, 0x0DA71E, 0x0DBA2C, 0x0DF19E, 0x0E001C, 0x0E4A6F, 0x0E801C, 0x0EC8B6,
        0x0F001C, 0x0F482F, 0x10801C, 0x10D1E7, 0x114803, 0x11F769,
        0x124C14, 0x129604, 0x12A820, 0x12B964, 0x13001C, 0x1318E0, 0x1324F0,
        0x133394, 0x1350E8, 0x135BB4, 0x13BCE4, 0x13F850, 0x14001C, 0x1439E0, 0x145663,
        0x145FD9, 0x147C01, 0x149BA6, 0x14AD51, 0x14BEE4, 0x14E8BA, 0x14F0B6, 0x15001C,
        0x1507F4, 0x15102F, 0x151792, 0x151F4F, 0x1526BB, 0x152E03, 0x153C78, 0x15517D,
        0x155811, 0x159E18, 0x15D7E5, 0x16001C, 0x160F23, 0x161CBF, 0x166995
    ]

    if len(setup_vram_bytes) > 0:
        assets.append((16, 0, 0, 0, setup_vram_bytes))         # ASSET_SETUP_VRAM
    if len(setup_cgram_bytes) > 0:
        assets.append((17, 0, 0, 0, setup_cgram_bytes))        # ASSET_SETUP_CGRAM

    if os.path.exists(rom_path):
        extra_audio_id = 18
        for off in rom_sample_offsets:
            if off < len(rom_data):
                pcm, _ = decode_brr_to_pcm(rom_data[off:])
                if len(pcm) > 0:
                    wav_bytes = make_wav_bytes(pcm, sample_rate=16000)
                    assets.append((extra_audio_id, 0, 0, off, wav_bytes))
                    extra_audio_id += 1

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
