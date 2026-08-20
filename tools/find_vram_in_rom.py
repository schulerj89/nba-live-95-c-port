"""Locate captured VRAM regions inside the ROM to find their uncompressed source."""
import os, sys

if len(sys.argv) != 2:
    raise SystemExit("Usage: python find_vram_in_rom.py <rom>")
root = os.path.join(os.path.dirname(__file__), "..")
rom = open(sys.argv[1], "rb").read()
vram = open(os.path.join(root, ".analysis", "setup_capture", "vram.bin"), "rb").read()

def lorom_addr(pc):
    bank = 0x80 + (pc // 0x8000)
    off = 0x8000 + (pc % 0x8000)
    return f"${bank:02X}:{off:04X}"

regions = [
    ("BG3 font tiles (2bpp)", 0x8000, 0x9600),
    ("BG3 tilemap",           0x0000, 0x0800),
    ("BG2 tilemap",           0x1000, 0x1A00),
    ("BG1 tilemap",           0x1800, 0x2000),
    ("BG2 chr (4bpp)",        0x2000, 0x4000),
    ("BG1 chr (4bpp)",        0x6000, 0x6800),
    ("high region C000",      0xC000, 0xD000),
]

for name, start, end in regions:
    blob = vram[start:end]
    # probe with a distinctive non-trivial slice
    found = None
    for probe_off in range(0, min(len(blob), 0x800), 16):
        probe = blob[probe_off:probe_off + 48]
        if len(probe) < 48 or probe.count(0) > 24:
            continue
        idx = rom.find(probe)
        if idx >= 0:
            found = (idx, probe_off)
            break
    if found:
        idx, po = found
        base = idx - po
        print(f"{name:24s} VRAM {start:04X}-{end:04X}  -> ROM 0x{base:06X} ({lorom_addr(base)})  [probe+{po:#x}]")
    else:
        print(f"{name:24s} VRAM {start:04X}-{end:04X}  -> NOT FOUND raw (compressed)")
