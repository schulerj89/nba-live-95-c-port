"""Independent SNES Mode-1 snapshot renderer used by parity tests.

This consumes raw Mesen VRAM/CGRAM/OAM and register state. It never consumes
port assets, C renderer output, or captured pixels while rendering; the PNG is
used only after rendering as the comparison oracle.
"""

import argparse
import json
from pathlib import Path

from PIL import Image

WIDTH, HEIGHT = 256, 224
RANKS_NORMAL = {
    ("BG3", 0): 1, ("OBJ", 0): 2, ("BG3", 1): 3,
    ("OBJ", 1): 4, ("BG2", 0): 5, ("BG1", 0): 6,
    ("OBJ", 2): 7, ("BG2", 1): 8, ("BG1", 1): 9,
    ("OBJ", 3): 10,
}


def parse_state(path):
    result = {}
    for line in Path(path).read_text().splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            result[key] = value
    return result


def integer(state, key):
    return int(state[key])


def boolean(state, key):
    return state.get(key) == "true"


def cgram_color(cgram, index, brightness):
    offset = (index * 2) & 0x1FF
    word = cgram[offset] | cgram[(offset + 1) & 0x1FF] << 8
    channels = [word & 31, (word >> 5) & 31, (word >> 10) & 31]
    rgb = [((value << 3) | (value >> 2)) * brightness // 15
           for value in channels]
    return tuple(rgb)


def tile_pixel(vram, offset, bits, x, y):
    value, bit = 0, 7 - x
    for plane in range(0, bits, 2):
        value |= ((vram[(offset + y * 2 + plane * 8) & 0xFFFF] >> bit) & 1) << plane
        value |= ((vram[(offset + y * 2 + 1 + plane * 8) & 0xFFFF] >> bit) & 1) << (plane + 1)
    return value


def window_visible(state, layer, x):
    if not boolean(state, f"ppu.windowMaskMain[{layer}]"):
        return True
    selected = []
    for window in range(2):
        if not boolean(state, f"ppu.window[{window}].activeLayers[{layer}]"):
            continue
        inside = (integer(state, f"ppu.window[{window}].left") <= x <=
                  integer(state, f"ppu.window[{window}].right"))
        if boolean(state, f"ppu.window[{window}].invertedLayers[{layer}]"):
            inside = not inside
        selected.append(inside)
    if not selected:
        return True
    # The captured game uses only one active layer window. Two-window logic is
    # covered independently by nba_snes_mode1_self_test.
    if len(selected) != 1:
        raise ValueError("snapshot uses two active windows without exported logic")
    return not selected[0]


def background_pixel(vram, cgram, state, layer, x, y, brightness):
    bits = 4 if layer < 2 else 2
    wide = boolean(state, f"ppu.layers[{layer}].doubleWidth")
    tall = boolean(state, f"ppu.layers[{layer}].doubleHeight")
    map_width, map_height = (512 if wide else 256), (512 if tall else 256)
    px = (x + integer(state, f"ppu.layers[{layer}].hscroll")) % map_width
    py = (y + integer(state, f"ppu.layers[{layer}].vscroll") + 1) % map_height
    tile_x, tile_y = px >> 3, py >> 3
    quadrant = (1 if wide and tile_x >= 32 else 0)
    if tall and tile_y >= 32:
        quadrant += 2 if wide else 1
    map_offset = (integer(state, f"ppu.layers[{layer}].tilemapAddress") * 2 +
                  quadrant * 0x800 + ((tile_y & 31) * 32 + (tile_x & 31)) * 2)
    entry = vram[map_offset & 0xFFFF] | vram[(map_offset + 1) & 0xFFFF] << 8
    sample_x, sample_y = px & 7, py & 7
    if entry & 0x4000:
        sample_x = 7 - sample_x
    if entry & 0x8000:
        sample_y = 7 - sample_y
    tile_offset = (integer(state, f"ppu.layers[{layer}].chrAddress") * 2 +
                   (entry & 0x3FF) * 8 * bits)
    color_index = tile_pixel(vram, tile_offset, bits, sample_x, sample_y)
    if not color_index:
        return None
    palette_index = ((entry >> 10) & 7) * (16 if bits == 4 else 4) + color_index
    name = f"BG{layer + 1}"
    priority = (entry >> 13) & 1
    return (name, priority, palette_index, color_index,
            cgram_color(cgram, palette_index, brightness))


