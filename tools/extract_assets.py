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


def decode_team_logo(vram, cgram, oam):
    """Decode the variable-length, palette-4 right-team OBJ logo."""
    width, height = 48, 56
    origin_x, origin_y = 182, 62
    pixels = [0] * (width * height)
    # Lower OAM indexes win ties, matching the SNES object priority ordering.
    for index in range(127, -1, -1):
        high = (oam[512 + index // 4] >> ((index & 3) * 2)) & 3
        x = oam[index * 4] | ((high & 1) << 8)
        if x >= 256:
            x -= 512
        y = oam[index * 4 + 1]
        tile = oam[index * 4 + 2]
        attr = oam[index * 4 + 3]
        size = 16 if high & 2 else 8
        palette = (attr >> 1) & 7
        # Logo piece counts vary by team (6..13 in the verified captures).
        # The following 15 objects are the palette-2 gold plate, not logo art.
        if palette != 4:
            continue
        tile += 256 if attr & 1 else 0
        for py in range(size):
            sy = size - 1 - py if attr & 0x80 else py
            for px in range(size):
                sx = size - 1 - px if attr & 0x40 else px
                subtile = tile + (sx >> 3) + (sy >> 3) * 16
                offset = 0xC000 + subtile * 32
                if offset + 32 > len(vram):
                    continue
                color = int(decode_4bpp_tile(vram[offset:offset + 32])[sy & 7, sx & 7])
                dx, dy = x + px - origin_x, y + py - origin_y
                if color and 0 <= dx < width and 0 <= dy < height:
                    palette_offset = (128 + palette * 16 + color) * 2
                    word = cgram[palette_offset] | (cgram[palette_offset + 1] << 8)
                    pixels[dy * width + dx] = bgr555_to_argb(word)
    return b"".join(struct.pack("<I", pixel) for pixel in pixels)


def decode_bg_layer(vram, cgram, map_base, chr_base, bits_per_pixel,
                    wide, tall, hscroll, vscroll):
    """Render one raw SNES background layer to ARGB pixels.

    The player-introduction court is BG2 in Mode 1.  This deliberately consumes
    VRAM/CGRAM, not a Mesen screenshot; the PNG capture remains visual evidence
    only.  The parameters are the live PPU state recorded during the lineup.
    """
    pixels = []
    map_width, map_height = (512 if wide else 256), (512 if tall else 256)
    bytes_per_tile = bits_per_pixel * 8
    for y in range(224):
        for x in range(256):
            px = (x + hscroll) % map_width
            py = (y + vscroll + 1) % map_height
            tile_x, tile_y = px >> 3, py >> 3
            quadrant = (1 if wide and tile_x >= 32 else 0)
            if tall and tile_y >= 32:
                quadrant += 2 if wide else 1
            entry_offset = (map_base + quadrant * 0x800 +
                            ((tile_y & 31) * 32 + (tile_x & 31)) * 2) & 0xffff
            entry = vram[entry_offset] | (vram[(entry_offset + 1) & 0xffff] << 8)
            sx = 7 - (px & 7) if entry & 0x4000 else px & 7
            sy = 7 - (py & 7) if entry & 0x8000 else py & 7
            tile_offset = (chr_base + (entry & 0x3ff) * bytes_per_tile) & 0xffff
            color = 0
            for plane in range(0, bits_per_pixel, 2):
                low = vram[(tile_offset + sy * 2 + plane * 8) & 0xffff]
                high = vram[(tile_offset + sy * 2 + 1 + plane * 8) & 0xffff]
                bit = 7 - sx
                color |= ((low >> bit) & 1) << plane
                color |= ((high >> bit) & 1) << (plane + 1)
            palette = (entry >> 10) & 7
            palette_index = palette * (1 << bits_per_pixel) + color
            word = cgram[palette_index * 2] | (cgram[palette_index * 2 + 1] << 8)
            pixels.append(bgr555_to_argb(word))
    return b"".join(struct.pack("<I", pixel) for pixel in pixels)


def decode_lineup_portrait(vram, cgram, oam):
    """Decode the ROM-built lineup portrait OBJ group from raw PPU memories."""
    origin_x, origin_y, width, height = 8, 144, 72, 72
    pixels = [0] * (width * height)
    for index in range(127, -1, -1):
        high = (oam[512 + index // 4] >> ((index & 3) * 2)) & 3
        x = oam[index * 4] | ((high & 1) << 8)
        if x >= 256:
            x -= 512
        y, tile, attr = oam[index * 4 + 1:index * 4 + 4]
        size = 16 if high & 2 else 8
        palette = (attr >> 1) & 7
        # $87:BE92's portrait occupies the palette-zero object group at the
        # lower left.  Exclude the yellow rule/text objects beginning at x=80.
        if palette != 0 or x >= 80 or y < 140:
            continue
        tile += 256 if attr & 1 else 0
        for py in range(size):
            sy = size - 1 - py if attr & 0x80 else py
            for px in range(size):
                sx = size - 1 - px if attr & 0x40 else px
                subtile = tile + (sx >> 3) + (sy >> 3) * 16
                offset = 0xc000 + subtile * 32
                if offset + 32 > len(vram):
                    continue
                color = int(decode_4bpp_tile(vram[offset:offset + 32])[sy & 7, sx & 7])
                dx, dy = x + px - origin_x, y + py - origin_y
                if color and 0 <= dx < width and 0 <= dy < height:
                    palette_offset = (128 + palette * 16 + color) * 2
                    word = cgram[palette_offset] | (cgram[palette_offset + 1] << 8)
                    pixels[dy * width + dx] = bgr555_to_argb(word)
    return b"".join(struct.pack("<I", pixel) for pixel in pixels)


def build_player_introduction_assets(court_capture_dir, away_portrait_dir,
                                     home_portrait_dir):
    """Build court/portrait assets from raw, ROM-produced PPU state.

    The portrait harness verifies the selected visitor or home team at
    $87:BE92 before it saves five raw PPU states. Team/roster/side keys prevent
    the C scene from ever displaying a portrait under the wrong player name or
    uniform variant.
    """
    def payload(directory, frame, suffix, expected):
        path = os.path.join(directory, f"frame_{frame:04d}_{suffix}.bin")
        if not os.path.exists(path):
            raise RuntimeError(f"Missing Player Introduction PPU asset: {path}")
        data = open(path, "rb").read()
        if len(data) != expected:
            raise RuntimeError(f"Invalid Player Introduction PPU asset {path}: "
                               f"expected {expected} bytes, got {len(data)}")
        return data

    court_vram = payload(court_capture_dir, 2550, "vram", 0x10000)
    court_cgram = payload(court_capture_dir, 2550, "cgram", 0x200)
    # Mesen reports tilemapAddress in SNES words; the raw VRAM dump is bytes.
    court = decode_bg_layer(court_vram, court_cgram, 0x1000, 0x4000,
                            4, True, False, 6, 6)
    cards = [(side, team, slot) for side in range(2)
             for team in range(29) for slot in range(5)]
    portrait_pack = bytearray(struct.pack("<8sIIII", b"NBINTRO1", 2,
                                           len(cards), 72, 72))
    portrait_hashes = [set(), set()]
    for side, team, slot in cards:
        capture_dir = away_portrait_dir if side == 0 else home_portrait_dir
        directory = os.path.join(capture_dir, f"team_{team:02d}")
        def portrait_payload(suffix, expected):
            path = os.path.join(directory, f"slot_{slot}_{suffix}.bin")
            if not os.path.exists(path):
                raise RuntimeError(f"Missing verified lineup portrait asset: {path}. "
                                   "Run mesen_player_intro_portraits.lua first.")
            data = open(path, "rb").read()
            if len(data) != expected:
                raise RuntimeError(f"Invalid lineup portrait PPU asset {path}: "
                                   f"expected {expected} bytes, got {len(data)}")
            return data
        portrait = decode_lineup_portrait(
            portrait_payload("vram", 0x10000),
            portrait_payload("cgram", 0x200),
            portrait_payload("oam", 0x220))
        if not any(portrait[index + 3] for index in range(0, len(portrait), 4)):
            raise RuntimeError(f"Lineup portrait team={team} slot={slot} is empty")
        portrait_hashes[side].add(hashlib.sha256(portrait).digest())
        portrait_pack.extend(struct.pack("<BBHI", team, slot, side, len(portrait)))
        portrait_pack.extend(portrait)
    if any(len(side_hashes) != 145 for side_hashes in portrait_hashes):
        raise RuntimeError("Verified lineup portrait side contains duplicate "
                           "decoded images; refusing a mislabeled asset pack")
    print("[ASSET EXTRACTOR] Packed 290 ROM-built lineup portraits "
          "(away/home x 29 teams x 5 starters)")
    return court, bytes(portrait_pack)


def build_player_intro_rating_balls(capture_dir):
    """Decode $83:F901's six 16x16 basketball OBJ poses from raw PPU state."""
    def read_capture(suffix, expected):
        path = os.path.join(capture_dir, f"frame_2300_{suffix}.bin")
        data = open(path, "rb").read()
        if len(data) != expected:
            raise RuntimeError(f"Invalid rating-ball PPU asset {path}")
        return data

    vram = read_capture("vram", 0x10000)
    cgram = read_capture("cgram", 0x200)
    poses = (0x60, 0x0c, 0x0e, 0x4a, 0x4c, 0x4e)
    pixels = bytearray()
    for tile in poses:
        for y in range(16):
            for x in range(16):
                subtile = tile + (x >> 3) + (y >> 3) * 16
                offset = 0xc000 + subtile * 32
                color = int(decode_4bpp_tile(vram[offset:offset + 32])[y & 7, x & 7])
                if color == 0:
                    pixels.extend(struct.pack("<I", 0))
                else:
                    palette_offset = (128 + color) * 2
                    word = cgram[palette_offset] | (cgram[palette_offset + 1] << 8)
                    pixels.extend(struct.pack("<I", bgr555_to_argb(word)))
    return bytes(pixels)


def lorom_offset(address):
    """Convert a verified 24-bit LoROM address to its headerless file offset."""
    return ((address >> 16) & 0x7f) * 0x8000 + (address & 0x7fff)


def build_player_roster_asset(rom_data):
    """Pack the ROM's 29 teams x 12 player records for the F9 Player Lab.

    $84:E640 contains 4-byte entries (reserved, 24-bit roster pointer). Each
    roster starts with twelve 16-bit record offsets. Ghidra/Mesen prove that
    +$06 selects the player palette variant, +$07 selects a five-direction
    head resource family, +$36/+$37 form the appearance key, and names begin
    at +$4A.
    """
    payload = bytearray(struct.pack("<8sIIII", b"NBPROST1", 1, 29, 12, 64))
    table = lorom_offset(0x84E640)
    for team in range(29):
        entry = table + team * 4
        pointer = (rom_data[entry + 1] | (rom_data[entry + 2] << 8) |
                   (rom_data[entry + 3] << 16))
        base = lorom_offset(pointer)
        offsets = [struct.unpack_from("<H", rom_data, base + i * 2)[0]
                   for i in range(12)]
        for player, relative in enumerate(offsets):
            start = base + relative
            record = rom_data[start:start + 0x80]
            if len(record) < 0x4b:
                raise RuntimeError(f"Truncated roster record team={team} player={player}")
            name_end = record.find(b"\0", 0x4a, 0x70)
            if name_end < 0:
                raise RuntimeError(f"Unterminated roster name team={team} player={player}")
            name = record[0x4a:name_end]
            if not name or any(byte < 0x20 or byte > 0x7e for byte in name):
                raise RuntimeError(f"Invalid roster name team={team} player={player}")
            appearance_a, appearance_b = record[0x36], record[0x37]
            fixed_name = name[:31] + b"\0" * (32 - len(name[:31]))
            head_raw = record[7]
            head_style = head_raw & 0x1f if head_raw >= 0x27 else head_raw
            head_base = 0x049c + head_style * 5
            packed = bytearray(64)
            struct.pack_into("<IBBBBBBBB", packed, 0, pointer + relative,
                             record[0], record[1], record[2], record[3],
                             appearance_a, appearance_b,
                             (appearance_a + appearance_b) & 0xff, player)
            struct.pack_into("<BBBBHH", packed, 12, min(record[6], 2),
                             head_raw, head_style, record[8], head_base,
                             head_base + 2)
            packed[32:64] = fixed_name
            payload.extend(packed)
    return bytes(payload)


def build_player_front_pose(rom_data):
    """Assemble the camera-facing default pose from exact ROM OBJ tiles.

    Mesen establishes the OAM layout only. Every art byte below is copied from
    its byte-for-byte matching ROM location; no screenshot or VRAM capture is
    consumed by the asset pack build.
    """
    tile_sources = {
        0x4c: 0x09c5cb, 0x48: 0x0587b1, 0x49: 0x0587d1,
        0x58: 0x058831, 0x59: 0x058851, 0x4a: 0x0587f1,
        0x4b: 0x058811, 0x60: 0x0b5c55, 0x61: 0x0b5c75,
        0x70: 0x0b5d15, 0x71: 0x0b5d35, 0x62: 0x0b5c95,
        0x63: 0x0b5cb5, 0x72: 0x0b5d55, 0x73: 0x0b5d75,
        0x64: 0x0b5cd5, 0x65: 0x0b5cf5,
    }
    vram = bytearray(0x10000)
    # $87:B01D-$B03A accepts selectors 0..38 directly (values >= $27 are
    # normalized with & $1F). Rambis uses 37 and Workman uses 38, so all 39
    # five-direction families must be present in the asset pack.
    head_count = 39 * 5
    source_manifest = bytearray(struct.pack("<8sIII", b"NBPTILE2", 2,
                                             len(tile_sources), head_count))
    for tile, offset in tile_sources.items():
        data = rom_data[offset:offset + 32]
        if len(data) != 32:
            raise RuntimeError(f"Truncated player tile at ROM file offset ${offset:06X}")
        vram[0xc000 + tile * 32:0xc000 + (tile + 1) * 32] = data
        source_manifest.extend(struct.pack("<B3xI32s", tile, offset, data))

    # $89:8000 is the four-byte relative sprite-resource table consumed by
    # $80:B348. $87:B01D-$B03A proves that roster +$07 selects a five-entry
    # family beginning at resource $049C. Entry +2 is the straight-on head.
    resource_table = lorom_offset(0x898000)
    for style in range(39):
        for orientation in range(5):
            resource_id = 0x049c + style * 5 + orientation
            entry = resource_table + resource_id * 4
            relative = struct.unpack_from("<I", rom_data, entry)[0]
            descriptor_address = 0x898000 + relative
            tile_address = descriptor_address + 17
            tile_offset = lorom_offset(tile_address)
            data = rom_data[tile_offset:tile_offset + 32]
            if len(data) != 32:
                raise RuntimeError(
                    f"Truncated head resource ${resource_id:04X} at ${tile_address:06X}")
            source_manifest.extend(struct.pack(
                "<HBBI32s", resource_id, style, orientation,
                tile_address, data))

    # Front-facing Orlando player: Mesen pre-tip OAM 42,43,44,46,47,48,49.
    # x,y,tile,attributes,size; $40 is horizontal flip, palette is OBJ palette 2.
    layout = [(25, 51, 0x4c, 0x64, 8), (20, 57, 0x48, 0x64, 16),
              (21, 73, 0x4a, 0x64, 16), (18, 71, 0x60, 0x64, 16),
              (19, 87, 0x62, 0x64, 16), (29, 103, 0x64, 0x64, 8),
              (21, 103, 0x65, 0x64, 8)]
    layout_asset = bytearray(struct.pack("<8sII", b"NBPPOSE2", 2, len(layout)))
    for entry in layout:
        layout_asset.extend(struct.pack("<hhBBB", *entry))

    # Hardware output of the ROM's palette builder. The complete source tables
    # ($AF:EF00-$AF:F19F) are packed beside the pose for continued mapping work.
    palette_words = [0x2656, 0x7c05, 0x6318, 0x469c, 0x363a, 0x25b6,
                     0x1132, 0x006b, 0x6318, 0x6318, 0x7fff, 0x6318,
                     0x0000, 0x7c05, 0x7fff, 0x5ad6]
    # Keep the hardware palette beside the OAM-like layout so the C port can
    # compose the pose from raw 4bpp assets at runtime. Asset 252 remains a
    # regression oracle only; it is not the Player Lab render source.
    layout_asset.extend(struct.pack("<16H", *palette_words))
    cgram = bytearray(0x200)
    for index, word in enumerate(palette_words):
        struct.pack_into("<H", cgram, (128 + 2 * 16 + index) * 2, word)

    width, height, origin_x, origin_y = 24, 64, 16, 47
    pixels = [0] * (width * height)
    for x, y, tile, attr, size in reversed(layout):
        palette = (attr >> 1) & 7
        for py in range(size):
            sy = size - 1 - py if attr & 0x80 else py
            for px in range(size):
                sx = size - 1 - px if attr & 0x40 else px
                subtile = tile + (sx >> 3) + (sy >> 3) * 16
                offset = 0xc000 + subtile * 32
                color = int(decode_4bpp_tile(vram[offset:offset + 32])
                            [sy & 7, sx & 7])
                dx, dy = x + px - origin_x, y + py - origin_y
                if color and 0 <= dx < width and 0 <= dy < height:
                    palette_offset = (128 + palette * 16 + color) * 2
                    word = cgram[palette_offset] | (cgram[palette_offset + 1] << 8)
                    pixels[dy * width + dx] = bgr555_to_argb(word)
    if not any(pixel >> 24 for pixel in pixels):
        raise RuntimeError("Gameplay default pose decoded as fully transparent")
    pose = b"".join(struct.pack("<I", pixel) for pixel in pixels)
    # $85:8DB0 expands one 16-color team palette and copies it three times.
    # It then replaces colors 3..7 in variants 1 and 2 from $AF:F022/F042.
    # $87:AFD4-$AFF4 maps roster +$06 (clamped to 2) to those variants.
    decompressed = []
    for bank, address in ((0xAB, 0xFDE2), (0xAE, 0xDB76)):
        emu = Snes65816Decompressor(rom_data)
        emu.decompress(bank, address, 0x7F, 0x0000)
        decompressed.append(bytes(emu.wram[0x10000:0x10000 + 29 * 32]))
    overlays = [None,
                rom_data[lorom_offset(0xAFF022):lorom_offset(0xAFF022) + 10],
                rom_data[lorom_offset(0xAFF042):lorom_offset(0xAFF042) + 10]]
    palette_asset = bytearray(struct.pack(
        "<8sIIIIIII", b"NBPALET2", 2, 29, 2, 3,
        0xABFDE2, 0xAEDB76, 0xAFF01C))
    for team in range(29):
        for side in range(2):
            base = decompressed[side][team * 32:(team + 1) * 32]
            for variant in range(3):
                colors = bytearray(base)
                if variant:
                    colors[6:16] = overlays[variant]
                for color in range(16):
                    word = struct.unpack_from("<H", colors, color * 2)[0] & 0x7fff
                    palette_asset.extend(struct.pack("<H", word))
    return pose, bytes(source_manifest), bytes(palette_asset), bytes(layout_asset)


def build_player_animation_asset(rom_data):
    """Pack the ROM's complete gameplay animation tables and sprite resources.

    Ghidra labels $87:AB38-$AD5A as the frame updater.  It indexes the two
    lower-body tables at $84:C218/$84:C28A and the upper-body table at
    $84:C2FC.  Each descriptor supplies a cadence mode, timing, frame count,
    and eight directional resource lists.  $80:AD92-$AEC1 attaches lower,
    upper, and head resources using the signed X/Y tables at
    $A9:D86E/$A9:D03E.  The payload keeps those ROM structures intact and
    bundles every referenced raw resource descriptor; no captured pixels are
    used.
    """
    state_tables = (0x84C218, 0x84C28A, 0x84C2FC)
    state_count = (0x84C36E - 0x84C2FC) // 2
    resources = set()
    for table in state_tables:
        for state in range(state_count):
            descriptor = struct.unpack_from(
                "<H", rom_data, lorom_offset(table + state * 2))[0]
            if descriptor < 0x8000:
                continue
            descriptor_offset = lorom_offset(0x840000 + descriptor)
            frame_count = struct.unpack_from(
                "<H", rom_data, descriptor_offset + 6)[0]
            if not 1 <= frame_count <= 64:
                raise RuntimeError(
                    f"Invalid player animation frame count at $84:{descriptor:04X}")
            for direction in range(8):
                frame_list = struct.unpack_from(
                    "<H", rom_data, descriptor_offset + 8 + direction * 2)[0]
                if frame_list < 0x8000:
                    raise RuntimeError(
                        f"Invalid player animation list at $84:{descriptor:04X}")
                list_offset = lorom_offset(0x840000 + frame_list)
                resources.update(struct.unpack_from(
                    f"<{frame_count}H", rom_data, list_offset))

    # The roster-selected five-direction head families occupy this exact
    # resource range; include them so the lab composes the selected player.
    resources.update(range(0x049C, 0x049C + 39 * 5))
    # $87:A98E selects one of these overlays for the generated jersey tile.
    resources.update((0x0591, 0x0592, 0x0593))
    resource_table = lorom_offset(0x898000)
    records = []
    for resource_id in sorted(resources):
        entry = resource_table + resource_id * 4
        relative = struct.unpack_from("<I", rom_data, entry)[0] & 0xFFFFFF
        descriptor_address = 0x898000 + relative
        descriptor_offset = lorom_offset(descriptor_address)
        part_word, _, graphics_a, graphics_b, _ = struct.unpack_from(
            "<5H", rom_data, descriptor_offset)
        part_count = part_word & 0x7FFF
        size = 10 + part_count * 7 + graphics_a + graphics_b
        if part_count > 32 or size > 0x10000 or descriptor_offset + size > len(rom_data):
            raise RuntimeError(
                f"Invalid player sprite resource ${resource_id:04X} at "
                f"${descriptor_address:06X}")
        records.append((resource_id, rom_data[descriptor_offset:descriptor_offset + size]))

    header_size = struct.calcsize("<8sIIIIIIIIIIII")
    bank84_offset = header_size
    attachment_offset = bank84_offset + 0x8000
    attachment_size = 0x830
    directory_offset = attachment_offset + attachment_size * 2
    directory_size = len(records) * struct.calcsize("<HHII")
    data_offset = directory_offset + directory_size
    resource_bytes = sum(len(data) for _, data in records)
    digit_source_offset = data_offset + resource_bytes
    bcd_table_offset = digit_source_offset + 90 * 32
    number_attachment_offset = bcd_table_offset + 100
    number_palette_offset = number_attachment_offset + attachment_size * 2
    number_visibility_offset = number_palette_offset + 64 + 29 * 4
    payload = bytearray(struct.pack(
        "<8sIIIIIIIIIIII", b"NBPANIM1", 4, state_count, len(records),
        bank84_offset, attachment_offset, directory_offset, data_offset,
        digit_source_offset, bcd_table_offset, number_attachment_offset,
        number_palette_offset, number_visibility_offset))
    payload.extend(rom_data[lorom_offset(0x848000):lorom_offset(0x848000) + 0x8000])
    payload.extend(rom_data[lorom_offset(0xA9D86E):lorom_offset(0xA9D86E) + attachment_size])
    payload.extend(rom_data[lorom_offset(0xA9D03E):lorom_offset(0xA9D03E) + attachment_size])
    cursor = data_offset
    for resource_id, data in records:
        payload.extend(struct.pack("<HHII", resource_id, 0, cursor, len(data)))
        cursor += len(data)
    for _, data in records:
        payload.extend(data)
    # $87:B05B sets the long source bank to $A6 before $87:B074-$B354
    # composites three direction-dependent jersey-number orientations.
    payload.extend(rom_data[lorom_offset(0xA6AFD6):lorom_offset(0xA6AFD6) + 90 * 32])
    # $87:B357-$B378 maps roster byte +$00 (binary jersey number) to BCD.
    payload.extend(rom_data[lorom_offset(0x80859C):lorom_offset(0x80859C) + 100])
    # $80:AE20/$AE3C place the separate number resource relative to the
    # current upper-body resource. Keep the complete signed X/Y tables.
    payload.extend(rom_data[lorom_offset(0xACD07B):lorom_offset(0xACD07B) + attachment_size])
    payload.extend(rom_data[lorom_offset(0xACAE1B):lorom_offset(0xACAE1B) + attachment_size])
    # $85:8CAE-$8CB9 copies this 64-byte ROM block to WRAM $7F:0000. The
    # second half becomes OBJ palette 7, used by $80:AE86's number overlay.
    # $85:8CBD-$8CD7 patches one team color for each side from $AF:DD76.
    payload.extend(rom_data[lorom_offset(0xAFE99F):lorom_offset(0xAFE99F) + 64])
    payload.extend(rom_data[lorom_offset(0xAFDD76):lorom_offset(0xAFDD76) + 29 * 4])
    # $87:A506-$A51E sign-extends $AC:C7E3[upper resource]. Negative entries
    # suppress the separate jersey-number overlay for that body frame.
    payload.extend(rom_data[lorom_offset(0xACC7E3):lorom_offset(0xACC7E3) + attachment_size])
    return bytes(payload)


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
    ea_final_candidate = os.path.join(intro_capture_dir, "ea_motion_131.png")
    ea_a_fixed_candidates = [
        os.path.join(intro_capture_dir, f"ea_motion_{frame:03d}.png")
        for frame in range(56, 67)
    ]
    missing_stages = [path for path in
                      ea_candidates + [ea_final_candidate] + ea_a_fixed_candidates
                      if not os.path.exists(path)]
    if missing_stages:
        raise RuntimeError(
            "Missing EA intro captures: " + ", ".join(missing_stages) +
            ". Run tools/mesen_intro_capture.lua with the ROM first."
        )

    w4, h4 = 0, 0
    ea_flags = 0
    ea_packed = []
    ea_a_layer_bytes = b""
    ea_e_layer_bytes = b""
    ea_sports_layer_bytes = b""
    ea_a_fixed_bytes = bytearray()

    if all(os.path.exists(p) for p in ea_candidates):
        snes_frames = []
        for p in ea_candidates + [ea_final_candidate]:
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

        # $82:F4C4 flashes the settled A as fixed OAM, not through Mode 7.
        # Preserve identity frames 56-57, all eight palette steps 58-65, and
        # the settled frame 66 in one typed sequence payload.
        for p in ea_a_fixed_candidates:
            frame = np.array(Image.open(p).convert('RGB'))
            crop = frame[urmin:urmax+1, ucmin:ucmax+1]
            for r in range(h4):
                for c in range(w4):
                    rgb = crop[r, c]
                    if np.all(rgb <= 10):
                        ea_a_fixed_bytes.extend(struct.pack("<I", 0x00000000))
                    else:
                        argb = (0xFF000000 | (int(rgb[0]) << 16) |
                                (int(rgb[1]) << 8) | int(rgb[2]))
                        ea_a_fixed_bytes.extend(struct.pack("<I", argb))

        # $82:F512 returns after $80:8FA3 has written A's independent Mode 7
        # tilegroup. Decode the native interleaved Mode 7 tilemap/character
        # plane: even VRAM bytes select tiles and odd bytes hold indexed pixels.
        def decode_ea_mode7_layer(stem, index_ranges, expected_bounds,
                                  routine, letter):
            vram_path = os.path.join(intro_capture_dir, stem + "_vram.bin")
            cgram_path = os.path.join(intro_capture_dir, stem + "_cgram.bin")
            if not os.path.exists(vram_path) or not os.path.exists(cgram_path):
                raise RuntimeError(f"Missing {routine} Mode 7 hardware capture; "
                                   "re-run tools/mesen_intro_capture.lua")
            mode7_vram = open(vram_path, "rb").read()
            mode7_cgram = open(cgram_path, "rb").read()
            if len(mode7_vram) != 0x10000 or len(mode7_cgram) != 0x200:
                raise RuntimeError(f"Invalid {routine} Mode 7 VRAM/CGRAM capture")
            mode7 = np.zeros((1024, 1024), dtype=np.uint8)
            for tile_y in range(128):
                for tile_x in range(128):
                    tile = mode7_vram[(tile_y * 128 + tile_x) * 2]
                    for pixel_y in range(8):
                        for pixel_x in range(8):
                            word = tile * 64 + pixel_y * 8 + pixel_x
                            mode7[tile_y * 8 + pixel_y,
                                  tile_x * 8 + pixel_x] = mode7_vram[word * 2 + 1]
            source = np.zeros(mode7.shape, dtype=bool)
            for index_low, index_high in index_ranges:
                source |= (mode7 >= index_low) & (mode7 <= index_high)
            ys, xs = np.where(source)
            bounds = (xs.min(), ys.min(), xs.max(), ys.max())
            if bounds != expected_bounds:
                raise RuntimeError(f"Unexpected {routine} {letter} tilegroup bounds")
            layer = np.zeros((h4, w4), dtype=np.uint32)
            for source_y, source_x in zip(ys, xs):
                local_x = int(source_x) - 382 - ucmin
                local_y = int(source_y) - 402 - urmin
                if not (0 <= local_x < w4 and 0 <= local_y < h4):
                    raise RuntimeError(f"{routine} {letter} pixel maps outside the EA canvas")
                index = int(mode7[source_y, source_x])
                bgr = mode7_cgram[index * 2] | (mode7_cgram[index * 2 + 1] << 8)
                r5, g5, b5 = bgr & 31, (bgr >> 5) & 31, (bgr >> 10) & 31
                r8, g8, b8 = ((r5 << 3) | (r5 >> 2),
                              (g5 << 3) | (g5 >> 2),
                              (b5 << 3) | (b5 >> 2))
                layer[local_y, local_x] = (
                    0xFF000000 | (r8 << 16) | (g8 << 8) | b8)
            print(f"[ASSET EXTRACTOR] Decoded {routine} Mode 7 {letter} layer: "
                  f"{len(xs)} indexed source pixels")
            return layer.tobytes()

        # M7X/M7Y and the captured scroll origin map source (382,402) to
        # native screen (0,0). E and A use separate palette-index blocks.
        ea_e_layer_bytes = decode_ea_mode7_layer(
            "ea_e_mode7", [(0x31, 0x3F)], (441, 449, 519, 524), "$82:F4F6", "E")
        ea_a_layer_bytes = decode_ea_mode7_layer(
            "ea_a_mode7", [(0x41, 0x4F)], (494, 449, 572, 524), "$82:F512", "A")
        # $82:F52E passes the ROM descriptor at $82:F6D8 to $80:8FA3 twice,
        # at tile rows $38 and $3D.  Indices $21-$2F are the visible blue
        # SPORTS word and trademark; the $11-$1F block is intentionally black
        # background/clearing data and is not artwork in the host's transparent
        # layer.  Deriving either from screenshot differences produces ghost
        # EA pixels during the zoom.
        ea_sports_layer_bytes = decode_ea_mode7_layer(
            "ea_sports_mode7", [(0x21, 0x2F)],
            (444, 530, 581, 559), "$82:F52E", "SPORTS")

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

    def read_setup_menu_capture(menu_name, filename, expected_size):
        path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..",
                            ".analysis", f"setup_{menu_name}", filename)
        if not os.path.exists(path):
            raise RuntimeError(f"Missing Set {menu_name.title()} capture: {path}. "
                               "Run tools/mesen_setup_menus_capture.lua first.")
        data = open(path, "rb").read()
        if len(data) != expected_size:
            raise RuntimeError(f"Invalid {path}: expected {expected_size} bytes, "
                               f"got {len(data)}")
        return data

    def read_capture_directory(directory_name, filename, expected_size):
        path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..",
                            ".analysis", directory_name, filename)
        if not os.path.exists(path):
            raise RuntimeError(f"Missing capture: {path}. Run "
                               "tools/capture_assets.ps1 first.")
        data = open(path, "rb").read()
        if len(data) != expected_size:
            raise RuntimeError(f"Invalid {path}: expected {expected_size} bytes, "
                               f"got {len(data)}")
        return data

    rules_vram_bytes = read_setup_menu_capture("rules", "menu_vram.bin", 0x10000)
    rules_cgram_bytes = read_setup_menu_capture("rules", "menu_cgram.bin", 0x200)
    options_vram_bytes = read_setup_menu_capture("options", "menu_vram.bin", 0x10000)
    options_cgram_bytes = read_setup_menu_capture("options", "menu_cgram.bin", 0x200)
    rules_oam_bytes = read_setup_menu_capture("rules", "menu_oam.bin", 0x220)
    options_oam_bytes = read_setup_menu_capture("options", "menu_oam.bin", 0x220)
    options_off_vram_bytes = read_capture_directory(
        "setup_option_values", "options_mode_off_vram.bin", 0x10000)
    options_mono_vram_bytes = read_capture_directory(
        "setup_option_values", "options_mode_mono_vram.bin", 0x10000)
    options_cpu_vram_bytes = read_capture_directory(
        "setup_option_values", "options_shot_cpu_vram.bin", 0x10000)
    options_crowd_off_vram_bytes = read_capture_directory(
        "setup_option_values", "options_crowd_off_vram.bin", 0x10000)
    options_slow_on_vram_bytes = read_capture_directory(
        "setup_option_values", "options_slow_on_vram.bin", 0x10000)
    options_assistance_on_vram_bytes = read_capture_directory(
        "setup_option_values", "options_assistance_on_vram.bin", 0x10000)

    def build_menu_ppu_trace(menu_name, prefix, first_frame, last_frame):
        base_vram = read_setup_menu_capture(menu_name, f"{prefix}_transition_vram.bin", 0x10000)
        base_cgram = read_setup_menu_capture(menu_name, f"{prefix}_transition_cgram.bin", 0x200)
        directory = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..",
                                 ".analysis", f"setup_{menu_name}")
        events = ({}, {})
        states = {}
        for event_index, memory_name in enumerate(("vram", "cgram")):
            path = os.path.join(directory, f"{prefix}_transition_{memory_name}_writes.txt")
            for raw in open(path, "r", encoding="ascii"):
                if not raw.strip() or raw.startswith("#"):
                    continue
                frame_text, address_text, value_text = raw.split()
                frame = int(frame_text)
                if frame < first_frame:
                    raise RuntimeError(f"Invalid submenu PPU trace frame: {raw.strip()}")
                # endFrame screenshots contain the scanout just presented,
                # while VRAM/CGRAM reads observe memory prepared for the next
                # scanout.  Delay deltas one frame so construction DMA cannot
                # leak raw tiles through the still-visible outgoing page.
                packed_frame = frame + 1 - first_frame
                if packed_frame < 0 or packed_frame >= last_frame - first_frame + 1:
                    continue
                events[event_index].setdefault(packed_frame, []).append(
                    (int(address_text, 16), int(value_text, 16)))
        state_path = os.path.join(directory, f"{prefix}_transition_ppu_states.txt")
        for raw in open(state_path, "r", encoding="ascii"):
            fields = list(map(int, raw.split()))
            if len(fields) != 22 or fields[0] < first_frame:
                raise RuntimeError(f"Invalid submenu PPU state: {raw.strip()}")
            if fields[0] <= last_frame:
                states[fields[0] - first_frame] = fields[1:]
        frame_count = last_frame - first_frame + 1
        if len(states) != frame_count:
            raise RuntimeError(f"Incomplete Set {menu_name.title()} {prefix} PPU states")
        trace = bytearray(struct.pack("<8sII", b"NBSPPU2\0", 2, frame_count))
        # endFrame exposes the PPU registers prepared for the following
        # scanout, just as it does VRAM/CGRAM. Pack the preceding state so the
        # shared $80:A2BF/$80:A3B8 scroll, fade, and map switch are presented
        # on the same frame as Mesen's completed image. Frame zero is already
        # the first transition state and therefore clamps to itself.
        for frame in range(frame_count):
            state = states[max(0, frame - 1)]
            trace.extend(struct.pack("<BBBB", state[0], state[1], state[2], 0))
            for layer in range(3):
                base = 3 + layer * 6
                trace.extend(struct.pack(
                    "<HHHHBB", state[base], state[base + 1],
                    state[base + 2] * 2, state[base + 3] * 2,
                    state[base + 4], state[base + 5]))
            vw, cw = events[0].get(frame, []), events[1].get(frame, [])
            trace.extend(struct.pack("<HH", len(vw), len(cw)))
            for address, value in vw:
                trace.extend(struct.pack("<HB", address, value))
            for address, value in cw:
                trace.extend(struct.pack("<HB", address, value))
        print(f"[ASSET EXTRACTOR] Packed Set {menu_name.title()} {prefix} transition: "
              f"{frame_count} frames, {sum(map(len, events[0].values()))} VRAM writes")
        return base_vram, base_cgram, bytes(trace)

    rules_open_transition = build_menu_ppu_trace("rules", "open", 471, 616)
    options_open_transition = build_menu_ppu_trace("options", "open", 471, 602)
    setup_return_transition = build_menu_ppu_trace("options", "return", 831, 962)
    rules_return_transition = build_menu_ppu_trace("rules", "return", 831, 962)

    setup_main_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..",
                                  ".analysis", "setup_main")
    def read_setup_main_capture(filename):
        path = os.path.join(setup_main_dir, filename)
        if not os.path.exists(path):
            raise RuntimeError(f"Missing main Game Setup capture: {path}. Run "
                               "tools/mesen_setup_main_capture.lua first.")
        data = open(path, "rb").read()
        if len(data) != 0x10000:
            raise RuntimeError(f"Invalid {path}: expected 65536 bytes, got {len(data)}")
        return data

    setup_main_variant_bytes = [
        read_setup_main_capture("row0_step1_vram.bin"), # Season
        read_setup_main_capture("row0_step2_vram.bin"), # Playoffs
        read_setup_main_capture("row0_step3_vram.bin"), # Load Series
        read_setup_main_capture("row1_step1_vram.bin"), # Custom
        read_setup_main_capture("row1_step2_vram.bin"), # Arcade
        read_setup_main_capture("row2_step1_vram.bin"), # Starter
        read_setup_main_capture("row2_step2_vram.bin"), # All-Star
        read_setup_main_capture("row3_step1_vram.bin"), # 5 Minutes
        read_setup_main_capture("row3_step2_vram.bin"), # 8 Minutes
        read_setup_main_capture("row3_step3_vram.bin"), # 12 Minutes
    ]

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

    # The transition-origin ARAM still contains the outgoing title bank. The
    # ROM uploads Setup's 30-source BRR directory during forced blank; capture
    # that completed bank immediately before the first Setup KON.
    setup_spc_ram_bytes = read_setup_transition(
        "setup_music_spc_ram.bin", 0x10000)
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

    setup_sample_assets = []
    setup_dir = setup_spc_dsp_bytes[0x5D] << 8
    for srcn in range(30):
        entry = setup_dir + srcn * 4
        start = setup_spc_ram_bytes[entry] | (setup_spc_ram_bytes[entry + 1] << 8)
        loop = setup_spc_ram_bytes[entry + 2] | (setup_spc_ram_bytes[entry + 3] << 8)
        if start == 0:
            raise RuntimeError(f"Setup SRCN ${srcn:02X} has no BRR start address")
        pcm, _ = decode_brr_to_pcm(setup_spc_ram_bytes[start:])
        if not pcm:
            raise RuntimeError(f"Setup SRCN ${srcn:02X} has invalid BRR data at ${start:04X}")
        setup_sample_assets.append(
            (94 + srcn, srcn, start, loop, make_wav_bytes(pcm, sample_rate=32000)))
    print(f"[ASSET EXTRACTOR] Packed F11 Setup BRR catalog: "
          f"{len(setup_sample_assets)} sources from S-DSP DIR ${setup_dir:04X}")

    team_capture_dir = os.environ.get("NBA95_TEAM_CAPTURE_DIR") or os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", ".analysis",
        "team_select_logos")
    team_logo_assets = []
    team_vram_assets = []
    team_cgram_assets = []
    for team in range(29):
        prefix = os.path.join(team_capture_dir, f"team_{team:02d}")
        paths = [prefix + "_vram.bin", prefix + "_cgram.bin", prefix + "_oam.bin"]
        expected = [0x10000, 0x200, 0x220]
        payloads = []
        for path, size in zip(paths, expected):
            if not os.path.exists(path):
                raise RuntimeError(f"Missing Team Select logo capture: {path}. Run "
                                   "mesen_team_select_capture.lua with NBA95_TEAM_LOGOS=1.")
            data = open(path, "rb").read()
            if len(data) != size:
                raise RuntimeError(f"Invalid Team Select capture {path}: "
                                   f"expected {size} bytes, got {len(data)}")
            payloads.append(data)
        logo = decode_team_logo(*payloads)
        if not any(logo[index + 3] for index in range(0, len(logo), 4)):
            raise RuntimeError(f"Team {team} logo decoded as fully transparent")
        team_logo_assets.append((160 + team, 48, 56, team, logo))
        team_vram_assets.append((192 + team, 0, 0, team, payloads[0]))
        team_cgram_assets.append((221 + team, 0, 0, team, payloads[1]))
    if len({hashlib.sha256(asset[4]).digest() for asset in team_logo_assets}) != 29:
        raise RuntimeError("Team Select logo captures are not unique for all 29 teams")
    print("[ASSET EXTRACTOR] Packed 29 Team Select logos from raw SNES VRAM/CGRAM/OAM")
    team_select_oam_asset = (189, 0, 0, 0,
                             open(os.path.join(team_capture_dir, "team_18_oam.bin"), "rb").read())
    # $82:8933-$8967 advances through seven overlapping 14-byte windows of
    # this 26-byte ROM table. Each window is copied to CGRAM $A1-$A7, so the
    # selected gold plate animates without changing OAM.
    team_selected_palette_cycle = rom_data[0x10968:0x10968 + 26]
    if len(team_selected_palette_cycle) != 26:
        raise RuntimeError("Team Select selected-plate palette table is truncated")

    player_rosters = build_player_roster_asset(rom_data)
    (player_default_pose, player_tile_sources, player_palette_tables,
     player_pose_layout) = build_player_front_pose(rom_data)
    player_animations = build_player_animation_asset(rom_data)

    player_setup_capture_dir = os.environ.get("NBA95_PLAYER_SETUP_CAPTURE_DIR") or os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", ".analysis", "player_setup")
    player_setup_payloads = []
    for name, size in (("player_setup_vram.bin", 0x10000),
                       ("player_setup_cgram.bin", 0x200),
                       ("player_setup_oam.bin", 0x220)):
        path = os.path.join(player_setup_capture_dir, name)
        if not os.path.exists(path):
            raise RuntimeError(f"Missing Player Setup PPU asset: {path}. Run "
                               "mesen_player_setup_capture.lua against the verified ROM.")
        payload = open(path, "rb").read()
        if len(payload) != size:
            raise RuntimeError(f"Invalid Player Setup PPU asset {path}: "
                               f"expected {size} bytes, got {len(payload)}")
        player_setup_payloads.append(payload)

    player_intro_capture_dir = os.environ.get("NBA95_PLAYER_INTRO_CAPTURE_DIR") or os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", ".analysis",
        "player-intro-full-20260823")
    player_intro_portrait_dir = os.environ.get("NBA95_PLAYER_INTRO_PORTRAIT_DIR") or os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", ".analysis",
        "player_intro_portraits_verified_20260823")
    player_intro_away_portrait_dir = os.environ.get(
        "NBA95_PLAYER_INTRO_AWAY_PORTRAIT_DIR") or os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", ".analysis",
        "player_intro_portraits_away_verified_20260823")
    player_intro_court, player_intro_portraits = build_player_introduction_assets(
        player_intro_capture_dir, player_intro_away_portrait_dir,
        player_intro_portrait_dir)
    player_intro_rating_balls = build_player_intro_rating_balls(
        player_intro_capture_dir)
    # Live $81:9756 calls identify the presentation font descriptor as
    # $A9:8000 (normalized LoROM offset $148000).  It contains the pointer
    # table, proportional advances, and original 8x16 2bpp glyphs.
    player_intro_font = rom_data[0x148000:0x149000]
    if len(player_intro_font) != 0x1000 or player_intro_font[:6] != \
            b"\x10\x00\x0e\x00\x01\x02":
        raise RuntimeError("Player Introduction ROM font descriptor is invalid")
    player_intro_audio_dir = os.environ.get(
        "NBA95_PLAYER_INTRO_AUDIO_DIR") or os.path.join(
            os.path.dirname(os.path.abspath(__file__)), "..", ".analysis",
            "player-intro-audio-assets-20260823")

    def read_player_intro_audio(name, expected_size=None):
        path = os.path.join(player_intro_audio_dir, name)
        if not os.path.exists(path):
            raise RuntimeError(f"Missing Player Introduction audio capture: {path}. "
                               "Run mesen_gameplay_player_capture.lua with "
                               "NBA95_PLAYER_INTRO_AUDIO_TRACE=1.")
        payload = open(path, "rb").read()
        if expected_size is not None and len(payload) != expected_size:
            raise RuntimeError(f"Invalid Player Introduction audio asset {path}: "
                               f"expected {expected_size} bytes, got {len(payload)}")
        return payload

    player_intro_spc_ram = read_player_intro_audio("player_intro_spc_ram.bin", 0x10000)
    player_intro_spc_dsp = read_player_intro_audio("player_intro_spc_dsp.bin", 0x80)
    player_intro_state_text = read_player_intro_audio(
        "player_intro_spc_state.txt").decode("ascii")

    def player_intro_state_int(key):
        match = re.search(r"(?:^|\s)" + re.escape(key) +
                          r"=(-?\d+)(?:\s|$)", player_intro_state_text)
        if not match:
            raise RuntimeError(f"Player Introduction capture is missing state key {key}")
        return int(match.group(1))

    player_intro_frames = player_intro_state_int("frames")
    player_intro_spc_state = b"NBPISPC1" + struct.pack(
        "<IH6B", 1, player_intro_state_int("pc"), player_intro_state_int("a"),
        player_intro_state_int("x"), player_intro_state_int("y"),
        player_intro_state_int("sp"), player_intro_state_int("ps"), 0)
    player_intro_dsp_events = []
    previous_cycle = -1
    for raw in read_player_intro_audio(
            "player_intro_dsp_cycle_trace.txt").decode("ascii").splitlines():
        if not raw or raw.startswith("#"):
            continue
        cycle_text, register_text, value_text = raw.split()
        event = (int(cycle_text) // 2, int(register_text, 16), int(value_text, 16))
        if (event[0] < previous_cycle or not 0 <= event[1] < 0x80 or \
                not 0 <= event[2] <= 255):
            raise RuntimeError(f"Invalid Player Introduction DSP event: {event}")
        previous_cycle = event[0]
        player_intro_dsp_events.append(event)
    max_player_intro_cycles = player_intro_frames * 1024000 // 60
    if not player_intro_dsp_events or \
            player_intro_dsp_events[-1][0] > max_player_intro_cycles:
        raise RuntimeError("Player Introduction DSP trace exceeds its declared duration")
    player_intro_dsp_trace = bytearray(struct.pack(
        "<8sIII", b"NBPIDSP1", 1, player_intro_frames,
        len(player_intro_dsp_events)))
    for cycle, register, value in player_intro_dsp_events:
        player_intro_dsp_trace.extend(struct.pack("<IBB", cycle, register, value))
    print(f"[ASSET EXTRACTOR] Packed Player Introduction ROM BRR/S-DSP program: "
          f"{player_intro_frames} frames, {len(player_intro_dsp_events)} writes")
    # OBJ tile $EA has one byte-for-byte match in the ROM. Colors 5..10 are
    # the hardware palette entries selected by the tile during the jump ball.
    tipoff_ball = struct.pack(
        "<8sI32s6H", b"NBBALL1", 1,
        rom_data[0x0D9C27:0x0D9C27 + 32],
        0x32BF, 0x0DDE, 0x019B, 0x0177, 0x00F1, 0x00AC)
    tipoff_capture_dir = os.environ.get("NBA95_TIPOFF_CAPTURE_DIR") or os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", ".analysis",
        "tipoff-actor-probe-20260823")
    with open(os.path.join(tipoff_capture_dir, "tipoff_0140_vram.bin"), "rb") as f:
        gameplay_vram = f.read()
    with open(os.path.join(tipoff_capture_dir, "tipoff_0140_cgram.bin"), "rb") as f:
        gameplay_cgram = f.read()
    if len(gameplay_vram) != 0x10000 or len(gameplay_cgram) != 0x200:
        raise RuntimeError("Invalid settled tip-off PPU state")
    gameplay_court = decode_bg_layer(gameplay_vram, gameplay_cgram,
                                     0x1000, 0x4000, 4, True, False, 6, 6)

    assets = [
        (1, 128, 11, 0, nintendo_license_bytes),               # ASSET_NINTENDO_LICENSE
        (2, 256, num_legal_rows, start_y_legal, nba_legal_bytes), # ASSET_NBA_LEGAL_NOTICE (flags = start_y)
        (3, w4, h4, ea_flags, ea_packed[0]),                  # ASSET_EA_LOGO_STAGE1
        (4, w4, h4, ea_flags, ea_packed[1]),                  # ASSET_EA_LOGO_STAGE2
        (5, w4, h4, ea_flags, ea_packed[2]),                  # ASSET_EA_LOGO_STAGE3
        (6, w4, h4, ea_flags, ea_packed[3]),                  # ASSET_EA_LOGO_STAGE4
        (70, w4, h4, ea_flags, ea_a_layer_bytes),             # ASSET_EA_A_LAYER
        (71, w4, h4, ea_flags, ea_e_layer_bytes),             # ASSET_EA_E_LAYER
        (72, w4, h4, ea_flags, ea_packed[4]),                 # ASSET_EA_LOGO_FINAL
        (73, w4, h4, ea_flags, ea_a_fixed_bytes),             # ASSET_EA_A_FIXED_SEQUENCE
        (74, w4, h4, ea_flags, ea_sports_layer_bytes),        # ASSET_EA_SPORTS_LAYER
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
    assets.extend(setup_sample_assets)
    assets.extend(team_logo_assets)
    assets.append(team_select_oam_asset)
    assets.extend(team_vram_assets)
    assets.extend(team_cgram_assets)
    assets.append((250, 7, 7, 8, team_selected_palette_cycle))
    assets.extend([
        (251, 29, 12, 64, player_rosters),
        (252, 24, 64, 0, player_default_pose),
        (253, 16, 32, 0, player_tile_sources),
        (254, 0xAFEF00, 0x2A0, 0, player_palette_tables),
        (255, 7, 0, 0, player_pose_layout),
        (256, 57, 8, 4, player_animations),
        (257, 0, 0, 0, player_setup_payloads[0]),
        (258, 0, 0, 0, player_setup_payloads[1]),
        (259, 0, 0, 0, player_setup_payloads[2]),
        (260, 256, 224, 0, player_intro_court),
        (261, 72, 72, 290, player_intro_portraits),
        (262, 8, 8, 0x0D9C27, tipoff_ball),
        (263, 256, 224, 0, gameplay_court),
        (264, 16, 16, 6, player_intro_rating_balls),
        (265, 0, 0, 0, player_intro_spc_ram),
        (266, 0, 0, 0, player_intro_spc_dsp),
        (267, 0, 0, 0, player_intro_spc_state),
        (268, 0, 0, 0, bytes(player_intro_dsp_trace)),
        (269, 8, 16, 0xA98000, player_intro_font),
    ])
    assets.extend([
        (124, 0, 0, 0, rules_vram_bytes),
        (125, 0, 0, 0, rules_cgram_bytes),
        (126, 0, 0, 0, options_vram_bytes),
        (127, 0, 0, 0, options_cgram_bytes),
        (128, 0x6000, 0, 0, rules_oam_bytes),
        (129, 0x6000, 0, 0, options_oam_bytes),
        (130, 0, 0, 0, options_off_vram_bytes),
        (131, 0, 0, 0, options_mono_vram_bytes),
        (132, 0, 0, 0, options_cpu_vram_bytes),
    ])
    assets.extend((133 + index, 0, 0, 0, data)
                  for index, data in enumerate(setup_main_variant_bytes))
    assets.extend([
        (143, 0, 0, 0, rules_open_transition[0]),
        (144, 0, 0, 0, rules_open_transition[1]),
        (145, 0, 0, 0, rules_open_transition[2]),
        (146, 0, 0, 0, options_open_transition[0]),
        (147, 0, 0, 0, options_open_transition[1]),
        (148, 0, 0, 0, options_open_transition[2]),
        (149, 0, 0, 0, setup_return_transition[0]),
        (150, 0, 0, 0, setup_return_transition[1]),
        (151, 0, 0, 0, setup_return_transition[2]),
        (152, 0, 0, 0, options_crowd_off_vram_bytes),
        (153, 0, 0, 0, rules_return_transition[0]),
        (154, 0, 0, 0, rules_return_transition[1]),
        (155, 0, 0, 0, rules_return_transition[2]),
        (156, 0, 0, 0, options_slow_on_vram_bytes),
        (157, 0, 0, 0, options_assistance_on_vram_bytes),
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
    version = 20
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