def object_candidates(vram, cgram, oam, state, ranks, brightness):
    candidates = [None] * (WIDTH * HEIGHT)
    base = integer(state, "ppu.oamBaseAddress")
    name_offset = integer(state, "ppu.oamAddressOffset")
    mode = integer(state, "ppu.oamMode")
    if mode != 0:
        raise ValueError(f"oracle currently requires native OAM mode 0, got {mode}")
    first = ((integer(state, "ppu.oamRamAddress") // 4) % 128
             if boolean(state, "ppu.enableOamPriority") else 0)
    for order in range(128):
        index = (first + order) % 128
        high = (oam[512 + index // 4] >> (2 * (index % 4))) & 3
        x = oam[index * 4] | ((high & 1) << 8)
        if x >= 256:
            x -= 512
        y, tile, attributes = oam[index * 4 + 1:index * 4 + 4]
        size = 16 if high & 2 else 8
        for py in range(size):
            destination_y = (y + py) & 0xFF
            if destination_y >= HEIGHT:
                continue
            source_y = size - 1 - py if attributes & 0x80 else py
            for px in range(size):
                destination_x = x + px
                if destination_x < 0 or destination_x >= WIDTH:
                    continue
                source_x = size - 1 - px if attributes & 0x40 else px
                tile_id = (tile + (source_x >> 3) + (source_y >> 3) * 16) & 0xFF
                offset = ((base + tile_id * 16 +
                           (name_offset if attributes & 1 else 0)) * 2) & 0xFFFF
                color_index = tile_pixel(vram, offset, 4, source_x & 7, source_y & 7)
                position = destination_y * WIDTH + destination_x
                if not color_index or candidates[position] is not None:
                    continue
                palette_index = 128 + ((attributes >> 1) & 7) * 16 + color_index
                priority = (attributes >> 4) & 3
                candidates[position] = (
                    ranks[("OBJ", priority)], "OBJ", priority, palette_index,
                    color_index, index, cgram_color(cgram, palette_index, brightness))
    return candidates


def render_snapshot(vram, cgram, oam, state):
    if integer(state, "ppu.bgMode") != 1:
        raise ValueError("snapshot is not SNES Mode 1")
    brightness = integer(state, "ppu.screenBrightness")
    ranks = dict(RANKS_NORMAL)
    if boolean(state, "ppu.mode1Bg3Priority"):
        ranks[("BG3", 1)] = 11
    default_main = integer(state, "ppu.mainScreenLayers")
    backdrop = cgram_color(cgram, 0, brightness)
    backgrounds = []
    for y in range(HEIGHT):
        main = int(state.get(f"audit.mainScreenLayers[{y}]", default_main))
        for x in range(WIDTH):
            best = (0, "BACKDROP", 0, 0, 0, 127, backdrop)
            for layer in range(3):
                if not main & (1 << layer) or not window_visible(state, layer, x):
                    continue
                candidate = background_pixel(
                    vram, cgram, state, layer, x, y, brightness)
                if candidate:
                    name, priority, palette, color, rgb = candidate
                    rank = ranks[(name, priority)]
                    if rank > best[0]:
                        best = (rank, name, priority, palette, color, 127, rgb)
            backgrounds.append(best)
    objects = object_candidates(vram, cgram, oam, state, ranks, brightness)
    winners, pixels = [], []
    for position, (background, obj) in enumerate(zip(backgrounds, objects)):
        main = int(state.get(f"audit.mainScreenLayers[{position // WIDTH}]",
                             default_main))
        if not main & 0x10:
            obj = None
        winner = obj if obj and obj[0] > background[0] else background
        winners.append(winner[:-1])
        pixels.append(winner[-1])
    return pixels, winners


def load_snapshot(directory, frame):
    root = Path(directory)
    prefix = root / f"ppu_{frame:04d}"
    return (prefix.with_name(prefix.name + "_vram.bin").read_bytes(),
            prefix.with_name(prefix.name + "_cgram.bin").read_bytes(),
            prefix.with_name(prefix.name + "_oam.bin").read_bytes(),
            parse_state(prefix.with_name(prefix.name + "_state.txt")))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--native-dir", required=True)
    parser.add_argument("--from-frame", type=int, required=True)
    parser.add_argument("--to-frame", type=int, required=True)
    parser.add_argument("--report", required=True)
    args = parser.parse_args()
    root = Path(args.native_dir)
    report = {"frames": []}
    for frame in range(args.from_frame, args.to_frame + 1):
        native = list(Image.open(root / f"ppu_{frame:04d}.png").convert("RGB").getdata())
        candidates = []
        for state_frame in range(max(args.from_frame, frame - 1),
                                 min(args.to_frame, frame + 1) + 1):
            pixels, winners = render_snapshot(*load_snapshot(root, state_frame))
            mismatch = sum(left != right for left, right in zip(pixels, native))
            candidates.append({"state_frame": state_frame,
                               "mismatch_pixels": mismatch,
                               "mismatch_percent": mismatch * 100.0 / len(native)})
        report["frames"].append({"screen_frame": frame, "candidates": candidates})
    Path(args.report).write_text(json.dumps(report, indent=2) + "\n")
    for entry in report["frames"]:
        best = min(entry["candidates"], key=lambda item: item["mismatch_pixels"])
        print(f"screen={entry['screen_frame']} best_state={best['state_frame']} "
              f"mismatch={best['mismatch_pixels']} ({best['mismatch_percent']:.3f}%)")


if __name__ == "__main__":
    main()
